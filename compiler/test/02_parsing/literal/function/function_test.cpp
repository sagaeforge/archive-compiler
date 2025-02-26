#include "02_parsing/parser_test_case.h"

class FunctionTestCase : public ParserTestCase {};

TEST_F(FunctionTestCase, function_empty_parameters) {
    EXPECT_TRUE(LOAD_SAMPLE(function_empty_parameters));

    auto tokens = tokenizer.tokenize(u"fn add() {}");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(FunctionTestCase, function_with_parameters) {
    EXPECT_TRUE(LOAD_SAMPLE(function_with_parameters));

    auto tokens = tokenizer.tokenize(u"fn add(a: Int, b: Int) { 123 }");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(FunctionTestCase, function_with_parameters_set_default_value) {
    EXPECT_TRUE(LOAD_SAMPLE(function_with_parameters_set_default_value));

    auto tokens = tokenizer.tokenize(u"fn add(a: Int, b: Int = 2) { 123 }");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}