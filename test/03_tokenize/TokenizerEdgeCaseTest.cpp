#include "03_tokenize/TokenCategories.h"
#include "03_tokenize/Tokenizer.h"

#include <chrono>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace nugdev::compiler::tokenize;
using namespace nugdev::lib;

namespace nugdev::test {

class TokenizerEdgeCaseTest : public ::testing::Test {
protected:
  void SetUp() override { tokenizer = std::make_unique<Tokenizer>(); }

  void TearDown() override { tokenizer.reset(); }

  std::unique_ptr<Tokenizer> tokenizer;
};

// 숫자 경계 케이스 테스트
TEST_F(TokenizerEdgeCaseTest, NumberEdgeCases) {
  std::vector<std::pair<std::string, std::string>> validCases = {
      {"0.0", "0.0"},
      {"123.456789", "123.456789"},
      {"999999999", "999999999"},
      {"0000123", "0000123"}, // leading zeros
      {"1.", "1"},            // 소수점만 있고 뒤에 숫자가 없으면 정수로 처리
  };

  for (const auto &[input, expected] : validCases) {
    auto tokens = tokenizer->tokenize(String(input));
    ASSERT_GE(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), TokenType::Number) << "Input: " << input;
  }

  // 잘못된 숫자 형식들
  std::vector<std::string> invalidCases = {
      "123.456.789", // 소수점 2개
  };

  for (const auto &input : invalidCases) {
    auto tokens = tokenizer->tokenize(String(input));
    // 123.456.789의 경우 여러 토큰으로 분리되어야 함
    ASSERT_GT(tokens.size(), 1) << "Input: " << input;
  }

  // .123의 경우 이제 0.123으로 정규화되어 하나의 토큰이 됨
  auto dotNumberTokens = tokenizer->tokenize(String(".123"));
  ASSERT_EQ(dotNumberTokens.size(), 1) << "Input: .123";
  EXPECT_EQ(dotNumberTokens[0].get_type(), TokenType::Number) << "Input: .123";
  EXPECT_EQ(dotNumberTokens[0].get_literal().to_string(), "0.123")
      << "Input: .123";
}

// 문자열 이스케이프 및 특수 케이스 테스트
TEST_F(TokenizerEdgeCaseTest, StringEscapeAndSpecialCases) {
  std::vector<std::pair<std::string, std::string>> testCases = {
      {"\"hello\\nworld\"", "hello\\nworld"},
      {"\"tab\\there\"", "tab\\there"},
      {"\"quote\\\"inside\"", "quote\\\"inside"},
      {"\"backslash\\\\\"", "backslash\\\\"},
      {"'single\\\"quote'", "single\\\"quote"},
      {"`backtick string`", "backtick string"},
      {"\"\"", ""}, // 빈 문자열
      {"''", ""},   // 빈 단일 따옴표 문자열
      {"``", ""},   // 빈 백틱 문자열
      {"\"very long string with many words and spaces\"",
       "very long string with many words and spaces"},
  };

  for (const auto &[input, expected] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), TokenType::String) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), expected)
        << "Input: " << input;
  }
}

// 식별자 경계 케이스 테스트
TEST_F(TokenizerEdgeCaseTest, IdentifierEdgeCases) {
  std::vector<std::string> validCases = {
      "_",
      "_123",
      "_abc",
      "abc_",
      "abc_def",
      "a1b2c3",
      "CamelCase",
      "SCREAMING_CASE",
      "mix123ABC",
      "under_score_123",
      "veryLongIdentifierName",
  };

  for (const auto &input : validCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), TokenType::Identifier)
        << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), input) << "Input: " << input;
  }

  // 키워드와 유사하지만 다른 식별자들
  std::vector<std::string> nearKeywords = {"lett",  "iff",    "lett123",
                                           "true_", "_false", "null123"};

  for (const auto &input : nearKeywords) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), TokenType::Identifier)
        << "Input: " << input;
    EXPECT_FALSE(is_keyword(tokens[0].get_type())) << "Input: " << input;
  }
}

