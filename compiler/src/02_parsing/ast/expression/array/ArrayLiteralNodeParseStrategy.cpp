#include "ArrayLiteralNodeParseStrategy.h"

#include "02_parsing/ParseStrategy.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/array/ArrayLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool ArrayLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::LBracket; }

parsing::ParseStrategyResult ArrayLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy strategy{};

    auto workbench = tokens.clone();
    if (contains(workbench.current(), {tokenize::TokenType::RBracket})) {
        auto list = std::vector<std::shared_ptr<Expression>>();
        return parsing::ParseStrategyResult{std::make_shared<ArrayLiteralNode>(workbench.current().value(), list), tokens.current().next()};
    }

    std::vector<std::shared_ptr<ast::Expression>> list;
    do {
        auto [element, moveItr] = strategy.parse(workbench, ast::expression::ExpressionParseStrategy::Precedence::Lowest);
        workbench.move_at(moveItr);
        list.push_back(element->as<ast::Expression>());
    } while (contains(workbench.current(), {tokenize::TokenType::Comma}));

    if (!contains(workbench.current(), {tokenize::TokenType::RBracket})) {
        throw std::runtime_error("Expected ']'");
    }

    return {std::make_shared<ArrayLiteralNode>(workbench.current().value(), list), tokens.current().next()};
}
} // namespace nugdev::compiler::ast::expression
