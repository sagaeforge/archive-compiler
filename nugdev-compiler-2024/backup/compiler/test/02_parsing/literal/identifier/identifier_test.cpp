#include "02_parsing/parser_test_case.h"

class IdentifierTestCase : public ParserTestCase {};

TEST_F(IdentifierTestCase, identifier) {
    EXPECT_TRUE(LOAD_SAMPLE(identifier));

    auto tokens = tokenizer.tokenize(u"a");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(IdentifierTestCase, identifier_with_underscore) {
    EXPECT_TRUE(LOAD_SAMPLE(identifier_with_underscore));

    auto tokens = tokenizer.tokenize(u"_a");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}
