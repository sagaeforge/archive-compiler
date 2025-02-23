#include "BooleanLiteralNodeParseStrategy.h"

#include "02_parsing/ast/expression/boolean/BooleanLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool BooleanLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) {
    return tokens.current()->get_type() == tokenize::TokenType::True || tokens.current()->get_type() == tokenize::TokenType::False;
}

parsing::ParseStrategyResult BooleanLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto itr = tokens.current();
    auto value = itr->get_literal() == "true";
    return parsing::ParseStrategyResult{std::make_shared<BooleanLiteralNode>(*itr, value), tokens.current().next()};
}

} // namespace nugdev::compiler::ast::expression
