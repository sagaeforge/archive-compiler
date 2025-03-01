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

    auto workbench = tokens.clone(); // current: 'fn'
    workbench.next();

    auto [identifier, identifierItr] = identifierStrategy.parse(workbench);
    workbench.move_at(identifierItr);

    if (!contains(workbench.current(), {tokenize::TokenType::LParen})) {
        throw std::runtime_error("Expected '('");
    }

    auto parameters = std::vector<std::tuple<std::shared_ptr<Expression>, std::shared_ptr<Expression>, std::shared_ptr<Expression>>>();
    do {
        workbench.next();

        if (contains(workbench.current(), {tokenize::TokenType::RParen})) {
            break;
        }

        auto [element, identifierItr] = identifierStrategy.parse(workbench);
        workbench.move_at(identifierItr);

        if (!contains(workbench.current(), {tokenize::TokenType::Colon})) {
            throw std::runtime_error("Expected ':'");
        }
        workbench.next();

        auto [type, typeItr] = expressionStrategy.parse(workbench);
        workbench.move_at(typeItr);

        // 만약에 = 이 있다면, 기본 값을 추가해서 대응한다.
        if (contains(workbench.current(), {tokenize::TokenType::Assign})) {
            workbench.next();
            auto [defaultValue, defaultValueItr] = expressionStrategy.parse(workbench);
            workbench.move_at(defaultValueItr);
            parameters.push_back(std::make_tuple(element->as<ast::Expression>(), type->as<ast::Expression>(), defaultValue->as<ast::Expression>()));
            continue;
        }

        parameters.push_back(std::make_tuple(element->as<ast::Expression>(), type->as<ast::Expression>(), nullptr));

    } while (contains(workbench.current(), {tokenize::TokenType::Comma}));

    if (workbench.current()->get_type() != tokenize::TokenType::RParen) {
        throw std::runtime_error("Expected ')'");
    }
    workbench.next();

    auto [body, bodyItr] = blockStrategy.parse(workbench);
    workbench.move_at(bodyItr);
    return {std::make_shared<FunctionLiteralNode>(tokens.current().value(), parameters, body->as<ast::Statement>()),
            tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::expression
