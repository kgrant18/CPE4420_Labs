/**************************************************************************
* File: sobel_filter.cpp
* 
* Description: Reads each frame of a 4K video @ 60Hz, converts it to
grayscale, then applies a Sobel edge-detection filter for edge detection
* 
* Author: Kevin Grant
*
* Revision history
* 
**************************************************************************/

#ifndef _SOBEL_FILTER_H
#define _SOBEL_FILTER_H

#include <opencv2/opencv.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>

cv::Mat generateGrayImage(cv::Mat& image);
cv::Mat performSobelOperation(cv::Mat& grayImage); 
void performStreamingSobel(const cv::Mat &frame, cv::Mat &sobelImage, int startRow, int endRow, int64 &grayTicks, int64 &sobelTicks);
cv::Mat assignWorkersForSobel(const cv::Mat &frame, int numThreads, int64 &grayTicks, int64 &sobelTicks); 

#endif // _SOBEL_FILTER_H
