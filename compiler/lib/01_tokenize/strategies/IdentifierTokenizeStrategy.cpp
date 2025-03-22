#include "IdentifierTokenizeStrategy.h"

namespace nugdev::compiler::tokenize {

IdentifierTokenizeStrategy::IdentifierTokenizeStrategy()
    : m_keywordMap{{u"in", TokenType::In},
                   {u"and", TokenType::And},
                   {u"or", TokenType::Or},
                   {u"let", TokenType::Let},
                   {u"if", TokenType::If},
                   {u"elif", TokenType::Elif},
                   {u"else", TokenType::Else},
                   {u"when", TokenType::When},
                   {u"for", TokenType::For},
                   {u"break", TokenType::Break},
                   {u"continue", TokenType::Continue},
                   {u"function", TokenType::Function},
                   {u"return", TokenType::Return},
                   {u"struct", TokenType::Struct}} {}

bool IdentifierTokenizeStrategy::can_handle(const lib::iterator::Workbench<lib::Char>::command_t &command) {
    if (command.valid()) {
        auto ch = command.value();
        return ch == '_' || std::isalpha(ch);
    }
    return false;
}

std::optional<Token> IdentifierTokenizeStrategy::handle(const lib::iterator::Workbench<lib::Char>::command_t &command) {
    lib::String literal;

    for (; command.valid(); command.next()) {
        auto ch = command.value();
        if (std::isalnum(ch) || ch == '_') {
            literal += ch;
        } else {
            break;
        }
    }

    if (auto keyword = m_keywordMap.find(literal); keyword != m_keywordMap.end()) {
        return Token(keyword->second, literal);
    }
    return Token(TokenType::Identifier, literal);
}

} // namespace nugdev::compiler::tokenize
