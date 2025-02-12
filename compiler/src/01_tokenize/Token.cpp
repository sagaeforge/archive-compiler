#include "Token.h"

#include <sstream>
#include <string>

namespace nugdev::compiler::tokenize {

Token::Token(TokenType type, icu::UnicodeString literal) : type(type), literal(literal) {}

Token Token::Empty() { return Token(TokenType::Illegal, icu::UnicodeString::fromUTF8("")); }

Token Token::from(TokenType type, icu::UnicodeString literal) { return Token(type, literal); }

icu::UnicodeString Token::to_str() {
    std::string literal_str;
    literal.toUTF8String(literal_str);

    std::string type_str = std::to_string(static_cast<int>(type));

    std::stringstream ss;
    ss << "{ type: " << type_str << ", literal: " << literal_str << " }";
    return icu::UnicodeString::fromUTF8(ss.str());
}

} // namespace nugdev::compiler::tokenize
