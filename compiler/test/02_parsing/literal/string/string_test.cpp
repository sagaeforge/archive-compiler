#include "02_parsing/parser_test_case.h"

class StringTestCase : public ParserTestCase {};

TEST_F(StringTestCase, string) {
    EXPECT_TRUE(LOAD_SAMPLE(string));

    auto tokens = tokenizer.tokenize(u"\"hello\"");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(StringTestCase, string_with_escape_character) {
    EXPECT_TRUE(LOAD_SAMPLE(string_with_escape_character));

    auto tokens = tokenizer.tokenize(u"\"hello\\n\"");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}
