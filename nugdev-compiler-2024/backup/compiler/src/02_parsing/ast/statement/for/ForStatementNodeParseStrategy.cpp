#include "ForStatementNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h"
#include "02_parsing/ast/statement/expression/ExpressionStatementNodeParseStrategy.h"
#include "02_parsing/ast/statement/for/ForStatementNode.h"
#include "02_parsing/ast/statement/let/LetStatementNodeParseStrategy.h"

namespace nugdev::compiler::ast::statement {

/*
이렇게 문법 제공할 예정.
for { blockstatement }
for (i < 10) { blockstatement }
for (i < 10; i++) { blockstatement }
for (let i = 0; i < 10; i++) { blockstatement }
*/

bool ForStatementNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    // 2개 케이스가 있음.
    // identifier@for 이렇게 시작하는 케이스
    // for 이렇게 시작하는 케이스
    auto workbench = tokens.clone();
    bool isLabel = contains(workbench.current(), {tokenize::TokenType::Ident}) && contains(workbench.move_next().current(), {tokenize::TokenType::At}) &&
                   contains(workbench.move_next().move_next().current(), {tokenize::TokenType::For});
    bool isFor = contains(workbench.current(), {tokenize::TokenType::For});
    return isLabel || isFor;
}

parsing::ParseStrategyResult ForStatementNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    static statement::LetStatementNodeParseStrategy letStrategy{};
    static expression::ExpressionParseStrategy expressionStrategy{};
    static statement::BlockStatementNodeParseStrategy blockStatementStrategy{};
    static expression::IdentifierLiteralNodeParseStrategy identifierLiteralNodeParseStrategy{};
    static statement::ExpressionStatementNodeParseStrategy expressionStatementStrategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        auto forToken = workbench.current();
        std::shared_ptr<Expression> label = nullptr;
        if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::Ident})) {
            auto [labelNode, labelMoveItr] = identifierLiteralNodeParseStrategy.parse(parser, workbench);
            workbench.move_at(labelMoveItr);

            if (!contains(workbench.current(), {tokenize::TokenType::At})) {
                throw std::runtime_error("Expected '@' after identifier");
            }
            workbench.move_next();
            label = labelNode->as<Expression>();
        }

        if (!workbench.current().valid() || !contains(workbench.current(), {tokenize::TokenType::For})) {
            throw std::runtime_error("Expected 'for' keyword");
        }
        workbench.move_next();

        std::shared_ptr<Expression> init = nullptr;
        std::shared_ptr<Expression> condition = nullptr;
        std::shared_ptr<Statement> post = nullptr;
        if (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::LParen})) {
            workbench.move_next();

            // current: init?
            if (letStrategy.can_parse(parser, workbench)) {
                auto [initNode, initMoveItr] = letStrategy.parse(parser, workbench);
                workbench.move_at(initMoveItr);
                init = initNode->as<Expression>();

                if (!contains(workbench.current(), {tokenize::TokenType::SemiColon})) {
                    throw std::runtime_error("Expected ';' after 'for' init");
                }
                workbench.move_next();
            }

            // current: condition!
            auto [conditionNode, conditionMoveItr] = expressionStrategy.parse(parser, workbench);
            workbench.move_at(conditionMoveItr);
            condition = conditionNode->as<Expression>();

            if (contains(workbench.current(), {tokenize::TokenType::SemiColon})) {
                workbench.move_next();
                // current: post!
                auto [postNode, postMoveItr] = expressionStatementStrategy.parse(parser, workbench);
                workbench.move_at(postMoveItr);
                post = postNode->as<Statement>();
            }

            if (!contains(workbench.current(), {tokenize::TokenType::RParen})) {
                throw std::runtime_error("Expected ')' after 'for' condition");
            }
            workbench.move_next();
        }

        auto [consequence, consequenceMoveItr] = blockStatementStrategy.parse(parser, workbench);
        workbench.move_at(consequenceMoveItr);
        return std::make_shared<ForStatementNode>(forToken.value(), label, init, condition, post, consequence->as<Statement>());
    });

    return {node, itr};
}

} // namespace nugdev::compiler::ast::statement
