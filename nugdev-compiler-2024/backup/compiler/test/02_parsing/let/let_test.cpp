#include "02_parsing/parser_test_case.h"

class LetTestCase : public ParserTestCase {};

TEST_F(LetTestCase, let_set_value) {
    EXPECT_TRUE(LOAD_SAMPLE(let_set_value));

    auto tokens = tokenizer.tokenize(u"let a = 10");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(LetTestCase, let_define_with_type) {
    EXPECT_TRUE(LOAD_SAMPLE(let_define_with_type));

    auto tokens = tokenizer.tokenize(u"let a: i32");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(LetTestCase, let_set_value_with_type) {
    EXPECT_TRUE(LOAD_SAMPLE(let_set_value_with_type));

    auto tokens = tokenizer.tokenize(u"let a: i32 = 10");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}
