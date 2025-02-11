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

bool KeywordTokenFactory::canHandle(const stream::StringStreamIterator &it) {
    auto ch = *it;
    return ::iswalpha(ch);
}

std::tuple<Token, stream::StringStreamIterator> KeywordTokenFactory::createToken(const stream::StringStreamIterator &it) {
    icu::UnicodeString value;
    auto itr = it;
    for (; canHandle(itr); itr++) {
        value += *itr;
    }

    auto keyword = keywordMap.find(value);
    if (keyword == keywordMap.end()) {
        throw std::runtime_error("Invalid keyword");
    }

    return std::make_tuple(Token::from(keyword->second, value), itr);
}

} // namespace nugdev::compiler::tokenize
