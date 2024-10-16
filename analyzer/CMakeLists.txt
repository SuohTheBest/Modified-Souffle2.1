cmake_minimum_required(VERSION 3.10)
project(analyzer)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(analyzer
        ProofTreeBuilder.cpp
        ProofTreeBuilder.h main.cpp)
