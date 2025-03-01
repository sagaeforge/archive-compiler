#include "NumberTokenFactory.h"

#include "00_app/stream/StreamWorkbench.hpp"

namespace nugdev::compiler::tokenize {

bool NumberTokenFactory::can_handle(const stream::StringStream &stream) {
    if (!stream.current().valid()) {
        return false;
    }

    return ::iswdigit(stream.current().value()) || stream.current().value() == '-';
}

std::tuple<Token, stream::StringStreamIterator> NumberTokenFactory::create_token(const stream::StringStream &stream) {
    return stream::workbench(stream, [this](stream::StringStream &workbench) {
        icu::UnicodeString value;
        bool has_decimal = false;
        bool has_exponent = false;

        if (*workbench.current() == '-') {
            value += workbench.current().value();

            workbench.next();
        }

        while (workbench.current().valid() && ::iswdigit(workbench.current().value())) {
            value += workbench.current().value();
            workbench.next();
        }

        if (workbench.current().valid() && workbench.current().value() == '.') {
            has_decimal = true;
            value += workbench.current().value();
            workbench.next();
            while (workbench.current().valid() && ::iswdigit(workbench.current().value())) {
                value += workbench.current().value();
                workbench.next();
            }
        }

        if (workbench.current().valid() && (workbench.current().value() == 'e' || workbench.current().value() == 'E')) {
            has_exponent = true;
            value += workbench.current().value();
            workbench.next();

            if (workbench.current().valid() && (workbench.current().value() == '+' || workbench.current().value() == '-')) {
                value += workbench.current().value();
                workbench.next();
            }

            if (workbench.current().valid() && !::iswdigit(workbench.current().value())) {
                throw std::runtime_error("Invalid number format: exponent without digits");
            }
            while (workbench.current().valid() && ::iswdigit(workbench.current().value())) {
                value += workbench.current().value();
                workbench.next();
            }
        }

        return Token::from(TokenType::Number, value);
    });
}

} // namespace nugdev::compiler::tokenize
