#include "GroupExpressionParseStrategy.h"

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"

namespace nugdev::compiler::ast::expression {

bool GroupExpressionParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::LParen; }

parsing::ParseStrategyResult GroupExpressionParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy strategy;
    return strategy.parse(tokens);
}

} // namespace nugdev::compiler::ast::expression
