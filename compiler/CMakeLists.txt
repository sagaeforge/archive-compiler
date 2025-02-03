cmake_minimum_required(VERSION 3.25)
project(nugdev_compiler)

add_library(
    ${PROJECT_NAME}_lib
    
    src/00_app/repl/repl.cpp
    src/00_app/repl/repl.h
    src/00_app/lib/DontCopyable.h
    src/00_app/lib/PointerHelper.hpp
    src/00_app/lib/Singleton.h
)

add_executable(
    ${PROJECT_NAME}_exec
    
    src/00_app/main.cpp
)

add_executable(
    ${PROJECT_NAME}_test

    test/test.cpp
)

target_include_directories(
    ${PROJECT_NAME}_lib
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(
    ${PROJECT_NAME}_exec
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_include_directories(
    ${PROJECT_NAME}_test
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test
    ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME}_lib
PRIVATE
    ICU::uc
    ICU::i18n
    ICU::data
)

target_link_libraries(
    ${PROJECT_NAME}_test
PRIVATE
    GTest::gtest_main
)

target_link_libraries(
    ${PROJECT_NAME}_exec
PRIVATE
    ${PROJECT_NAME}_lib
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_test)