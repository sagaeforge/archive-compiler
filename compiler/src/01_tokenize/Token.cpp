#include "Token.h"

#include <sstream>
#include <string>

namespace nugdev::compiler::tokenize {

Token::Token(TokenType type, icu::UnicodeString literal) : type(type), literal(literal) {}

Token Token::empty() { return Token(TokenType::Illegal, icu::UnicodeString::fromUTF8("")); }

Token Token::from(TokenType type, icu::UnicodeString literal) { return Token(type, literal); }

icu::UnicodeString Token::to_str() {
    std::string literal_str;
    literal.toUTF8String(literal_str);

    std::string type_str = std::to_string(static_cast<int>(type));

    std::stringstream ss;
    ss << "{ type: " << type_str << ", literal: " << literal_str << " }";
    return icu::UnicodeString::fromUTF8(ss.str());
}

TokenType Token::get_type() const { return this->type; }

icu::UnicodeString Token::get_literal() const { return this->literal; }

} // namespace nugdev::compiler::tokenize

namespace std {

std::string to_string(const nugdev::compiler::tokenize::TokenType &type) {
    switch (type) {
    case nugdev::compiler::tokenize::TokenType::Illegal:
        return "Illegal";
    case nugdev::compiler::tokenize::TokenType::EoF:
        return "EOF";

    // Identifiers + literals
    case nugdev::compiler::tokenize::TokenType::Ident:
        return "Ident";
    case nugdev::compiler::tokenize::TokenType::Number:
        return "Number";
    case nugdev::compiler::tokenize::TokenType::String:
        return "String";

    // Single character operators
    case nugdev::compiler::tokenize::TokenType::ExclamationMark:
        return "ExclamationMark";
    case nugdev::compiler::tokenize::TokenType::Hash:
        return "Hash";
    case nugdev::compiler::tokenize::TokenType::Dollar:
        return "Dollar";
    case nugdev::compiler::tokenize::TokenType::Percent:
        return "Percent";
    case nugdev::compiler::tokenize::TokenType::Ampersand:
        return "Ampersand";
    case nugdev::compiler::tokenize::TokenType::LParen:
        return "LParen";
    case nugdev::compiler::tokenize::TokenType::RParen:
        return "RParen";
    case nugdev::compiler::tokenize::TokenType::Asterisk:
        return "Asterisk";
    case nugdev::compiler::tokenize::TokenType::Plus:
        return "Plus";
    case nugdev::compiler::tokenize::TokenType::Comma:
        return "Comma";
    case nugdev::compiler::tokenize::TokenType::Minus:
        return "Minus";
    case nugdev::compiler::tokenize::TokenType::Period:
        return "Period";
    case nugdev::compiler::tokenize::TokenType::Slash:
        return "Slash";
    case nugdev::compiler::tokenize::TokenType::Colon:
        return "Colon";
    case nugdev::compiler::tokenize::TokenType::SemiColon:
        return "SemiColon";
    case nugdev::compiler::tokenize::TokenType::LessThan:
        return "LessThan";
    case nugdev::compiler::tokenize::TokenType::Assign:
        return "Assign";
    case nugdev::compiler::tokenize::TokenType::GreaterThan:
        return "GreaterThan";
    case nugdev::compiler::tokenize::TokenType::QuestionMark:
        return "QuestionMark";
    case nugdev::compiler::tokenize::TokenType::At:
        return "At";
    case nugdev::compiler::tokenize::TokenType::LBracket:
        return "LBracket";
    case nugdev::compiler::tokenize::TokenType::RBracket:
        return "RBracket";
    case nugdev::compiler::tokenize::TokenType::Caret:
        return "Caret";
    case nugdev::compiler::tokenize::TokenType::LBrace:
        return "LBrace";
    case nugdev::compiler::tokenize::TokenType::Pipe:
        return "Pipe";
    case nugdev::compiler::tokenize::TokenType::RBrace:
        return "RBrace";
    case nugdev::compiler::tokenize::TokenType::Tilde:
        return "Tilde";

    // Multi-character operators
    case nugdev::compiler::tokenize::TokenType::Equal:
        return "Equal";
    case nugdev::compiler::tokenize::TokenType::NotEqual:
        return "NotEqual";
    case nugdev::compiler::tokenize::TokenType::Inc:
        return "Inc";
    case nugdev::compiler::tokenize::TokenType::Dec:
        return "Dec";
    case nugdev::compiler::tokenize::TokenType::PlusEqual:
        return "PlusEqual";
    case nugdev::compiler::tokenize::TokenType::MinusEqual:
        return "MinusEqual";
    case nugdev::compiler::tokenize::TokenType::LeftArrow:
        return "LeftArrow";

    // Keywords
    case nugdev::compiler::tokenize::TokenType::Function:
        return "Function";
    case nugdev::compiler::tokenize::TokenType::Let:
        return "Let";
    case nugdev::compiler::tokenize::TokenType::True:
        return "True";
    case nugdev::compiler::tokenize::TokenType::False:
        return "False";
    case nugdev::compiler::tokenize::TokenType::If:
        return "If";
    case nugdev::compiler::tokenize::TokenType::Elif:
        return "Elif";
    case nugdev::compiler::tokenize::TokenType::Else:
        return "Else";
    case nugdev::compiler::tokenize::TokenType::Return:
        return "Return";
    case nugdev::compiler::tokenize::TokenType::For:
        return "For";
    case nugdev::compiler::tokenize::TokenType::Break:
        return "Break";
    case nugdev::compiler::tokenize::TokenType::Continue:
        return "Continue";
    case nugdev::compiler::tokenize::TokenType::When:
        return "When";
    case nugdev::compiler::tokenize::TokenType::Override:
        return "Override";

    default:
        return "Unknown";
    }
}
} // namespace std