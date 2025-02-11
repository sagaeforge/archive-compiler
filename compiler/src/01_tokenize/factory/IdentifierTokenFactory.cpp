#include "IdentifierTokenFactory.h"

namespace nugdev::compiler::tokenize {

bool IdentifierTokenFactory::canHandle(const stream::StringStreamIterator &it) {
    auto ch = *it;
    return iswalpha(ch) || ch == L'_';
}

std::tuple<Token, stream::StringStreamIterator> IdentifierTokenFactory::createToken(const stream::StringStreamIterator &it) {
    icu::UnicodeString value;

    auto itr = it;
    for (; isIdentifierChar(itr); itr++) {
        value += *itr;
    }

    return std::make_tuple(Token::from(TokenType::Ident, value), itr);
}

bool IdentifierTokenFactory::isIdentifierChar(const stream::StringStreamIterator &it) {
    // 문자 혹은 숫자 혹은 언더바임, 단 유니코드도 지원하기에.
    auto ch = *it;
    return ::iswalnum(ch) || ch == L'_';
}

} // namespace nugdev::compiler::tokenize
