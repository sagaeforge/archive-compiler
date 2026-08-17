cmake_minimum_required(VERSION 3.20)
project(compiler VERSION 1.0 LANGUAGES CXX)

# C++ 표준 설정
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

if(NOT DEFINED CMAKE_CXX_STANDARD_REQUIRED)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)

# 빌드 타입이 지정되지 않은 경우 Release로 설정
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 컴파일러 경고 설정
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

# 라이브러리 빌드
add_subdirectory(src)

# 테스트 빌드
enable_testing()
add_subdirectory(test)