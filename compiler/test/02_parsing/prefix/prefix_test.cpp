#include "02_parsing/parser_test_case.h"

class PrefixTestCase : public ParserTestCase {};

TEST_F(PrefixTestCase, prefix_plus) {
    EXPECT_TRUE(LOAD_SAMPLE(prefix_plus));

    auto tokens = tokenizer.tokenize(u"+1");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}
