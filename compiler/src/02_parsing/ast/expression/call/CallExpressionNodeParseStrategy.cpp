#include "CallExpressionNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/call/CallExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool CallExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) {
    return tokens.current().valid() && contains(tokens.current(), {tokenize::TokenType::LParen});
}

parsing::ParseStrategyResult CallExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) { throw std::runtime_error("Not implemented"); }

parsing::ParseStrategyResult CallExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> callee) {
    static ExpressionParseStrategy strategy{};
    auto [node, itr] = stream::workbench(tokens, [this, &tokens, callee](tokenize::TokenStream &workbench) {
        std::vector<std::shared_ptr<ast::Expression>> list;
        do {
            workbench.next();

            if (contains(workbench.current(), {tokenize::TokenType::RParen})) {
                workbench.next();
                return std::make_shared<CallExpressionNode>(tokens.current().value(), callee, list);
            }

            auto [element, moveItr] = strategy.parse(workbench, ast::expression::ExpressionParseStrategy::Precedence::Lowest);
            workbench.move_at(moveItr);
            list.push_back(element->as<ast::Expression>());
        } while (contains(workbench.current(), {tokenize::TokenType::Comma}));

        if (!contains(workbench.current(), {tokenize::TokenType::RParen})) {
            throw std::runtime_error("Expected ')'");
        }
        workbench.next();

        return std::make_shared<CallExpressionNode>(tokens.current().value(), callee, list);
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
