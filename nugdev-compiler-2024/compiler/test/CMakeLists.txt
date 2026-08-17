cmake_minimum_required(VERSION 3.25)
project(nugdev-compiler-test)

add_executable(
    ${PROJECT_NAME}
    
    ${CMAKE_CURRENT_SOURCE_DIR}/interfafce/TestBase.h
    ${CMAKE_CURRENT_SOURCE_DIR}/interfafce/TestBase.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/01_tokenize/TokenTestFixture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/01_tokenize/TokenTestFixture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/01_tokenize/integration/IntegrationTest.cpp
)

target_include_directories(
    ${PROJECT_NAME}
PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../test
    ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME}
PRIVATE
    nugdev-compiler-lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})