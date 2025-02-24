#include "ReturnNodeParseStrategy.h"

#include <stdexcept>

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/statement/return/ReturnNode.h"

namespace nugdev::compiler::ast::statement {

bool ReturnNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current().value().get_type() == tokenize::TokenType::Return; }

parsing::ParseStrategyResult ReturnNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static expression::ExpressionParseStrategy expressionStrategy{};
    static expression::IdentifierLiteralNodeParseStrategy identifierStrategy{};

    auto workbench = tokens.clone(); // current : return
    workbench.next();

    auto return_node = std::make_shared<ReturnNode>(workbench.current().value(), static_cast<std::shared_ptr<Expression>>(nullptr),
                                                    static_cast<std::shared_ptr<Expression>>(nullptr));
    if (contains(workbench.current(), {tokenize::TokenType::At})) {
        workbench.next();
        if (workbench.current().valid() == false) {
            throw std::runtime_error("Invalid token stream");
        }

        auto [label, itr] = identifierStrategy.parse(workbench);
        workbench.move_at(itr);
        return_node->set_label(label->as<Expression>());
    }

    auto [return_expression, itr] = expressionStrategy.parse(workbench);
    workbench.move_at(itr);
    return_node->set_return_expression(return_expression->as<Expression>());

    return {return_node, tokens.begin() + workbench.current().distance()};
}
} // namespace nugdev::compiler::ast::statement
