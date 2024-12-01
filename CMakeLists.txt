cmake_minimum_required(VERSION 3.29)
project(OS_Lab2)

set(CMAKE_CXX_STANDARD 20)

# Dynamic library
add_library(lab2_cache SHARED API/lab2_cache.cpp)

# io-lat-write
add_executable(OS_Lab2_write io-lat-write.cpp
        Bench/IOLatencyWriteBenchmark.cpp
        Bench/IOLatencyWriteBenchmark.h)
target_link_libraries(OS_Lab2_write lab2_cache)
target_include_directories(OS_Lab2_write PRIVATE API)

# io-lat-read
add_executable(OS_Lab2_read io-lat-read.cpp
        Bench/IOLatencyReadBenchmark.cpp
        Bench/IOLatencyReadBenchmark.h)
target_link_libraries(OS_Lab2_read lab2_cache)
target_include_directories(OS_Lab2_read PRIVATE API)
