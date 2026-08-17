#include "CallExpressionNodeParseStrategy.h"

#include <ranges>

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/Parser.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/call/CallExpressionNode.h"
#include "02_parsing/ast/expression/function/FunctionExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool CallExpressionNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return tokens.current().valid() && contains(tokens.current(), {tokenize::TokenType::LParen});
}

parsing::ParseStrategyResult CallExpressionNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    throw std::runtime_error("Not implemented");
}

parsing::ParseStrategyResult CallExpressionNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens,
                                                                    std::shared_ptr<Expression> callee) {
    static ExpressionParseStrategy strategy{};
    auto [node, itr] = stream::workbench(tokens, [this, &parser, callee](tokenize::TokenStream &workbench) {
        auto callToken = workbench.current();
        std::vector<std::shared_ptr<ast::Expression>> list;
        do {
            workbench.move_next();

            if (contains(workbench.current(), {tokenize::TokenType::RParen})) {
                workbench.move_next();
                return std::make_shared<CallExpressionNode>(callToken.value(), callee, list);
            }

            auto [element, moveItr] = strategy.parse(parser, workbench, parsing::Precedence::Lowest);
            workbench.move_at(moveItr);
            list.push_back(element->as<ast::Expression>());
        } while (contains(workbench.current(), {tokenize::TokenType::Comma}));

        if (!contains(workbench.current(), {tokenize::TokenType::RParen})) {
            throw std::runtime_error("Expected ')'");
        }
        workbench.move_next();

        return std::make_shared<CallExpressionNode>(callToken.value(), callee, list);
    });

    // callee -> function으로 매칭해주는 작업.
    parsing::Parser::Mangling mangling{
        .name = callee->get_token().get_literal(),
        .paramTypes = {},
    };
    for (const auto &arg : node->get_arguments()) {
        mangling.paramTypes.push_back(arg->get_type_info());
    }
    parser.add_function_symbol_fetch(
        mangling,
        [](const ast::ASTNodePtr &self, const ast::ASTNodePtr &callee) {
            auto call = std::dynamic_pointer_cast<CallExpressionNode>(self);
            auto function = std::dynamic_pointer_cast<FunctionExpressionNode>(callee);
            if (!call || !function) {
                throw std::runtime_error("Invalid call or callee");
            }
            call->set_target(function);
        },
        node);

    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
