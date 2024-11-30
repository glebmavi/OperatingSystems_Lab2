cmake_minimum_required(VERSION 3.29)
project(OS_Lab2)

set(CMAKE_CXX_STANDARD 20)

add_executable(OS_Lab2 main.cpp
        API/lab2_cache.cpp
        API/lab2_cache.h
        Bench/IOLatencyWriteBenchmark.cpp
        Bench/IOLatencyWriteBenchmark.h)
