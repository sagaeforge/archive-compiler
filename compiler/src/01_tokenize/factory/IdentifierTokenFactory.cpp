#include "IdentifierTokenFactory.h"

namespace nugdev::compiler::tokenize {

bool IdentifierTokenFactory::can_handle(const stream::StringStreamIterator &it) { return it.valid() && (::iswalpha(*it) || *it == L'_'); }

std::tuple<Token, stream::StringStreamIterator> IdentifierTokenFactory::create_token(const stream::StringStreamIterator &it) {
    icu::UnicodeString value;

    auto itr = it;
    for (; isIdentifierChar(itr); itr++) {
        value += *itr;
    }

    return std::make_tuple(Token::from(TokenType::Ident, value), itr);
}

bool IdentifierTokenFactory::isIdentifierChar(const stream::StringStreamIterator &it) {
    // 문자 혹은 숫자 혹은 언더바임, 단 유니코드도 지원하기에.
    return it.valid() && (::iswalnum(*it) || *it == L'_');
}

} // namespace nugdev::compiler::tokenize
