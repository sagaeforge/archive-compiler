project(compiler_04_parsing)

add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/ASTNodeTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/LiteralsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/ExpressionsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/VisitorTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/FactoryTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/WhenConditionsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/ComplexExpressionsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/AdvancedExpressionsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/ControlFlowTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/StatementsTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/ProgramTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/ImportExportTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ast/ParametersTypesTest.cpp
    
    # Parser tests organized by functionality
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/ParserBaseTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/LiteralParsingTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/ExpressionParsingTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/StatementParsingTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/ControlFlowParsingTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/TypeParsingTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/ImportExportParsingTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/AdvancedExpressionParsingTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/StructInterfaceParsingTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/parsing/ErrorCoverageTest.cpp
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
gtest_discover_tests(${PROJECT_NAME} TEST_PREFIX "04_parsing.")