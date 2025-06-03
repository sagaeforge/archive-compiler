#include "03_tokenize/Tokenizer.h"
#include "03_tokenize/TokenCategories.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace nugdev::compiler::tokenize;
using namespace nugdev::lib;

namespace nugdev::test {

class TokenizerTest : public ::testing::Test {
protected:
  void SetUp() override { tokenizer = std::make_unique<Tokenizer>(); }

  void TearDown() override { tokenizer.reset(); }

  std::unique_ptr<Tokenizer> tokenizer;
};

// 기본 토크나이징 테스트
TEST_F(TokenizerTest, BasicTokenization) {
  auto tokens = tokenizer->tokenize(String("let x = 42;"));

  ASSERT_EQ(tokens.size(), 5);
  EXPECT_EQ(tokens[0].get_type(), TokenType::Let);
  EXPECT_EQ(tokens[1].get_type(), TokenType::Identifier);
  EXPECT_EQ(tokens[2].get_type(), TokenType::Assign);
  EXPECT_EQ(tokens[3].get_type(), TokenType::Number);
  EXPECT_EQ(tokens[4].get_type(), TokenType::Semicolon);
}

// 숫자 리터럴 테스트
TEST_F(TokenizerTest, NumberLiterals) {
  std::vector<std::pair<std::string, std::string>> testCases = {
      {"42", "42"},     {"0", "0"},     {"123", "123"},
      {"3.14", "3.14"}, {"0.5", "0.5"}, {"999.999", "999.999"}};

  for (const auto &[input, expected] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), TokenType::Number) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), expected)
        << "Input: " << input;
  }
}

// 문자열 리터럴 테스트
TEST_F(TokenizerTest, StringLiterals) {
  std::vector<std::pair<std::string, std::string>> testCases = {
      {"\"hello\"", "hello"},
      {"'world'", "world"},
      {"`template`", "template"},
      {"\"\"", ""},
      {"\"hello world\"", "hello world"},
      {"\"string with spaces\"", "string with spaces"}};

  for (const auto &[input, expected] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), TokenType::String) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), expected)
        << "Input: " << input;
  }
}

// 키워드 테스트
TEST_F(TokenizerTest, Keywords) {
  std::vector<std::pair<std::string, TokenType>> testCases = {
      {"let", TokenType::Let},        {"mut", TokenType::Mut},
      {"if", TokenType::If},          {"else", TokenType::Else},
      {"for", TokenType::For},        {"function", TokenType::Function},
      {"return", TokenType::Return},  {"true", TokenType::True},
      {"false", TokenType::False},    {"null", TokenType::Null},
      {"and", TokenType::LogicalAnd}, {"or", TokenType::LogicalOr},
      {"not", TokenType::LogicalNot}};

  for (const auto &[input, expectedType] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), expectedType) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), input) << "Input: " << input;
    EXPECT_TRUE(is_keyword(tokens[0].get_type())) << "Input: " << input;
  }
}

// 식별자 테스트
TEST_F(TokenizerTest, Identifiers) {
  std::vector<std::string> testCases = {
      "variable",   "myVar", "_private", "var123",          "camelCase",
      "snake_case", "_",     "a",        "longVariableName"};

  for (const auto &input : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), TokenType::Identifier)
        << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), input) << "Input: " << input;
    EXPECT_TRUE(is_identifier(tokens[0].get_type())) << "Input: " << input;
  }
}

// 산술 연산자 테스트
TEST_F(TokenizerTest, ArithmeticOperators) {
  std::vector<std::pair<std::string, TokenType>> testCases = {
      {"+", TokenType::Plus},      {"-", TokenType::Minus},
      {"*", TokenType::Asterisk},  {"/", TokenType::Slash},
      {"%", TokenType::Percent},   {"++", TokenType::Increment},
      {"--", TokenType::Decrement}};

  for (const auto &[input, expectedType] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), expectedType) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), input) << "Input: " << input;
  }
}

// 할당 연산자 테스트
TEST_F(TokenizerTest, AssignmentOperators) {
  std::vector<std::pair<std::string, TokenType>> testCases = {
      {"=", TokenType::Assign},           {"+=", TokenType::PlusAssign},
      {"-=", TokenType::MinusAssign},     {"*=", TokenType::AsteriskAssign},
      {"/=", TokenType::SlashAssign},     {"%=", TokenType::PercentAssign},
      {"&=", TokenType::AmpersandAssign}, {"|=", TokenType::PipeAssign},
      {"^=", TokenType::CaretAssign},     {"~=", TokenType::TildeAssign}};

  for (const auto &[input, expectedType] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), expectedType) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), input) << "Input: " << input;
    EXPECT_TRUE(is_assignment_operator(tokens[0].get_type()))
        << "Input: " << input;
  }
}

// 비교 연산자 테스트
TEST_F(TokenizerTest, ComparisonOperators) {
  std::vector<std::pair<std::string, TokenType>> testCases = {
      {"==", TokenType::Equal},         {"!=", TokenType::NotEqual},
      {"<", TokenType::LessThan},       {">", TokenType::GreaterThan},
      {"<=", TokenType::LessThanEqual}, {">=", TokenType::GreaterThanEqual}};

  for (const auto &[input, expectedType] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), expectedType) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), input) << "Input: " << input;
    EXPECT_TRUE(is_comparison_operator(tokens[0].get_type()))
        << "Input: " << input;
  }
}

