cmake_minimum_required(VERSION 3.25)
project(nugdev-compiler-app)

add_executable(nugdev-compiler-app ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(nugdev-compiler-app nugdev-compiler-lib)
target_include_directories(nugdev-compiler-app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../lib)

