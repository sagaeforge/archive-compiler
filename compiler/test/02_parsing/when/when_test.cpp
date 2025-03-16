#include "02_parsing/parser_test_case.h"

class WhenTestCase : public ParserTestCase {};

TEST_F(WhenTestCase, when) {
    EXPECT_TRUE(LOAD_SAMPLE(when));

    auto tokens = tokenizer.tokenize(u"when { true -> a false -> b }");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(WhenTestCase, when_with_else) {
    EXPECT_TRUE(LOAD_SAMPLE(when_with_else));

    auto tokens = tokenizer.tokenize(u"when { true -> a else -> b }");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(WhenTestCase, when_else) {
    EXPECT_TRUE(LOAD_SAMPLE(when_else));

    auto tokens = tokenizer.tokenize(u"when { else -> a }");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(WhenTestCase, when_target) {
    EXPECT_TRUE(LOAD_SAMPLE(when_target));

    auto tokens = tokenizer.tokenize(u"when (a) { b -> c }");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(WhenTestCase, when_target_with_else) {
    EXPECT_TRUE(LOAD_SAMPLE(when_target_with_else));

    auto tokens = tokenizer.tokenize(u"when (a) { b -> c else -> d }");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(WhenTestCase, when_in) {
    EXPECT_TRUE(LOAD_SAMPLE(when_in));

    auto tokens = tokenizer.tokenize(u"when (a) { in b -> c }");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}
