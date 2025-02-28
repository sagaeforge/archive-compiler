#include "02_parsing/parser_test_case.h"

class ForTestCase : public ParserTestCase {};

TEST_F(ForTestCase, for) {
    EXPECT_TRUE(LOAD_SAMPLE(for));

    auto tokens = tokenizer.tokenize(u"for { statements }");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(ForTestCase, for_with_label) {
    EXPECT_TRUE(LOAD_SAMPLE(for_with_label));

    auto tokens = tokenizer.tokenize(u"label@for { statements }");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(ForTestCase, for_with_condition) {
    EXPECT_TRUE(LOAD_SAMPLE(for_with_condition));

    auto tokens = tokenizer.tokenize(u"for (i < 10) { statements }");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(ForTestCase, for_with_post) {
    EXPECT_TRUE(LOAD_SAMPLE(for_with_post));

    auto tokens = tokenizer.tokenize(u"for (i < 10; i++) { statements }");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(ForTestCase, for_with_init_and_condition) {
    EXPECT_TRUE(LOAD_SAMPLE(for_with_init_and_condition));

    auto tokens = tokenizer.tokenize(u"for (let i = 0; i < 10; i++) { statements }");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}