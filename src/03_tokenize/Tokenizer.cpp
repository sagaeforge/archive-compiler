#include "Tokenizer.h"
#include "TokenCategories.h"
#include <cctype>
#include <sstream>
#include <unordered_map>

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
      {"++", TokenType::Increment},         {"--", TokenType::Decrement},
      {"+=", TokenType::PlusAssign},        {"-=", TokenType::MinusAssign},
      {"*=", TokenType::AsteriskAssign},    {"/=", TokenType::SlashAssign},
      {"%=", TokenType::PercentAssign},     {"==", TokenType::Equal},
      {"!=", TokenType::NotEqual},          {"<=", TokenType::LessThanEqual},
      {">=", TokenType::GreaterThanEqual},  {"<<", TokenType::BitwiseShiftLeft},
      {">>", TokenType::BitwiseShiftRight}, {"&=", TokenType::AmpersandAssign},
      {"|=", TokenType::PipeAssign},        {"^=", TokenType::CaretAssign},
      {"~=", TokenType::TildeAssign},       {"?:", TokenType::NullElvis},
      {"?.", TokenType::NullSafeAccess},    {"!!", TokenType::NullAssertion}};
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