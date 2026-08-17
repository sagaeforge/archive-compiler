cmake_minimum_required(VERSION 3.25)
project(nugdev::compiler::test)

enable_testing()

add_executable(${PROJECT_NAME}
    test.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    nugdev::compiler
)