#include "OperatorTokenFactory.h"

#include "00_app/stream/StreamWorkbench.hpp"

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
    operatorMap[L'~'] = TokenType::Tilde;
    operatorMap[L'='] = TokenType::Assign;
}

bool OperatorTokenFactory::can_handle(const stream::StringStream &stream) {
    if (!stream.current().valid()) {
        return false;
    }

    return operatorMap.find(stream.current().value()) != operatorMap.end();
}

std::tuple<Token, stream::StringStreamIterator> OperatorTokenFactory::create_token(const stream::StringStream &stream) {
    return stream::workbench(stream, [this](stream::StringStream &workbench) {
        auto ch = workbench.current().value();
        workbench.next();

        if (workbench.current().valid()) {
            if (ch == u'=') {
                auto nextCh = workbench.current().value();
                if (nextCh == u'=') {
                    workbench.next();
                    return Token::from(TokenType::Equal, icu::UnicodeString::fromUTF8("=="));
                }
            }

            if (ch == L'!') {
                auto nextCh = workbench.current().value();
                if (nextCh == L'=') {
                    workbench.next();
                    return Token::from(TokenType::NotEqual, icu::UnicodeString::fromUTF8("!="));
                }
            }

            if (ch == L'+') {
                auto nextCh = workbench.current().value();
                if (nextCh == L'+') {
                    workbench.next();
                    return Token::from(TokenType::Inc, icu::UnicodeString::fromUTF8("++"));
                }

                if (nextCh == L'=') {
                    workbench.next();
                    return Token::from(TokenType::PlusEqual, icu::UnicodeString::fromUTF8("+="));
                }
            }

            if (ch == L'-') {
                auto nextCh = workbench.current().value();
                if (nextCh == L'-') {
                    workbench.next();
                    return Token::from(TokenType::Dec, icu::UnicodeString::fromUTF8("--"));
                }

                if (nextCh == L'>') {
                    workbench.next();
                    return Token::from(TokenType::LeftArrow, icu::UnicodeString::fromUTF8("<-"));
                }

                if (nextCh == L'=') {
                    workbench.next();
                    return Token::from(TokenType::MinusEqual, icu::UnicodeString::fromUTF8("-="));
                }
            }
        }

        return Token::from(operatorMap[ch], ch);
    });
}

} // namespace nugdev::compiler::tokenize