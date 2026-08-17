#include "StringTokenizeStrategy.h"

namespace nugdev::compiler::tokenize {

bool StringTokenizeStrategy::can_handle(const lib::iterator::Workbench<lib::Char>::command_t &command) {
    if (command.valid()) {
        auto ch = command.value();
        return ch == L'"' || ch == L'\'';
    }
    return false;
}

std::optional<Token> StringTokenizeStrategy::handle(const lib::iterator::Workbench<lib::Char>::command_t &command) {
    if (command.valid()) {
        auto ch = command.value();
        command.next();

        lib::String literal;
        while (command.valid() && command.value() != ch) {
            literal += command.value();
            command.next();
        }

        if (command.valid() && command.value() == ch) {
            command.next();
        }

        return Token(TokenType::String, literal);
    }
    return std::nullopt;
}

}  // namespace nugdev::compiler::tokenize
