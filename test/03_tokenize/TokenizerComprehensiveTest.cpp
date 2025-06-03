#include "03_tokenize/TokenCategories.h"
#include "03_tokenize/Tokenizer.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace nugdev::compiler::tokenize;
using namespace nugdev::lib;

namespace nugdev::test {

class TokenizerComprehensiveTest : public ::testing::Test {
protected:
  void SetUp() override { tokenizer = std::make_unique<Tokenizer>(); }

  void TearDown() override { tokenizer.reset(); }

  std::unique_ptr<Tokenizer> tokenizer;
};

// 누락된 특수 토큰 테스트
TEST_F(TokenizerComprehensiveTest, MissingSpecialTokens) {
  std::vector<std::pair<std::string, TokenType>> testCases = {
      {"@", TokenType::At},
      {"$", TokenType::Dollar},
      {"\\", TokenType::Backslash},
      {"?.", TokenType::NullSafeAccess},
  };

  for (const auto &[input, expectedType] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), expectedType) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), input) << "Input: " << input;
  }
}

// 누락된 키워드 테스트
TEST_F(TokenizerComprehensiveTest, MissingKeywords) {
  std::vector<std::pair<std::string, TokenType>> testCases = {
      {"elif", TokenType::Elif},
      {"break", TokenType::Break},
      {"continue", TokenType::Continue},
      {"when", TokenType::When},
  };

  for (const auto &[input, expectedType] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), expectedType) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), input) << "Input: " << input;
    EXPECT_TRUE(is_keyword(tokens[0].get_type())) << "Input: " << input;
  }
}

// 복잡한 숫자 형식 테스트
TEST_F(TokenizerComprehensiveTest, ComplexNumberFormats) {
  std::vector<std::pair<std::string, std::string>> testCases = {
      // 유효한 숫자들
      {"0", "0"},
      {"123", "123"},
      {"3.14", "3.14"},
      {"0.0", "0.0"},
      {"999.999", "999.999"},
      {"0000123", "0000123"}, // leading zeros

      // 정규화되는 케이스들
      {".123", "0.123"}, // .123 → 0.123
      {"1.", "1.0"},     // 1. → 1.0
      {"123.", "123.0"}, // 123. → 123.0
  };

  for (const auto &[input, expected] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), TokenType::Number) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), expected)
        << "Input: " << input;
  }

  // 잘못된 형식들 (여러 토큰으로 분리되어야 함)
  std::vector<std::string> multiTokenCases = {
      "123.456.789", // 소수점 2개
  };

  for (const auto &input : multiTokenCases) {
    auto tokens = tokenizer->tokenize(String(input));
    ASSERT_GT(tokens.size(), 1) << "Input: " << input;
  }
}

// 잘못된 연산자 조합 테스트
TEST_F(TokenizerComprehensiveTest, InvalidOperatorCombinations) {
  std::vector<std::pair<std::string, std::vector<TokenType>>> testCases = {
      // 이들은 개별 토큰으로 분리되어야 함
      {"===", {TokenType::Equal, TokenType::Assign}},              // == 와 =
      {"!==", {TokenType::NotEqual, TokenType::Assign}},           // != 와 =
      {"+++", {TokenType::Increment, TokenType::Plus}},            // ++ 와 +
      {"---", {TokenType::Decrement, TokenType::Minus}},           // -- 와 -
      {"<<<", {TokenType::BitwiseShiftLeft, TokenType::LessThan}}, // << 와 <
      {">>>",
       {TokenType::BitwiseShiftRight, TokenType::GreaterThan}}, // >> 와 >
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

// 중첩된 따옴표와 이스케이프 테스트
TEST_F(TokenizerComprehensiveTest, NestedQuotesAndEscapes) {
  std::vector<std::pair<std::string, std::string>> testCases = {
      // 다양한 이스케이프 시퀀스
      {"\"hello\\\"world\\\"\"", "hello\\\"world\\\""},
      {"'can\\'t'", "can\\'t"},
      {"`template\\`string`", "template\\`string"},

      // 복잡한 이스케이프
      {"\"line1\\nline2\\ttab\"", "line1\\nline2\\ttab"},
      {"\"path\\\\to\\\\file\"", "path\\\\to\\\\file"},
      {"\"unicode\\u0041\"", "unicode\\u0041"},

      // 빈 문자열들
      {"\"\"", ""},
      {"''", ""},
      {"``", ""},
  };

  for (const auto &[input, expected] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_EQ(tokens.size(), 1) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), TokenType::String) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_literal().to_string(), expected)
        << "Input: " << input;
  }
}

