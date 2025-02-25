#include "InfixExpressionNodeParseStrategy.h"

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/infix/InfixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool InfixExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult InfixExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) { throw std::runtime_error("Not implemented"); }

parsing::ParseStrategyResult InfixExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> left) {
    static ExpressionParseStrategy expressionStrategy{};

    auto workbench = tokens.clone(); // current: '+' | '-' | '*' | '/' | '%' | '==' | '!=' | '<' | '>' | '<=' | '>='
    auto [right, rightItr] = expressionStrategy.parse(workbench.next(), ExpressionParseStrategy::get_precedence(workbench.current()->get_type()));
    workbench.move_at(rightItr);

    return {std::make_shared<InfixExpressionNode>(*tokens.current(), left->as<Expression>(), tokens.current()->get_literal(), right->as<Expression>()),
            tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::expression
