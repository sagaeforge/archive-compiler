project(compiler_01_lib)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/StringTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CharTest.cpp
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        compiler_lib
        GTest::gtest_main
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME} TEST_PREFIX "01_lib.")