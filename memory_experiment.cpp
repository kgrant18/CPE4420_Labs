#include <cmath>
#include <chrono>
#include <iostream>
#include <cstdint> 

int main(void) {
    constexpr int rows = 10000; 
    constexpr int columns = 10000; 
    constexpr double elements = static_cast<double>(rows) * columns;

    static std::uint32_t test_list[rows][columns] = {};
    
    // initialize the 2D array
    int val = 0; 
    for (int row = 0; row < rows; row++) {
        for (int column = 0; column < columns; column++) {
            test_list[row][column] = val; 
            val++; 
        }
    }
   
    //row-column traversal
    long long sum1 = 0; 
    auto start1 = std::chrono::steady_clock::now(); 
    // process elements in the 2D array
    for (int row = 0; row < rows; row++) {
        for (int column = 0; column < columns; column++) {
            sum1 += test_list[row][column];
        }
    }
    auto end1 = std::chrono::steady_clock::now(); 
    std::chrono::duration<double, std::milli> elapsed1 = end1 - start1;
    
    //column-row traversal 
    long long sum2 = 0; 
    auto start2 = std::chrono::steady_clock::now(); 
    for (int column = 0; column < columns; column++) {
        for (int row = 0; row < rows; row++) {
            sum2 += test_list[row][column];
        }
    }

    auto end2 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> elapsed2 = end2- start2;

    // calculate average time per element
    double average1_ns = (elapsed1.count() * 1'000'000.0) / elements; 
    double average2_ns = (elapsed2.count() * 1'000'000.0) / elements; 

    std::cout << "Row-Column Traversal: " << std::endl;
    std::cout << "\tChecksum 1: " << sum1 << std::endl;
    std::cout << "\tAverage time per element: " << average1_ns << " ns" << std::endl; 
    std::cout << "\tTime Elapsed 1: " << elapsed1.count() << " ms\n" << std::endl; 
    
    std::cout << "Column-Row Traversal: " << std::endl;
    std::cout << "\tChecksum 2: " << sum2 << std::endl; 
    std::cout << "\tAverage time per element: " << average2_ns << " ns" << std::endl;
    std::cout << "\tTime Elapsed 2: " << elapsed2.count() << " ms" << std::endl; 


}
