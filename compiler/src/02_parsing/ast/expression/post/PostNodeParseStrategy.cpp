#include "PostNodeParseStrategy.h"

#include "02_parsing/ast/expression/post/PostNode.h"

namespace nugdev::compiler::ast::expression {

bool PostNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult PostNodeParseStrategy::parse(const tokenize::TokenStream &tokens) { throw std::runtime_error("Not implemented"); }

parsing::ParseStrategyResult PostNodeParseStrategy::parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> left) {
    // current: '++' | '--'
    return {std::make_shared<PostNode>(*tokens.current(), left, tokens.current()->get_literal()), tokens.clone().next().current()};
}

} // namespace nugdev::compiler::ast::expression
