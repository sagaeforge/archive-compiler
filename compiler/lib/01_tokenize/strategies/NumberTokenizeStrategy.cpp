#include "NumberTokenizeStrategy.h"

namespace nugdev::compiler::tokenize {

bool NumberTokenizeStrategy::can_handle(const lib::iterator::Workbench<lib::Char>::command_t &command) {
    if (command.valid()) {
        auto ch = command.value();
        return std::isdigit(ch) || ch == '-' || ch == '.';
    }
    return false;
}

std::optional<Token> NumberTokenizeStrategy::handle(const lib::iterator::Workbench<lib::Char>::command_t &command) {
    lib::String literal;
    bool has_any_digit = false;

    // 부호 처리
    if (command.valid() && command.value() == '-') {
        literal += command.value();
        command.next();
    }

    // 정수 부분 파싱
    while (command.valid() && ::iswdigit(command.value())) {
        has_any_digit = true;
        literal += command.value();
        command.next();
    }

    // 소수점 처리
    if (command.valid() && command.value() == '.') {
        // 소수점으로 시작하는 경우 (예: .123) 앞에 0 추가
        if (literal.isEmpty()) {
            literal += '0';
        }

        literal += command.value();
        command.next();

        // 소수 부분 파싱
        bool had_digits_after_decimal = false;
        while (command.valid() && ::iswdigit(command.value())) {
            has_any_digit = true;
            had_digits_after_decimal = true;
            literal += command.value();
            command.next();
        }

        // 소수점 뒤에 숫자가 없으면 자동으로 '0' 추가
        if (!had_digits_after_decimal) {
            literal += '0';
        }
    }

    // 지수 부분 파싱
    if (command.valid() && (command.value() == 'e' || command.value() == 'E')) {
        literal += command.value();
        command.next();

        // 지수 부호 처리
        if (command.valid() && (command.value() == '+' || command.value() == '-')) {
            literal += command.value();
            command.next();
        }

        // 지수 숫자 파싱
        while (command.valid() && ::iswdigit(command.value())) {
            has_any_digit = true;
            literal += command.value();
            command.next();
        }
    }

    return Token(TokenType::Number, literal);
}

}  // namespace nugdev::compiler::tokenize
