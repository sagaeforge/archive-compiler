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

    // 유효한 TokenType 값들만 추출
    auto allTokenTypes = magic_enum::enum_values<TokenType>();
    validTokenTypes.assign(allTokenTypes.begin(), allTokenTypes.end());
  }

  void TearDown() override { converter.reset(); }

  std::unique_ptr<TokenConverter> converter;
  std::vector<Token> testTokens;
  std::vector<TokenType> validTokenTypes;
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

// JSON 에러 케이스 상세 테스트
TEST_F(TokenConverterTest, JsonConversionDetailedErrorCases) {
  lib::JsonDocument doc;
  auto &allocator = doc.GetAllocator();

  // 1. type 필드만 있고 literal 필드가 없는 경우
  JsonValue json1;
  json1.SetObject();
  json1.AddMember("type", JsonValue("Identifier", allocator), allocator);
  Token result1 = converter->from_json(json1);
  EXPECT_EQ(result1.get_type(), TokenType::Illegal);

  // 2. literal 필드만 있고 type 필드가 없는 경우
  JsonValue json2;
  json2.SetObject();
  json2.AddMember("literal", JsonValue("test", allocator), allocator);
  Token result2 = converter->from_json(json2);
  EXPECT_EQ(result2.get_type(), TokenType::Illegal);

  // 3. type 필드가 숫자인 경우
  JsonValue json3;
  json3.SetObject();
  json3.AddMember("type", JsonValue(42), allocator);
  json3.AddMember("literal", JsonValue("test", allocator), allocator);
  Token result3 = converter->from_json(json3);
  EXPECT_EQ(result3.get_type(), TokenType::Illegal);

  // 4. literal 필드가 숫자인 경우
  JsonValue json4;
  json4.SetObject();
  json4.AddMember("type", JsonValue("Identifier", allocator), allocator);
  json4.AddMember("literal", JsonValue(42), allocator);
  Token result4 = converter->from_json(json4);
  EXPECT_EQ(result4.get_type(), TokenType::Illegal);

  // 5. 잘못된 TokenType 이름
  JsonValue json5;
  json5.SetObject();
  json5.AddMember("type", JsonValue("InvalidTokenType", allocator), allocator);
  json5.AddMember("literal", JsonValue("test", allocator), allocator);
  Token result5 = converter->from_json(json5);
  EXPECT_EQ(result5.get_type(), TokenType::Illegal);

  // 6. 빈 TokenType 이름
  JsonValue json6;
  json6.SetObject();
  json6.AddMember("type", JsonValue("", allocator), allocator);
  json6.AddMember("literal", JsonValue("test", allocator), allocator);
  Token result6 = converter->from_json(json6);
  EXPECT_EQ(result6.get_type(), TokenType::Illegal);

  // 7. 추가 필드가 있는 경우 (정상 동작해야 함)
  JsonValue json7;
  json7.SetObject();
  json7.AddMember("type", JsonValue("Identifier", allocator), allocator);
  json7.AddMember("literal", JsonValue("test", allocator), allocator);
  json7.AddMember("extra_field", JsonValue("should_be_ignored", allocator),
                  allocator);
  Token result7 = converter->from_json(json7);
  EXPECT_EQ(result7.get_type(), TokenType::Identifier);
  EXPECT_EQ(result7.get_literal().to_string(), "test");
}

// JSON 직렬화 실패 테스트
TEST_F(TokenConverterTest, JsonSerializationEdgeCases) {
  // 정상적인 시리얼라이제이션이 잘 작동하는지 확인
  Token normalToken(TokenType::Identifier, String("normal"));
  JsonValue json = converter->to_json(normalToken);

  // JSON이 올바른 구조를 가지는지 확인
  EXPECT_TRUE(json.IsObject());
  EXPECT_TRUE(json.HasMember("type"));
  EXPECT_TRUE(json.HasMember("literal"));
  EXPECT_TRUE(json["type"].IsString());
  EXPECT_TRUE(json["literal"].IsString());

  // 실제 값 확인
  EXPECT_EQ(std::string(json["type"].GetString()), "Identifier");
  EXPECT_EQ(std::string(json["literal"].GetString()), "normal");
}

