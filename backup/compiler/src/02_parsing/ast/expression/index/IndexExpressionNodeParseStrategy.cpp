#include "IndexExpressionNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/Parser.h"
#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/index/IndexExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool IndexExpressionNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return tokens.current()->get_type() == tokenize::TokenType::LBracket;
}

parsing::ParseStrategyResult IndexExpressionNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    throw std::runtime_error("Not implemented");
}

parsing::ParseStrategyResult IndexExpressionNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens,
                                                                     std::shared_ptr<Expression> left) {
    static ExpressionParseStrategy expressionStrategy{};

    auto [node, itr] = stream::workbench(tokens, [this, &parser, &left](tokenize::TokenStream &workbench) {
        // current: '['
        auto indexToken = workbench.current();
        workbench.move_next();

        // current: index
        auto [index, indexItr] = expressionStrategy.parse(parser, workbench, parsing::Precedence::Lowest);
        workbench.move_at(indexItr);

        if (!contains(workbench.current(), {tokenize::TokenType::RBracket})) {
            throw std::runtime_error("Expected ']'");
        }
        workbench.move_next();

        return std::make_shared<IndexExpressionNode>(indexToken.value(), left->as<Expression>(), index->as<Expression>());
    });

    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
