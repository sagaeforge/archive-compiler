#include "02_parsing/parser_test_case.h"

class IfTestCase : public ParserTestCase {};

TEST_F(IfTestCase, if) {
    EXPECT_TRUE(LOAD_SAMPLE(if));

    auto tokens = tokenizer.tokenize(u"if (a) { a }");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(IfTestCase, if_else) {
    EXPECT_TRUE(LOAD_SAMPLE(if_else));

    auto tokens = tokenizer.tokenize(u"if (a) { a } else { b }");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(IfTestCase, if_elif_else) {
    EXPECT_TRUE(LOAD_SAMPLE(if_elif_else));

    auto tokens = tokenizer.tokenize(u"if (a) { a } elif (b) { b } else { c }");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(IfTestCase, if_elif_elif) {
    EXPECT_TRUE(LOAD_SAMPLE(if_elif_elif));

    auto tokens = tokenizer.tokenize(u"if (a) { a } elif (b) { b } elif (c) { c }");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}