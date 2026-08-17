#include "Precedence.h"

namespace nugdev::compiler::ast {

Precedence get_precedence(tokenize::TokenType type) {
    switch (type) {
    case tokenize::TokenType::In:
        return Precedence::In;
    case tokenize::TokenType::Equal:
    case tokenize::TokenType::NotEqual:
        return Precedence::Equals;
    case tokenize::TokenType::LessEqual:
    case tokenize::TokenType::GreaterEqual:
        return Precedence::LessGreater;
    case tokenize::TokenType::Plus:
    case tokenize::TokenType::Minus:
        return Precedence::Sum;
    case tokenize::TokenType::Asterisk:
    case tokenize::TokenType::Slash:
        return Precedence::Product;
    case tokenize::TokenType::Increment:
    case tokenize::TokenType::Decrement:
        return Precedence::Postfix;
    case tokenize::TokenType::LeftParen:
        return Precedence::Call;
    case tokenize::TokenType::LeftBracket:
        return Precedence::Index;
    default:
        return Precedence::Lowest;
    }
}

}  // namespace nugdev::compiler::ast
