#include "IdentifierTokenFactory.h"

#include "00_app/stream/Stream.hpp"
#include "00_app/stream/StreamWorkbench.hpp"

namespace nugdev::compiler::tokenize {

bool IdentifierTokenFactory::can_handle(const stream::StringStream &stream) {
    if (!stream.current().valid()) {
        return false;
    }

    auto ch = stream.current().value();
    return ::iswalpha(ch) || ch == L'_';
}

std::tuple<Token, stream::StringStreamIterator> IdentifierTokenFactory::create_token(const stream::StringStream &stream) {
    return stream::workbench(stream, [this](stream::StringStream &workbench) {
        icu::UnicodeString value;
        for (; isIdentifierChar(workbench); workbench.next()) {
            value += workbench.current().value();
        }

        return Token::from(TokenType::Ident, value);
    });
}

bool IdentifierTokenFactory::isIdentifierChar(const stream::StringStream &stream) {
    // 문자 혹은 숫자 혹은 언더바임, 단 유니코드도 지원하기에.
    return stream.current().valid() && (::iswalnum(stream.current().value()) || stream.current().value() == L'_');
}

} // namespace nugdev::compiler::tokenize
