/**************************************************************************
* File: sobel_filter.cpp
* 
* Description: Reads each frame of a 4K video @ 60Hz, converts it to
grayscale, then applies a Sobel edge-detection filter
* 
* Author: Kevin Grant
*
* Revision history
* 
**************************************************************************/

#include "sobel_filter.h"

void initWorkspace(Workspace& workspace, int rows, int columns, int numThreads) {
    // initializing to zeroes is important so that row/col 0 and last row/col are deterministic
    workspace.out = cv::Mat::zeros(rows, columns, CV_8UC1);     

    // scale according to thread count
    workspace.threadBuffers.resize(numThreads);

    // resize each vector length
    for (auto& b: workspace.threadBuffers) {
        b.prev.resize(columns);
        b.curr.resize(columns); 
        b.next.resize(columns); 
    }
}

/**************************************************************************
* Function: performStreamingSobel
* 
* Description: combines grayscale + sobel operations into one function using only 3 row-buffers
*              reduces the memory passes
*
* param grayImage: cv::Mat&: The current (color) frame being processed
*                  cv::Mat &SobelImage: the image being written to by all threads
*                  RowBuffers& bufs: prev, curr, and next buffers for each row
*                  int startRow: the row number for the given thread to start
*                  int endRow: the row number for the given thread to end
*
* return: void
*
**************************************************************************/
void performStreamingSobel(const cv::Mat &frame, cv::Mat &out, RowBuffers& bufs, int startRow, int endRow) {
    const int rows = frame.rows;
    const int cols = frame.cols; 
        
    // define first row, guarantees we never start before 1
    int first = std::max(startRow, 1);

    // define end row, guarantees we stop before row - 1
    int last = std::min(endRow, rows - 1); 
  
    // sanity check
    if (first >= last || cols < 3) {
       return;  
    }


    // assign the 3 row buffers without realloation each func call
    uchar *prev = bufs.prev.data(); 
    uchar *curr = bufs.curr.data(); 
    uchar *next = bufs.next.data();

    const cv::Vec3b* srcRow0 = frame.ptr<cv::Vec3b>(first - 1); 
    const cv::Vec3b* srcRow1 = frame.ptr<cv::Vec3b>(first); 
    // priming loop
    for (int x = 0; x < cols; x++) {
        // converts first two rows to grayscale
        prev[x] = (54*srcRow0[x][2] + 183*srcRow0[x][1] + 19*srcRow0[x][0]) >> 8; 
        curr[x] = (54*srcRow1[x][2] + 183*srcRow1[x][1] + 19*srcRow1[x][0]) >> 8;
    }
    
    // main loop 
    for (int y = first; y < last; y++) {
        
        // convert color row y into the free buffer
        const cv::Vec3b* src = frame.ptr<cv::Vec3b>(y + 1); 
        for (int x = 0; x < cols; x++) {
            next[x] = (54*src[x][2] + 183*src[x][1] + 19*src[x][0]) >> 8; 
        }

        //prev=y-2, curr=y-1, next=y
        uchar* dst = out.ptr<uchar>(y);  
        for (int x = 1; x < cols - 1; x++) {
            // sobel operation per usual
            int p00 = prev[x-1]; 
            int p01 = prev[x]; 
            int p02 = prev[x+1]; 

            int p10 = curr[x-1]; 
            int p11 = curr[x]; 
            int p12 = curr[x+1]; 

            int p20 = next[x-1];
            int p21 = next[x]; 
            int p22 = next[x+1]; 
 
            int Gx = (-1 * p00) + (1 * p02) + (-2 * p10) + (2 * p12) + (-1 * p20) + (1 * p22);
            int Gy = (1 * p00) + ( 2 * p01) + (1 * p02) + (-1 * p20) + (-2 * p21) + (-1 * p22);

            int G = std::abs(Gx) + std::abs(Gy); 
            if (G > 255)
                G = 255; 
            
            dst[x] = G; 
        }
        
        
        // rotate oldest buffer 
        uchar* tmp = prev;
        prev = curr; 
        curr = next; 
        next = tmp; 
    }
}

/**************************************************************************
* Function: assignWorkersForSobel
* 
* Description: creates numThreads worker threads for parallelization 
*
* param grayImage: cv::Mat &frame: current frame being processed
*                  int numThreads: number of threads to create
*                  int64 &grayTicks: accumulated time for grayscale across all threads
*                  int64 &sobelTicks: accumulated time for sobel across all threads

* return: void
*
**************************************************************************/