// 비트 연산자 테스트
TEST_F(TokenizerTest, BitwiseOperators) {
  std::vector<std::pair<std::string, TokenType>> testCases = {
      {"&", TokenType::Ampersand},
      {"|", TokenType::Pipe},
      {"^", TokenType::Caret},
      {"~", TokenType::Tilde},
      {"<<", TokenType::BitwiseShiftLeft},
      {">>", TokenType::BitwiseShiftRight}};

  for (const auto &[input, expectedType] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), expectedType) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), input) << "Input: " << input;
  }
}

// 구분자 테스트
TEST_F(TokenizerTest, Delimiters) {
  std::vector<std::pair<std::string, TokenType>> testCases = {
      {"(", TokenType::LeftParen},   {")", TokenType::RightParen},
      {"{", TokenType::LeftBrace},   {"}", TokenType::RightBrace},
      {"[", TokenType::LeftBracket}, {"]", TokenType::RightBracket},
      {",", TokenType::Comma},       {";", TokenType::Semicolon},
      {":", TokenType::Colon},       {".", TokenType::Dot}};

  for (const auto &[input, expectedType] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), expectedType) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), input) << "Input: " << input;
    EXPECT_TRUE(is_delimiter(tokens[0].get_type())) << "Input: " << input;
  }
}

// 특수 연산자 테스트
TEST_F(TokenizerTest, SpecialOperators) {
  std::vector<std::pair<std::string, TokenType>> testCases = {
      {"!", TokenType::Exclamation},
      {"!!", TokenType::NullAssertion},
      {"?", TokenType::Question},
      {"?:", TokenType::NullElvis}};

  for (const auto &[input, expectedType] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), expectedType) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), input) << "Input: " << input;
  }
}

// 복합 표현식 테스트
TEST_F(TokenizerTest, BasicComplexExpressions) {
  auto tokens = tokenizer->tokenize(String("let result = (a + b) * 2;"));

  std::vector<TokenType> expectedTypes = {
      TokenType::Let,
      TokenType::Identifier, // result
      TokenType::Assign,     TokenType::LeftParen,
      TokenType::Identifier, // a
      TokenType::Plus,
      TokenType::Identifier, // b
      TokenType::RightParen, TokenType::Asterisk,
      TokenType::Number, // 2
      TokenType::Semicolon};

  ASSERT_EQ(tokens.size(), expectedTypes.size());

  for (size_t i = 0; i < tokens.size(); ++i) {
    EXPECT_EQ(tokens[i].get_type(), expectedTypes[i]) << "Token index: " << i;
  }
}

// 주석 처리 테스트
TEST_F(TokenizerTest, Comments) {
  // // 스타일 주석 테스트
  auto tokens1 =
      tokenizer->tokenize(String("let x = 42; // this is a comment"));

  // 주석은 무시되어야 함
  ASSERT_EQ(tokens1.size(), 5);
  EXPECT_EQ(tokens1[0].get_type(), TokenType::Let);
  EXPECT_EQ(tokens1[1].get_type(), TokenType::Identifier);
  EXPECT_EQ(tokens1[2].get_type(), TokenType::Assign);
  EXPECT_EQ(tokens1[3].get_type(), TokenType::Number);
  EXPECT_EQ(tokens1[4].get_type(), TokenType::Semicolon);

  // # 스타일 주석 테스트
  auto tokens2 =
      tokenizer->tokenize(String("let y = 24; # this is also a comment"));

  // 주석은 무시되어야 함
  ASSERT_EQ(tokens2.size(), 5);
  EXPECT_EQ(tokens2[0].get_type(), TokenType::Let);
  EXPECT_EQ(tokens2[1].get_type(), TokenType::Identifier);
  EXPECT_EQ(tokens2[2].get_type(), TokenType::Assign);
  EXPECT_EQ(tokens2[3].get_type(), TokenType::Number);
  EXPECT_EQ(tokens2[4].get_type(), TokenType::Semicolon);
}

// 공백 처리 테스트
TEST_F(TokenizerTest, WhitespaceHandling) {
  auto tokens = tokenizer->tokenize(String("  let   x   =   42  ;  "));

  ASSERT_EQ(tokens.size(), 5);
  EXPECT_EQ(tokens[0].get_type(), TokenType::Let);
  EXPECT_EQ(tokens[1].get_type(), TokenType::Identifier);
  EXPECT_EQ(tokens[2].get_type(), TokenType::Assign);
  EXPECT_EQ(tokens[3].get_type(), TokenType::Number);
  EXPECT_EQ(tokens[4].get_type(), TokenType::Semicolon);
}

// 여러 라인 처리 테스트
TEST_F(TokenizerTest, MultipleLines) {
  std::vector<String> lines = {String("let x = 42;"),
                               String("let y = 'hello';"),
                               String("return x + y;")};

  auto tokens = tokenizer->tokenize_lines(lines);

  EXPECT_GT(tokens.size(), 0);

  // 첫 번째 라인 확인
  EXPECT_EQ(tokens[0].get_type(), TokenType::Let);
  EXPECT_EQ(tokens[1].get_type(), TokenType::Identifier);
  EXPECT_EQ(tokens[2].get_type(), TokenType::Assign);
  EXPECT_EQ(tokens[3].get_type(), TokenType::Number);
  EXPECT_EQ(tokens[4].get_type(), TokenType::Semicolon);
}

// 빈 입력 테스트
TEST_F(TokenizerTest, EmptyInput) {
  auto tokens = tokenizer->tokenize(String(""));
  EXPECT_EQ(tokens.size(), 0);

  auto tokens2 = tokenizer->tokenize(String("   "));
  EXPECT_EQ(tokens2.size(), 0);
}

} // namespace nugdev::test