#include "ForNodeParseStrategy.h"

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h"
#include "02_parsing/ast/statement/for/ForNode.h"
#include "02_parsing/ast/statement/let/LetNodeParseStrategy.h"

namespace nugdev::compiler::ast::statement {

/*
이렇게 문법 제공할 예정.
for { blockstatement }
for (i < 10) { blockstatement }
for (i < 10; i++) { blockstatement }
for (let i = 0; i < 10; i++) { blockstatement }
*/

bool ForNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) {
    // 2개 케이스가 있음.
    // identifier@for 이렇게 시작하는 케이스
    // for 이렇게 시작하는 케이스
    auto workbench = tokens.clone();
    bool isLabel = contains(workbench.current(), {tokenize::TokenType::Ident}) && contains(workbench.next().current(), {tokenize::TokenType::At}) &&
                   contains(workbench.next().next().current(), {tokenize::TokenType::For});
    bool isFor = contains(workbench.current(), {tokenize::TokenType::For});
    return isLabel || isFor;
}

parsing::ParseStrategyResult ForNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static statement::LetNodeParseStrategy letStrategy{};
    static expression::ExpressionParseStrategy expressionStrategy{};
    static statement::BlockStatementNodeParseStrategy blockStatementStrategy{};
    static expression::IdentifierLiteralNodeParseStrategy identifierLiteralNodeParseStrategy{};

    auto workbench = tokens.clone(); // current : for | ident

    std::shared_ptr<Expression> label = nullptr;
    if (contains(workbench.current(), {tokenize::TokenType::Ident})) {
        auto [labelNode, labelMoveItr] = identifierLiteralNodeParseStrategy.parse(workbench);
        workbench.move_at(labelMoveItr);

        if (!contains(workbench.current(), {tokenize::TokenType::At})) {
            throw std::runtime_error("Expected '@' after identifier");
        }
        workbench.next();
        label = labelNode->as<Expression>();
    }

    if (!contains(workbench.current(), {tokenize::TokenType::For})) {
        throw std::runtime_error("Expected 'for' keyword");
    }
    workbench.next();

    std::shared_ptr<Expression> init = nullptr;
    std::shared_ptr<Expression> condition = nullptr;
    std::shared_ptr<Expression> post = nullptr;
    if (contains(workbench.current(), {tokenize::TokenType::LParen})) {
        workbench.next();

        // current: init?
        if (letStrategy.can_parse(workbench)) {
            auto [initNode, initMoveItr] = letStrategy.parse(workbench);
            workbench.move_at(initMoveItr);
            init = initNode->as<Expression>();

            if (!contains(workbench.current(), {tokenize::TokenType::SemiColon})) {
                throw std::runtime_error("Expected ';' after 'for' init");
            }
            workbench.next();
        }

        // current: condition!
        auto [conditionNode, conditionMoveItr] = expressionStrategy.parse(workbench);
        workbench.move_at(conditionMoveItr);
        condition = conditionNode->as<Expression>();

        if (contains(workbench.current(), {tokenize::TokenType::SemiColon})) {
            workbench.next();
            // current: post!
            auto [postNode, postMoveItr] = expressionStrategy.parse(workbench);
            workbench.move_at(postMoveItr);
            post = postNode->as<Expression>();
        }

        if (!contains(workbench.current(), {tokenize::TokenType::RParen})) {
            throw std::runtime_error("Expected ')' after 'for' condition");
        }
        workbench.next();
    }

    auto [consequence, consequenceMoveItr] = blockStatementStrategy.parse(workbench);
    workbench.move_at(consequenceMoveItr);
    return parsing::ParseStrategyResult{std::make_shared<ForNode>(tokens.current().value(), label, init, condition, post, consequence->as<Statement>()),
                                        tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::statement
