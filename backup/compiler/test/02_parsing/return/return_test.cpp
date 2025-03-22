#include "02_parsing/parser_test_case.h"

class ReturnTestCase : public ParserTestCase {};

TEST_F(ReturnTestCase, return) {
    EXPECT_TRUE(LOAD_SAMPLE(return));

    auto tokens = tokenizer.tokenize(u"return");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(ReturnTestCase, return_with_value) {
    EXPECT_TRUE(LOAD_SAMPLE(return_with_value));

    auto tokens = tokenizer.tokenize(u"return 1");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(ReturnTestCase, return_with_label) {
    EXPECT_TRUE(LOAD_SAMPLE(return_with_label));

    auto tokens = tokenizer.tokenize(u"return@label");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(ReturnTestCase, return_with_value_and_label) {
    EXPECT_TRUE(LOAD_SAMPLE(return_with_value_and_label));

    auto tokens = tokenizer.tokenize(u"return@label 1");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}
