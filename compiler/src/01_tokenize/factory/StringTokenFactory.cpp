#include "StringTokenFactory.h"

namespace nugdev::compiler::tokenize {

bool StringTokenFactory::canHandle(wchar_t ch) { return ch == L'"' || ch == L'\''; }

Token StringTokenFactory::createToken(std::wistream &stream) {
    icu::UnicodeString value;
    auto quote = stream.get();
    while (stream && !stream.eof()) {
        auto ch = stream.get();
        if (ch == quote) {
            stream.get();
            break;
        }
        value += ch;
    }

    return Token::from(TokenType::String, value);
}

} // namespace nugdev::compiler::tokenize
