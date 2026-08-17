#include "01_tokenize/token/Token.h"

namespace nugdev::compiler::tokenize {

Token::Token(const TokenType type, const lib::String &literal) : m_type(type), m_literal(literal) {
}

TokenType Token::get_type() const {
    return m_type;
}

const lib::String &Token::get_literal() const {
    return m_literal;
}

}  // namespace nugdev::compiler::tokenize
