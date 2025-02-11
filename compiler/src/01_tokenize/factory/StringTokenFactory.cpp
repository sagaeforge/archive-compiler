#include "StringTokenFactory.h"

namespace nugdev::compiler::tokenize {

bool StringTokenFactory::canHandle(const stream::StringStreamIterator &it) {
    auto ch = *it;
    return ch == L'"' || ch == L'\'';
}

std::tuple<Token, stream::StringStreamIterator> StringTokenFactory::createToken(const stream::StringStreamIterator &it) {
    icu::UnicodeString value;
    auto quote = *it;
    auto itr = it + 1;
    while (itr.vaild()) {
        auto ch = *itr;
        if (ch == quote) {
            itr++;
            break;
        }
        itr++;
        value += ch;
    }

    return std::make_tuple(Token::from(TokenType::String, value), itr);
}

} // namespace nugdev::compiler::tokenize
