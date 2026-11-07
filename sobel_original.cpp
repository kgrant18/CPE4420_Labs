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

/**************************************************************************
* Function: generateGrayImage
* 
* Description: Converts each frame into a gray scale value
*
* param frame: cv::Mat&: The current frame to be converted into gray scale 
* param gray: cv::Mat&: the output grayImage that feeds into sobel operation
*
* return: cv::Mat 
*
**************************************************************************/


void generateGrayImage(cv::Mat& frame, cv::Mat& gray) {
    // to iterate over each pixel, need the x and y coordinates
    for (int y = 0; y < frame.rows; y++) {
        for (int x = 0; x < frame.cols; x++) {
            // Extract a pixel
            cv::Vec3b pixel = frame.at<cv::Vec3b>(y, x);

            // OpenCV stores in BGR format
            unsigned char B = pixel[0];
            unsigned char G = pixel[1];
            unsigned char R = pixel[2];

            // apply grascale algorithm (ITU-R BT.709) 
            int g = (54*R + 183*G + 19*B) >> 8;
            //int gray = 0.2126*R + 0.715*G + 0.0722*B;

            // put grayscale value into grayImage
            gray.at<unsigned char>(y, x) = static_cast<uchar>(g);
            //unsigned char* row = grayImage.ptr<unsigned char>(y); 
        }
    }
     
}

/**************************************************************************
* Function: performSobelOperation
* 
* Description: Performs the sobel operation on each frame for edge detection
*
* param gray: cv::Mat&: The gray scale version of the original 4k video
* param out        cv::Mat&: The output image
*
* return: cv::Mat 
*
**************************************************************************/

void performSobelOperation(cv::Mat& gray, cv::Mat& out) {
    
    for (int y = 1; y < gray.rows - 1; y++) {
        // get pointers to the 3 rows
         for (int x = 1 ; x < gray.cols - 1; x++) {
            // grab the 3x3 "neighborhood" of pixels
            int p00 = gray.at<uchar>(y - 1, x - 1);                                                            
            int p01 = gray.at<uchar>(y - 1, x);                                                                
            int p02 = gray.at<uchar>(y - 1, x + 1);                                                            
                                                                                                                     
            int p10 = gray.at<uchar>(y,     x - 1);                                                            
            int p11 = gray.at<uchar>(y,     x);                                                                
            int p12 = gray.at<uchar>(y,     x + 1);                                                            
                                                                                                                    
            int p20 = gray.at<uchar>(y + 1, x - 1);                                                            
            int p21 = gray.at<uchar>(y + 1, x);                                                                
            int p22 = gray.at<uchar>(y + 1, x + 1);

            int Gx = (-1 * p00) + (1 * p02) + (-2 * p10) + (2 * p12) + (-1 * p20) + (1 * p22);
            int Gy = (1 * p00) + ( 2 * p01) + (1 * p02) + (-1 * p20) + (-2 * p21) + (-1 * p22);

            // magnitude approximation (don't want to square root --> expensive!) 
            int mag = std::abs(Gx) + std::abs(Gy);

            // cap the magnitude at 255 
            if (mag > 255) {
                mag = 255;
            }

            out.at<unsigned char>(y, x) = mag;

        }
    }

}

/**************************************************************************
* Function: checksum
* 
* Description: 64-bit FNV-la checksum function applied to every byte of an image
*              in row-major order to detect if the image has changed (adopted 
*              from Claude Code)
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
    int iterations = 30; 
    std::string input = "4k_video.mp4"; 
    bool dump = false; 

    // handle command line arguments
    for (int i = 1; i < argc; i++) {
        std::string argument = argv[i]; 

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

    // initialize grayImage and outputImage outside loop
    cv::Mat gray(frame.rows, frame.cols, CV_8UC1); 
    cv::Mat out = cv::Mat::zeros(frame.rows, frame.cols, CV_8UC1); 

    // loop through and perform function calls
    auto t0 = std::chrono::steady_clock::now(); 
    for (int i = 0; i < iterations; i++) {
        generateGrayImage(frame, gray); 
        performSobelOperation(gray, out); 
    }
    auto t1 = std::chrono::steady_clock::now(); 

    // time in ms
    double loopMs = std::chrono::duration<double, std::milli>(t1 - t0).count(); 
    std::printf("config=original threads=1 iterations=%d checksum=%016llx "   
                "loop_ms=%.1f ms_per_frame=%.3f\n",
                iterations, (unsigned long long)checksum(out),
                loopMs, loopMs / iterations); 

    if (dump) {
        cv::imwrite("out_original_" + std::to_string(numThreads) + "t.png", out);
    }

    return 0;
}

