cmake_minimum_required(VERSION 3.19)
project(physc)

set(CMAKE_CXX_STANDARD 17)

set(CXX_FLAGS "-O2 -Wall -ljemalloc")

add_executable(tool tool.cpp)
