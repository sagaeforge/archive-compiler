#pragma once

#include "01_lib/String.h"
#include "TokenType.h"
#include <compare>
#include <iostream>

namespace nugdev::compiler::tokenize {

class Token {
public:
  using self_t = Token;

public:
  // 기본 생성자 (컨테이너 사용을 위해 필수)
  Token() : m_type(TokenType::Illegal), m_literal("") {}

  Token(const TokenType type, const lib::String &literal)
      : m_type(type), m_literal(literal) {}

  TokenType get_type() const;
  lib::String get_literal() const;

  // C++20 우주선 연산자로 모든 비교 연산자 자동 생성
  std::strong_ordering operator<=>(const Token &other) const;
  bool operator==(const Token &other) const;

public:
  // 디버깅을 위한 필수 메서드들
  lib::String to_debug_string() const;
  friend std::ostream &operator<<(std::ostream &os, const Token &token);

private:
  TokenType m_type;
  lib::String m_literal;
};

} // namespace nugdev::compiler::tokenize