#include "../parser_test_case.h"

TEST_F(ParserTestCase, let_define) {
    EXPECT_TRUE(LOAD_SAMPLE(let_define));

    auto tokens = tokenizer.tokenize(u"let a");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(ParserTestCase, let_set_value) {
    EXPECT_TRUE(LOAD_SAMPLE(let_set_value));

    auto tokens = tokenizer.tokenize(u"let a = 10");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(ParserTestCase, let_define_with_type) {
    EXPECT_TRUE(LOAD_SAMPLE(let_define_with_type));

    auto tokens = tokenizer.tokenize(u"let a: int");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(ParserTestCase, let_set_value_with_type) {
    EXPECT_TRUE(LOAD_SAMPLE(let_set_value_with_type));

    auto tokens = tokenizer.tokenize(u"let a: int = 10");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}
