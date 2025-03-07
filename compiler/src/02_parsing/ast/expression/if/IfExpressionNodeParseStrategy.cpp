#include "IfExpressionNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/if/IfExpressionNode.h"
#include "02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h"

namespace nugdev::compiler::ast::expression {

bool IfExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::If}); }

parsing::ParseStrategyResult IfExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy expressionStrategy{};
    static statement::BlockStatementNodeParseStrategy blockStrategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &tokens](tokenize::TokenStream &workbench) {
        // current: 'if' | 'elif'
        workbench.next();

        std::shared_ptr<Expression> condition;
        if (workbench.current().valid() && !contains(workbench.current(), {tokenize::TokenType::LBrace})) {
            // current: condition
            auto [conditionNode, conditionItr] = expressionStrategy.parse(workbench);
            workbench.move_at(conditionItr);
            condition = conditionNode->as<Expression>();
        }

        // current: consequence
        auto [consequence, consequenceItr] = blockStrategy.parse(workbench);
        workbench.move_at(consequenceItr);

        // current: 'elif'
        if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::Elif})) {
            auto [alternative, alternativeItr] = parse(workbench);
            workbench.move_at(alternativeItr);
            return std::make_shared<IfExpressionNode>(*tokens.current(), condition != nullptr ? condition->as<Expression>() : nullptr,
                                                      consequence->as<Statement>(), alternative->as<Statement>());
        }

        // current: 'else'
        if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::Else})) {
            workbench.next();
            auto [alternative, alternativeItr] = blockStrategy.parse(workbench);
            workbench.move_at(alternativeItr);
            return std::make_shared<IfExpressionNode>(*tokens.current(), condition != nullptr ? condition->as<Expression>() : nullptr,
                                                      consequence->as<Statement>(), alternative->as<Statement>());
        }

        return std::make_shared<IfExpressionNode>(*tokens.current(), condition->as<Expression>(), consequence->as<Statement>(), nullptr);
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
