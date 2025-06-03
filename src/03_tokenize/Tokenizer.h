#pragma once

#include "01_lib/Char.h"
#include "01_lib/String.h"
#include "Token.h"
#include "TokenType.h"
#include <unordered_map>
#include <vector>

namespace nugdev::compiler::tokenize {

/**
 * @brief Scanner로부터 나온 줄 단위 문자열들을 Token 배열로 변환하는 클래스
 */
class Tokenizer {
public:
  Tokenizer();
  ~Tokenizer() = default;

public:
  // 단일 라인을 토크나이징
  std::vector<Token> tokenize_line(const lib::String &line);

  // 여러 라인을 토크나이징 (Scanner 출력 처리)
  std::vector<Token> tokenize_lines(const std::vector<lib::String> &lines);

  // 단일 문자열을 토크나이징
  std::vector<Token> tokenize(const lib::String &input);

private:
  // 키워드 매핑 테이블
  std::unordered_map<std::string, TokenType> m_keywords;

  // 현재 파싱 상태
  size_t m_current_pos;
  size_t m_current_line;
  size_t m_current_column;
  lib::String m_current_input;
  std::string m_cached_input; // 성능 최적화를 위한 캐시

private:
  // 초기화 메서드들
  void initialize_keywords();

  // 파싱 헬퍼 메서드들
  bool is_at_end() const;
  char current_char() const;
  char peek_char(size_t offset = 1) const;
  char advance();
  void skip_whitespace();
  void skip_to_line_end(); // 주석 처리용

  // 토큰 생성 메서드들
  Token make_token(TokenType type, const lib::String &literal);
  Token make_error_token(const lib::String &message);

  // 개별 토큰 타입 파싱 메서드들
  Token parse_number();
  Token parse_string();
  Token parse_identifier_or_keyword();
  Token parse_operator_or_punctuation(char first_char);

  // 유틸리티
  TokenType lookup_keyword(const std::string &text) const;
};

} // namespace nugdev::compiler::tokenize