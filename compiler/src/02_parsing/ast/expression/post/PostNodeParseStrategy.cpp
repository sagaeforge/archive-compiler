#include "PostNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/post/PostNode.h"

namespace nugdev::compiler::ast::expression {

bool PostNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) {
    return contains(tokens.current(), {tokenize::TokenType::Inc, tokenize::TokenType::Dec});
}

parsing::ParseStrategyResult PostNodeParseStrategy::parse(const tokenize::TokenStream &tokens) { throw std::runtime_error("Not implemented"); }

parsing::ParseStrategyResult PostNodeParseStrategy::parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> left) {
    // current: '++' | '--'
    return {std::make_shared<PostNode>(*tokens.current(), left, tokens.current()->get_literal()), tokens.current().next()};
}

} // namespace nugdev::compiler::ast::expression
