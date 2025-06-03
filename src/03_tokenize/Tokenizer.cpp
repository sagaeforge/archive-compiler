#include "Tokenizer.h"
#include "TokenCategories.h"
#include <cctype>
#include <sstream>

namespace nugdev::compiler::tokenize {

Tokenizer::Tokenizer()
    : m_current_pos(0), m_current_line(0), m_current_column(0) {
  initialize_keywords();
}

void Tokenizer::initialize_keywords() {
  m_keywords = {{"let", TokenType::Let},
                {"mut", TokenType::Mut},
                {"if", TokenType::If},
                {"elif", TokenType::Elif},
                {"else", TokenType::Else},
                {"for", TokenType::For},
                {"break", TokenType::Break},
                {"continue", TokenType::Continue},
                {"function", TokenType::Function},
                {"return", TokenType::Return},
                {"when", TokenType::When},
                {"true", TokenType::True},
                {"false", TokenType::False},
                {"null", TokenType::Null},
                {"and", TokenType::LogicalAnd},
                {"or", TokenType::LogicalOr},
                {"not", TokenType::LogicalNot}};
}

std::vector<Token> Tokenizer::tokenize_line(const lib::String &line) {
  return tokenize(line);
}

std::vector<Token>
Tokenizer::tokenize_lines(const std::vector<lib::String> &lines) {
  std::vector<Token> all_tokens;

  for (size_t line_num = 0; line_num < lines.size(); ++line_num) {
    m_current_line = line_num;
    auto line_tokens = tokenize_line(lines[line_num]);
    all_tokens.insert(all_tokens.end(), line_tokens.begin(), line_tokens.end());
  }

  return all_tokens;
}

std::vector<Token> Tokenizer::tokenize(const lib::String &input) {
  std::vector<Token> tokens;
  m_current_input = input;
  m_cached_input = input.to_string(); // 캐시 설정
  m_current_pos = 0;
  m_current_column = 0;

  while (!is_at_end()) {
    skip_whitespace();

    if (is_at_end())
      break;

    char c = current_char();
    Token token = Token(TokenType::Illegal, lib::String(""));

    // 숫자 리터럴 (일반 숫자 또는 .123 형태)
    if (lib::Char(c).isDigit() ||
        (c == '.' && lib::Char(peek_char()).isDigit())) {
      token = parse_number();
    }
    // 문자열 리터럴
    else if (c == '"' || c == '\'' || c == '`') {
      token = parse_string();
    }
    // 식별자 또는 키워드
    else if (lib::Char(c).isAlpha() || c == '_') {
      token = parse_identifier_or_keyword();
    }
    // 주석 (// 또는 #)
    else if ((c == '/' && peek_char() == '/') || c == '#') {
      skip_to_line_end();
      continue; // 주석은 토큰으로 만들지 않고 다음으로
    }
    // 연산자 및 구분자
    else {
      token = parse_operator_or_punctuation(c);
    }

    if (token.get_type() != TokenType::Illegal ||
        !token.get_literal().to_string().empty()) {
      tokens.push_back(token);
    }
  }

  return tokens;
}

// 파싱 헬퍼 메서드들
bool Tokenizer::is_at_end() const {
  return m_current_pos >= m_cached_input.length();
}

char Tokenizer::current_char() const {
  if (is_at_end())
    return '\0';
  return m_cached_input[m_current_pos];
}

char Tokenizer::peek_char(size_t offset) const {
  size_t pos = m_current_pos + offset;
  if (pos >= m_cached_input.length())
    return '\0';
  return m_cached_input[pos];
}

char Tokenizer::advance() {
  if (is_at_end())
    return '\0';
  char c = m_cached_input[m_current_pos];
  m_current_pos++;
  m_current_column++;
  return c;
}

void Tokenizer::skip_whitespace() {
  while (!is_at_end() && lib::Char(current_char()).isSpace()) {
    advance();
  }
}

void Tokenizer::skip_to_line_end() {
  while (!is_at_end() && current_char() != '\n') {
    advance();
  }
}

Token Tokenizer::make_token(TokenType type, const lib::String &literal) {
  return Token(type, literal);
}

Token Tokenizer::make_error_token(const lib::String &message) {
  return Token(TokenType::Illegal, message);
}

Token Tokenizer::parse_number() {
  size_t start = m_current_pos;
  std::string number_str;

  // .123 형태 처리 (앞에 0 추가)
  if (current_char() == '.') {
    number_str = "0";
    advance(); // '.' 건너뛰기
    if (!is_at_end() && lib::Char(current_char()).isDigit()) {
      number_str += ".";
      while (!is_at_end() && lib::Char(current_char()).isDigit()) {
        number_str += current_char();
        advance();
      }
    }
  }
  // 일반 숫자 처리
  else {
    // 정수 부분
    while (!is_at_end() && lib::Char(current_char()).isDigit()) {
      number_str += current_char();
      advance();
    }

    // 소수점이 있는 경우
    if (!is_at_end() && current_char() == '.') {
      number_str += ".";
      advance(); // '.' 건너뛰기

      // 소수점 뒤에 숫자가 있으면 처리
      if (!is_at_end() && lib::Char(current_char()).isDigit()) {
        while (!is_at_end() && lib::Char(current_char()).isDigit()) {
          number_str += current_char();
          advance();
        }
      }
      // 123. 형태면 123.0으로 정규화
      else {
        number_str += "0";
      }
    }
  }

  return make_token(TokenType::Number, lib::String(number_str));
}

Token Tokenizer::parse_string() {
  char quote_char = advance(); // 시작 따옴표 저장하고 넘어감
  size_t start = m_current_pos;

  while (!is_at_end() && current_char() != quote_char) {
    if (current_char() == '\\' && !is_at_end()) {
      advance(); // 이스케이프 문자 건너뛰기
    }
    advance();
  }

  if (is_at_end()) {
    return make_error_token(lib::String("Unterminated string"));
  }

  lib::String string_content = m_current_input.slice(start, m_current_pos);
  advance(); // 끝 따옴표 건너뛰기

  return make_token(TokenType::String, string_content);
}

Token Tokenizer::parse_identifier_or_keyword() {
  size_t start = m_current_pos;

  while (!is_at_end() &&
         (lib::Char(current_char()).isAlnum() || current_char() == '_')) {
    advance();
  }

  lib::String identifier_text = m_current_input.slice(start, m_current_pos);
  std::string text = identifier_text.to_string();

  // 키워드 체크
  TokenType type = lookup_keyword(text);
  if (type != TokenType::Illegal) {
    return make_token(type, identifier_text);
  }

  return make_token(TokenType::Identifier, identifier_text);
}

Token Tokenizer::parse_operator_or_punctuation(char first_char) {
  advance(); // 첫 번째 문자 소비

  switch (first_char) {
  // 단순 연산자들
  case '+':
    if (!is_at_end()) {
      if (current_char() == '+') {
        advance();
        return make_token(TokenType::Increment, lib::String("++"));
      }
      if (current_char() == '=') {
        advance();
        return make_token(TokenType::PlusAssign, lib::String("+="));
      }
    }
    return make_token(TokenType::Plus, lib::String("+"));

  case '-':
    if (!is_at_end()) {
      if (current_char() == '-') {
        advance();
        return make_token(TokenType::Decrement, lib::String("--"));
      }
      if (current_char() == '=') {
        advance();
        return make_token(TokenType::MinusAssign, lib::String("-="));
      }
    }
    return make_token(TokenType::Minus, lib::String("-"));

  case '*':
    if (!is_at_end() && current_char() == '=') {
      advance();
      return make_token(TokenType::AsteriskAssign, lib::String("*="));
    }
    return make_token(TokenType::Asterisk, lib::String("*"));

  case '/':
    if (!is_at_end() && current_char() == '=') {
      advance();
      return make_token(TokenType::SlashAssign, lib::String("/="));
    }
    return make_token(TokenType::Slash, lib::String("/"));

  case '%':
    if (!is_at_end() && current_char() == '=') {
      advance();
      return make_token(TokenType::PercentAssign, lib::String("%="));
    }
    return make_token(TokenType::Percent, lib::String("%"));

  case '=':
    if (!is_at_end() && current_char() == '=') {
      advance();
      return make_token(TokenType::Equal, lib::String("=="));
    }
    return make_token(TokenType::Assign, lib::String("="));

  case '!':
    if (!is_at_end()) {
      if (current_char() == '=') {
        advance();
        return make_token(TokenType::NotEqual, lib::String("!="));
      }
      if (current_char() == '!') {
        advance();
        return make_token(TokenType::NullAssertion, lib::String("!!"));
      }
    }
    return make_token(TokenType::Exclamation, lib::String("!"));

  case '<':
    if (!is_at_end()) {
      if (current_char() == '=') {
        advance();
        return make_token(TokenType::LessThanEqual, lib::String("<="));
      }
      if (current_char() == '<') {
        advance();
        return make_token(TokenType::BitwiseShiftLeft, lib::String("<<"));
      }
    }
    return make_token(TokenType::LessThan, lib::String("<"));

  case '>':
    if (!is_at_end()) {
      if (current_char() == '=') {
        advance();
        return make_token(TokenType::GreaterThanEqual, lib::String(">="));
      }
      if (current_char() == '>') {
        advance();
        return make_token(TokenType::BitwiseShiftRight, lib::String(">>"));
      }
    }
    return make_token(TokenType::GreaterThan, lib::String(">"));

  case '&':
    if (!is_at_end() && current_char() == '=') {
      advance();
      return make_token(TokenType::AmpersandAssign, lib::String("&="));
    }
    return make_token(TokenType::Ampersand, lib::String("&"));

  case '|':
    if (!is_at_end() && current_char() == '=') {
      advance();
      return make_token(TokenType::PipeAssign, lib::String("|="));
    }
    return make_token(TokenType::Pipe, lib::String("|"));

  case '^':
    if (!is_at_end() && current_char() == '=') {
      advance();
      return make_token(TokenType::CaretAssign, lib::String("^="));
    }
    return make_token(TokenType::Caret, lib::String("^"));

  case '~':
    if (!is_at_end() && current_char() == '=') {
      advance();
      return make_token(TokenType::TildeAssign, lib::String("~="));
    }
    return make_token(TokenType::Tilde, lib::String("~"));

  case '?':
    if (!is_at_end()) {
      if (current_char() == ':') {
        advance();
        return make_token(TokenType::NullElvis, lib::String("?:"));
      }
      if (current_char() == '.') {
        advance();
        return make_token(TokenType::NullSafeAccess, lib::String("?."));
      }
    }
    return make_token(TokenType::Question, lib::String("?"));

  // 구분자들
  case '(':
    return make_token(TokenType::LeftParen, lib::String("("));
  case ')':
    return make_token(TokenType::RightParen, lib::String(")"));
  case '{':
    return make_token(TokenType::LeftBrace, lib::String("{"));
  case '}':
    return make_token(TokenType::RightBrace, lib::String("}"));
  case '[':
    return make_token(TokenType::LeftBracket, lib::String("["));
  case ']':
    return make_token(TokenType::RightBracket, lib::String("]"));
  case ',':
    return make_token(TokenType::Comma, lib::String(","));
  case ';':
    return make_token(TokenType::Semicolon, lib::String(";"));
  case ':':
    return make_token(TokenType::Colon, lib::String(":"));
  case '.':
    return make_token(TokenType::Dot, lib::String("."));
  case '\\':
    return make_token(TokenType::Backslash, lib::String("\\"));
  case '@':
    return make_token(TokenType::At, lib::String("@"));
  case '$':
    return make_token(TokenType::Dollar, lib::String("$"));

  default:
    return make_error_token(lib::String("Unexpected character: ") +
                            lib::String(std::string(1, first_char)));
  }
}

TokenType Tokenizer::lookup_keyword(const std::string &text) const {
  auto it = m_keywords.find(text);
  if (it != m_keywords.end()) {
    return it->second;
  }
  return TokenType::Illegal;
}

} // namespace nugdev::compiler::tokenize