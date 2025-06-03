#include "Token.h"

namespace nugdev::compiler::tokenize {

Token::Token(const TokenType type, const lib::String &literal)
    : m_type(type), m_literal(literal) {}

TokenType Token::get_type() const { return m_type; }

lib::String Token::get_literal() const { return m_literal; }

bool Token::operator==(const Token &other) const {
  return m_type == other.m_type && m_literal == other.m_literal;
}

bool Token::operator!=(const Token &other) const { return !(*this == other); }

} // namespace nugdev::compiler::tokenize