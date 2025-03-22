#include "01_tokenize/strategies/OperatorTokenizeStrategy.h"

namespace nugdev::compiler::tokenize {

OperatorTokenizeStrategy::OperatorTokenizeStrategy()
    : m_operatorMap{{L'!', TokenType::ExclamationMark}, {L'#', TokenType::Hash},      {L'$', TokenType::Dollar},
                    {L'%', TokenType::Percent},         {L'&', TokenType::Ampersand}, {L'(', TokenType::LeftParen},
                    {L')', TokenType::RightParen},      {L'*', TokenType::Asterisk},  {L'+', TokenType::Plus},
                    {L',', TokenType::Comma},           {L'-', TokenType::Minus},     {L'.', TokenType::Period},
                    {L'/', TokenType::Slash},           {L':', TokenType::Colon},     {L';', TokenType::SemiColon},
                    {L'<', TokenType::LeftAngle},       {L'=', TokenType::Assign},    {L'>', TokenType::RightAngle},
                    {L'?', TokenType::QuestionMark},    {L'@', TokenType::At},        {L'[', TokenType::LeftBracket},
                    {L']', TokenType::RightBracket},    {L'^', TokenType::Caret},     {L'{', TokenType::LeftBrace},
                    {L'}', TokenType::RightBrace}} {}

bool OperatorTokenizeStrategy::can_handle(const lib::iterator::Workbench<lib::Char>::command_t &command) {
    if (command.valid()) {
        auto ch = command.value();
        return m_operatorMap.find(ch) != m_operatorMap.end();
    }
    return false;
}

std::optional<Token> OperatorTokenizeStrategy::handle(const lib::iterator::Workbench<lib::Char>::command_t &command) {
    if (command.valid()) {
        auto ch = command.value();
        command.next();

        if (command.valid()) {
            auto next_ch = command.value();
            if (ch == '=' && next_ch == '=') {
                command.next();
                return Token(TokenType::Equal, "==");
            }

            if (ch == '!' && next_ch == '=') {
                command.next();
                return Token(TokenType::NotEqual, "!=");
            }

            if (ch == '>' && next_ch == '=') {
                command.next();
                return Token(TokenType::GreaterEqual, ">=");
            }

            if (ch == '<' && next_ch == '=') {
                command.next();
                return Token(TokenType::LessEqual, "<=");
            }

            if (ch == '+' && next_ch == '=') {
                command.next();
                return Token(TokenType::PlusEqual, "+=");
            }

            if (ch == '-' && next_ch == '=') {
                command.next();
                return Token(TokenType::MinusEqual, "-=");
            }

            if (ch == '+' && next_ch == '+') {
                command.next();
                return Token(TokenType::Increment, "++");
            }

            if (ch == '-' && next_ch == '-') {
                command.next();
                return Token(TokenType::Decrement, "--");
            }
        }

        return Token(m_operatorMap[ch], ch);
    }
    return std::nullopt;
}

} // namespace nugdev::compiler::tokenize
