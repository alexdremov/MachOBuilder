cmake_minimum_required(VERSION 3.17)
project(MachOBuilderExamples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(DEFAULT_BUILD_TYPE "Release")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wpedantic -Wall -Wextra -I/usr/local/include")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -I/usr/local/include")

SET(CMAKE_EXE_LINKER_FLAGS "-L/usr/local/lib")

add_executable(returnZeroApp
        returnZeroApp.cpp)

target_link_libraries(returnZeroApp MachOBuilder)