#include "FunctionExpressionNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/function/FunctionExpressionNode.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h"

namespace nugdev::compiler::ast::expression {

bool FunctionExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) {
    return tokens.current().valid() && contains(tokens.current(), {tokenize::TokenType::Function});
}

parsing::ParseStrategyResult FunctionExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static IdentifierLiteralNodeParseStrategy identifierStrategy{};
    static ExpressionParseStrategy expressionStrategy{};
    static statement::BlockStatementNodeParseStrategy blockStrategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &tokens](tokenize::TokenStream &workbench) {
        // current: fn
        workbench.next();

        // current: identifier
        auto [element, identifierItr] = identifierStrategy.parse(workbench);
        workbench.move_at(identifierItr);

        // current: '('
        if (!contains(workbench.current(), {tokenize::TokenType::LParen})) {
            throw std::runtime_error("Expected '('");
        }

        auto parameters = std::vector<std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, std::shared_ptr<Expression>>>();
        do {
            workbench.next();

            // current: ')'?
            if (contains(workbench.current(), {tokenize::TokenType::RParen})) {
                break;
            }

            // current: identifier
            auto [element, identifierItr] = identifierStrategy.parse(workbench);
            workbench.move_at(identifierItr);

            // current: ':'
            if (!contains(workbench.current(), {tokenize::TokenType::Colon})) {
                throw std::runtime_error("Expected ':'");
            }
            workbench.next();

            // current: type
            auto [type, typeItr] = expressionStrategy.parse(workbench);
            workbench.move_at(typeItr);

            // current: '='?
            if (contains(workbench.current(), {tokenize::TokenType::Assign})) {
                workbench.next();

                // current: defaultValue
                auto [defaultValue, defaultValueItr] = expressionStrategy.parse(workbench);
                workbench.move_at(defaultValueItr);
                parameters.push_back(std::make_tuple(element->as<ast::Expression>(), type->as<ast::Expression>(), defaultValue->as<ast::Expression>()));
                continue;
            }

            parameters.push_back(std::make_tuple(element->as<ast::Expression>(), type->as<ast::Expression>(), nullptr));
        } while (contains(workbench.current(), {tokenize::TokenType::Comma}));

        // current: ')'
        if (workbench.current()->get_type() != tokenize::TokenType::RParen) {
            throw std::runtime_error("Expected ')'");
        }
        workbench.next();

        // current: '{'
        auto [body, bodyItr] = blockStrategy.parse(workbench);
        workbench.move_at(bodyItr);
        return std::make_shared<FunctionExpressionNode>(tokens.current().value(), parameters, body->as<ast::Statement>());
    });

    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
