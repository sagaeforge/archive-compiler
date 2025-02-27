#include "02_parsing/parser_test_case.h"

class InfixTestCase : public ParserTestCase {};

TEST_F(InfixTestCase, infix_plus) {
    EXPECT_TRUE(LOAD_SAMPLE(infix_plus));

    auto tokens = tokenizer.tokenize(u"1 + 2");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}