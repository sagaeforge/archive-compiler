#include "KeywordTokenFactory.h"

namespace nugdev::compiler::tokenize {

KeywordTokenFactory::KeywordTokenFactory() {
    keywordMap.insert({icu::UnicodeString::fromUTF8("function"), TokenType::Function});
    keywordMap.insert({icu::UnicodeString::fromUTF8("let"), TokenType::Let});
    keywordMap.insert({icu::UnicodeString::fromUTF8("true"), TokenType::True});
    keywordMap.insert({icu::UnicodeString::fromUTF8("false"), TokenType::False});
    keywordMap.insert({icu::UnicodeString::fromUTF8("if"), TokenType::If});
    keywordMap.insert({icu::UnicodeString::fromUTF8("else"), TokenType::Else});
    keywordMap.insert({icu::UnicodeString::fromUTF8("return"), TokenType::Return});
}

bool KeywordTokenFactory::canHandle(wchar_t ch) { return ::iswalpha(ch); }

Token KeywordTokenFactory::createToken(std::wistream &stream) {
    icu::UnicodeString value;
    while (stream && !stream.eof()) {
        auto ch = stream.get();
        if (!::iswalnum(ch)) {
            break;
        }
        value += ch;
    }

    auto keyword = keywordMap.find(value);
    if (keyword == keywordMap.end()) {
        throw std::runtime_error("Invalid keyword");
    }

    return Token::from(keyword->second, value);
}

} // namespace nugdev::compiler::tokenize
