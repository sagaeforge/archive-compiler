#include "OperatorTokenFactory.h"

namespace nugdev::compiler::tokenize {

OperatorTokenFactory::OperatorTokenFactory() {
    operatorMap[L'!'] = TokenType::ExclamationMark;
    operatorMap[L'#'] = TokenType::Hash;
    operatorMap[L'$'] = TokenType::Dollar;
    operatorMap[L'%'] = TokenType::Percent;
    operatorMap[L'&'] = TokenType::Ampersand;
    operatorMap[L'('] = TokenType::LParen;
    operatorMap[L')'] = TokenType::RParen;
    operatorMap[L'*'] = TokenType::Asterisk;
    operatorMap[L'+'] = TokenType::Plus;
    operatorMap[L','] = TokenType::Comma;
    operatorMap[L'-'] = TokenType::Minus;
    operatorMap[L'.'] = TokenType::Period;
    operatorMap[L'/'] = TokenType::Slash;
    operatorMap[L':'] = TokenType::Colon;
    operatorMap[L';'] = TokenType::SemiColon;
    operatorMap[L'<'] = TokenType::LessThan;
    operatorMap[L'>'] = TokenType::GreaterThan;
    operatorMap[L'?'] = TokenType::QuestionMark;
    operatorMap[L'@'] = TokenType::At;
    operatorMap[L'['] = TokenType::LBracket;
    operatorMap[L']'] = TokenType::RBracket;
    operatorMap[L'^'] = TokenType::Caret;
    // operatorMap[L'`'] = TokenType::BackTick;
    operatorMap[L'{'] = TokenType::LBrace;
    operatorMap[L'|'] = TokenType::Pipe;
    operatorMap[L'}'] = TokenType::RBrace;
}

bool OperatorTokenFactory::canHandle(const stream::StringStreamIterator &it) {
    auto ch = *it;
    return operatorMap.find(ch) != operatorMap.end();
}

std::tuple<Token, stream::StringStreamIterator> OperatorTokenFactory::createToken(const stream::StringStreamIterator &it) {
    auto ch = *it;

    if (ch == L'=') {
        auto nextCh = *(it + 1);
        if (nextCh == L'=') {
            return std::make_tuple(Token::from(TokenType::Equal, icu::UnicodeString::fromUTF8("==")), it + 2);
        }
    }

    if (ch == L'!') {
        auto nextCh = *(it + 1);
        if (nextCh == L'=') {
            return std::make_tuple(Token::from(TokenType::NotEqual, icu::UnicodeString::fromUTF8("!=")), it + 2);
        }
    }

    if (ch == L'+') {
        auto nextCh = *(it + 1);
        if (nextCh == L'+') {
            return std::make_tuple(Token::from(TokenType::Inc, icu::UnicodeString::fromUTF8("++")), it + 2);
        }

        if (nextCh == L'=') {
            return std::make_tuple(Token::from(TokenType::PlusEqual, icu::UnicodeString::fromUTF8("+=")), it + 2);
        }
    }

    if (ch == L'-') {
        auto nextCh = *(it + 1);
        if (nextCh == L'-') {
            return std::make_tuple(Token::from(TokenType::Dec, icu::UnicodeString::fromUTF8("--")), it + 2);
        }
    }

    if (ch == L'-') {
        auto nextCh = *(it + 1);
        if (nextCh == L'>') {
            return std::make_tuple(Token::from(TokenType::LeftArrow, icu::UnicodeString::fromUTF8("<-")), it + 2);
        }
    }

    return std::make_tuple(Token::from(operatorMap[ch], ch), it + 1);
}

} // namespace nugdev::compiler::tokenize