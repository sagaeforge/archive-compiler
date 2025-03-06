#include "WhenExpressionNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/infix/InfixExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/when/WhenExpressionNode.h"
#include "02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h"

namespace nugdev::compiler::ast::expression {

bool WhenExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::When}); }

parsing::ParseStrategyResult WhenExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy expressionStrategy{};
    static statement::BlockStatementNodeParseStrategy blockStatementStrategy{};
    static expression::InfixExpressionNodeParseStrategy infixExpressionNodeParseStrategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &tokens](tokenize::TokenStream &workbench) {
        // current: 'when'
        workbench.next();

        std::shared_ptr<Expression> target = nullptr;
        if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::LParen})) {
            workbench.next();

            auto [targetNode, targetItr] = expressionStrategy.parse(workbench);
            workbench.move_at(targetItr);
            target = targetNode->as<Expression>();

            if (!contains(workbench.current(), {tokenize::TokenType::RParen})) {
                throw std::runtime_error("Expected ')' after target");
            }
            workbench.next();
        }

        if (!workbench.current().valid() || !contains(workbench.current(), {tokenize::TokenType::LBrace})) {
            throw std::runtime_error("Expected '{' after when");
        }
        workbench.next();

        // 여기는 expression 여러개 존재할수 있음.
        bool isAlternative = false;
        WhenExpressionNode::Consequence alternative = {};
        std::map<std::shared_ptr<Expression>, WhenExpressionNode::Consequence> conditions;
        do {

            // 선두에 올 수 있는 식 먼저 확인하고, '->' 다음 statement 혹은
            std::shared_ptr<Expression> condition = nullptr;
            if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::In})) {
                auto token = *workbench.current();
                if (target == nullptr) {
                    throw std::runtime_error("Expected target before 'in' in when");
                }
                workbench.next();

                auto [expressionNode, expressionItr] = expressionStrategy.parse(workbench);
                workbench.move_at(expressionItr);
                condition = infixExpressionNodeParseStrategy.create_node(token, target, expressionNode->as<Expression>())->as<Expression>();
            } else if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::Else})) {
                if (isAlternative) {
                    // 이미 else가 있다는 의미인데, 또 나오면 오류.
                    throw std::runtime_error("Duplicate 'else' in when");
                }

                isAlternative = true;
                workbench.next();
            } else { // expression
                auto [expressionNode, expressionItr] = expressionStrategy.parse(workbench);
                workbench.move_at(expressionItr);

                if (target != nullptr) {
                    condition =
                        infixExpressionNodeParseStrategy.create_node(*workbench.current(), target, expressionNode->as<Expression>(), u"==")->as<Expression>();
                } else {
                    condition = expressionNode->as<Expression>();
                }
            }

            if (!workbench.current().valid() || !contains(workbench.current(), {tokenize::TokenType::LeftArrow})) {
                throw std::runtime_error("Expected '->' after condition");
            }
            workbench.next();

            // 만약에 '{' 이라면 blockstatement로 파싱.
            // 아니라면 expression으로 파싱.
            WhenExpressionNode::Consequence result;
            if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::LBrace})) {
                auto [blockStatementNode, blockStatementItr] = blockStatementStrategy.parse(workbench);
                workbench.move_at(blockStatementItr);
                result = blockStatementNode->as<Statement>();
            } else {
                auto [expressionNode, expressionItr] = expressionStrategy.parse(workbench);
                workbench.move_at(expressionItr);
                result = expressionNode->as<Expression>();
            }

            if (isAlternative) {
                alternative = result;
            } else {
                conditions[condition] = result;
            }
        } while (!contains(workbench.current(), {tokenize::TokenType::RBrace}));
        workbench.next();

        return std::make_shared<WhenExpressionNode>(tokens.current().value(), target, conditions, alternative);
    });

    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
