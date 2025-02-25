#include "IfExpressionNodeParseStrategy.h"

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/if/IfExpressionNode.h"
#include "02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h"

namespace nugdev::compiler::ast::expression {

bool IfExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::If}); }

parsing::ParseStrategyResult IfExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy expressionStrategy{};
    static statement::BlockStatementNodeParseStrategy blockStrategy{};

    auto workbench = tokens.clone().next(); // current: 'if' | 'elif'

    auto [condition, conditionItr] = expressionStrategy.parse(workbench);
    workbench.move_at(conditionItr);

    if (workbench.current()->get_type() != tokenize::TokenType::LBrace) {
        throw std::runtime_error("Expected '{'");
    }

    auto [consequence, consequenceItr] = blockStrategy.parse(workbench);
    workbench.move_at(consequenceItr);

    // ealiy-return elif else case
    if (!contains(workbench.current(), {tokenize::TokenType::Else, tokenize::TokenType::Elif})) {
        return {std::make_shared<IfExpressionNode>(*tokens.current(), condition->as<Expression>(), consequence->as<Statement>(), nullptr),
                tokens.begin() + workbench.current().distance()};
    }

    if (contains(workbench.current(), {tokenize::TokenType::Elif})) {
        auto [alternative, alternativeItr] = parse(workbench);
        workbench.move_at(alternativeItr);
        return {std::make_shared<IfExpressionNode>(*tokens.current(), condition->as<Expression>(), consequence->as<Statement>(), alternative->as<Statement>()),
                tokens.begin() + workbench.current().distance()};
    }

    if (contains(workbench.current(), {tokenize::TokenType::Else})) {
        workbench.next();
        auto [alternative, alternativeItr] = blockStrategy.parse(workbench);
        workbench.move_at(alternativeItr);
        return {std::make_shared<IfExpressionNode>(*tokens.current(), condition->as<Expression>(), consequence->as<Statement>(), alternative->as<Statement>()),
                tokens.begin() + workbench.current().distance()};
    }

    return {std::make_shared<IfExpressionNode>(*tokens.current(), condition->as<Expression>(), consequence->as<Statement>(), nullptr),
            tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::expression
