#include "03_tokenize/TokenConverter.h"

#include <chrono>
#include <gtest/gtest.h>
#include <random>
#include <string>
#include <vector>

using namespace nugdev::compiler::tokenize;
using namespace nugdev::lib;

namespace nugdev::test {

class TokenConverterTest : public ::testing::Test {
protected:
  void SetUp() override {
    converter = std::make_unique<TokenConverter>();

    // 테스트용 토큰들 준비
    testTokens = {Token(TokenType::Identifier, String("myVariable")),
                  Token(TokenType::Number, String("42")),
                  Token(TokenType::String, String("Hello, World!")),
                  Token(TokenType::Boolean, String("true")),
                  Token(TokenType::Plus, String("+")),
                  Token(TokenType::Assign, String("=")),
                  Token(TokenType::LeftBrace, String("{")),
                  Token(TokenType::RightBrace, String("}")),
                  Token(TokenType::Function, String("function")),
                  Token(TokenType::If, String("if")),
                  Token(TokenType::Illegal, String(""))};
  }

  void TearDown() override { converter.reset(); }

  std::unique_ptr<TokenConverter> converter;
  std::vector<Token> testTokens;
};

// JSON 변환 기본 테스트
TEST_F(TokenConverterTest, JsonConversionBasicTokens) {
  std::vector<std::pair<TokenType, String>> testCases = {
      {TokenType::Identifier, String("variable")},
      {TokenType::Number, String("123")},
      {TokenType::String, String("hello")},
      {TokenType::Boolean, String("true")},
      {TokenType::Plus, String("+")},
      {TokenType::Assign, String("=")},
      {TokenType::LeftParen, String("(")},
      {TokenType::RightParen, String(")")},
      {TokenType::If, String("if")},
      {TokenType::Function, String("function")}};

  for (const auto &[tokenType, literal] : testCases) {
    Token originalToken(tokenType, literal);

    JsonValue json = converter->to_json(originalToken);
    Token convertedToken = converter->from_json(json);

    EXPECT_EQ(originalToken.get_type(), convertedToken.get_type())
        << "Token type mismatch for: " << literal.to_string();
    EXPECT_EQ(originalToken.get_literal().to_string(),
              convertedToken.get_literal().to_string())
        << "Token literal mismatch for: " << literal.to_string();
  }
}

// 특수 문자 및 유니코드 테스트
TEST_F(TokenConverterTest, JsonConversionSpecialCharacters) {
  std::vector<std::pair<TokenType, String>> specialCases = {
      {TokenType::String, String("\"quoted string\"")},
      {TokenType::String, String("string with\nnewline")},
      {TokenType::String, String("string with\ttab")},
      {TokenType::Identifier, String("한글변수명")},
      {TokenType::String, String("")},          // 빈 문자열
      {TokenType::String, String("🔥emoji🔥")}, // 이모지
      {TokenType::String, String("special!@#$%^&*()characters")},
      {TokenType::String,
       String("very long string that might cause issues with memory allocation "
              "and string handling in the conversion process")}};

  for (const auto &[tokenType, literal] : specialCases) {
    Token originalToken(tokenType, literal);

    JsonValue json = converter->to_json(originalToken);
    Token convertedToken = converter->from_json(json);

    EXPECT_EQ(originalToken.get_type(), convertedToken.get_type());
    EXPECT_EQ(originalToken.get_literal().to_string(),
              convertedToken.get_literal().to_string());
  }
}

// 유효하지 않은 JSON 테스트
TEST_F(TokenConverterTest, JsonConversionInvalidJson) {
  // null JSON 테스트
  JsonValue invalidJson1;
  Token result1 = converter->from_json(invalidJson1);
  EXPECT_EQ(result1.get_type(), TokenType::Illegal);

  // 배열 JSON 테스트
  JsonValue invalidJson2;
  invalidJson2.SetArray();
  Token result2 = converter->from_json(invalidJson2);
  EXPECT_EQ(result2.get_type(), TokenType::Illegal);

  // 잘못된 필드를 가진 객체 테스트
  JsonValue invalidJson3;
  invalidJson3.SetObject();
  // 필드가 없는 빈 객체
  Token result3 = converter->from_json(invalidJson3);
  EXPECT_EQ(result3.get_type(), TokenType::Illegal);
}

// 연산자 토큰 테스트
TEST_F(TokenConverterTest, OperatorTokens) {
  std::vector<std::pair<TokenType, String>> operators = {
      {TokenType::Plus, String("+")},
      {TokenType::Minus, String("-")},
      {TokenType::Asterisk, String("*")},
      {TokenType::Slash, String("/")},
      {TokenType::Percent, String("%")},
      {TokenType::Equal, String("==")},
      {TokenType::NotEqual, String("!=")},
      {TokenType::LessThan, String("<")},
      {TokenType::GreaterThan, String(">")},
      {TokenType::LessThanEqual, String("<=")},
      {TokenType::GreaterThanEqual, String(">=")},
      {TokenType::LogicalAnd, String("&&")},
      {TokenType::LogicalOr, String("||")},
      {TokenType::Increment, String("++")},
      {TokenType::Decrement, String("--")}};

  for (const auto &[tokenType, literal] : operators) {
    Token originalToken(tokenType, literal);

    // JSON 변환 테스트
    JsonValue json = converter->to_json(originalToken);
    Token jsonConverted = converter->from_json(json);
    EXPECT_EQ(originalToken.get_type(), jsonConverted.get_type());
    EXPECT_EQ(originalToken.get_literal().to_string(),
              jsonConverted.get_literal().to_string());
  }
}

// 키워드 토큰 테스트
TEST_F(TokenConverterTest, KeywordTokens) {
  std::vector<std::pair<TokenType, String>> keywords = {
      {TokenType::Let, String("let")},
      {TokenType::Mut, String("mut")},
      {TokenType::If, String("if")},
      {TokenType::Elif, String("elif")},
      {TokenType::Else, String("else")},
      {TokenType::For, String("for")},
      {TokenType::Break, String("break")},
      {TokenType::Continue, String("continue")},
      {TokenType::Function, String("function")},
      {TokenType::Return, String("return")},
      {TokenType::When, String("when")},
      {TokenType::True, String("true")},
      {TokenType::False, String("false")},
      {TokenType::Null, String("null")}};

  for (const auto &[tokenType, literal] : keywords) {
    Token originalToken(tokenType, literal);

    // JSON 변환 테스트
    JsonValue json = converter->to_json(originalToken);
    Token jsonConverted = converter->from_json(json);
    EXPECT_EQ(originalToken.get_type(), jsonConverted.get_type());
    EXPECT_EQ(originalToken.get_literal().to_string(),
              jsonConverted.get_literal().to_string());
  }
}

// 대칭성 테스트 (roundtrip test)
TEST_F(TokenConverterTest, RoundtripConversion) {
  for (const Token &originalToken : testTokens) {
    // JSON roundtrip
    JsonValue json = converter->to_json(originalToken);
    Token jsonRoundtrip = converter->from_json(json);
    EXPECT_EQ(originalToken.get_type(), jsonRoundtrip.get_type());
    EXPECT_EQ(originalToken.get_literal().to_string(),
              jsonRoundtrip.get_literal().to_string());
  }
}

// 성능 테스트
TEST_F(TokenConverterTest, PerformanceTest) {
  Token testToken(TokenType::Identifier, String("testVariable"));

  auto start = std::chrono::high_resolution_clock::now();

  // JSON 변환 성능 테스트
  for (int i = 0; i < 1000; ++i) {
    JsonValue json = converter->to_json(testToken);
    Token converted = converter->from_json(json);
    EXPECT_EQ(testToken.get_type(), converted.get_type());
  }

  auto jsonEnd = std::chrono::high_resolution_clock::now();
  auto jsonDuration =
      std::chrono::duration_cast<std::chrono::milliseconds>(jsonEnd - start);

  // 성능 정보 출력 (참고용)
  std::cout << "JSON conversion time for 1000 iterations: "
            << jsonDuration.count() << "ms" << std::endl;
}

// 대용량 데이터 테스트
TEST_F(TokenConverterTest, LargeDataTest) {
  // 매우 긴 문자열 리터럴
  std::string longLiteral(10000, 'A'); // 10KB 문자열
  Token longToken(TokenType::String, String(longLiteral));

  // JSON 변환
  JsonValue json = converter->to_json(longToken);
  Token jsonConverted = converter->from_json(json);
  EXPECT_EQ(longToken.get_type(), jsonConverted.get_type());
  EXPECT_EQ(longToken.get_literal().to_string(),
            jsonConverted.get_literal().to_string());
}

// 다중 토큰 배치 테스트
TEST_F(TokenConverterTest, BatchConversionTest) {
  std::vector<Token> batchTokens;
  std::vector<JsonValue> jsonResults;

  // 다양한 토큰들을 배치로 생성
  for (int i = 0; i < 100; ++i) {
    TokenType type = static_cast<TokenType>((i % 10) + 2); // Number부터 시작
    String literal("token_" + std::to_string(i));
    batchTokens.emplace_back(type, literal);
  }

  // 배치 JSON 변환
  for (const auto &token : batchTokens) {
    jsonResults.push_back(converter->to_json(token));
  }

  // 배치 역변환 및 검증
  for (size_t i = 0; i < batchTokens.size(); ++i) {
    Token jsonConverted = converter->from_json(jsonResults[i]);

    EXPECT_EQ(batchTokens[i].get_type(), jsonConverted.get_type())
        << "JSON batch conversion failed at index " << i;
    EXPECT_EQ(batchTokens[i].get_literal().to_string(),
              jsonConverted.get_literal().to_string())
        << "JSON batch conversion failed at index " << i;
  }
}

// 기본 생성자 테스트
TEST_F(TokenConverterTest, DefaultConstructorTest) {
  TokenConverter testConverter;
  Token testToken(TokenType::Identifier, String("test"));

  JsonValue json = testConverter.to_json(testToken);
  Token converted = testConverter.from_json(json);

  EXPECT_EQ(testToken.get_type(), converted.get_type());
  EXPECT_EQ(testToken.get_literal().to_string(),
            converted.get_literal().to_string());
}

// 랜덤 테스트
TEST_F(TokenConverterTest, RandomizedTest) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> typeDis(0, 167);   // TokenType 범위
  std::uniform_int_distribution<> lengthDis(0, 100); // 문자열 길이

  for (int i = 0; i < 50; ++i) {
    TokenType randomType = static_cast<TokenType>(typeDis(gen));

    // 랜덤 문자열 생성
    std::string randomLiteral;
    int length = lengthDis(gen);
    for (int j = 0; j < length; ++j) {
      randomLiteral += static_cast<char>('a' + (gen() % 26));
    }

    Token randomToken(randomType, String(randomLiteral));

    // 변환 테스트
    JsonValue json = converter->to_json(randomToken);
    Token jsonConverted = converter->from_json(json);

    EXPECT_EQ(randomToken.get_type(), jsonConverted.get_type())
        << "Random test failed for type: " << static_cast<int>(randomType);
    EXPECT_EQ(randomToken.get_literal().to_string(),
              jsonConverted.get_literal().to_string())
        << "Random test failed for literal: " << randomLiteral;
  }
}

} // namespace nugdev::test