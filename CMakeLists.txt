cmake_minimum_required(VERSION 3.17)
project(MachOBuilder VERSION 1.0.0 DESCRIPTION "Mach-O file builder interface" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(DEFAULT_BUILD_TYPE "Release")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wpedantic -Wall -Wextra")
SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")

set(SOURCE_FILES src/src/loadCommands.cpp
        src/src/machoBinFile.cpp
        src/src/machoStructure.cpp
        src/src/binaryFile.cpp
        src/src/fastList.cpp
        src/src/relocateStruct.cpp
        src/src/stringTable.cpp
        src/src/objectGenerator.cpp
        src/include/hash/hashes.cpp
        )


set(PUBLIC_INCLUDE_DIRECTORIES src/include/ src/include/public)
set(PRIVATE_INCLUDE_DIRECTORIES "src/src/ src/include/ src/include/public")
set(MY_PUBLIC_HEADERS "src/include/public/MachOBuilder.h src/include/public/FastList.h")

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE ${PRIVATE_INCLUDE_DIRECTORIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${PUBLIC_INCLUDE_DIRECTORIES})

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION 1)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "src/include/public/MachOBuilder.h;src/include/public/FastList.h")

install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_BINDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

add_custom_command(TARGET ${PROJECT_NAME}
        PRE_BUILD
        COMMAND cd ${CMAKE_SOURCE_DIR}/src/include/public && python3 publicHeaderGen.py)

add_subdirectory(examples)