cmake_minimum_required(VERSION 3.25)
project(nugdev_compiler)

add_library(
    ${PROJECT_NAME}_lib
    
    src/00_app/repl/repl.cpp
    src/00_app/repl/repl.h
    src/00_app/lib/DontCopyable.h
    src/00_app/lib/PointerHelper.hpp
    src/00_app/lib/Singleton.h
    src/00_app/stream/Stream.hpp
    src/00_app/stream/Stream.cpp
    
    src/01_tokenize/Tokenizer.h
    src/01_tokenize/Tokenizer.cpp
    src/01_tokenize/Token.h
    src/01_tokenize/Token.cpp
    src/01_tokenize/TokenFactory.h
    src/01_tokenize/factory/IdentifierTokenFactory.h
    src/01_tokenize/factory/IdentifierTokenFactory.cpp
    src/01_tokenize/factory/OperatorTokenFactory.h
    src/01_tokenize/factory/OperatorTokenFactory.cpp
    src/01_tokenize/factory/KeywordTokenFactory.h
    src/01_tokenize/factory/KeywordTokenFactory.cpp
    src/01_tokenize/factory/NumberTokenFactory.h
    src/01_tokenize/factory/NumberTokenFactory.cpp
    src/01_tokenize/factory/StringTokenFactory.h
    src/01_tokenize/factory/StringTokenFactory.cpp

    src/02_parsing/Parser.h
    src/02_parsing/ParseStrategy.h
    src/02_parsing/ast/AST.h
    src/02_parsing/ast/AST.cpp
    src/02_parsing/ast/ASTNodeAspect.h
    src/02_parsing/ast/ASTNodeVisitor.h

    src/02_parsing/ast/module/program/ProgramNode.h
    src/02_parsing/ast/module/program/ProgramNode.cpp
    src/02_parsing/ast/module/program/ProgramNodeParseStrategy.h
    src/02_parsing/ast/module/program/ProgramNodeParseStrategy.cpp

    src/02_parsing/ast/statement/StatementParseStrategy.h
    src/02_parsing/ast/statement/StatementParseStrategy.cpp
    src/02_parsing/ast/statement/break/BreakNode.h
    src/02_parsing/ast/statement/break/BreakNode.cpp
    src/02_parsing/ast/statement/break/BreakNodeParseStrategy.h
    src/02_parsing/ast/statement/break/BreakNodeParseStrategy.cpp
    src/02_parsing/ast/statement/continue/ContinueNode.h
    src/02_parsing/ast/statement/continue/ContinueNode.cpp
    src/02_parsing/ast/statement/continue/ContinueNodeParseStrategy.h
    src/02_parsing/ast/statement/continue/ContinueNodeParseStrategy.cpp
    src/02_parsing/ast/statement/return/ReturnNode.h
    src/02_parsing/ast/statement/return/ReturnNode.cpp
    src/02_parsing/ast/statement/return/ReturnNodeParseStrategy.h
    src/02_parsing/ast/statement/return/ReturnNodeParseStrategy.cpp
    src/02_parsing/ast/statement/let/LetNode.h
    src/02_parsing/ast/statement/let/LetNode.cpp
    src/02_parsing/ast/statement/let/LetNodeParseStrategy.h
    src/02_parsing/ast/statement/let/LetNodeParseStrategy.cpp
    src/02_parsing/ast/statement/expression/ExpressionStatementNode.h
    src/02_parsing/ast/statement/expression/ExpressionStatementNode.cpp
    src/02_parsing/ast/statement/expression/ExpressionStatementNodeParseStrategy.h
    src/02_parsing/ast/statement/expression/ExpressionStatementNodeParseStrategy.cpp
    src/02_parsing/ast/statement/block/BlockStatementNode.h
    src/02_parsing/ast/statement/block/BlockStatementNode.cpp
    src/02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h
    src/02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.cpp
)

add_executable(
    ${PROJECT_NAME}_exec
    
    src/00_app/main.cpp
)

add_executable(
    ${PROJECT_NAME}_test

    test/test.cpp
    
    test/01_tokenize/token_test_case.h
    test/01_tokenize/token_test_case.cpp

    test/01_tokenize/factory_test/identifier_test.cpp
    test/01_tokenize/factory_test/operator_test.cpp
    test/01_tokenize/factory_test/keyword_test.cpp
    test/01_tokenize/factory_test/string_test.cpp
    test/01_tokenize/factory_test/number_test.cpp

    test/01_tokenize/01_init_token/tokenizer_test.cpp

    src/02_parsing/ast/module/program/test/create_test.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME}_lib
PUBLIC
    ICU::uc
    ICU::i18n
    ICU::data
    rapidjson
)

target_link_libraries(
    ${PROJECT_NAME}_test
PRIVATE
    ${PROJECT_NAME}_lib
    GTest::gtest_main
)

target_link_libraries(
    ${PROJECT_NAME}_exec
PRIVATE
    ${PROJECT_NAME}_lib
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_test)