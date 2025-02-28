#include "NumberTokenFactory.h"

namespace nugdev::compiler::tokenize {

bool NumberTokenFactory::can_handle(const stream::StringStreamIterator &it) { return it.valid() && (::iswdigit(*it) || *it == '-'); }

std::tuple<Token, stream::StringStreamIterator> NumberTokenFactory::create_token(const stream::StringStreamIterator &it) {
    icu::UnicodeString value;
    auto itr = it;
    bool has_decimal = false;
    bool has_exponent = false;

    if (*itr == '-') {
        value += *itr++;
    }

    while (::iswdigit(*itr)) {
        value += *itr++;
    }

    if (*itr == '.') {
        has_decimal = true;
        value += *itr++;
        while (::iswdigit(*itr)) {
            value += *itr++;
        }
    }

    if (*itr == 'e' || *itr == 'E') {
        has_exponent = true;
        value += *itr++;

        if (*itr == '+' || *itr == '-') {
            value += *itr++;
        }

        if (!::iswdigit(*itr)) {
            throw std::runtime_error("Invalid number format: exponent without digits");
        }
        while (::iswdigit(*itr)) {
            value += *itr++;
        }
    }

    return std::make_tuple(Token::from(TokenType::Number, value), itr);
}

} // namespace nugdev::compiler::tokenize
