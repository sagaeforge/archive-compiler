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
    auto workbench = tokens.clone();
    auto prefix = m_prefixParseFns.find(workbench.current().value().get_type());
    if (prefix == m_prefixParseFns.end()) {
        throw std::runtime_error("Invalid token stream");
    }

    auto [leftExpr, leftMoveItr] = prefix->second(workbench);
    workbench.move_at(leftMoveItr);
    while (precedence < get_precedence(workbench.current()->get_type())) {
        auto infix = m_infixParseFns.find(workbench.current()->get_type());
        if (infix == m_infixParseFns.end()) {
            return parsing::ParseStrategyResult{leftExpr, tokens.current()};
        }

        workbench.next();
        auto [rightExpr, rightMoveItr] = infix->second(workbench, leftExpr->as<ast::Expression>());
        workbench.move_at(rightMoveItr);
        leftExpr = rightExpr;
    }

    return parsing::ParseStrategyResult{leftExpr, workbench.current() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::expression