// 모든 유효한 TokenType 테스트
TEST_F(TokenConverterTest, AllValidTokenTypesTest) {
  // magic_enum으로 모든 유효한 TokenType을 테스트
  for (const auto &tokenType : validTokenTypes) {
    String testLiteral("test_" + lib::to_string(tokenType).to_string());
    Token originalToken(tokenType, testLiteral);

    JsonValue json = converter->to_json(originalToken);
    Token convertedToken = converter->from_json(json);

    EXPECT_EQ(originalToken.get_type(), convertedToken.get_type())
        << "Failed for TokenType: " << lib::to_string(tokenType).to_string();
    EXPECT_EQ(originalToken.get_literal().to_string(),
              convertedToken.get_literal().to_string())
        << "Failed for TokenType: " << lib::to_string(tokenType).to_string();
  }
}

// 경계값 테스트
TEST_F(TokenConverterTest, BoundaryValueTests) {
  // 빈 리터럴
  Token emptyToken(TokenType::String, String(""));
  JsonValue emptyJson = converter->to_json(emptyToken);
  Token emptyConverted = converter->from_json(emptyJson);
  EXPECT_EQ(emptyToken.get_type(), emptyConverted.get_type());
  EXPECT_EQ(emptyToken.get_literal().to_string(),
            emptyConverted.get_literal().to_string());

  // 매우 짧은 리터럴
  Token shortToken(TokenType::Identifier, String("a"));
  JsonValue shortJson = converter->to_json(shortToken);
  Token shortConverted = converter->from_json(shortJson);
  EXPECT_EQ(shortToken.get_type(), shortConverted.get_type());
  EXPECT_EQ(shortToken.get_literal().to_string(),
            shortConverted.get_literal().to_string());

  // 특수 토큰 타입들
  std::vector<TokenType> specialTypes = {TokenType::Illegal, TokenType::Null,
                                         TokenType::True, TokenType::False};

  for (const auto &type : specialTypes) {
    Token specialToken(type, String("test"));
    JsonValue specialJson = converter->to_json(specialToken);
    Token specialConverted = converter->from_json(specialJson);
    EXPECT_EQ(specialToken.get_type(), specialConverted.get_type())
        << "Failed for special type: " << lib::to_string(type).to_string();
  }
}

// 문자 인코딩 테스트
TEST_F(TokenConverterTest, CharacterEncodingTests) {
  std::vector<std::pair<String, std::string>> encodingTests = {
      {String("ASCII"), "ASCII only"},
      {String("UTF-8: 한글"), "Korean characters"},
      {String("UTF-8: 日本語"), "Japanese characters"},
      {String("UTF-8: العربية"), "Arabic characters"},
      {String("UTF-8: русский"), "Russian characters"},
      {String("Emojis: 🚀🎉💯"), "Emoji characters"},
      {String("Mixed: Hello世界🌍"), "Mixed encoding"},
      {String("\x01\x02\x03"), "Control characters"},
      {String("\"'`\\"), "Quote characters"}};

  for (const auto &[testString, description] : encodingTests) {
    Token encodingToken(TokenType::String, testString);
    JsonValue encodingJson = converter->to_json(encodingToken);
    Token encodingConverted = converter->from_json(encodingJson);

    EXPECT_EQ(encodingToken.get_type(), encodingConverted.get_type())
        << "Encoding test failed for: " << description;
    EXPECT_EQ(encodingToken.get_literal().to_string(),
              encodingConverted.get_literal().to_string())
        << "Encoding test failed for: " << description;
  }
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

  // 유효한 토큰들을 배치로 생성 (validTokenTypes 사용)
  for (int i = 0; i < 100; ++i) {
    TokenType type = validTokenTypes[i % validTokenTypes.size()];
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

// 랜덤 테스트 (유효한 TokenType만 사용)
TEST_F(TokenConverterTest, RandomizedTest) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> typeDis(0, validTokenTypes.size() - 1);
  std::uniform_int_distribution<> lengthDis(0, 100); // 문자열 길이

  for (int i = 0; i < 50; ++i) {
    TokenType randomType = validTokenTypes[typeDis(gen)];

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
        << "Random test failed for type: "
        << lib::to_string(randomType).to_string();
    EXPECT_EQ(randomToken.get_literal().to_string(),
              jsonConverted.get_literal().to_string())
        << "Random test failed for literal: " << randomLiteral;
  }
}

} // namespace nugdev::test