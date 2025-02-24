#include "CallExpressionNodeParseStrategy.h"

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/call/CallExpressionNode.h"
#include <vector>

namespace nugdev::compiler::ast::expression {

bool CallExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::LParen; }

parsing::ParseStrategyResult CallExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) { throw std::runtime_error("Not implemented"); }

parsing::ParseStrategyResult CallExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> callee) {
    static ExpressionParseStrategy strategy{};

    auto workbench = tokens.clone();
    if (contains(workbench.current(), {tokenize::TokenType::RParen})) {
        return {std::make_shared<CallExpressionNode>(workbench.current().value(), callee, std::vector<std::shared_ptr<ast::Expression>>()),
                tokens.current().next()};
    }

    std::vector<std::shared_ptr<ast::Expression>> list;
    do {
        auto [element, moveItr] = strategy.parse(workbench, ast::expression::ExpressionParseStrategy::Precedence::Lowest);
        workbench.move_at(moveItr);
        list.push_back(element->as<ast::Expression>());
    } while (contains(workbench.current(), {tokenize::TokenType::Comma}));

    if (!contains(workbench.current(), {tokenize::TokenType::RParen})) {
        throw std::runtime_error("Expected ')'");
    }

    return {std::make_shared<CallExpressionNode>(workbench.current().value(), callee, list), tokens.current().next()};
}

} // namespace nugdev::compiler::ast::expression
