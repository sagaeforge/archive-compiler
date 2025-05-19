add_executable(compiler_test
    test_main.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/StringTest.cpp
)

target_link_libraries(compiler_test
    PRIVATE
        compiler_lib
        GTest::gtest_main
)

target_include_directories(compiler_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

include(GoogleTest)
gtest_discover_tests(compiler_test) 