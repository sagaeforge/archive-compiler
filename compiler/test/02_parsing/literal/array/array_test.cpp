#include "02_parsing/parser_test_case.h"

class ArrayTestCase : public ParserTestCase {};

TEST_F(ArrayTestCase, array) {
    EXPECT_TRUE(LOAD_SAMPLE(array));

    auto tokens = tokenizer.tokenize(u"[1, 2, 3]");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(ArrayTestCase, array_with_empty_elements) {
    EXPECT_TRUE(LOAD_SAMPLE(array_with_empty_elements));

    auto tokens = tokenizer.tokenize(u"[]");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}