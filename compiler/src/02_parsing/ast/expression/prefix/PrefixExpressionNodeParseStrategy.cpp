#include "PrefixExpressionNodeParseStrategy.h"

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/prefix/PrefixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool PrefixExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult PrefixExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy expressionStrategy{};

    auto workbench = tokens.clone().next(); // current: '-' | '!'
    auto [right, rightItr] = expressionStrategy.parse(workbench, ExpressionParseStrategy::get_precedence(workbench.current()->get_type()));
    workbench.move_at(rightItr);

    return {std::make_shared<PrefixExpressionNode>(*tokens.current(), tokens.current()->get_literal(), right->as<Expression>()),
            tokens.current() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::expression
