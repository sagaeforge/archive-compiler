#include "NumberTokenFactory.h"

namespace nugdev::compiler::tokenize {

bool NumberTokenFactory::can_handle(const stream::StringStreamIterator &it) {
    auto ch = *it;
    return ::iswdigit(ch);
}

std::tuple<Token, stream::StringStreamIterator> NumberTokenFactory::create_token(const stream::StringStreamIterator &it) {
    icu::UnicodeString value;
    auto itr = it;
    for (; can_handle(itr); itr++) {
        value += *itr;
    }

    return std::make_tuple(Token::from(TokenType::Number, value), itr);
}

} // namespace nugdev::compiler::tokenize
