#include "FunctionLiteralNodeParseStrategy.h"

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/function/FunctionLiteralNode.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h"

namespace nugdev::compiler::ast::expression {

bool FunctionLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::Function; }

parsing::ParseStrategyResult FunctionLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static IdentifierLiteralNodeParseStrategy identifierStrategy{};
    static ExpressionParseStrategy expressionStrategy{};
    static statement::BlockStatementNodeParseStrategy blockStrategy{};

    auto workbench = tokens.clone();
    if (!contains(workbench.current(), {tokenize::TokenType::RParen})) {
        throw std::runtime_error("Expected ')'");
    }
    workbench.next();

    if (contains(workbench.current(), {tokenize::TokenType::RParen})) {
        return {std::make_shared<FunctionLiteralNode>(workbench.current().value(), std::vector<std::shared_ptr<Expression>>(), nullptr),
                tokens.current().next()};
    }
    workbench.next();

    auto identifiers = std::vector<std::shared_ptr<Expression>>();
    do {
        auto [element, identifierItr] = identifierStrategy.parse(workbench);
        workbench.move_at(identifierItr);
        identifiers.push_back(element->as<ast::Expression>());

        if (!contains(workbench.current(), {tokenize::TokenType::Colon})) {
            throw std::runtime_error("Expected ':'");
        }
        workbench.next();

        auto [type, typeItr] = expressionStrategy.parse(workbench);
        workbench.move_at(typeItr);
        identifiers.push_back(type->as<ast::Expression>());
    } while (contains(workbench.current(), {tokenize::TokenType::Comma}));

    if (!contains(workbench.current(), {tokenize::TokenType::RParen})) {
        throw std::runtime_error("Expected ')'");
    }
    workbench.next();

    auto [body, bodyItr] = blockStrategy.parse(workbench);
    workbench.move_at(bodyItr);
    return {std::make_shared<FunctionLiteralNode>(workbench.current().value(), identifiers, body->as<ast::Statement>()), tokens.current().next()};
}

} // namespace nugdev::compiler::ast::expression
