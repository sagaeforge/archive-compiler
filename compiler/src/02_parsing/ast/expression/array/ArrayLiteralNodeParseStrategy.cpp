#include "ArrayLiteralNodeParseStrategy.h"

#include "02_parsing/ParseStrategy.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/array/ArrayLiteralNode.h"
#include <vector>

namespace nugdev::compiler::ast::expression {

bool ArrayLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::LBracket; }

parsing::ParseStrategyResult ArrayLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto stream = tokens.clone();
    auto itr = stream.current();
    if (itr->get_type() == tokenize::TokenType::RBracket) {
        auto list = std::vector<std::shared_ptr<Expression>>();
        return parsing::ParseStrategyResult{std::make_shared<ArrayLiteralNode>(*itr, list), tokens.current().next()};
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

    if (itr.next()->get_type() != tokenize::TokenType::RBracket) {
        throw std::runtime_error("Expected ']'");
    }

    return parsing::ParseStrategyResult{std::make_shared<ArrayLiteralNode>(*itr, list), stream.current() + itr.distance()};
}
} // namespace nugdev::compiler::ast::expression
