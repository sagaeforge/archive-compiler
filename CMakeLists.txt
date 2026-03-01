cmake_minimum_required(VERSION 3.20)
project(kern LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Warnings
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# Include paths
include_directories(${CMAKE_SOURCE_DIR}/include)

# Libraries
add_subdirectory(lib/Support)
add_subdirectory(lib/Lexer)
add_subdirectory(lib/Parser)
add_subdirectory(lib/Sema)
add_subdirectory(lib/IR)
add_subdirectory(lib/CodeGen)
add_subdirectory(lib/HIR)
add_subdirectory(lib/LIR)

# Compiler driver
add_subdirectory(tools/kernc)

# Testing
option(KERN_BUILD_TESTS "Build tests" ON)
if(KERN_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()
