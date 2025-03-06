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
    src/00_app/tag/Tag.h
    src/00_app/tag/Tag.cpp
    
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
    src/02_parsing/Parser.cpp
    src/02_parsing/ParseStrategy.h
    src/02_parsing/ast/AST.h
    src/02_parsing/ast/AST.cpp
    src/02_parsing/ast/ASTNodeAspect.h
    src/02_parsing/ast/ASTNodeVisitor.h
    src/02_parsing/ast/ASTNodeVisitor.cpp
    src/02_parsing/ParseStrategy.h
    src/02_parsing/ParseStrategy.cpp

    src/02_parsing/ast/module/program/ProgramNode.h
    src/02_parsing/ast/module/program/ProgramNode.cpp
    src/02_parsing/ast/module/program/ProgramNodeParseStrategy.h
    src/02_parsing/ast/module/program/ProgramNodeParseStrategy.cpp

    src/02_parsing/ast/statement/StatementParseStrategy.h
    src/02_parsing/ast/statement/StatementParseStrategy.cpp
    src/02_parsing/ast/statement/break/BreakStatementNode.h
    src/02_parsing/ast/statement/break/BreakStatementNode.cpp
    src/02_parsing/ast/statement/break/BreakStatementNodeParseStrategy.h
    src/02_parsing/ast/statement/break/BreakStatementNodeParseStrategy.cpp
    src/02_parsing/ast/statement/continue/ContinueStatementNode.h
    src/02_parsing/ast/statement/continue/ContinueStatementNode.cpp
    src/02_parsing/ast/statement/continue/ContinueStatementNodeParseStrategy.h
    src/02_parsing/ast/statement/continue/ContinueStatementNodeParseStrategy.cpp
    src/02_parsing/ast/statement/return/ReturnStatementNode.h
    src/02_parsing/ast/statement/return/ReturnStatementNode.cpp
    src/02_parsing/ast/statement/return/ReturnStatementNodeParseStrategy.h
    src/02_parsing/ast/statement/return/ReturnStatementNodeParseStrategy.cpp
    src/02_parsing/ast/statement/let/LetStatementNode.h
    src/02_parsing/ast/statement/let/LetStatementNode.cpp
    src/02_parsing/ast/statement/let/LetStatementNodeParseStrategy.h
    src/02_parsing/ast/statement/let/LetStatementNodeParseStrategy.cpp
    src/02_parsing/ast/statement/expression/ExpressionStatementNode.h
    src/02_parsing/ast/statement/expression/ExpressionStatementNode.cpp
    src/02_parsing/ast/statement/expression/ExpressionStatementNodeParseStrategy.h
    src/02_parsing/ast/statement/expression/ExpressionStatementNodeParseStrategy.cpp
    src/02_parsing/ast/statement/block/BlockStatementNode.h
    src/02_parsing/ast/statement/block/BlockStatementNode.cpp
    src/02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h
    src/02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.cpp
    src/02_parsing/ast/statement/for/ForStatementNode.h
    src/02_parsing/ast/statement/for/ForStatementNode.cpp
    src/02_parsing/ast/statement/for/ForStatementNodeParseStrategy.h
    src/02_parsing/ast/statement/for/ForStatementNodeParseStrategy.cpp

    src/02_parsing/ast/expression/ExpressionParseStrategy.h
    src/02_parsing/ast/expression/ExpressionParseStrategy.cpp
    src/02_parsing/ast/expression/GroupExpressionParseStrategy.h
    src/02_parsing/ast/expression/GroupExpressionParseStrategy.cpp
    src/02_parsing/ast/expression/array/ArrayLiteralNode.h
    src/02_parsing/ast/expression/array/ArrayLiteralNode.cpp
    src/02_parsing/ast/expression/array/ArrayLiteralNodeParseStrategy.h
    src/02_parsing/ast/expression/array/ArrayLiteralNodeParseStrategy.cpp
    src/02_parsing/ast/expression/boolean/BooleanLiteralNode.h
    src/02_parsing/ast/expression/boolean/BooleanLiteralNode.cpp
    src/02_parsing/ast/expression/boolean/BooleanLiteralNodeParseStrategy.h
    src/02_parsing/ast/expression/boolean/BooleanLiteralNodeParseStrategy.cpp
    src/02_parsing/ast/expression/identifier/IdentifierLiteralNode.h
    src/02_parsing/ast/expression/identifier/IdentifierLiteralNode.cpp
    src/02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h
    src/02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.cpp
    src/02_parsing/ast/expression/number/NumberLiteralNode.h
    src/02_parsing/ast/expression/number/NumberLiteralNode.cpp
    src/02_parsing/ast/expression/number/NumberLiteralNodeParseStrategy.h
    src/02_parsing/ast/expression/number/NumberLiteralNodeParseStrategy.cpp
    src/02_parsing/ast/expression/string/StringLiteralNode.h
    src/02_parsing/ast/expression/string/StringLiteralNode.cpp
    src/02_parsing/ast/expression/string/StringLiteralNodeParseStrategy.h
    src/02_parsing/ast/expression/string/StringLiteralNodeParseStrategy.cpp
    src/02_parsing/ast/expression/call/CallExpressionNode.h
    src/02_parsing/ast/expression/call/CallExpressionNode.cpp
    src/02_parsing/ast/expression/call/CallExpressionNodeParseStrategy.h
    src/02_parsing/ast/expression/call/CallExpressionNodeParseStrategy.cpp
    src/02_parsing/ast/expression/function/FunctionExpressionNode.h
    src/02_parsing/ast/expression/function/FunctionExpressionNode.cpp
    src/02_parsing/ast/expression/function/FunctionExpressionNodeParseStrategy.h
    src/02_parsing/ast/expression/function/FunctionExpressionNodeParseStrategy.cpp
    src/02_parsing/ast/expression/if/IfExpressionNode.h
    src/02_parsing/ast/expression/if/IfExpressionNode.cpp
    src/02_parsing/ast/expression/if/IfExpressionNodeParseStrategy.h
    src/02_parsing/ast/expression/if/IfExpressionNodeParseStrategy.cpp
    src/02_parsing/ast/expression/index/IndexExpressionNode.h
    src/02_parsing/ast/expression/index/IndexExpressionNode.cpp
    src/02_parsing/ast/expression/index/IndexExpressionNodeParseStrategy.h
    src/02_parsing/ast/expression/index/IndexExpressionNodeParseStrategy.cpp
    src/02_parsing/ast/expression/infix/InfixExpressionNode.h
    src/02_parsing/ast/expression/infix/InfixExpressionNode.cpp
    src/02_parsing/ast/expression/infix/InfixExpressionNodeParseStrategy.h
    src/02_parsing/ast/expression/infix/InfixExpressionNodeParseStrategy.cpp
    src/02_parsing/ast/expression/prefix/PrefixExpressionNode.h
    src/02_parsing/ast/expression/prefix/PrefixExpressionNode.cpp
    src/02_parsing/ast/expression/prefix/PrefixExpressionNodeParseStrategy.h
    src/02_parsing/ast/expression/prefix/PrefixExpressionNodeParseStrategy.cpp
    src/02_parsing/ast/expression/when/WhenExpressionNode.h
    src/02_parsing/ast/expression/when/WhenExpressionNode.cpp
    src/02_parsing/ast/expression/when/WhenExpressionNodeParseStrategy.h
    src/02_parsing/ast/expression/when/WhenExpressionNodeParseStrategy.cpp
    src/02_parsing/ast/expression/post/PostExpressionNode.h
    src/02_parsing/ast/expression/post/PostExpressionNode.cpp
    src/02_parsing/ast/expression/post/PostExpressionNodeParseStrategy.h
    src/02_parsing/ast/expression/post/PostExpressionNodeParseStrategy.cpp

    src/04_generation/opcode/BytecodeGenerator.h
    src/04_generation/opcode/BytecodeGenerator.cpp
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

    test/02_parsing/parser_test_case.h
    test/02_parsing/parser_test_case.cpp
    test/02_parsing/let/let_test.cpp
    test/02_parsing/call/call_test.cpp
    test/02_parsing/literal/boolean/boolean_test.cpp
    test/02_parsing/literal/number/number_test.cpp
    test/02_parsing/literal/string/string_test.cpp
    test/02_parsing/literal/array/array_test.cpp
    test/02_parsing/literal/function/function_test.cpp
    test/02_parsing/literal/identifier/identifier_test.cpp
    test/02_parsing/if/if_test.cpp
    test/02_parsing/index/index_test.cpp
    test/02_parsing/infix/infix_test.cpp
    test/02_parsing/prefix/prefix_test.cpp
    test/02_parsing/break/break_test.cpp
    test/02_parsing/continue/continue_test.cpp
    test/02_parsing/return/return_test.cpp
    test/02_parsing/for/for_test.cpp
    test/02_parsing/when/when_test.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/test
    ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME}_lib
PUBLIC
    ICU::uc
    ICU::i18n
    ICU::data
    rapidjson
    stduuid
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