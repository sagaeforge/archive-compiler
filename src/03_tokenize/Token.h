#pragma once

#include "01_lib/String.h"
#include "TokenType.h"

namespace nugdev::compiler::tokenize {

class Token {
public:
  using self_t = Token;

public:
  Token(const TokenType type, const lib::String &literal);

  TokenType get_type() const;
  lib::String get_literal() const;

  bool operator==(const Token &other) const;
  bool operator!=(const Token &other) const;

private:
  TokenType m_type;
  lib::String m_literal;
};

} // namespace nugdev::compiler::tokenize