// 연산자 조합 및 경계 케이스 테스트
TEST_F(TokenizerEdgeCaseTest, OperatorCombinations) {
  std::vector<std::pair<std::string, std::vector<TokenType>>> testCases = {
      {"++--", {TokenType::Increment, TokenType::Decrement}},
      {"+-*/",
       {TokenType::Plus, TokenType::Minus, TokenType::Asterisk,
        TokenType::Slash}},
      {"==!=", {TokenType::Equal, TokenType::NotEqual}},
      {"<<>>", {TokenType::BitwiseShiftLeft, TokenType::BitwiseShiftRight}},
      {"<=>=", {TokenType::LessThanEqual, TokenType::GreaterThanEqual}},
      {"!!?:", {TokenType::NullAssertion, TokenType::NullElvis}},
      {"&=|=", {TokenType::AmpersandAssign, TokenType::PipeAssign}},
      {"^=~=", {TokenType::CaretAssign, TokenType::TildeAssign}},
  };

  for (const auto &[input, expectedTypes] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), expectedTypes.size()) << "Input: " << input;
    for (size_t i = 0; i < expectedTypes.size(); ++i) {
      EXPECT_EQ(tokens[i].get_type(), expectedTypes[i])
          << "Input: " << input << ", Token index: " << i;
    }
  }
}

// 주석 패턴 테스트
TEST_F(TokenizerEdgeCaseTest, CommentPatterns) {
  std::vector<std::pair<std::string, size_t>> testCases = {
      {"// comment only", 0},
      {"# hash comment only", 0},
      {"let x = 5; // end comment", 5},
      {"let y = 10; # hash end comment", 5},
      {"# start comment\nlet z = 15;", 5},
      {"// start comment\nlet w = 20;", 5},
      {"let a = 1; // comment\nlet b = 2;", 10}, // 두 라인
      {"# comment\nlet c = 3; # another comment", 5},
      {"//", 0}, // 빈 주석
      {"#", 0},  // 빈 해시 주석
      {"/// triple slash", 0},
      {"### triple hash", 0},
  };

  for (const auto &[input, expectedTokenCount] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    EXPECT_EQ(tokens.size(), expectedTokenCount) << "Input: " << input;

    // 주석 토큰은 없어야 함
    for (const auto &token : tokens) {
      EXPECT_NE(token.get_type(), TokenType::Illegal) << "Input: " << input;
    }
  }
}

// 공백 및 제어 문자 처리 테스트
TEST_F(TokenizerEdgeCaseTest, WhitespaceAndControlChars) {
  std::vector<std::pair<std::string, size_t>> testCases = {
      {"   \t  \n  let \r\n x \t = \n 42 \r ; \t  ", 5},
      {"\n\n\nlet\n\nx\n\n=\n\n42\n\n;\n\n", 5},
      {"let\tx\t=\t42\t;", 5},
      {" let x = 42 ; ", 5},
      {"let x=42;", 5},                        // 공백 없음
      {"\r\nlet\r\nx\r\n=\r\n42\r\n;\r\n", 5}, // Windows 스타일 줄바꿈
  };

  for (const auto &[input, expectedTokenCount] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    EXPECT_EQ(tokens.size(), expectedTokenCount) << "Input: [" << input << "]";
    if (expectedTokenCount == 5) {
      EXPECT_EQ(tokens[0].get_type(), TokenType::Let);
      EXPECT_EQ(tokens[1].get_type(), TokenType::Identifier);
      EXPECT_EQ(tokens[2].get_type(), TokenType::Assign);
      EXPECT_EQ(tokens[3].get_type(), TokenType::Number);
      EXPECT_EQ(tokens[4].get_type(), TokenType::Semicolon);
    }
  }
}

