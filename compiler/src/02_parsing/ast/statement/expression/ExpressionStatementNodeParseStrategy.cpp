#include "ExpressionStatementNodeParseStrategy.h"

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/statement/expression/ExpressionStatementNode.h"

namespace nugdev::compiler::ast::statement {

bool ExpressionStatementNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) {
    static expression::ExpressionParseStrategy strategy{};
    return strategy.can_parse(tokens);
}

parsing::ParseStrategyResult ExpressionStatementNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static expression::ExpressionParseStrategy strategy{};

    auto workbench = tokens.clone();

    auto [expression, itr] = strategy.parse(workbench);
    workbench.move_at(itr);
    return {std::make_shared<ExpressionStatementNode>(expression->as<Expression>()), tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::statement
