#include "NumberTokenFactory.h"

namespace nugdev::compiler::tokenize {

bool NumberTokenFactory::canHandle(const stream::StringStreamIterator &it) {
    auto ch = *it;
    return ::iswdigit(ch);
}

std::tuple<Token, stream::StringStreamIterator> NumberTokenFactory::createToken(const stream::StringStreamIterator &it) {
    icu::UnicodeString value;
    auto itr = it;
    for (; canHandle(itr); itr++) {
        value += *itr;
    }

    return std::make_tuple(Token::from(TokenType::Digit, value), itr);
}

} // namespace nugdev::compiler::tokenize
