#include "02_parsing/parser_test_case.h"

class ContinueTestCase : public ParserTestCase {};

TEST_F(ContinueTestCase, continue) {
    EXPECT_TRUE(LOAD_SAMPLE(continue));

    auto tokens = tokenizer.tokenize(u"continue");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(ContinueTestCase, continue_with_label) {
    EXPECT_TRUE(LOAD_SAMPLE(continue_with_label));

    auto tokens = tokenizer.tokenize(u"continue@label");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}