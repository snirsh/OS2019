cmake_minimum_required(VERSION 3.13)
project(ex3)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS -pthread)

include_directories(.)

add_executable(ex3
        Barrier.cpp
        Barrier.h
        MapReduceClient.h
        MapReduceFramework.cpp
        MapReduceFramework.h
        SampleClient.cpp)
