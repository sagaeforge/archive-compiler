#include "02_parsing/parser_test_case.h"

class BreakTestCase : public ParserTestCase {};

TEST_F(BreakTestCase, break) {
    EXPECT_TRUE(LOAD_SAMPLE(break));

    auto tokens = tokenizer.tokenize(u"break");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(BreakTestCase, break_with_label) {
    EXPECT_TRUE(LOAD_SAMPLE(break_with_label));

    auto tokens = tokenizer.tokenize(u"break@label");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}
