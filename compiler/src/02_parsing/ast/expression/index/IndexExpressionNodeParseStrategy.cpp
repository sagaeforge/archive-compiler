#include "IndexExpressionNodeParseStrategy.h"

#include "02_parsing/ast/expression/ExpressionParseStrategy.h"
#include "02_parsing/ast/expression/index/IndexExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool IndexExpressionNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::LBracket; }

parsing::ParseStrategyResult IndexExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens) { throw std::runtime_error("Not implemented"); }

parsing::ParseStrategyResult IndexExpressionNodeParseStrategy::parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> left) {
    auto stream = tokens.clone();
    auto itr = stream.current().next().next();

    auto [index, indexItr] = ExpressionParseStrategy().parse(stream, ExpressionParseStrategy::Precedence::Lowest);
    if (index == nullptr) {
        throw std::runtime_error("Expected index expression");
    }

    if (itr.next()->get_type() != tokenize::TokenType::RBracket) {
        throw std::runtime_error("Expected ']'");
    }

    return parsing::ParseStrategyResult{std::make_shared<IndexExpressionNode>(*itr, left, index->as<Expression>()), stream.current() + itr.distance()};
}

} // namespace nugdev::compiler::ast::expression
