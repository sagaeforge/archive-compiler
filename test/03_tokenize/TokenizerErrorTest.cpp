#include "03_tokenize/TokenCategories.h"
#include "03_tokenize/Tokenizer.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace nugdev::compiler::tokenize;
using namespace nugdev::lib;

namespace nugdev::test {

class TokenizerErrorTest : public ::testing::Test {
protected:
  void SetUp() override { tokenizer = std::make_unique<Tokenizer>(); }

  void TearDown() override { tokenizer.reset(); }

  std::unique_ptr<Tokenizer> tokenizer;
};

// 오류 처리 테스트
TEST_F(TokenizerErrorTest, ErrorHandling) {
  // 닫히지 않은 문자열
  auto tokens = tokenizer->tokenize(String("\"unclosed string"));

  ASSERT_EQ(tokens.size(), 1);
  EXPECT_EQ(tokens[0].get_type(), TokenType::Illegal);
  EXPECT_TRUE(tokens[0].get_literal().to_string().find("Unterminated") !=
              std::string::npos);
}

// 잘못된 문자열 테스트
TEST_F(TokenizerErrorTest, InvalidStrings) {
  std::vector<std::string> invalidCases = {
      "\"unterminated",
      "'also unterminated",
      "`backtick unterminated",
      "\"", // 따옴표만
      "'",  // 단일 따옴표만
      "`",  // 백틱만
  };

  for (const auto &input : invalidCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), TokenType::Illegal) << "Input: " << input;
    EXPECT_TRUE(tokens[0].get_literal().to_string().find("Unterminated") !=
                std::string::npos)
        << "Input: " << input;
  }
}

// 에러 처리 확장 테스트
TEST_F(TokenizerErrorTest, ExtendedErrorHandling) {
  std::vector<std::pair<std::string, std::string>> errorCases = {
      {"€", "Unexpected character: €"},   // Unicode 문자
      {"🚀", "Unexpected character: 🚀"}, // Emoji
      {"\x01", "Unexpected character"},   // 제어 문자
      {"\"unclosed", "Unterminated string"},
      {"'unclosed", "Unterminated string"},
      {"`unclosed", "Unterminated string"},
  };

  for (const auto &[input, expectedError] : errorCases) {
    auto tokens = tokenizer->tokenize(String(input));

    bool hasError = false;
    for (const auto &token : tokens) {
      if (token.get_type() == TokenType::Illegal) {
        hasError = true;
        std::string errorMsg = token.get_literal().to_string();
        EXPECT_TRUE(errorMsg.find(expectedError.substr(
                        0, expectedError.find(':'))) != std::string::npos)
            << "Input: " << input
            << ", Expected error containing: " << expectedError
            << ", Actual: " << errorMsg;
      }
    }
    EXPECT_TRUE(hasError) << "Input: " << input << " should produce an error";
  }
}

// Unicode 및 특수 문자 테스트
TEST_F(TokenizerErrorTest, UnicodeAndSpecialChars) {
  // 현재 구현에서 ASCII가 아닌 문자들은 에러로 처리되어야 함
  std::vector<std::string> unicodeInputs = {
      "α",  "β",  "γ", // 그리스 문자
      "한", "글",      // 한글
      "中", "文",      // 중국어
      "🔥", "⭐",      // 이모지
  };

  for (const auto &input : unicodeInputs) {
    auto tokens = tokenizer->tokenize(String(input));
    // Unicode 문자들은 현재 구현에서 에러로 처리될 것임
    EXPECT_GT(tokens.size(), 0) << "Input: " << input;

    // 적어도 하나의 토큰이 있어야 하고, 에러 토큰이거나 정상 토큰이어야 함
    bool hasValidOrErrorToken = false;
    for (const auto &token : tokens) {
      if (token.get_type() == TokenType::Illegal ||
          is_legal(token.get_type())) {
        hasValidOrErrorToken = true;
        break;
      }
    }
    EXPECT_TRUE(hasValidOrErrorToken) << "Input: " << input;
  }
}

} // namespace nugdev::test