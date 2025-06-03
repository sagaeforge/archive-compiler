#include "03_tokenize/Token.h"
#include "03_tokenize/TokenCategories.h"

#include <chrono>
#include <gtest/gtest.h>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace nugdev::compiler::tokenize;
using namespace nugdev::lib;

namespace nugdev::test {

class TokenTest : public ::testing::Test {
protected:
  void SetUp() override {
    // 테스트용 토큰들 준비
    testTokens = {Token(TokenType::Identifier, String("myVariable")),
                  Token(TokenType::Number, String("42")),
                  Token(TokenType::String, String("Hello, World!")),
                  Token(TokenType::Boolean, String("true")),
                  Token(TokenType::True, String("true")),
                  Token(TokenType::False, String("false")),
                  Token(TokenType::Null, String("null")),
                  Token(TokenType::Plus, String("+")),
                  Token(TokenType::Minus, String("-")),
                  Token(TokenType::Asterisk, String("*")),
                  Token(TokenType::Assign, String("=")),
                  Token(TokenType::PlusAssign, String("+=")),
                  Token(TokenType::Equal, String("==")),
                  Token(TokenType::NotEqual, String("!=")),
                  Token(TokenType::LessThan, String("<")),
                  Token(TokenType::GreaterThan, String(">")),
                  Token(TokenType::LogicalAnd, String("and")),
                  Token(TokenType::LogicalOr, String("or")),
                  Token(TokenType::LogicalNot, String("not")),
                  Token(TokenType::LeftParen, String("(")),
                  Token(TokenType::RightParen, String(")")),
                  Token(TokenType::LeftBrace, String("{")),
                  Token(TokenType::RightBrace, String("}")),
                  Token(TokenType::Comma, String(",")),
                  Token(TokenType::Semicolon, String(";")),
                  Token(TokenType::Let, String("let")),
                  Token(TokenType::If, String("if")),
                  Token(TokenType::Function, String("function")),
                  Token(TokenType::Return, String("return")),
                  Token(TokenType::Illegal, String(""))};
  }