// 상세한 복합 표현식 테스트
TEST_F(TokenizerEdgeCaseTest, DetailedComplexExpressions) {
  std::vector<std::pair<std::string, std::vector<TokenType>>> testCases = {
      // 함수 호출
      {"func(arg1, arg2)",
       {TokenType::Identifier, TokenType::LeftParen, TokenType::Identifier,
        TokenType::Comma, TokenType::Identifier, TokenType::RightParen}},

      // 배열 접근
      {"arr[index]",
       {TokenType::Identifier, TokenType::LeftBracket, TokenType::Identifier,
        TokenType::RightBracket}},

      // 조건 연산자
      {"a ? b : c",
       {TokenType::Identifier, TokenType::Question, TokenType::Identifier,
        TokenType::Colon, TokenType::Identifier}},

      // Null 병합 연산자
      {"value ?: default",
       {TokenType::Identifier, TokenType::NullElvis, TokenType::Identifier}},

      // 복합 할당과 증감 연산자
      {"i++ + --j",
       {TokenType::Identifier, TokenType::Increment, TokenType::Plus,
        TokenType::Decrement, TokenType::Identifier}},

      // 비트 시프트와 비교
      {"x << 2 >= y >> 1",
       {TokenType::Identifier, TokenType::BitwiseShiftLeft, TokenType::Number,
        TokenType::GreaterThanEqual, TokenType::Identifier,
        TokenType::BitwiseShiftRight, TokenType::Number}},
  };

  for (const auto &[input, expectedTypes] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), expectedTypes.size()) << "Input: " << input;
    for (size_t i = 0; i < expectedTypes.size(); ++i) {
      EXPECT_EQ(tokens[i].get_type(), expectedTypes[i])
          << "Input: " << input << ", Token index: " << i
          << ", Expected: " << static_cast<int>(expectedTypes[i])
          << ", Actual: " << static_cast<int>(tokens[i].get_type());
    }
  }
}

// 토큰화 함수별 테스트 (tokenize vs tokenize_line vs tokenize_lines)
TEST_F(TokenizerEdgeCaseTest, TokenizationMethods) {
  String singleLine = String("let x = 42;");
  std::vector<String> multipleLines = {String("let x = 42;"),
                                       String("let y = 'hello';"),
                                       String("return x + y;")};

  // tokenize와 tokenize_line이 같은 결과를 내는지 확인
  auto tokens1 = tokenizer->tokenize(singleLine);
  auto tokens2 = tokenizer->tokenize_line(singleLine);

  ASSERT_EQ(tokens1.size(), tokens2.size());
  for (size_t i = 0; i < tokens1.size(); ++i) {
    EXPECT_EQ(tokens1[i].get_type(), tokens2[i].get_type());
    EXPECT_EQ(tokens1[i].get_literal().to_string(),
              tokens2[i].get_literal().to_string());
  }

  // tokenize_lines 테스트
  auto tokensMulti = tokenizer->tokenize_lines(multipleLines);
  EXPECT_GT(tokensMulti.size(), 0);

  // 첫 번째 라인의 토큰들이 올바른지 확인
  EXPECT_EQ(tokensMulti[0].get_type(), TokenType::Let);
  EXPECT_EQ(tokensMulti[1].get_type(), TokenType::Identifier);
  EXPECT_EQ(tokensMulti[2].get_type(), TokenType::Assign);
  EXPECT_EQ(tokensMulti[3].get_type(), TokenType::Number);
  EXPECT_EQ(tokensMulti[4].get_type(), TokenType::Semicolon);
}

// 경계값 및 극단 케이스 테스트
TEST_F(TokenizerEdgeCaseTest, BoundaryAndExtremeCases) {
  // 매우 긴 식별자
  std::string longIdentifier(1000, 'a');
  auto tokens = tokenizer->tokenize(String(longIdentifier));
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].get_type(), TokenType::Identifier);

  // 매우 긴 문자열
  std::string longString = "\"" + std::string(1000, 'x') + "\"";
  tokens = tokenizer->tokenize(String(longString));
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].get_type(), TokenType::String);

  // 매우 긴 숫자
  std::string longNumber(100, '9');
  tokens = tokenizer->tokenize(String(longNumber));
  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].get_type(), TokenType::Number);

  // 중첩된 구조
  tokens = tokenizer->tokenize(String("(((())))"));
  ASSERT_EQ(tokens.size(), 8);
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(tokens[i].get_type(), TokenType::LeftParen);
    EXPECT_EQ(tokens[i + 4].get_type(), TokenType::RightParen);
  }
}

// 성능 테스트 (간단한)
TEST_F(TokenizerEdgeCaseTest, PerformanceTest) {
  std::string largeInput;
  for (int i = 0; i < 500; ++i) {
    largeInput +=
        "let var" + std::to_string(i) + " = " + std::to_string(i) + "; ";
  }

  auto start = std::chrono::high_resolution_clock::now();
  auto tokens = tokenizer->tokenize(String(largeInput));
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  EXPECT_GT(tokens.size(), 0);
  EXPECT_LT(duration.count(), 5000); // 5초 이내
}

} // namespace nugdev::test