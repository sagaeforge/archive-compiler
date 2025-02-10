#include "Token.h"

namespace nugdev::compiler::tokenize {

Token::Token(TokenType type, icu::UnicodeString literal) : type(type), literal(literal) {}

Token Token::Empty() { return Token(TokenType::Illegal, icu::UnicodeString::fromUTF8("")); }

Token Token::from(TokenType type, icu::UnicodeString literal) { return Token(type, literal); }

icu::UnicodeString Token::to_str() { return literal; }

} // namespace nugdev::compiler::tokenize
