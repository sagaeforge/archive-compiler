#include "IdentifierTokenFactory.h"

#include "IdentifierToken.h"
#include <cwctype>

namespace nugdev::compiler::tokenize {

bool IdentifierTokenFactory::canHandle(wchar_t ch) { return iswalpha(ch) || ch == L'_'; }

std::shared_ptr<Token> IdentifierTokenFactory::createToken(std::wistream &stream) {
    icu::UnicodeString value;
    while (stream && !stream.eof()) {
        auto ch = stream.get();
        if (!isIdentifierChar(ch)) {
            break;
        }
        value += ch;
    }

    return std::make_shared<IdentifierToken>(value);
}

bool IdentifierTokenFactory::isIdentifierChar(wchar_t ch) {
    // 문자 혹은 숫자 혹은 언더바임, 단 유니코드도 지원하기에.
    return ::iswalnum(ch) || ch == L'_';
}

} // namespace nugdev::compiler::tokenize
