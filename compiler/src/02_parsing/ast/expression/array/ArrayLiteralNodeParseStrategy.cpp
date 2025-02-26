#include "ArrayLiteralNodeParseStrategy.h"

#include "02_parsing/ParseStrategy.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/array/ArrayLiteralNode.h"
#include <vector>

namespace nugdev::compiler::ast::expression {

bool ArrayLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::LBracket; }

parsing::ParseStrategyResult ArrayLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy strategy{};

    std::vector<std::shared_ptr<ast::Expression>> list;

    auto workbench = tokens.clone(); // current: '['
    do {
        workbench.next();

        if (contains(workbench.current(), {tokenize::TokenType::RBracket})) {
            workbench.next();

            return parsing::ParseStrategyResult{
                std::make_shared<ArrayLiteralNode>(workbench.current().value(), std::vector<std::shared_ptr<ast::Expression>>()),
                tokens.begin() + workbench.current().distance()};
        }

        auto [element, moveItr] = strategy.parse(workbench, ast::expression::ExpressionParseStrategy::Precedence::Lowest);
        workbench.move_at(moveItr);
        list.push_back(element->as<ast::Expression>());
    } while (contains(workbench.current(), {tokenize::TokenType::Comma}));

    if (!contains(workbench.current(), {tokenize::TokenType::RBracket})) {
        throw std::runtime_error("Expected ']'");
    }
    workbench.next();

    return {std::make_shared<ArrayLiteralNode>(workbench.current().value(), list), tokens.begin() + workbench.current().distance()};
}
} // namespace nugdev::compiler::ast::expression
