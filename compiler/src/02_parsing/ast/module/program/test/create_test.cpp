#include "gtest/gtest.h"

#include "01_tokenize/Tokenizer.h"

#include "02_parsing/ast/module/program/ProgramNode.h"
#include "02_parsing/ast/module/program/ProgramNodeParseStrategy.h"

using namespace nugdev::compiler;

TEST(ProgramNodeParseStrategy, empty_statement) {
    auto tokenizer = tokenize::Tokenizer();
    auto tokens = tokenizer.tokenize(u"");
    auto token_stream = tokenize::TokenStream(tokens);

    auto strategy = ast::module::ProgramNodeParseStrategy();
    auto result = strategy.parse(token_stream);
    EXPECT_TRUE(result.node->is<ast::module::ProgramNode>());
}