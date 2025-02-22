#include "gtest/gtest.h"

#include "01_tokenize/Tokenizer.h"

#include "02_parsing/ast/module/program/ProgramNodeParseStrategy.h"

TEST(ProgramNodeParseStrategy, empty_statement) {
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer();
    auto tokens = tokenizer.tokenize(u"");
    auto token_stream = nugdev::compiler::tokenize::TokenStream(tokens);

    auto strategy = nugdev::compiler::ast::module::ProgramNodeParseStrategy();
    auto result = strategy.parse(token_stream);
    EXPECT_EQ(result.node->get_type(), u"Program");
}