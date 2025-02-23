#include "InfixExpressionNodeParseStrategy.h"

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/infix/InfixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool InfixExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult InfixExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) { throw std::runtime_error("Not implemented"); }

parsing::ParseStrategyResult InfixExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> left) {
    auto stream = tokens.clone();
    auto itr = stream.current();
    auto op = itr->get_literal();
    auto precedence = ExpressionParseStrategy::get_precedence(itr->get_type());

    stream = stream.move(itr.next());

    auto [right, rightItr] = ExpressionParseStrategy().parse(stream, precedence);
    if (right == nullptr) {
        throw std::runtime_error("Expected right expression");
    }

    return parsing::ParseStrategyResult{std::make_shared<InfixExpressionNode>(*itr, left, op, right->as<Expression>()), rightItr};
}

} // namespace nugdev::compiler::ast::expression
