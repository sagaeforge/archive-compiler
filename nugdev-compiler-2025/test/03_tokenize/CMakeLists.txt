project(compiler_03_tokenize)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/TokenConverterTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TokenTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TokenizerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TokenizerErrorTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TokenizerEdgeCaseTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TokenizerComprehensiveTest.cpp
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
gtest_discover_tests(${PROJECT_NAME} TEST_PREFIX "03_tokenize.")