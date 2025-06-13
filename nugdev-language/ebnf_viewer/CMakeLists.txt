cmake_minimum_required(VERSION 3.15)
project(ebnf_viewer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SFML Configuration
set(SFML_DIR "${CMAKE_SOURCE_DIR}/SFML-3.0.0/lib/cmake/SFML")
set(SFML_ROOT "${CMAKE_SOURCE_DIR}/SFML-3.0.0")
set(SFML_INCLUDE_DIR "${SFML_ROOT}/include")
set(SFML_LIBRARY_DIR "${SFML_ROOT}/lib")

# Find SFML
find_package(SFML 3.0 COMPONENTS Graphics Window System REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SFML_INCLUDE_DIR}
)

# Link directories
link_directories(${SFML_LIBRARY_DIR})

# Source files
set(SOURCES
    src/main.cpp
    src/parser.cpp
    src/ast.cpp
    src/viewer.cpp
) 

# Header files
set(HEADERS
    include/parser.hpp
    include/ast.hpp
    include/viewer.hpp
)

# Create executable 
add_executable(ebnf_viewer ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(ebnf_viewer PRIVATE 
    ${SFML_LIBRARY_DIR}/sfml-graphics.lib
    ${SFML_LIBRARY_DIR}/sfml-window.lib
    ${SFML_LIBRARY_DIR}/sfml-system.lib
) 