#pragma once

#include "00_lib/lib/String.h"
#include "01_tokenize/token/TokenType.h"

namespace nugdev::compiler::tokenize {

class Token {
public:
    Token(const TokenType type, const lib::String &literal);

public:
    TokenType get_type() const;
    const lib::String &get_literal() const;

private:
    TokenType m_type;
    lib::String m_literal;
};

}  // namespace nugdev::compiler::tokenize
