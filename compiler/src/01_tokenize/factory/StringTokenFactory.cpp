#include "StringTokenFactory.h"

#include "00_app/stream/StreamWorkbench.hpp"

namespace nugdev::compiler::tokenize {

bool StringTokenFactory::can_handle(const stream::StringStream &stream) {
    if (!stream.current().valid()) {
        return false;
    }

    auto ch = stream.current().value();
    return ch == L'"' || ch == L'\'';
}

std::tuple<Token, stream::StringStreamIterator> StringTokenFactory::create_token(const stream::StringStream &stream) {
    return stream::workbench(stream, [this](stream::StringStream &workbench) {
        icu::UnicodeString value;
        auto quote = workbench.current().value();
        workbench.next();

        for (; workbench.current().valid(); workbench.next()) {
            auto ch = workbench.current().value();
            if (ch == quote) {
                break;
            }
            value += ch;
        }

        workbench.next();
        return Token::from(TokenType::String, value);
    });
}

} // namespace nugdev::compiler::tokenize
