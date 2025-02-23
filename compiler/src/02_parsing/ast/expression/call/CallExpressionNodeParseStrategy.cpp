#include "CallExpressionNodeParseStrategy.h"

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/call/CallExpressionNode.h"
#include <vector>

namespace nugdev::compiler::ast::expression {

bool CallExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::LParen; }

parsing::ParseStrategyResult CallExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) { throw std::runtime_error("Not implemented"); }

parsing::ParseStrategyResult CallExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> callee) {
    auto stream = tokens.clone();
    auto itr = stream.current();
    if (itr->get_type() == tokenize::TokenType::RParen) {
        return parsing::ParseStrategyResult{std::make_shared<CallExpressionNode>(*itr, callee, std::vector<std::shared_ptr<ast::Expression>>()),
                                            tokens.current().next()};
    }

    static auto expressionParseStrategy = ast::expression::ExpressionParseStrategy();

    std::vector<std::shared_ptr<ast::Expression>> list;
    auto [firstElement, moveItr] = expressionParseStrategy.parse(stream, ast::expression::ExpressionParseStrategy::Precedence::Lowest);
    if (firstElement != nullptr) {
        list.push_back(firstElement->as<ast::Expression>());
    }

    while (itr.next()->get_type() == tokenize::TokenType::Comma) {
        stream = stream.move(itr.next().next());
        auto [element, moveItr] = expressionParseStrategy.parse(stream, ast::expression::ExpressionParseStrategy::Precedence::Lowest);
        if (element != nullptr) {
            list.push_back(element->as<ast::Expression>());
        }
    }

    if (itr.next()->get_type() != tokenize::TokenType::RParen) {
        throw std::runtime_error("Expected ')'");
    }

    return parsing::ParseStrategyResult{std::make_shared<CallExpressionNode>(*itr, callee, list), stream.current() + itr.distance()};
}

} // namespace nugdev::compiler::ast::expression
