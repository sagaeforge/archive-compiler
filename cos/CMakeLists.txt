cmake_minimum_required(VERSION 4.0)
project(cos)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

cmake_policy(SET CMP0077 NEW)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


add_library(${PROJECT_NAME}_lib
        src/Logger.hpp
        src/Common.h
        src/00_core/Callable.h
        src/00_core/Comparable.h
        src/00_core/Pointerable.h
        src/00_core/Printable.hpp
        src/00_Core/nlohmann/json.hpp
        src/00_Core/nlohmann/json_fwd.hpp
        src/00_Core/magic_enum/magic_enum.hpp
        src/00_Core/magic_enum/magic_enum_all.hpp
        src/00_Core/magic_enum/magic_enum_containers.hpp
        src/00_Core/magic_enum/magic_enum_flags.hpp
        src/00_Core/magic_enum/magic_enum_format.hpp
        src/00_Core/magic_enum/magic_enum_fuse.hpp
        src/00_Core/magic_enum/magic_enum_iostream.hpp
        src/00_Core/magic_enum/magic_enum_switch.hpp
        src/00_Core/magic_enum/magic_enum_utility.hpp
        src/module.cpp
        src/01_tokenize/token/token.cpp
        src/01_tokenize/token/token.h
        src/01_tokenize/token/token_type.h
        src/01_tokenize/token_converter.h
        src/01_tokenize/token_converter.cpp
        src/01_tokenize/tokenizer.cpp
        src/01_tokenize/tokenizer.h
        src/02_parsing/parser.h
        src/02_parsing/parser.cpp
        src/02_parsing/precedence.h
        src/02_parsing/precedence.cpp
        src/02_parsing/ast/node.h
        src/02_parsing/ast/module/Program.cpp
        src/02_parsing/ast/module/Program.h
        src/02_parsing/ast/statement/block_statement.cpp
        src/02_parsing/ast/statement/block_statement.h
        src/02_parsing/ast/statement/variable_statement.cpp
        src/02_parsing/ast/statement/variable_statement.h
        src/02_parsing/ast/statement/return_statement.cpp
        src/02_parsing/ast/statement/return_statement.h
        src/02_parsing/ast/statement/expression_statement.cpp
        src/02_parsing/ast/statement/expression_statement.h
        src/02_parsing/ast/expression/function_expression.cpp
        src/02_parsing/ast/expression/function_expression.h
        src/02_parsing/ast/expression/identifier_expression.cpp
        src/02_parsing/ast/expression/identifier_expression.h
        src/02_parsing/ast/expression/if_expression.cpp
        src/02_parsing/ast/expression/if_expression.h
        src/02_parsing/ast/expression/infix_expression.cpp
        src/02_parsing/ast/expression/infix_expression.h
        src/02_parsing/ast/expression/string_expression.cpp
        src/02_parsing/ast/expression/string_expression.h
        src/02_parsing/ast/expression/number_expression.cpp
        src/02_parsing/ast/expression/number_expression.h
        src/02_parsing/ast/expression/prefix_expression.cpp
        src/02_parsing/ast/expression/prefix_expression.h
        src/02_parsing/ast/expression/type_expression.cpp
        src/02_parsing/ast/expression/type_expression.h
        src/02_parsing/ast/util/visitor.h
        src/02_parsing/ast/expression/call_expression.cpp
        src/02_parsing/ast/expression/call_expression.h
        src/02_parsing/ast/util/ast_json_exporter.cpp
        src/02_parsing/ast/util/ast_json_exporter.h
        src/04_generate/Operand.h
        src/04_generate/Instruction.h
        src/04_generate/Compiler.cpp
        src/04_generate/Compiler.h
        src/04_generate/register_allocator.cpp
        src/04_generate/register_allocator.h
        src/04_generate/operand.cpp
        src/04_generate/instruction.cpp
)
target_include_directories(${PROJECT_NAME}_lib
        PUBLIC
        src/
)

enable_testing()
add_executable(${PROJECT_NAME}_test
        test/gtest.cpp
        test/01_tokenize/token/token_unit_test.cpp
        "test/milestone/1/01. tokenize/tokenizer_test.cpp"
        test/01_tokenize/number_tokenize.cpp
        test/01_tokenize/string_tokenize.cpp
        "test/milestone/1/02. parsing/parser_test.cpp"
        "test/milestone/1/04. generate/generate_test.cpp"
)

target_link_libraries(
        ${PROJECT_NAME}_test
        ${PROJECT_NAME}_lib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}_test)