#include "PrefixExpressionNodeParseStrategy.h"

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/prefix/PrefixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool PrefixExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult PrefixExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto stream = tokens.clone();
    auto itr = stream.current();

    auto op = itr->get_literal();
    auto precedence = ExpressionParseStrategy::get_precedence(itr->get_type());

    stream = stream.move(itr.next());

    auto [right, rightItr] = ExpressionParseStrategy().parse(stream, precedence);
    if (right == nullptr) {
        throw std::runtime_error("Expected right expression");
    }

    return parsing::ParseStrategyResult{std::make_shared<PrefixExpressionNode>(*itr, op, right->as<Expression>()), stream.current() + itr.distance()};
}

} // namespace nugdev::compiler::ast::expression
