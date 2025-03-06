#include "PostExpressionNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/post/PostExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool PostExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) {
    return contains(tokens.current(), {tokenize::TokenType::Inc, tokenize::TokenType::Dec});
}

parsing::ParseStrategyResult PostExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) { throw std::runtime_error("Not implemented"); }

parsing::ParseStrategyResult PostExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> left) {
    // current: '++' | '--'
    return {std::make_shared<PostExpressionNode>(*tokens.current(), left, tokens.current()->get_literal()), tokens.current().next()};
}

} // namespace nugdev::compiler::ast::expression
