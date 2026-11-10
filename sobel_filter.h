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
#include <chrono>
#include <cstdio>
#include <string>

struct RowBuffers {
    std::vector<uchar> prev, curr, next;    // one set per thread
}; 

struct Workspace {
    cv:: Mat out;                         // zeroes, allocated once
    std::vector<RowBuffers> threadBuffers;     // row buffers, allocated once
};

void initWorkspace(Workspace &workspace, int rows, int columns, int numThreads); 
void generateGrayImage(const cv::Mat& frame, cv::Mat& gray); 
void performSobelOperation(const cv::Mat& gray, cv::Mat& out); 
void performStreamingSobel(const cv::Mat& frame, cv::Mat& out, RowBuffers& bufs, int startRow, int endRow);
void assignWorkersForSobel(const cv::Mat& frame, Workspace& ws, int numThreads); 
bool loadFrame(const std::string& file_path, cv::Mat& frame); 
uint64_t checksum(const cv::Mat& image); 

#endif // _SOBEL_FILTER_H
