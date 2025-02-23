#include "IfExpressionNodeParseStrategy.h"

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/if/IfExpressionNode.h"
#include "02_parsing/ast/statement/block/BlockStatementNodeParseStrategy.h"

namespace nugdev::compiler::ast::expression {

bool IfExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::If; }

parsing::ParseStrategyResult IfExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto stream = tokens.clone();
    auto itr = stream.current().next();

    if (itr->get_type() != tokenize::TokenType::LParen) {
        throw std::runtime_error("Expected '('");
    }

    stream.move(itr.next());
    itr = stream.current();

    auto [condition, conditionItr] = ExpressionParseStrategy().parse(stream);
    if (condition == nullptr) {
        throw std::runtime_error("Expected condition");
    }

    stream.move(conditionItr);
    itr = stream.current();

    if (itr->get_type() != tokenize::TokenType::RParen) {
        throw std::runtime_error("Expected ')'");
    }

    if (itr.next()->get_type() != tokenize::TokenType::LBrace) {
        throw std::runtime_error("Expected '{'");
    }

    auto [consequence, consequenceItr] = statement::BlockStatementNodeParseStrategy().parse(stream);
    if (consequence == nullptr) {
        throw std::runtime_error("Expected consequence");
    }

    if (consequenceItr.next()->get_type() != tokenize::TokenType::RBrace) {
        throw std::runtime_error("Expected '}'");
    }

    if (itr.next()->get_type() != tokenize::TokenType::Else) {
        return parsing::ParseStrategyResult{std::make_shared<IfExpressionNode>(*itr, condition->as<Expression>(), consequence->as<Expression>(), nullptr),
                                            stream.current() + itr.distance()};
    }

    stream.move(itr.next().next());
    itr = stream.current();

    auto [alternative, alternativeItr] = statement::BlockStatementNodeParseStrategy().parse(stream);
    if (alternative == nullptr) {
        throw std::runtime_error("Expected alternative");
    }

    if (alternativeItr.next()->get_type() != tokenize::TokenType::RBrace) {
        throw std::runtime_error("Expected '}'");
    }

    stream.move(alternativeItr.next());
    itr = stream.current();

    return parsing::ParseStrategyResult{
        std::make_shared<IfExpressionNode>(*itr, condition->as<Expression>(), consequence->as<Expression>(), alternative->as<Expression>()),
        stream.current() + itr.distance()};
}

} // namespace nugdev::compiler::ast::expression