// 에러 메시지 정확성 테스트
TEST_F(TokenizerComprehensiveTest, ErrorMessageAccuracy) {
  std::vector<std::pair<std::string, std::string>> errorCases = {
      {"\"unterminated", "Unterminated string"},
      {"'unterminated", "Unterminated string"},
      {"`unterminated", "Unterminated string"},
      {"€", "Unexpected character"},
      {"🚀", "Unexpected character"},
      {"\x01", "Unexpected character"},
  };

  for (const auto &[input, expectedErrorContains] : errorCases) {
    auto tokens = tokenizer->tokenize(String(input));

    bool hasError = false;
    for (const auto &token : tokens) {
      if (token.get_type() == TokenType::Illegal) {
        hasError = true;
        std::string errorMsg = token.get_literal().to_string();
        EXPECT_TRUE(errorMsg.find(expectedErrorContains) != std::string::npos)
            << "Input: " << input
            << ", Expected error containing: " << expectedErrorContains
            << ", Actual: " << errorMsg;
        break;
      }
    }
    EXPECT_TRUE(hasError) << "Input: " << input << " should produce an error";
  }
}

// 복잡한 표현식 조합 테스트
TEST_F(TokenizerComprehensiveTest, ComplexExpressionCombinations) {
  std::vector<std::pair<std::string, size_t>> testCases = {
      // 복잡한 산술 표현식
      {"((a + b) * c) / (d - e)", 15},

      // 함수 호출과 배열 접근 조합 - obj.method(arr[i], func(x, y))
      // obj . method ( arr [ i ] , func ( x , y ) ) = 16개
      {"obj.method(arr[i], func(x, y))", 16},

      // 조건부 표현식 - condition ? value1 : value2
      // condition ? value1 : value2 = 5개 (공백으로 분리됨)
      {"condition ? value1 : value2", 5},

      // Null 안전 접근 연산자 조합 - obj?.property ?: defaultValue
      // obj ?. property ?: defaultValue = 5개 (?.가 하나의 토큰)
      {"obj?.property ?: defaultValue", 5},

      // 비트 연산과 비교 조합
      {"(x << 2) | (y >> 1) >= z", 13},

      // 할당과 증감 연산자 조합 - arr[i++] = ++counter
      // arr [ i ++ ] = ++ counter = 8개
      {"arr[i++] = ++counter", 8},
  };

  for (const auto &[input, expectedTokenCount] : testCases) {
    auto tokens = tokenizer->tokenize(String(input));

    EXPECT_EQ(tokens.size(), expectedTokenCount) << "Input: " << input;

    // 모든 토큰이 유효한지 확인
    for (const auto &token : tokens) {
      EXPECT_NE(token.get_type(), TokenType::Illegal)
          << "Input: " << input
          << ", Illegal token: " << token.get_literal().to_string();
    }
  }
}

// 메모리 효율성 테스트
TEST_F(TokenizerComprehensiveTest, MemoryEfficiencyTest) {
  // 큰 입력에 대한 메모리 사용량 테스트
  std::string largeInput;
  const int iterations = 1000;

  for (int i = 0; i < iterations; ++i) {
    largeInput += "let variable" + std::to_string(i) + " = " +
                  std::to_string(i * 2) + " + " + std::to_string(i * 3) + "; ";
  }

  auto tokens = tokenizer->tokenize(String(largeInput));

  // 예상 토큰 수: 각 라인당 7개 토큰 (let, variable, =, number, +, number, ;)
  size_t expectedTokens = iterations * 7;
  EXPECT_EQ(tokens.size(), expectedTokens);

  // 모든 토큰이 유효한지 확인
  for (const auto &token : tokens) {
    EXPECT_NE(token.get_type(), TokenType::Illegal);
  }
}

