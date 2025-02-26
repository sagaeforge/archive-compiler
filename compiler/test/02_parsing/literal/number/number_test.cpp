#include "02_parsing/parser_test_case.h"

class NumberTestCase : public ParserTestCase {};

TEST_F(NumberTestCase, integer) {
    EXPECT_TRUE(LOAD_SAMPLE(integer));

    auto tokens = tokenizer.tokenize(u"10");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(NumberTestCase, float) {
    EXPECT_TRUE(LOAD_SAMPLE(float));

    auto tokens = tokenizer.tokenize(u"10.5");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(NumberTestCase, float_with_exponent) {
    EXPECT_TRUE(LOAD_SAMPLE(float_with_exponent));

    auto tokens = tokenizer.tokenize(u"10.5e-2");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}