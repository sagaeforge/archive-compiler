#include "02_parsing/parser_test_case.h"

class CallTestCase : public ParserTestCase {};

TEST_F(CallTestCase, call) {
    EXPECT_TRUE(LOAD_SAMPLE(call));

    auto tokens = tokenizer.tokenize(u"add(1, 2)");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}

TEST_F(CallTestCase, call_with_empty_arguments) {
    EXPECT_TRUE(LOAD_SAMPLE(call_with_empty_arguments));

    auto tokens = tokenizer.tokenize(u"add()");
    auto ast = parser.parse(tokens);

    expect_ast(ast);
}
