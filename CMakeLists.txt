
# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.10)

# Project's name
project(Linkito)

set(CMAKE_CXX_FLAGS -std=c++17)

add_compile_options(-Wno-comment -Wno-writable-strings -g)

# Set the output folder where your program will be created
# set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
# set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})
# 

add_executable(linkito Common.cpp Macho.cpp Debugger.cpp Tester.cpp LoadCommands.cpp ExecutableFileBuilder.cpp ExecutableFileBuilder3.cpp MachoParser.cpp main.cpp)

add_executable(empty empty.c)
