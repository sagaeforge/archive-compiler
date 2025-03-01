#include "KeywordTokenFactory.h"

#include "00_app/exception/TokenizeException.hpp"
#include "00_app/stream/StreamWorkbench.hpp"
namespace nugdev::compiler::tokenize {

KeywordTokenFactory::KeywordTokenFactory() {
    keywordMap.insert({icu::UnicodeString::fromUTF8("fn"), TokenType::Function});
    keywordMap.insert({icu::UnicodeString::fromUTF8("let"), TokenType::Let});
    keywordMap.insert({icu::UnicodeString::fromUTF8("true"), TokenType::True});
    keywordMap.insert({icu::UnicodeString::fromUTF8("false"), TokenType::False});
    keywordMap.insert({icu::UnicodeString::fromUTF8("if"), TokenType::If});
    keywordMap.insert({icu::UnicodeString::fromUTF8("elif"), TokenType::Elif});
    keywordMap.insert({icu::UnicodeString::fromUTF8("else"), TokenType::Else});
    keywordMap.insert({icu::UnicodeString::fromUTF8("return"), TokenType::Return});
    keywordMap.insert({icu::UnicodeString::fromUTF8("for"), TokenType::For});
    keywordMap.insert({icu::UnicodeString::fromUTF8("break"), TokenType::Break});
    keywordMap.insert({icu::UnicodeString::fromUTF8("continue"), TokenType::Continue});
    keywordMap.insert({icu::UnicodeString::fromUTF8("struct"), TokenType::Struct});
    keywordMap.insert({icu::UnicodeString::fromUTF8("when"), TokenType::When});
    keywordMap.insert({icu::UnicodeString::fromUTF8("override"), TokenType::Override});
    keywordMap.insert({icu::UnicodeString::fromUTF8("in"), TokenType::In});
}

bool KeywordTokenFactory::can_handle(const stream::StringStream &stream) {
    if (!stream.current().valid()) {
        return false;
    }

    return ::iswalpha(stream.current().value());
}

std::tuple<Token, stream::StringStreamIterator> KeywordTokenFactory::create_token(const stream::StringStream &stream) {
    return stream::workbench(stream, [this](stream::StringStream &workbench) {
        icu::UnicodeString value;
        for (; can_handle(workbench); workbench.next()) {
            value += *workbench.current();
        }

        auto keyword = keywordMap.find(value);
        if (keyword == keywordMap.end()) {
            throw exception::InvalidKeywordException(value);
        }

        return Token::from(keyword->second, value);
    });
}

} // namespace nugdev::compiler::tokenize
