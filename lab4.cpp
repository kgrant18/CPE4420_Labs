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
*
* return: cv::Mat 
*
**************************************************************************/


cv::Mat generateGrayImage(cv::Mat& frame) {
    //create grayImage for storing gray frames/pixels
    cv::Mat grayImage(frame.rows, frame.cols, CV_8UC1);

    // to iterate over each pixel, need the x and y coordinates
    for (int y = 0; y < frame.rows; y++) {
        for (int x = 0; x < frame.cols; x++) {
            // Extract a pixel
            // cv::Vec3b pixel = frame.at<cv::Vec3b>(y, x);
            const cv::Vec3b* row_ptr = frame.ptr<cv::Vec3b>(y); 
            cv::Vec3b pixel = row_ptr[x];

            // OpenCV stores in BGR format
            unsigned char B = pixel[0];
            unsigned char G = pixel[1];
            unsigned char R = pixel[2];

            // apply grascale algorithm (ITU-R BT.709) 
            int gray = (54*R + 183*G + 19*B) >> 8;
            //int gray = 0.2126*R + 0.715*G + 0.0722*B;

            // put grayscale value into grayImage
            //grayImage.at<unsigned char>(y, x) = gray;
            unsigned char* row = grayImage.ptr<unsigned char>(y); 
            row[x] = gray;
        }
    }
    
    return grayImage; 
}

/**************************************************************************
* Function: performSobelOperation
* 
* Description: Performs the sobel operation on each frame for edge detection
*
* param grayImage: cv::Mat&: The gray scale version of the original 4k video
*
* return: cv::Mat 
*
**************************************************************************/


cv::Mat performSobelOperation(cv::Mat& grayImage) {
    //declare sobelImage for storing result
    cv::Mat sobelImage(grayImage.rows, grayImage.cols, CV_8UC1);
    
    for (int y = 1; y < grayImage.rows - 1; y++) {
        // get pointers to the 3 rows
        const unsigned char* row_above = grayImage.ptr<unsigned char>(y - 1);
        const unsigned char* row_curr = grayImage.ptr<unsigned char>(y); 
        const unsigned char* row_below = grayImage.ptr<unsigned char>(y + 1); 

        for (int x = 1 ; x < grayImage.cols - 1; x++) {
            // grab the 3x3 "neighborhood" of pixels
            int p00 = row_above[x - 1];
            int p01 = row_above[x]; 
            int p02 = row_above[x + 1];
            
            int p10 = row_curr[x - 1]; 
            int p11 = row_curr[x]; 
            int p12 = row_curr[x + 1]; 
            
            int p20 = row_below[x - 1];
            int p21 = row_below[x];
            int p22 = row_below[x + 1];

                                                                                                                                                                                                                             
            
            int Gx = (-1 * p00) + (1 * p02) + (-2 * p10) + (2 * p12) + (-1 * p20) + (1 * p22);
            int Gy = (1 * p00) + ( 2 * p01) + (1 * p02) + (-1 * p20) + (-2 * p21) + (-1 * p22);

            // magnitude approximation (don't want to square root --> expensive!) 
            int mag = std::abs(Gx) + std::abs(Gy);
            //int G = std::sqrt(std::pow(Gx, 2.0) + std::pow(Gy, 2.0));
            //int G = std::max(std::abs(Gx), std::abs(Gy)) + (std::min(std::abs(Gx), std::abs(Gy)) >> 1);

            // cap the magnitude at 255 
            if (mag > 255) {
                mag = 255;
            }

            //sobelImage.at<unsigned char>(y, x) = mag;
            unsigned char* row = sobelImage.ptr<unsigned char>(y); 
            row[x] = mag; 

        }
    }

    return sobelImage; 
}

