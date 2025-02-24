#include "IndexExpressionNodeParseStrategy.h"

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/index/IndexExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool IndexExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::LBracket; }

parsing::ParseStrategyResult IndexExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) { throw std::runtime_error("Not implemented"); }

parsing::ParseStrategyResult IndexExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> left) {
    static ExpressionParseStrategy expressionStrategy{};

    auto workbench = tokens.clone().next(); // current: '['

    auto [index, indexItr] = expressionStrategy.parse(workbench, ExpressionParseStrategy::Precedence::Lowest);
    workbench.move_at(indexItr);

    if (!contains(workbench.current(), {tokenize::TokenType::RBracket})) {
        throw std::runtime_error("Expected ']'");
    }
    workbench.next();

    return {std::make_shared<IndexExpressionNode>(*tokens.current(), left->as<Expression>(), index->as<Expression>()),
            tokens.current() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::expression