// 스트림 일관성 테스트
TEST_F(TokenizerComprehensiveTest, StreamConsistencyTest) {
  std::string input = "let x = 42; let y = 'hello'; return x + y;";

  // 전체를 한 번에 토큰화
  auto allAtOnce = tokenizer->tokenize(String(input));

  // 부분별로 토큰화해서 합치기
  std::vector<Token> accumulated;

  std::vector<std::string> parts = {"let x = 42; ", "let y = 'hello'; ",
                                    "return x + y;"};

  for (const auto &part : parts) {
    auto partTokens = tokenizer->tokenize(String(part));
    accumulated.insert(accumulated.end(), partTokens.begin(), partTokens.end());
  }

  // 결과가 동일해야 함
  ASSERT_EQ(allAtOnce.size(), accumulated.size());

  for (size_t i = 0; i < allAtOnce.size(); ++i) {
    EXPECT_EQ(allAtOnce[i].get_type(), accumulated[i].get_type())
        << "Token index: " << i;
    EXPECT_EQ(allAtOnce[i].get_literal().to_string(),
              accumulated[i].get_literal().to_string())
        << "Token index: " << i;
  }
}

// 카테고리 함수 완전성 테스트
TEST_F(TokenizerComprehensiveTest, CategoryFunctionCompleteness) {
  // 모든 TokenType이 적절한 카테고리 함수에서 처리되는지 확인
  std::vector<std::pair<std::string, TokenType>> allTokens = {
      // 리터럴들
      {"42", TokenType::Number},
      {"\"string\"", TokenType::String},
      {"identifier", TokenType::Identifier},
      {"true", TokenType::True},
      {"false", TokenType::False},
      {"null", TokenType::Null},

      // 연산자들
      {"+", TokenType::Plus},
      {"-", TokenType::Minus},
      {"*", TokenType::Asterisk},
      {"/", TokenType::Slash},
      {"%", TokenType::Percent},
      {"==", TokenType::Equal},
      {"!=", TokenType::NotEqual},
      {"<", TokenType::LessThan},
      {">", TokenType::GreaterThan},
      {"<=", TokenType::LessThanEqual},
      {">=", TokenType::GreaterThanEqual},

      // 할당 연산자들
      {"=", TokenType::Assign},
      {"+=", TokenType::PlusAssign},
      {"-=", TokenType::MinusAssign},

      // 구분자들
      {"(", TokenType::LeftParen},
      {")", TokenType::RightParen},
      {"{", TokenType::LeftBrace},
      {"}", TokenType::RightBrace},
      {"[", TokenType::LeftBracket},
      {"]", TokenType::RightBracket},
      {",", TokenType::Comma},
      {";", TokenType::Semicolon},

      // 키워드들
      {"let", TokenType::Let},
      {"if", TokenType::If},
      {"else", TokenType::Else},
      {"for", TokenType::For},
      {"function", TokenType::Function},
      {"return", TokenType::Return},
  };

  for (const auto &[input, expectedType] : allTokens) {
    auto tokens = tokenizer->tokenize(String(input));

    ASSERT_GT(tokens.size(), 0) << "Input: " << input;
    EXPECT_EQ(tokens[0].get_type(), expectedType) << "Input: " << input;

    // 카테고리 함수들이 올바르게 작동하는지 확인
    TokenType type = tokens[0].get_type();

    // 각 토큰이 적어도 하나의 카테고리에는 속해야 함
    bool isInSomeCategory =
        is_literal(type) || is_operator(type) || is_delimiter(type) ||
        is_keyword(type) || is_identifier(type) ||
        is_assignment_operator(type) || is_comparison_operator(type);

    EXPECT_TRUE(isInSomeCategory)
        << "Token " << input << " is not in any category";
  }
}

} // namespace nugdev::test