/**************************************************************************
* Function: performStreamingSobel
* 
* Description: combines grayscale + sobel operations into one function using only 3 row-buffers
*              reduces the memory passes
*
* param grayImage: cv::Mat&: The current (color) frame being processed
*
* return: cv::Mat 
*
**************************************************************************/
cv::Mat performStreamingSobel(cv::Mat& frame) {
    const int rows = frame.rows;
    const int cols = frame.cols; 

    // 3 rotating buffers
    std::vector<uchar>bufA(cols), bufB(cols), bufC(cols);

    // buffer to store sobelImage
    cv::Mat sobelImage(rows, cols, CV_8UC1); 

    cv::Vec3b* srcRow0 = frame.ptr<cv::Vec3b>(0); 
    cv::Vec3b* srcRow1 = frame.ptr<cv::Vec3b>(1); 
    
    // points to address of gray row y-1
    uchar* prev = bufA.data();

    // points to address gray row y
    uchar* curr = bufB.data(); 

    // points to address gray row y+1
    uchar* next = bufC.data(); 

    for (int x = 0; x < cols; x++) {
        // converts first two rows to grayscale
        prev[x] = (54*srcRow0[x][2] + 183*srcRow0[x][1] + 19*srcRow0[x][0]) >> 8; 
        curr[x] = (54*srcRow1[x][2] + 183*srcRow1[x][1] + 19*srcRow1[x][0]) >> 8;
    }
    
    // main loop 
    for (int y = 2; y < rows; y++) {
        // convert color row y into the free buffer
        cv::Vec3b* src = frame.ptr<cv::Vec3b>(y); 
        for (int x = 0; x < cols; x++) {
            next[x] = (54*src[x][2] + 183*src[x][1] + 19*src[x][0]) >> 8; 
        }

        //prev=y-2, curr=y-1, next=y
        uchar* out = sobelImage.ptr<uchar>(y-1); 
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
            
            out[x] = G; 
        }
        
        // rotate oldest buffer 
        uchar* tmp = prev;
        prev = curr; 
        curr = next; 
        next = tmp; 
    }

    return sobelImage; 
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

int main() {
    //create openCV object
    cv::VideoCapture video("4k_video.mp4");
    
    // error checking for safety
    if (!video.isOpened()) {
        std::cerr << "Could not open video\n";
        return 1;
    }

    // ensures the output cv video is fullscreen and not any larger (problem I had initially)
    cv::namedWindow("Sobel", cv::WINDOW_NORMAL);
    cv::setWindowProperty("Sobel", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);

    cv::Mat frame; 

    //calculate FPS of the filter
    int64 start = cv::getTickCount();
    long frames = 0; 
    const long N = 800;

    int64 grayTicks = 0; 
    int64 sobelTicks = 0;
    int64 streamTicks = 0; 

    // read each frame
    while (video.read(frame) && frames < N) {
        int64 t1 = cv::getTickCount();
        cv::Mat sobelImage = performStreamingSobel(frame); 
        int64 t2 = cv::getTickCount(); 
       
        streamTicks += t2 - t1; 
        frames++;
        
        // this is turned off when measuring performance
        //cv::imshow("Sobel", sobelImage);  

        // Wait 1 ms for a key press
        //if (cv::waitKey(1) == 'q') {
        //  break;
        //}
    }

    int64 end = cv::getTickCount();
    
    double freq = cv::getTickFrequency();                                                                       
    double elapsedSec = (end - start) / freq;                                                                   
    double fps = frames / elapsedSec;                                                                           
    double streamAvg = (double)(streamTicks / frames) / freq;                                                   
                                                                                                                 
    std::cout << "FPS: " << fps << std::endl;                                                                   
    std::cout << "Avg stream time: " << streamAvg * 1000.0 << "ms" << std::endl;                                
    std::cout << "Avg total time: " << (elapsedSec / frames) * 1000.0 << " ms" << std::endl;                    
    std::cout << "Stream time total: " << (streamTicks / freq) << "s" << std::endl;                             
    std::cout << "Total time: " << elapsedSec << " s" << std::endl;
/*
    double freq = cv::getTickFrequency(); 
    double elapsedSec = (end - start) / freq;
    double fps = frames / elapsedSec;
    double grayAvg = (double)(grayTicks / frames) / freq; 
    double sobelAvg = (double)(sobelTicks / frames) / freq; 
    
    std::cout << "FPS: " << fps << std::endl; 
    std::cout << "Avg gray time: " << grayAvg * 1000.0 << "ms" << std::endl;
    std::cout << "Avg sobel time: " << sobelAvg * 1000.0 << "ms" << std::endl;
    std::cout << "Avg total time: " << (elapsedSec / frames) * 1000.0 << " ms" << std::endl;
    std::cout << "\nGrayscale time: " << (grayTicks / freq) << "s" << std::endl; 
    std::cout << "Sobel time: " << (sobelTicks / freq) << "s" << std::endl; 
    std::cout << "Total time: " << ((end - start) / freq) << " s" << std::endl; */

    return 0;
}
