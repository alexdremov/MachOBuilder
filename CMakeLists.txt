cmake_minimum_required(VERSION 3.17)
project(MachOBuilder LANGUAGES CXX)

SET(CMAKE_CXX_FLAGS_DEBUG      "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wpedantic -Wall -Wextra")
SET(CMAKE_CXX_FLAGS_RELEASE    "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")
set(CMAKE_CXX_STANDARD 20)

set(PROJECT_VERSION "1.0.0")

add_executable(returnZeroApp examples/returnZeroApp.cpp)
include_directories(src)