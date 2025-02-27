#include "02_parsing/parser_test_case.h"

class IndexTestCase : public ParserTestCase {};

TEST_F(IndexTestCase, index) {
    EXPECT_TRUE(LOAD_SAMPLE(index));

    auto tokens = tokenizer.tokenize(u"a[1]");
    auto ast = parser.parse(tokens);

    expect_ast(parser.to_json(ast));
}
