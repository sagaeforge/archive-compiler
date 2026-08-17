#include "ExpressionStatementNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/statement/expression/ExpressionStatementNode.h"

namespace nugdev::compiler::ast::statement {

bool ExpressionStatementNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    static expression::ExpressionParseStrategy strategy{};
    return strategy.can_parse(parser, tokens);
}

parsing::ParseStrategyResult ExpressionStatementNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    static expression::ExpressionParseStrategy strategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        auto [expression, itr] = strategy.parse(parser, workbench);
        workbench.move_at(itr);
        return std::make_shared<ExpressionStatementNode>(expression->as<Expression>());
    });

    return {node, itr};
}

} // namespace nugdev::compiler::ast::statement
