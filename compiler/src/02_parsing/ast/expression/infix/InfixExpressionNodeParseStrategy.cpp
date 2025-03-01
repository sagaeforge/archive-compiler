#include "InfixExpressionNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/infix/InfixExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool InfixExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult InfixExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) { throw std::runtime_error("Not implemented"); }

parsing::ParseStrategyResult InfixExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> left) {
    static ExpressionParseStrategy expressionStrategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &tokens, left](tokenize::TokenStream &workbench) {
        workbench.next();

        // current: '+' | '-' | '*' | '/' | '%' | '==' | '!=' | '<' | '>' | '<=' | '>=' | 'in'
        auto [right, rightItr] = expressionStrategy.parse(workbench, ExpressionParseStrategy::get_precedence(workbench.current()->get_type()));
        workbench.move_at(rightItr);
        return create_node(*tokens.current(), left->as<Expression>(), right->as<Expression>(), tokens.current()->get_literal());
    });

    return {node, itr};
}

std::shared_ptr<ast::ASTNode> InfixExpressionNodeParseStrategy::create_node(const tokenize::Token &token, std::shared_ptr<Expression> left,
                                                                            std::shared_ptr<Expression> right, std::optional<icu::UnicodeString> op) {
    return std::make_shared<InfixExpressionNode>(token, left->as<Expression>(), op.value_or(token.get_literal()), right->as<Expression>());
}

} // namespace nugdev::compiler::ast::expression
