#include "GroupExpressionParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"

namespace nugdev::compiler::ast::expression {

bool GroupExpressionParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return contains(tokens.current(), {tokenize::TokenType::LParen}); }

parsing::ParseStrategyResult GroupExpressionParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static ExpressionParseStrategy strategy;

    auto [node, itr] = stream::workbench(tokens, [this](tokenize::TokenStream &workbench) {
        // current: '('
        workbench.next();

        // current: expression
        auto [expr, exprItr] = strategy.parse(workbench);
        workbench.move_at(exprItr);

        // current: ')'
        if (!contains(workbench.current(), {tokenize::TokenType::RParen})) {
            throw std::runtime_error("Expected ')' after expression");
        }
        workbench.next();

        return expr;
    });

    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
