#include "IfExpressionNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/if/IfExpressionNode.h"
#include "02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h"

namespace nugdev::compiler::ast::expression {

bool IfExpressionNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return contains(tokens.current(), {tokenize::TokenType::If});
}

parsing::ParseStrategyResult IfExpressionNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy expressionStrategy{};
    static statement::BlockStatementNodeParseStrategy blockStrategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        auto ifToken = workbench.current();
        workbench.move_next();

        std::shared_ptr<Expression> condition;
        if (workbench.current().valid() && !contains(workbench.current(), {tokenize::TokenType::LBrace})) {
            // current: condition
            auto [conditionNode, conditionItr] = expressionStrategy.parse(parser, workbench);
            workbench.move_at(conditionItr);
            condition = conditionNode->as<Expression>();
        }

        // current: consequence
        auto [consequence, consequenceItr] = blockStrategy.parse(parser, workbench);
        workbench.move_at(consequenceItr);

        // current: 'elif'
        if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::Elif})) {
            auto [alternative, alternativeItr] = parse(parser, workbench);
            workbench.move_at(alternativeItr);
            return std::make_shared<IfExpressionNode>(ifToken.value(), condition != nullptr ? condition->as<Expression>() : nullptr,
                                                      consequence->as<Statement>(), alternative->as<Statement>());
        }

        // current: 'else'
        if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::Else})) {
            workbench.move_next();
            auto [alternative, alternativeItr] = blockStrategy.parse(parser, workbench);
            workbench.move_at(alternativeItr);
            return std::make_shared<IfExpressionNode>(ifToken.value(), condition != nullptr ? condition->as<Expression>() : nullptr,
                                                      consequence->as<Statement>(), alternative->as<Statement>());
        }

        return std::make_shared<IfExpressionNode>(ifToken.value(), condition->as<Expression>(), consequence->as<Statement>(), nullptr);
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
