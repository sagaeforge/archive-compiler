#include "01_tokenize/strategies/CommentTokenizeStrategy.h"

namespace nugdev::compiler::tokenize {

bool CommentTokenizeStrategy::can_handle(const lib::iterator::Workbench<lib::Char>::command_t &command) {
    if (command.valid()) {
        auto ch = command.value();
        return ch == '#';
    }
    return false;
}

std::optional<Token> CommentTokenizeStrategy::handle(const lib::iterator::Workbench<lib::Char>::command_t &command) {
    lib::String literal;

    for (; command.valid(); command.next()) {
        auto ch = command.value();
        if (ch == '\n') {
            break;
        }
        literal += ch;
    }

    return Token(TokenType::Comment, literal);
}

} // namespace nugdev::compiler::tokenize
