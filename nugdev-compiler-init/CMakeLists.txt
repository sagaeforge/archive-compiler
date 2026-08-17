cmake_minimum_required(VERSION 3.25)
project(nugdev::compiler)

# Set C++20 as the standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
elseif(CMAKE_CXX_STANDARD LESS 20)
    message(FATAL_ERROR "nugdev::compiler requires at least C++20")
endif()

# clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
