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

cv::Mat generateGrayImage(cv::Mat& image);
cv::Mat performSobelOperation(cv::Mat& grayImage); 

#endif // _SOBEL_FILTER_H
