#include "02_parsing/parser_test_case.h"

class BooleanTestCase : public ParserTestCase {};

TEST_F(BooleanTestCase, true_literal) {
    EXPECT_TRUE(LOAD_SAMPLE(true_literal));

    auto tokens = tokenizer.tokenize(u"true");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}

TEST_F(BooleanTestCase, false_literal) {
    EXPECT_TRUE(LOAD_SAMPLE(false_literal));

    auto tokens = tokenizer.tokenize(u"false");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}