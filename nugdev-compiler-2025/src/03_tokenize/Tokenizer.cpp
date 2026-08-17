#include "Tokenizer.h"
#include <cctype>
#include <unordered_map>

namespace nugdev::compiler::tokenize {

Tokenizer::Tokenizer()
    : m_current_pos(0), m_current_line(0), m_current_column(0) {
  initialize_keywords();
}

void Tokenizer::initialize_keywords() {
  m_keywords = {
      // Reserved keywords from grammar:
      // let, mut, fun, if, elif, else, when, for, in,
      // break, continue, return, true, false, null, None, and, or, not,
      // import, export, as, is, struct, interface
      //
      // NOTE: Type names like 'number', 'string', 'boolean', 'object', 'void',
      // 'any'
      // are NOT reserved keywords - they are regular identifiers.

      // Statement keywords
      {"let", TokenType::Let},
      {"mut", TokenType::Mut},
      {"if", TokenType::If},
      {"elif", TokenType::Elif},
      {"else", TokenType::Else},
      {"when", TokenType::When},
      {"for", TokenType::For},
      {"in", TokenType::In},
      {"break", TokenType::Break},
      {"continue", TokenType::Continue},
      {"fun", TokenType::Function}, // 'fun' not 'function'
      {"return", TokenType::Return},

      // Boolean literals
      {"true", TokenType::True},
      {"false", TokenType::False},

      // Null/None literals
      {"null", TokenType::Null},
      {"None", TokenType::None},

      // Logical operators
      {"and", TokenType::LogicalAnd},
      {"or", TokenType::LogicalOr},
      {"not", TokenType::LogicalNot},

      // Module keywords
      {"import", TokenType::Import},
      {"export", TokenType::Export},
      {"as", TokenType::As},
      {"is", TokenType::Is},

      // Type declaration keywords
      {"struct", TokenType::Struct},
      {"interface", TokenType::Interface}};
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
    // 원시 문자열 (r"...")
    else if (c == 'r' && peek_char() == '"') {
      token = parse_string();
    }
    // 문자열 및 문자 리터럴
    else if (c == '"' || c == '\'' || c == '`') {
      token = parse_string();
    }
    // 식별자 또는 키워드 (r 문자로 시작하지만 r"가 아닌 경우도 포함)
    else if (lib::Char(c).isAlpha() || c == '_') {
      token = parse_identifier_or_keyword();
    }
    // 주석 처리 (/* ... */ 또는 #)
    else if ((c == '/' && peek_char() == '*') || c == '#') {

      if (c == '/' && peek_char() == '*') {
        // 블록 주석 (/* ... */)
        advance(); // '/'
        advance(); // '*'
        while (!is_at_end()) {
          if (current_char() == '*' && peek_char() == '/') {
            advance(); // '*'
            advance(); // '/'
            break;
          }
          advance();
        }
      } else if (c == '#') {
        // 라인 주석 (#)
        skip_to_line_end();
      }
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
  std::string number_str;

  // .123 형태 처리 (앞에 0 추가)
  if (current_char() == '.') {
    number_str = "0";
    advance(); // '.' 건너뛰기
    if (!is_at_end() && lib::Char(current_char()).isDigit()) {
      number_str += ".";
      while (!is_at_end() &&
             (lib::Char(current_char()).isDigit() || current_char() == '_')) {
        if (current_char() != '_') {
          number_str += current_char();
        }
        advance();
      }
    }
  }
  // 0으로 시작하는 특수 진법 리터럴 처리
  else if (current_char() == '0' && !is_at_end()) {
    char next = peek_char();

    // 16진수: 0x, 0X
    if (next == 'x' || next == 'X') {
      number_str += advance(); // '0'
      number_str += advance(); // 'x' or 'X'

      while (!is_at_end() &&
             (lib::Char(current_char()).isDigit() ||
              (current_char() >= 'a' && current_char() <= 'f') ||
              (current_char() >= 'A' && current_char() <= 'F') ||
              current_char() == '_')) {
        if (current_char() != '_') {
          number_str += current_char();
        }
        advance();
      }
    }
    // 2진수: 0b, 0B
    else if (next == 'b' || next == 'B') {
      number_str += advance(); // '0'
      number_str += advance(); // 'b' or 'B'

      while (!is_at_end() && (current_char() == '0' || current_char() == '1' ||
                              current_char() == '_')) {
        if (current_char() != '_') {
          number_str += current_char();
        }
        advance();
      }
    }
    // 8진수: 0o, 0O
    else if (next == 'o' || next == 'O') {
      number_str += advance(); // '0'
      number_str += advance(); // 'o' or 'O'

      while (!is_at_end() &&
             ((current_char() >= '0' && current_char() <= '7') ||
              current_char() == '_')) {
        if (current_char() != '_') {
          number_str += current_char();
        }
        advance();
      }
    }
    // 일반 십진수 (0.123 등)
    else {
      number_str += advance(); // '0'

      // 소수점이 있는 경우 처리
      if (!is_at_end() && current_char() == '.') {
        // 다음 문자 확인: .. (range)인지 아닌지
        if (peek_char() == '.') {
          // Range operator이므로 소수점 처리하지 않음
        } else {
          // 소수점 추가
          number_str += advance(); // '.'

          // 소수점 뒤에 숫자가 있으면 처리 (없어도 유효한 float)
          while (!is_at_end() && (lib::Char(current_char()).isDigit() ||
                                  current_char() == '_')) {
            if (current_char() != '_') {
              number_str += current_char();
            }
            advance();
          }
        }
      }

      // 과학적 표기법 처리 (e, E)
      if (!is_at_end() && (current_char() == 'e' || current_char() == 'E')) {
        number_str += advance(); // 'e' or 'E'

        // 부호 처리 (+, -)
        if (!is_at_end() && (current_char() == '+' || current_char() == '-')) {
          number_str += advance();
        }

        // 지수 숫자 처리
        while (!is_at_end() &&
               (lib::Char(current_char()).isDigit() || current_char() == '_')) {
          if (current_char() != '_') {
            number_str += current_char();
          }
          advance();
        }
      }
    }
  }
  // 일반 숫자 처리 (1부터 9로 시작)
  else {
    // 정수 부분
    while (!is_at_end() &&
           (lib::Char(current_char()).isDigit() || current_char() == '_')) {
      if (current_char() != '_') {
        number_str += current_char();
      }
      advance();
    }

    // 소수점이 있는 경우 처리
    if (!is_at_end() && current_char() == '.') {
      // 다음 문자들 확인:
      // - .. (range operator) - 소수점으로 처리하지 않음
      // - ... (3개 점 있으면 앞의 정수에 . 붙이고 range operator로 처리)
      if (peek_char() == '.') {
        // 다음에 점이 하나 더 있는지 확인
        if (m_current_pos + 2 < m_cached_input.length() &&
            m_cached_input[m_current_pos + 2] == '.') {
          // 1...5 케이스: 이것을 1. + .. + 5로 파싱하려면
          // 현재 숫자에 .을 추가하고 끝냄
          number_str += advance(); // '.'
          // .. 부분은 다음 토큰으로 처리됨
        } else {
          // 1..10 케이스: 정수로 끝내고 .. 는 별도 토큰으로 처리
        }
      } else {
        // 일반적인 소수점 (1.23 등)
        number_str += advance(); // '.'

        // 소수점 뒤에 숫자가 있으면 처리 (없어도 유효한 float)
        while (!is_at_end() &&
               (lib::Char(current_char()).isDigit() || current_char() == '_')) {
          if (current_char() != '_') {
            number_str += current_char();
          }
          advance();
        }
      }
    }

    // 과학적 표기법 처리 (e, E)
    if (!is_at_end() && (current_char() == 'e' || current_char() == 'E')) {
      number_str += advance(); // 'e' or 'E'

      // 부호 처리 (+, -)
      if (!is_at_end() && (current_char() == '+' || current_char() == '-')) {
        number_str += advance();
      }

      // 지수 숫자 처리
      while (!is_at_end() &&
             (lib::Char(current_char()).isDigit() || current_char() == '_')) {
        if (current_char() != '_') {
          number_str += current_char();
        }
        advance();
      }
    }
  }

  return make_token(TokenType::Number, lib::String(number_str));
}

Token Tokenizer::parse_string() {
  char quote_char = current_char();

  // 원시 문자열 처리 (r"...")
  if (quote_char == 'r' && peek_char() == '"') {
    advance(); // 'r' 건너뛰기
    advance(); // '"' 건너뛰기
    size_t start = m_current_pos;

    // 원시 문자열에서는 이스케이프 처리 안함
    while (!is_at_end() && current_char() != '"') {
      advance();
    }

    if (is_at_end()) {
      return make_error_token(lib::String("Unterminated raw string"));
    }

    lib::String string_content = m_current_input.slice(start, m_current_pos);
    advance(); // 끝 '"' 건너뛰기

    return make_token(TokenType::String, string_content);
  }

  // 템플릿 문자열 처리 (`...`)
  else if (quote_char == '`') {
    advance(); // '`' 건너뛰기
    size_t start = m_current_pos;

    while (!is_at_end() && current_char() != '`') {
      // 템플릿 표현식 ${...} 처리 (여기서는 단순히 건너뛰기)
      if (current_char() == '$' && peek_char() == '{') {
        // 나중에 더 정교한 파싱이 필요할 수 있음
        advance(); // '$'
        advance(); // '{'

        int brace_count = 1;
        while (!is_at_end() && brace_count > 0) {
          if (current_char() == '{')
            brace_count++;
          else if (current_char() == '}')
            brace_count--;
          advance();
        }
      } else {
        if (current_char() == '\\' && !is_at_end()) {
          advance(); // 이스케이프 문자 건너뛰기
        }
        advance();
      }
    }

    if (is_at_end()) {
      return make_error_token(lib::String("Unterminated template string"));
    }

    lib::String string_content = m_current_input.slice(start, m_current_pos);
    advance(); // 끝 '`' 건너뛰기

    return make_token(TokenType::String, string_content);
  }

  // 문자 리터럴 처리 ('a', '\n' 등)
  else if (quote_char == '\'') {
    advance(); // '\'' 건너뛰기
    size_t start = m_current_pos;

    // 이스케이프 문자 처리
    if (current_char() == '\\' && !is_at_end()) {
      advance(); // '\' 건너뛰기
      if (!is_at_end()) {
        char escape_char = current_char();
        advance(); // 이스케이프 타입 문자 건너뛰기

        // 유니코드 이스케이프 시퀀스 처리 (\u 또는 \U)
        if (escape_char == 'u') {
          // \uXXXX 형태 - 4개의 16진수
          for (int i = 0; i < 4 && !is_at_end(); ++i) {
            if (!((current_char() >= '0' && current_char() <= '9') ||
                  (current_char() >= 'a' && current_char() <= 'f') ||
                  (current_char() >= 'A' && current_char() <= 'F'))) {
              return make_error_token(
                  lib::String("Invalid unicode escape sequence"));
            }
            advance();
          }
        } else if (escape_char == 'U') {
          // \UXXXXXXXX 형태 - 8개의 16진수
          for (int i = 0; i < 8 && !is_at_end(); ++i) {
            if (!((current_char() >= '0' && current_char() <= '9') ||
                  (current_char() >= 'a' && current_char() <= 'f') ||
                  (current_char() >= 'A' && current_char() <= 'F'))) {
              return make_error_token(
                  lib::String("Invalid unicode escape sequence"));
            }
            advance();
          }
        } else if (escape_char == 'x') {
          // \xXX 형태 - 2개의 16진수
          for (int i = 0; i < 2 && !is_at_end(); ++i) {
            if (!((current_char() >= '0' && current_char() <= '9') ||
                  (current_char() >= 'a' && current_char() <= 'f') ||
                  (current_char() >= 'A' && current_char() <= 'F'))) {
              return make_error_token(
                  lib::String("Invalid hex escape sequence"));
            }
            advance();
          }
        }
        // 다른 이스케이프 문자들은 이미 처리됨
      }
    }
    // 일반 문자
    else if (!is_at_end() && current_char() != '\'') {
      advance();
    }

    if (is_at_end() || current_char() != '\'') {
      return make_error_token(lib::String("Unterminated character literal"));
    }

    lib::String char_content = m_current_input.slice(start, m_current_pos);
    advance(); // 끝 '\'' 건너뛰기

    return make_token(TokenType::Character, char_content);
  }

  // 일반 문자열 처리 ("...")
  else {
    advance(); // 시작 따옴표 건너뛰기
    size_t start = m_current_pos;

    while (!is_at_end() && current_char() != quote_char) {
      if (current_char() == '\\' && !is_at_end()) {
        advance(); // 이스케이프 문자 건너뛰기
        if (!is_at_end()) {
          advance(); // 이스케이프된 문자 건너뛰기
        }
      } else {
        advance();
      }
    }

    if (is_at_end()) {
      return make_error_token(lib::String("Unterminated string"));
    }

    lib::String string_content = m_current_input.slice(start, m_current_pos);
    advance(); // 끝 따옴표 건너뛰기

    return make_token(TokenType::String, string_content);
  }
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

  // 세 문자 연산자 확인 (... 등)
  if (!is_at_end()) {
    std::string three_char =
        std::string(1, first_char) + current_char() + peek_char();

    if (three_char == "...") {
      advance(); // '.'
      advance(); // '.'
      return make_token(TokenType::Spread, lib::String("..."));
    }
  }

  // 두 문자 연산자 확인
  if (!is_at_end()) {
    std::string two_char = std::string(1, first_char) + current_char();
    const auto &two_char_ops = get_two_char_operators();
    auto it = two_char_ops.find(two_char);
    if (it != two_char_ops.end()) {
      advance(); // 두 번째 문자 소비
      return make_token(it->second, lib::String(two_char));
    }
  }

  // 단일 문자 토큰 확인
  const auto &single_char_ops = get_single_char_tokens();
  auto single_it = single_char_ops.find(first_char);
  if (single_it != single_char_ops.end()) {
    return make_token(single_it->second,
                      lib::String(std::string(1, first_char)));
  }

  // 알 수 없는 문자
  return make_error_token(lib::String("Unexpected character: ") +
                          lib::String(std::string(1, first_char)));
}

TokenType Tokenizer::lookup_keyword(const std::string &text) const {
  auto it = m_keywords.find(text);
  if (it != m_keywords.end()) {
    return it->second;
  }
  return TokenType::Illegal;
}

// Static lookup table getters for better memory management
const std::unordered_map<std::string, TokenType> &
Tokenizer::get_two_char_operators() {
  static const std::unordered_map<std::string, TokenType> two_char_operators = {
      // 증감 연산자
      {"++", TokenType::Increment},
      {"--", TokenType::Decrement},

      // 할당 연산자
      {"+=", TokenType::PlusAssign},
      {"-=", TokenType::MinusAssign},
      {"*=", TokenType::AsteriskAssign},
      {"/=", TokenType::SlashAssign},
      {"%=", TokenType::PercentAssign},
      {"&=", TokenType::AmpersandAssign},
      {"|=", TokenType::PipeAssign},
      {"^=", TokenType::CaretAssign},
      {"~=", TokenType::TildeAssign},

      // 비교 연산자
      {"==", TokenType::Equal},
      {"!=", TokenType::NotEqual},
      {"<=", TokenType::LessThanEqual},
      {">=", TokenType::GreaterThanEqual},

      // 비트 연산자
      {"<<", TokenType::BitwiseShiftLeft},
      {">>", TokenType::BitwiseShiftRight},

      // Null 관련 연산자
      {"??", TokenType::NullCoalescing}, // Added: null coalescing
      {"?:", TokenType::NullElvis},
      {"?.", TokenType::NullSafeAccess},
      {"!!", TokenType::NullAssertion},

      // 특수 연산자
      {"->", TokenType::Arrow},    // Added: arrow for function types
      {"=>", TokenType::FatArrow}, // Added: fat arrow for lambdas
      {"..", TokenType::Range},    // Added: range operator

      // 주석 (처리용)
      {"/*", TokenType::COMMENT}};
  return two_char_operators;
}

const std::unordered_map<char, TokenType> &Tokenizer::get_single_char_tokens() {
  static const std::unordered_map<char, TokenType> single_char_tokens = {
      {'+', TokenType::Plus},        {'-', TokenType::Minus},
      {'*', TokenType::Asterisk},    {'/', TokenType::Slash},
      {'%', TokenType::Percent},     {'=', TokenType::Assign},
      {'!', TokenType::Exclamation}, {'<', TokenType::LessThan},
      {'>', TokenType::GreaterThan}, {'&', TokenType::Ampersand},
      {'|', TokenType::Pipe},        {'^', TokenType::Caret},
      {'~', TokenType::Tilde},       {'?', TokenType::Question},
      {'(', TokenType::LeftParen},   {')', TokenType::RightParen},
      {'{', TokenType::LeftBrace},   {'}', TokenType::RightBrace},
      {'[', TokenType::LeftBracket}, {']', TokenType::RightBracket},
      {',', TokenType::Comma},       {';', TokenType::Semicolon},
      {':', TokenType::Colon},       {'.', TokenType::Dot},
      {'\\', TokenType::Backslash},  {'@', TokenType::At},
      {'$', TokenType::Dollar}};
  return single_char_tokens;
}

} // namespace nugdev::compiler::tokenize