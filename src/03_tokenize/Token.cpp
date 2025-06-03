#include "Token.h"
#include <compare>
#include <sstream>

namespace nugdev::compiler::tokenize {

TokenType Token::get_type() const { return m_type; }

lib::String Token::get_literal() const { return m_literal; }

// C++20 우주선 연산자 구현
std::strong_ordering Token::operator<=>(const Token &other) const {
  // 먼저 타입으로 비교
  if (auto cmp = static_cast<int>(m_type) <=> static_cast<int>(other.m_type);
      cmp != 0) {
    return cmp;
  }
  // 타입이 같으면 리터럴로 비교 (일단 string 비교로 처리)
  auto thisStr = m_literal.to_string();
  auto otherStr = other.m_literal.to_string();
  return thisStr <=> otherStr;
}

bool Token::operator==(const Token &other) const {
  return m_type == other.m_type && m_literal == other.m_literal;
}

// 디버깅 메서드들 (개발 과정에서 필수)
lib::String Token::to_debug_string() const {
  std::ostringstream oss;
  oss << "Token{type=" << static_cast<int>(m_type) << ", literal=\""
      << m_literal.to_string() << "\"}";
  return lib::String(oss.str());
}

std::ostream &operator<<(std::ostream &os, const Token &token) {
  os << token.to_debug_string().to_string();
  return os;
}

} // namespace nugdev::compiler::tokenize