#include "GroupExpressionParseStrategy.h"

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"

namespace nugdev::compiler::ast::expression {

bool GroupExpressionParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::LParen; }

parsing::ParseStrategyResult GroupExpressionParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy strategy;

    auto workbench = tokens.clone().next(); // current : '('
    auto [expr, itr] = strategy.parse(workbench);
    workbench.move_at(itr);
    workbench.next(); // 이래야 ')'를 먹어줄수 있을 것 같은데.
    return {expr->as<Expression>(), tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::expression
