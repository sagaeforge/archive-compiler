#include "ExpressionParseStrategy.h"

namespace nugdev::compiler::ast::expression {

ExpressionParseStrategy::ExpressionParseStrategy() {
    m_prefixParseFns = {};
    m_infixParseFns = {};
}

ExpressionParseStrategy::ExpressionParseStrategy(std::unordered_map<tokenize::TokenType, prefixParseFn> prefixParseFns,
                                                 std::unordered_map<tokenize::TokenType, infixParseFn> infixParseFns) {
    m_prefixParseFns = prefixParseFns;
    m_infixParseFns = infixParseFns;
}

ExpressionParseStrategy::Precedence ExpressionParseStrategy::get_precedence(tokenize::TokenType type) {
    switch (type) {
    case tokenize::TokenType::Equal:
    case tokenize::TokenType::NotEqual:
        return Precedence::Equals;
    case tokenize::TokenType::LessThan:
    case tokenize::TokenType::GreaterThan:
        return Precedence::LessGreater;
    case tokenize::TokenType::Plus:
    case tokenize::TokenType::Minus:
        return Precedence::Sum;
    case tokenize::TokenType::Asterisk:
    case tokenize::TokenType::Slash:
        return Precedence::Product;
    case tokenize::TokenType::LParen:
        return Precedence::Call;
    case tokenize::TokenType::LBracket:
        return Precedence::Index;
    default:
        return Precedence::Unknown;
    }
}

bool ExpressionParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult ExpressionParseStrategy::parse(const tokenize::TokenStream &tokens) { return parse(tokens, Precedence::Lowest); }

parsing::ParseStrategyResult ExpressionParseStrategy::parse(const tokenize::TokenStream &tokens, Precedence precedence) {
    auto stream = tokens.clone();
    auto itr = stream.current();
    auto prefix = m_prefixParseFns.find(itr->get_type());
    if (prefix == m_prefixParseFns.end()) {
        return parsing::ParseStrategyResult{nullptr, tokens.current()};
    }

    auto [leftExpr, leftMoveItr] = prefix->second(stream);
    stream.move(leftMoveItr);
    itr = stream.current();
    while (precedence < get_precedence(itr->get_type())) {
        auto infix = m_infixParseFns.find(itr->get_type());
        if (infix == m_infixParseFns.end()) {
            return parsing::ParseStrategyResult{leftExpr, tokens.current()};
        }

        itr = itr.next();
        auto [rightExpr, rightMoveItr] = infix->second(stream, leftExpr->as<ast::Expression>());
        stream.move(rightMoveItr);
        itr = stream.current();
        leftExpr = rightExpr;
    }

    return parsing::ParseStrategyResult{leftExpr, stream.current() + itr.distance()};
}

} // namespace nugdev::compiler::ast::expression
