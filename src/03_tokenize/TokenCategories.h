#pragma once

#include "TokenType.h"
#include <unordered_set>

namespace nugdev::compiler::tokenize {

// 토큰 카테고리별 집합 정의 (inline으로 단일 인스턴스 보장)
namespace TokenCategories {

// 리터럴 타입들
inline const std::unordered_set<TokenType> LITERALS = {
    TokenType::Number,  TokenType::String, TokenType::Character,
    TokenType::Boolean, TokenType::True,   TokenType::False,
    TokenType::Null,    TokenType::None,   TokenType::Range};

// 키워드 타입들
inline const std::unordered_set<TokenType> KEYWORDS = {
    TokenType::Let, TokenType::Mut, TokenType::If, TokenType::Elif,
    TokenType::Else, TokenType::For, TokenType::Break, TokenType::Continue,
    TokenType::Function, TokenType::Return, TokenType::When, TokenType::True,
    TokenType::False, TokenType::Null, TokenType::None, TokenType::LogicalAnd,
    TokenType::LogicalOr, TokenType::LogicalNot,
    // Added: New keywords from EBNF
    TokenType::In, TokenType::Import, TokenType::Export, TokenType::As,
    TokenType::Is, TokenType::Struct, TokenType::Interface};

// 산술 연산자들
inline const std::unordered_set<TokenType> ARITHMETIC_OPERATORS = {
    TokenType::Plus, TokenType::Minus, TokenType::Asterisk, TokenType::Slash,
    TokenType::Percent};

// 할당 연산자들
inline const std::unordered_set<TokenType> ASSIGNMENT_OPERATORS = {
    TokenType::Assign,          TokenType::PlusAssign,
    TokenType::MinusAssign,     TokenType::AsteriskAssign,
    TokenType::SlashAssign,     TokenType::PercentAssign,
    TokenType::AmpersandAssign, TokenType::PipeAssign,
    TokenType::CaretAssign,     TokenType::TildeAssign};

// 비교 연산자들
inline const std::unordered_set<TokenType> COMPARISON_OPERATORS = {
    TokenType::Equal,         TokenType::NotEqual,
    TokenType::LessThan,      TokenType::GreaterThan,
    TokenType::LessThanEqual, TokenType::GreaterThanEqual};

// 논리 연산자들
inline const std::unordered_set<TokenType> LOGICAL_OPERATORS = {
    TokenType::LogicalAnd, TokenType::LogicalOr, TokenType::LogicalNot};

// 단항 연산자들
inline const std::unordered_set<TokenType> UNARY_OPERATORS = {
    TokenType::Increment,   TokenType::Decrement,
    TokenType::Exclamation, TokenType::LogicalNot,
    TokenType::Tilde, // Added: bitwise NOT
    TokenType::Minus, // 단항 마이너스
    TokenType::Plus   // 단항 플러스
};

// 비트 연산자들
inline const std::unordered_set<TokenType> BITWISE_OPERATORS = {
    TokenType::Ampersand,
    TokenType::Pipe,
    TokenType::Caret,
    TokenType::Tilde,
    TokenType::BitwiseShiftLeft,
    TokenType::BitwiseShiftRight};

// Null 관련 연산자들
inline const std::unordered_set<TokenType> NULL_OPERATORS = {
    TokenType::NullCoalescing, // ??
    TokenType::NullElvis,      // ?:
    TokenType::NullAssertion,  // !!
    TokenType::NullSafeAccess  // ?.
};

// 타입 관련 연산자들
inline const std::unordered_set<TokenType> TYPE_OPERATORS = {
    TokenType::As, // as (unsafe cast)
    TokenType::Is  // is (type check)
};

// 특수 연산자들
inline const std::unordered_set<TokenType> SPECIAL_OPERATORS = {
    TokenType::Arrow,    // ->
    TokenType::FatArrow, // =>
    TokenType::Spread,   // ...
    TokenType::Question, // ? (ternary)
    TokenType::Colon     // : (ternary)
};

// 구분자들 (delimiters)
inline const std::unordered_set<TokenType> DELIMITERS = {
    TokenType::LeftParen,  TokenType::RightParen,  TokenType::LeftBrace,
    TokenType::RightBrace, TokenType::LeftBracket, TokenType::RightBracket,
    TokenType::Comma,      TokenType::Semicolon,   TokenType::Colon,
    TokenType::Dot,        TokenType::At};

// 문자열 관련 토큰들
inline const std::unordered_set<TokenType> STRING_TOKENS = {
    TokenType::String,
    TokenType::TemplateStringStart,
    TokenType::TemplateStringEnd,
    TokenType::TemplateExprStart,
    TokenType::TemplateExprEnd,
    TokenType::RawStringStart,
    TokenType::RawStringEnd,
    TokenType::SingleQuote,
    TokenType::DoubleQuote,
    TokenType::Backtick};

// 모든 연산자들 (통합) - 함수로 변경하여 초기화 순서 문제 해결
inline const std::unordered_set<TokenType> &get_all_operators() {
  static const std::unordered_set<TokenType> ALL_OPERATORS = []() {
    std::unordered_set<TokenType> result;

    // 각 연산자 카테고리를 통합
    result.insert(ARITHMETIC_OPERATORS.begin(), ARITHMETIC_OPERATORS.end());
    result.insert(ASSIGNMENT_OPERATORS.begin(), ASSIGNMENT_OPERATORS.end());
    result.insert(COMPARISON_OPERATORS.begin(), COMPARISON_OPERATORS.end());
    result.insert(LOGICAL_OPERATORS.begin(), LOGICAL_OPERATORS.end());
    result.insert(BITWISE_OPERATORS.begin(), BITWISE_OPERATORS.end());
    result.insert(NULL_OPERATORS.begin(), NULL_OPERATORS.end());
    result.insert(TYPE_OPERATORS.begin(), TYPE_OPERATORS.end());
    result.insert(SPECIAL_OPERATORS.begin(), SPECIAL_OPERATORS.end());

    return result;
  }();

  return ALL_OPERATORS;
}

} // namespace TokenCategories

// 편의 함수들 (전역 함수로 제공)
inline bool is_literal(TokenType type) {
  return TokenCategories::LITERALS.count(type) > 0;
}

inline bool is_keyword(TokenType type) {
  return TokenCategories::KEYWORDS.count(type) > 0;
}

inline bool is_operator(TokenType type) {
  return TokenCategories::get_all_operators().count(type) > 0;
}

inline bool is_arithmetic_operator(TokenType type) {
  return TokenCategories::ARITHMETIC_OPERATORS.count(type) > 0;
}

inline bool is_assignment_operator(TokenType type) {
  return TokenCategories::ASSIGNMENT_OPERATORS.count(type) > 0;
}

inline bool is_comparison_operator(TokenType type) {
  return TokenCategories::COMPARISON_OPERATORS.count(type) > 0;
}

inline bool is_logical_operator(TokenType type) {
  return TokenCategories::LOGICAL_OPERATORS.count(type) > 0;
}

inline bool is_unary_operator(TokenType type) {
  return TokenCategories::UNARY_OPERATORS.count(type) > 0;
}

inline bool is_binary_operator(TokenType type) {
  return is_arithmetic_operator(type) || is_comparison_operator(type) ||
         is_logical_operator(type) || is_assignment_operator(type);
}

inline bool is_bitwise_operator(TokenType type) {
  return TokenCategories::BITWISE_OPERATORS.count(type) > 0;
}

inline bool is_null_operator(TokenType type) {
  return TokenCategories::NULL_OPERATORS.count(type) > 0;
}

inline bool is_type_operator(TokenType type) {
  return TokenCategories::TYPE_OPERATORS.count(type) > 0;
}

inline bool is_special_operator(TokenType type) {
  return TokenCategories::SPECIAL_OPERATORS.count(type) > 0;
}

inline bool is_delimiter(TokenType type) {
  return TokenCategories::DELIMITERS.count(type) > 0;
}

inline bool is_string_token(TokenType type) {
  return TokenCategories::STRING_TOKENS.count(type) > 0;
}

inline bool is_identifier(TokenType type) {
  return type == TokenType::Identifier;
}

inline bool is_legal(TokenType type) { return type != TokenType::Illegal; }

inline bool is_postfix_operator(TokenType type) {
  return type == TokenType::Increment || type == TokenType::Decrement ||
         type == TokenType::Dot || type == TokenType::NullSafeAccess ||
         type == TokenType::LeftBracket || type == TokenType::LeftParen ||
         type == TokenType::As;
}

} // namespace nugdev::compiler::tokenize