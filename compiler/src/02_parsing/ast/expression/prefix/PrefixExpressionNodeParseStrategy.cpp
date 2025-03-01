#include "PrefixExpressionNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/prefix/PrefixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool PrefixExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) {
    return contains(tokens.current(), {tokenize::TokenType::Minus, tokenize::TokenType::ExclamationMark});
}

parsing::ParseStrategyResult PrefixExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy expressionStrategy{};
    auto [node, itr] = stream::workbench(tokens, [this, &tokens](tokenize::TokenStream &workbench) {
        // current: '-' | '!'
        workbench.next();

        // current: right
        auto [right, rightItr] = expressionStrategy.parse(workbench, ExpressionParseStrategy::get_precedence(workbench.current()->get_type()));
        workbench.move_at(rightItr);
        return std::make_shared<PrefixExpressionNode>(*tokens.current(), tokens.current()->get_literal(), right->as<Expression>());
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
