cmake_minimum_required ( VERSION 3.25 )
project(
    "Compiler"
    VERSION 0.1
    DESCRIPTION "A simple compiler"
    LANGUAGES CXX
)

# cxx standard
set(CMAKE_CXX_STANDARD 20)

# set the output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# add subdirectories
add_subdirectory(./src)
