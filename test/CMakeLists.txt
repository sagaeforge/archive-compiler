add_executable(compiler_test
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/StringTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/01_lib/CharTest.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/02_scanning/ScannerTest.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenConverterTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenizerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenizerErrorTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenizerEdgeCaseTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/03_tokenize/TokenizerComprehensiveTest.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ASTNodeTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/LiteralsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ExpressionsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/VisitorTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/FactoryTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/WhenConditionsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ComplexExpressionsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/AdvancedExpressionsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ControlFlowTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/StatementsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ProgramTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ImportExportTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/04_parsing/ParametersTypesTest.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/sample.cpp
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