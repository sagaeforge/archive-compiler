#include "FunctionExpressionNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/function/FunctionExpressionNode.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h"

namespace nugdev::compiler::ast::expression {

bool FunctionExpressionNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return tokens.current().valid() && contains(tokens.current(), {tokenize::TokenType::Function});
}

parsing::ParseStrategyResult FunctionExpressionNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    static IdentifierLiteralNodeParseStrategy identifierStrategy{};
    static ExpressionParseStrategy expressionStrategy{};
    static statement::BlockStatementNodeParseStrategy blockStrategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        auto functionToken = workbench.current();
        workbench.move_next();

        // current: identifier
        auto [element, identifierItr] = identifierStrategy.parse(parser, workbench);
        workbench.move_at(identifierItr);

        // current: '('
        if (!contains(workbench.current(), {tokenize::TokenType::LParen})) {
            throw std::runtime_error("Expected '('");
        }

        auto parameters = std::vector<std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, std::shared_ptr<Expression>>>();
        do {
            workbench.move_next();

            // current: ')'?
            if (contains(workbench.current(), {tokenize::TokenType::RParen})) {
                break;
            }

            // current: identifier
            auto [element, identifierItr] = identifierStrategy.parse(parser, workbench);
            workbench.move_at(identifierItr);

            // current: ':'
            if (!contains(workbench.current(), {tokenize::TokenType::Colon})) {
                throw std::runtime_error("Expected ':'");
            }
            workbench.move_next();

            // current: type
            auto [type, typeItr] = expressionStrategy.parse(parser, workbench);
            workbench.move_at(typeItr);

            // current: '='?
            if (contains(workbench.current(), {tokenize::TokenType::Assign})) {
                workbench.move_next();

                // current: defaultValue
                auto [defaultValue, defaultValueItr] = expressionStrategy.parse(parser, workbench);
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
        workbench.move_next();

        // current: '{'
        auto [body, bodyItr] = blockStrategy.parse(parser, workbench);
        workbench.move_at(bodyItr);
        return std::make_shared<FunctionExpressionNode>(functionToken.value(), parameters, body->as<ast::Statement>());
    });

    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