void assignWorkersForSobel(const cv::Mat &frame, Workspace& workspace, int numThreads) {
    const int workRows = frame.rows - 2; 

    // prepare the threads
    std::vector<std::thread> workers; 
    workers.reserve(numThreads); 

    // launch the threads
    for (int i = 0; i < numThreads; i++) {
        // calculate startRow and endRow for each thread, divide work equally
        int startRow = 1 + i * workRows / numThreads; 
        int endRow = 1 + (i + 1) * workRows / numThreads;

        workers.emplace_back(
            performStreamingSobel,
            std::cref(frame),                       // passes input frame by constant reference 
            std::ref(workspace.out),                // passes shared output image by reference so all threads can write to it
            std::ref(workspace.threadBuffers[i]),   // passes the thread buffers
            startRow,
            endRow
        ); 
    }

    // waits for all threads to finish
    for (auto& worker : workers) {
        worker.join(); 
    }

}

/**************************************************************************
* Function: checksum
* 
* Description: 64-bit FNV-la checksum function applied to every byte of an image
*              in row-major order to detect if an image has changed (adopted from 
*              Claude Code)
*
* return: uint64_t
*
**************************************************************************/

uint64_t checksum(const cv::Mat& image) {
   uint64_t h = 1469598103934665603ULL;
   for (int y = 0; y < image.rows; y++) {
        const uchar* p = image.ptr<uchar>(y); 
        for (int x = 0; x < image.cols; x++) {
            h ^= p[x]; h *= 1099511628211ULL;
        }
   }

   return h;
}


/**************************************************************************
* Function: main
* 
* Description: Converts each frame to grayscale, performs the sobel 
               operation on each frame, and then displays the image
*
* return: int
*
**************************************************************************/

int main(int argc, char *argv[]) {
    // default values 
    int numThreads = 1;
    int iterations = 30; 
    std::string input = "4k_video.mp4"; 
    bool dump = false; 

    // handle command line arguments
    for (int i = 1; i < argc; i++) {
        std::string argument = argv[i]; 

        if (argument == "--threads" && (i + 1 < argc)) {
            numThreads = std::stoi(argv[++i]); 
        }

        else if (argument == "--iterations" && (i + 1 < argc)) {
            iterations = std::stoi(argv[++i]);
        }

        else if (argument == "--input" && (i + 1 < argc)) {
            input = argv[++i];
        }
        
        else if (argument == "--dump") {
            dump = true; 
        }

        else {
            std::fprintf(stderr, "Usage: %s --threads N --iterations K [--input f] [--dump]\n", argv[0]);
            return 1; 
        }
    }

    if (numThreads < 1 || iterations < 1) {
        std::fprintf(stderr, "bad arguments\n");
        return 1; 
    }

    // Setup video outside main loop
    cv::Mat frame; 
    {
        cv::VideoCapture video(input);
            
        // error check for safety
        if (!video.isOpened() || !video.read(frame)) {
            std::fprintf(stderr, "Could not read frame from %s\n", input.c_str()); 
            return 1; 
        }

    }

    // ensures the output cv video is fullscreen and not any larger (problem I had initially)
    //cv::namedWindow("Sobel", cv::WINDOW_NORMAL);
    //cv::setWindowProperty("Sobel", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);

    // initialize workspace outside main loop
    Workspace workspace;
    initWorkspace(workspace, frame.rows, frame.cols, numThreads); 

    // start initial timer
    auto t0 = std::chrono::steady_clock::now(); 
    for (int i = 0; i < iterations; i++) {
        assignWorkersForSobel(frame, workspace, numThreads); 
    }
    auto t1 = std::chrono::steady_clock::now(); 

    // time in ms
    double loopMs = std::chrono::duration<double, std::milli>(t1 - t0).count(); 
    std::printf("config=optimized threads=%d iterations=%d checksum=%016llx "   
                "loop_ms=%.1f ms_per_frame=%.3f\n",
                numThreads, iterations, (unsigned long long)checksum(workspace.out),
                loopMs, loopMs / iterations); 

    if (dump) {
        cv::imwrite("out_optimized_" + std::to_string(numThreads) + "t.png", workspace.out);
    }


    

    return 0;
}
