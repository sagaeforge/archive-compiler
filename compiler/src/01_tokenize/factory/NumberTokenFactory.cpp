#include "NumberTokenFactory.h"

namespace nugdev::compiler::tokenize {

bool NumberTokenFactory::canHandle(wchar_t ch) { return ::iswdigit(ch); }

Token NumberTokenFactory::createToken(std::wistream &stream) {
    icu::UnicodeString value;
    while (stream && !stream.eof()) {
        auto ch = stream.get();
        if (!::iswdigit(ch)) {
            break;
        }
        value += ch;
    }

    return Token::from(TokenType::Digit, value);
}

} // namespace nugdev::compiler::tokenize