  std::vector<Token> testTokens;
};

// 기본 생성자 테스트
TEST_F(TokenTest, DefaultConstructor) {
  Token token;

  EXPECT_EQ(token.get_type(), TokenType::Illegal);
  EXPECT_EQ(token.get_literal().to_string(), "");
  EXPECT_FALSE(is_legal(token.get_type()));
}

// 매개변수 생성자 테스트
TEST_F(TokenTest, ParameterizedConstructor) {
  Token token(TokenType::Identifier, String("myVar"));

  EXPECT_EQ(token.get_type(), TokenType::Identifier);
  EXPECT_EQ(token.get_literal().to_string(), "myVar");
  EXPECT_TRUE(is_legal(token.get_type()));
}

// 기본 접근자 테스트
TEST_F(TokenTest, BasicAccessors) {
  for (const auto &token : testTokens) {
    EXPECT_NO_THROW(token.get_type());
    EXPECT_NO_THROW(token.get_literal());

    // 타입과 리터럴이 일관성 있는지 확인
    if (token.get_type() == TokenType::Illegal) {
      EXPECT_FALSE(is_legal(token.get_type()));
    } else {
      EXPECT_TRUE(is_legal(token.get_type()));
    }
  }
}

// is_literal() 메서드 테스트
TEST_F(TokenTest, IsLiteralMethod) {
  // 리터럴이어야 하는 토큰들
  std::vector<TokenType> literalTypes = {TokenType::Number,  TokenType::String,
                                         TokenType::Boolean, TokenType::True,
                                         TokenType::False,   TokenType::Null};

  for (const auto &type : literalTypes) {
    Token token(type, String("test"));
    EXPECT_TRUE(is_literal(type))
        << "TokenType " << static_cast<int>(type) << " should be literal";
  }

  // 리터럴이 아닌 토큰들
  std::vector<TokenType> nonLiteralTypes = {
      TokenType::Identifier, TokenType::Plus, TokenType::If,
      TokenType::LeftParen, TokenType::Illegal};

  for (const auto &type : nonLiteralTypes) {
    Token token(type, String("test"));
    EXPECT_FALSE(is_literal(type))
        << "TokenType " << static_cast<int>(type) << " should not be literal";
  }
}

// is_keyword() 메서드 테스트
TEST_F(TokenTest, IsKeywordMethod) {
  // 키워드인 토큰들
  std::vector<TokenType> keywordTypes = {TokenType::Let,      TokenType::If,
                                         TokenType::Function, TokenType::Return,
                                         TokenType::For,      TokenType::Break};

  for (const auto &type : keywordTypes) {
    Token token(type, String("test"));
    EXPECT_TRUE(is_keyword(type))
        << "TokenType " << static_cast<int>(type) << " should be keyword";
  }

  // 키워드가 아닌 토큰들
  std::vector<TokenType> nonKeywordTypes = {
      TokenType::Identifier, TokenType::Number, TokenType::Plus,
      TokenType::LeftParen, TokenType::Illegal};

  for (const auto &type : nonKeywordTypes) {
    Token token(type, String("test"));
    EXPECT_FALSE(is_keyword(type))
        << "TokenType " << static_cast<int>(type) << " should not be keyword";
  }
}

// is_operator() 메서드 테스트
TEST_F(TokenTest, IsOperatorMethod) {
  // 연산자인 토큰들
  std::vector<TokenType> operatorTypes = {
      TokenType::Plus,       TokenType::Minus,      TokenType::Asterisk,
      TokenType::Assign,     TokenType::PlusAssign, TokenType::Equal,
      TokenType::LogicalAnd, TokenType::LogicalOr};

  for (const auto &type : operatorTypes) {
    Token token(type, String("test"));
    EXPECT_TRUE(is_operator(type))
        << "TokenType " << static_cast<int>(type) << " should be operator";
  }

  // 연산자가 아닌 토큰들
  std::vector<TokenType> nonOperatorTypes = {
      TokenType::Identifier, TokenType::Number, TokenType::LeftParen,
      TokenType::If, TokenType::Illegal};

  for (const auto &type : nonOperatorTypes) {
    Token token(type, String("test"));
    EXPECT_FALSE(is_operator(type))
        << "TokenType " << static_cast<int>(type) << " should not be operator";
  }
}

// is_identifier() 메서드 테스트
TEST_F(TokenTest, IsIdentifierMethod) {
  Token identifierToken(TokenType::Identifier, String("myVar"));
  EXPECT_TRUE(is_identifier(identifierToken.get_type()));

  Token nonIdentifierToken(TokenType::Number, String("42"));
  EXPECT_FALSE(is_identifier(nonIdentifierToken.get_type()));
}

// 비교 연산자 테스트 (C++20 우주선 연산자)
TEST_F(TokenTest, ComparisonOperators) {
  Token token1(TokenType::Identifier, String("abc"));
  Token token2(TokenType::Identifier, String("abc"));
  Token token3(TokenType::Identifier, String("def"));
  Token token4(TokenType::Number, String("abc"));

  // 같음 테스트
  EXPECT_TRUE(token1 == token2);
  EXPECT_FALSE(token1 == token3);
  EXPECT_FALSE(token1 == token4);

  // 다름 테스트
  EXPECT_FALSE(token1 != token2);
  EXPECT_TRUE(token1 != token3);
  EXPECT_TRUE(token1 != token4);

  // 순서 비교 테스트 (타입 우선, 그 다음 리터럴)
  EXPECT_TRUE(token1 <= token2);                   // 같음
  EXPECT_TRUE(token1 < token3 || token1 > token3); // 다름 (순서는 상관없음)
}

// 컨테이너 호환성 테스트
TEST_F(TokenTest, ContainerCompatibility) {
  // std::vector 테스트
  std::vector<Token> tokens(5); // 기본 생성자 필요
  EXPECT_EQ(tokens.size(), 5);
  for (const auto &token : tokens) {
    EXPECT_EQ(token.get_type(), TokenType::Illegal);
  }

  // std::set 테스트 (비교 연산자 필요)
  std::set<Token> tokenSet;
  tokenSet.insert(Token(TokenType::Identifier, String("a")));
  tokenSet.insert(Token(TokenType::Identifier, String("b")));
  tokenSet.insert(Token(TokenType::Identifier, String("a"))); // 중복
  EXPECT_EQ(tokenSet.size(), 2);                              // 중복 제거됨

  // std::unordered_set 테스트 (해시 함수는 추후 구현 시)
}

// 디버깅 메서드 테스트
TEST_F(TokenTest, DebuggingMethods) {
  Token token(TokenType::Identifier, String("myVar"));

  // to_debug_string() 테스트
  lib::String debugStr = token.to_debug_string();
  std::string debugStdStr = debugStr.to_string();

  EXPECT_TRUE(debugStdStr.find("Token{") != std::string::npos);
  EXPECT_TRUE(debugStdStr.find("type=") != std::string::npos);
  EXPECT_TRUE(debugStdStr.find("literal=") != std::string::npos);
  EXPECT_TRUE(debugStdStr.find("myVar") != std::string::npos);

  // 스트림 출력 연산자 테스트
  std::ostringstream oss;
  oss << token;
  std::string streamOutput = oss.str();

  EXPECT_FALSE(streamOutput.empty());
  EXPECT_TRUE(streamOutput.find("myVar") != std::string::npos);
}

// 특수 문자 및 유니코드 테스트
TEST_F(TokenTest, SpecialCharactersAndUnicode) {
  std::vector<std::pair<TokenType, String>> specialCases = {
      {TokenType::String, String("\"quoted string\"")},
      {TokenType::String, String("string with\nnewline")},
      {TokenType::String, String("string with\ttab")},
      {TokenType::Identifier, String("한글변수명")},
      {TokenType::String, String("")},          // 빈 문자열
      {TokenType::String, String("🔥emoji🔥")}, // 이모지
      {TokenType::String, String("special!@#$%^&*()characters")}};

  for (const auto &[tokenType, literal] : specialCases) {
    Token token(tokenType, literal);

    EXPECT_EQ(token.get_type(), tokenType);
    EXPECT_EQ(token.get_literal().to_string(), literal.to_string());

    // 디버그 출력이 정상 동작하는지 확인
    EXPECT_NO_THROW(token.to_debug_string());
  }
}

// 경계값 테스트
TEST_F(TokenTest, BoundaryValues) {
  // 매우 긴 리터럴
  std::string longLiteral(10000, 'a');
  Token longToken(TokenType::String, String(longLiteral));

  EXPECT_EQ(longToken.get_literal().to_string(), longLiteral);
  EXPECT_NO_THROW(longToken.to_debug_string());

  // 빈 리터럴
  Token emptyToken(TokenType::String, String(""));
  EXPECT_EQ(emptyToken.get_literal().to_string(), "");
  EXPECT_TRUE(is_legal(emptyToken.get_type()));
}

// 토큰 카테고리 일관성 테스트
TEST_F(TokenTest, CategoryConsistency) {
  for (const auto &token : testTokens) {
    // 하나의 토큰은 여러 카테고리에 속할 수 있지만,
    // Illegal 토큰만은 다른 어떤 카테고리에도 속하지 않아야 함
    if (token.get_type() == TokenType::Illegal) {
      EXPECT_FALSE(is_literal(token.get_type()));
      EXPECT_FALSE(is_keyword(token.get_type()));
      EXPECT_FALSE(is_operator(token.get_type()));
      EXPECT_FALSE(is_identifier(token.get_type()));
      EXPECT_FALSE(is_legal(token.get_type()));
    }

    // 식별자는 다른 카테고리에 속하지 않음
    if (is_identifier(token.get_type())) {
      EXPECT_FALSE(is_literal(token.get_type()));
      EXPECT_FALSE(is_keyword(token.get_type()));
      EXPECT_FALSE(is_operator(token.get_type()));
      EXPECT_TRUE(is_legal(token.get_type()));
    }
  }
}

// 성능 테스트 (간단한)
TEST_F(TokenTest, PerformanceTest) {
  const int iterations = 10000;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < iterations; ++i) {
    Token token(TokenType::Identifier, String("test"));
    volatile bool result =
        is_identifier(token.get_type()); // volatile로 최적화 방지
    (void)result;                        // 경고 제거
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  // 10000번 생성과 카테고리 확인이 1초 이내에 완료되어야 함
  EXPECT_LT(duration.count(), 1000000);
}

} // namespace nugdev::test