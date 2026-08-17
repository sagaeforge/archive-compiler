#include "03_tokenize/Token.h"
#include "03_tokenize/TokenType.h"
#include "04_parsing/Parser.hpp"
#include <gtest/gtest.h>

using namespace nugdev::compiler::parsing;
using namespace nugdev::compiler::tokenize;
using namespace nugdev::ast;

namespace nugdev::test {

/**
 * @brief Test class for literal parsing functionality
 */
class LiteralParsingTest : public ::testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}

  std::vector<Token> create_tokens(
      const std::vector<std::pair<TokenType, std::string>> &token_specs) {
    std::vector<Token> tokens;
    for (const auto &spec : token_specs) {
      tokens.emplace_back(spec.first, nugdev::lib::String(spec.second));
    }
    tokens.emplace_back(TokenType::EOF_TOKEN, nugdev::lib::String(""));
    return tokens;
  }

  std::unique_ptr<Program> safe_parse(const std::vector<Token> &tokens) {
    try {
      Parser parser(tokens);
      return parser.parse();
    } catch (const Parser::ParseException &e) {
      ADD_FAILURE() << "Parse error: " << e.what();
      return nullptr;
    }
  }
};

// ==================== Number Literal Tests ====================

TEST_F(LiteralParsingTest, DecimalNumbers) {
  struct TestCase {
    std::string input;
    std::string expected;
  };

  std::vector<TestCase> test_cases = {{"0", "0"},
                                      {"42", "42"},
                                      {"123", "123"},
                                      {"999", "999"},
                                      {"1000000", "1000000"}};

  for (const auto &test_case : test_cases) {
    auto tokens = create_tokens({{TokenType::Number, test_case.input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse: " << test_case.input;
  }
}

TEST_F(LiteralParsingTest, FloatingPointNumbers) {
  std::vector<std::string> test_cases = {"0.0", "3.14",    "123.456",
                                         "0.5", "999.999", "1.0"};

  for (const auto &input : test_cases) {
    auto tokens = create_tokens({{TokenType::Number, input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse floating point: " << input;
  }
}

TEST_F(LiteralParsingTest, HexadecimalNumbers) {
  std::vector<std::string> test_cases = {"0x0", "0xFF", "0xDEADBEEF", "0x123",
                                         "0xABC"};

  for (const auto &input : test_cases) {
    auto tokens = create_tokens({{TokenType::Number, input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse hexadecimal: " << input;
  }
}

TEST_F(LiteralParsingTest, BinaryNumbers) {
  std::vector<std::string> test_cases = {"0b0", "0b1", "0b101", "0b11110000",
                                         "0b1010101"};

  for (const auto &input : test_cases) {
    auto tokens = create_tokens({{TokenType::Number, input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse binary: " << input;
  }
}

TEST_F(LiteralParsingTest, OctalNumbers) {
  std::vector<std::string> test_cases = {"0o0", "0o7", "0o123", "0o777",
                                         "0o644"};

  for (const auto &input : test_cases) {
    auto tokens = create_tokens({{TokenType::Number, input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse octal: " << input;
  }
}

// ==================== String Literal Tests ====================

TEST_F(LiteralParsingTest, BasicStrings) {
  std::vector<std::string> test_cases = {"\"\"",
                                         "\"hello\"",
                                         "\"world\"",
                                         "\"Hello, World!\"",
                                         "\"string with spaces\"",
                                         "\"123\"",
                                         "\"mixed123abc\""};

  for (const auto &input : test_cases) {
    auto tokens = create_tokens({{TokenType::String, input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse string: " << input;
  }
}

TEST_F(LiteralParsingTest, StringsWithSpecialCharacters) {
  std::vector<std::string> test_cases = {
      "\"string\\nwith\\nnewlines\"", "\"string\\twith\\ttabs\"",
      "\"string with \\\"quotes\\\"\"", "\"path\\\\with\\\\backslashes\"",
      "\"unicode\\u0020test\""};

  for (const auto &input : test_cases) {
    auto tokens = create_tokens({{TokenType::String, input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr)
        << "Failed to parse string with special chars: " << input;
  }
}

// ==================== Boolean Literal Tests ====================

TEST_F(LiteralParsingTest, BooleanLiterals) {
  // Test true literal
  auto true_tokens = create_tokens({{TokenType::True, "true"}});
  auto true_result = safe_parse(true_tokens);
  EXPECT_NE(true_result, nullptr);

  // Test false literal
  auto false_tokens = create_tokens({{TokenType::False, "false"}});
  auto false_result = safe_parse(false_tokens);
  EXPECT_NE(false_result, nullptr);
}

// ==================== Null Literal Tests ====================

TEST_F(LiteralParsingTest, NullLiteral) {
  auto tokens = create_tokens({{TokenType::Null, "null"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Array Literal Tests ====================

TEST_F(LiteralParsingTest, EmptyArray) {
  auto tokens = create_tokens(
      {{TokenType::LeftBracket, "["}, {TokenType::RightBracket, "]"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(LiteralParsingTest, ArrayWithSingleElement) {
  auto tokens = create_tokens({{TokenType::LeftBracket, "["},
                               {TokenType::Number, "42"},
                               {TokenType::RightBracket, "]"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(LiteralParsingTest, ArrayWithMultipleElements) {
  auto tokens = create_tokens({{TokenType::LeftBracket, "["},
                               {TokenType::Number, "1"},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "2"},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "3"},
                               {TokenType::RightBracket, "]"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(LiteralParsingTest, ArrayWithMixedTypes) {
  auto tokens = create_tokens({{TokenType::LeftBracket, "["},
                               {TokenType::Number, "42"},
                               {TokenType::Comma, ","},
                               {TokenType::String, "\"hello\""},
                               {TokenType::Comma, ","},
                               {TokenType::True, "true"},
                               {TokenType::Comma, ","},
                               {TokenType::Null, "null"},
                               {TokenType::RightBracket, "]"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(LiteralParsingTest, NestedArrays) {
  auto tokens = create_tokens({{TokenType::LeftBracket, "["},
                               {TokenType::LeftBracket, "["},
                               {TokenType::Number, "1"},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "2"},
                               {TokenType::RightBracket, "]"},
                               {TokenType::Comma, ","},
                               {TokenType::LeftBracket, "["},
                               {TokenType::Number, "3"},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "4"},
                               {TokenType::RightBracket, "]"},
                               {TokenType::RightBracket, "]"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Object Literal Tests ====================

TEST_F(LiteralParsingTest, EmptyObject) {
  auto tokens = create_tokens(
      {{TokenType::LeftBrace, "{"}, {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(LiteralParsingTest, ObjectWithSingleProperty) {
  auto tokens = create_tokens({{TokenType::LeftBrace, "{"},
                               {TokenType::Identifier, "key"},
                               {TokenType::Colon, ":"},
                               {TokenType::String, "\"value\""},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(LiteralParsingTest, ObjectWithMultipleProperties) {
  auto tokens = create_tokens({{TokenType::LeftBrace, "{"},
                               {TokenType::Identifier, "name"},
                               {TokenType::Colon, ":"},
                               {TokenType::String, "\"John\""},
                               {TokenType::Comma, ","},
                               {TokenType::Identifier, "age"},
                               {TokenType::Colon, ":"},
                               {TokenType::Number, "30"},
                               {TokenType::Comma, ","},
                               {TokenType::Identifier, "active"},
                               {TokenType::Colon, ":"},
                               {TokenType::True, "true"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(LiteralParsingTest, ObjectWithComputedProperty) {
  auto tokens = create_tokens({{TokenType::LeftBrace, "{"},
                               {TokenType::LeftBracket, "["},
                               {TokenType::String, "\"computed\""},
                               {TokenType::RightBracket, "]"},
                               {TokenType::Colon, ":"},
                               {TokenType::Number, "42"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(LiteralParsingTest, ObjectShorthandProperty) {
  auto tokens = create_tokens({{TokenType::LeftBrace, "{"},
                               {TokenType::Identifier, "variable"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(LiteralParsingTest, NestedObjects) {
  auto tokens = create_tokens({{TokenType::LeftBrace, "{"},
                               {TokenType::Identifier, "nested"},
                               {TokenType::Colon, ":"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Identifier, "inner"},
                               {TokenType::Colon, ":"},
                               {TokenType::String, "\"value\""},
                               {TokenType::RightBrace, "}"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Error Cases ====================

TEST_F(LiteralParsingTest, ArrayErrorCases) {
  // Missing closing bracket
  auto missing_bracket = create_tokens({
      {TokenType::LeftBracket, "["}, {TokenType::Number, "42"}
      // Missing RightBracket
  });
  Parser parser1(missing_bracket);
  EXPECT_THROW(parser1.parse(), Parser::ParseException);

  // Extra comma
  auto extra_comma = create_tokens({{TokenType::LeftBracket, "["},
                                    {TokenType::Number, "1"},
                                    {TokenType::Comma, ","},
                                    {TokenType::Comma, ","},
                                    {TokenType::Number, "2"},
                                    {TokenType::RightBracket, "]"}});
  Parser parser2(extra_comma);
  EXPECT_THROW(parser2.parse(), Parser::ParseException);
}

TEST_F(LiteralParsingTest, ObjectErrorCases) {
  // Missing value
  auto missing_value = create_tokens({{TokenType::LeftBrace, "{"},
                                      {TokenType::Identifier, "key"},
                                      {TokenType::Colon, ":"},
                                      {TokenType::RightBrace, "}"}});
  Parser parser1(missing_value);
  EXPECT_THROW(parser1.parse(), Parser::ParseException);

  // Missing colon
  auto missing_colon = create_tokens({{TokenType::LeftBrace, "{"},
                                      {TokenType::Identifier, "key"},
                                      {TokenType::String, "\"value\""},
                                      {TokenType::RightBrace, "}"}});
  Parser parser2(missing_colon);
  EXPECT_THROW(parser2.parse(), Parser::ParseException);
}

// ==================== Numbers with Underscores ====================

TEST_F(LiteralParsingTest, NumbersWithUnderscores) {
  std::vector<std::string> test_cases = {
      "1_000",      "1_000_000",       "42_000",
      "999_999",    "0b1010_1100",     "0b1111_0000_1010_1100",
      "0xFF_FF_FF", "0xDEAD_BEEF",     "0xFF_00_FF",
      "0o755_644",  "0o777_123",       "3.14_159",
      "6.022e23",   "6.626_070_15e-34"};

  for (const auto &input : test_cases) {
    auto tokens = create_tokens({{TokenType::Number, input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr)
        << "Failed to parse number with underscores: " << input;
  }
}

// ==================== Scientific Notation Tests ====================

TEST_F(LiteralParsingTest, ScientificNotation) {
  std::vector<std::string> test_cases = {"1e10",      "1.5e10",  "1.5E10",
                                         "1.5e+10",   "1.5e-10", "6.022e23",
                                         "6.626e-34", "2.998E8", "9.109e-31"};

  for (const auto &input : test_cases) {
    auto tokens = create_tokens({{TokenType::Number, input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr)
        << "Failed to parse scientific notation: " << input;
  }
}

// ==================== Character Literal Tests ====================

TEST_F(LiteralParsingTest, CharacterLiterals) {
  std::vector<std::string> test_cases = {
      "A",      "z",   "5",   " ",                          // Basic characters
      "\\n",    "\\t", "\\r", "\\\\", "\\'", "\\\"", "\\0", // Escape sequences
      "\\u03A9"                                             // Unicode (Omega)
  };

  for (const auto &input : test_cases) {
    auto tokens = create_tokens({{TokenType::Character, input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse character: " << input;
  }
}

// ==================== Raw String Tests ====================

TEST_F(LiteralParsingTest, RawStrings) {
  std::vector<std::string> test_cases = {"hello world", "C:\\\\Users\\\\nugdev",
                                         "\\d+\\.\\d+", "no\\nescapes\\there",
                                         ""};

  for (const auto &input : test_cases) {
    auto tokens = create_tokens({{TokenType::String, input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse raw string: " << input;
  }
}

// ==================== Template String Tests ====================

TEST_F(LiteralParsingTest, TemplateStrings) {
  // Simple template strings (without expressions for now)
  std::vector<std::string> test_cases = {"hello world", "simple template", ""};

  for (const auto &input : test_cases) {
    auto tokens = create_tokens({{TokenType::String, input}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse template string: " << input;
  }
}

// ==================== None Literal Tests ====================

TEST_F(LiteralParsingTest, NoneLiteral) {
  auto tokens = create_tokens({{TokenType::None, "None"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Range Literal Tests ====================

TEST_F(LiteralParsingTest, RangeLiterals) {
  // Test 1..10 (bounded range)
  auto bounded_range_tokens = create_tokens({{TokenType::Number, "1"},
                                             {TokenType::Range, ".."},
                                             {TokenType::Number, "10"}});
  auto bounded_result = safe_parse(bounded_range_tokens);
  EXPECT_NE(bounded_result, nullptr);

  // Test 10.. (unbounded range)
  auto unbounded_range_tokens =
      create_tokens({{TokenType::Number, "10"}, {TokenType::Range, ".."}});
  auto unbounded_result = safe_parse(unbounded_range_tokens);
  EXPECT_NE(unbounded_result, nullptr);

  // Test character range 'a'..'z'
  auto char_range_tokens = create_tokens({{TokenType::Character, "a"},
                                          {TokenType::Range, ".."},
                                          {TokenType::Character, "z"}});
  auto char_result = safe_parse(char_range_tokens);
  EXPECT_NE(char_result, nullptr);

  // Test identifier range x..y
  auto identifier_range_tokens = create_tokens({{TokenType::Identifier, "x"},
                                                {TokenType::Range, ".."},
                                                {TokenType::Identifier, "y"}});
  auto identifier_result = safe_parse(identifier_range_tokens);
  EXPECT_NE(identifier_result, nullptr);
}

// ==================== Complex Literal Combinations ====================

TEST_F(LiteralParsingTest, ArraysWithNewLiterals) {
  // Array with character literals
  auto char_array_tokens = create_tokens({{TokenType::LeftBracket, "["},
                                          {TokenType::Character, "H"},
                                          {TokenType::Comma, ","},
                                          {TokenType::Character, "i"},
                                          {TokenType::RightBracket, "]"}});
  auto char_array_result = safe_parse(char_array_tokens);
  EXPECT_NE(char_array_result, nullptr);

  // Array with None literals
  auto none_array_tokens = create_tokens({{TokenType::LeftBracket, "["},
                                          {TokenType::None, "None"},
                                          {TokenType::Comma, ","},
                                          {TokenType::None, "None"},
                                          {TokenType::RightBracket, "]"}});
  auto none_array_result = safe_parse(none_array_tokens);
  EXPECT_NE(none_array_result, nullptr);

  // Array with numbers with underscores
  auto underscore_array_tokens =
      create_tokens({{TokenType::LeftBracket, "["},
                     {TokenType::Number, "1_000"},
                     {TokenType::Comma, ","},
                     {TokenType::Number, "2_000"},
                     {TokenType::RightBracket, "]"}});
  auto underscore_array_result = safe_parse(underscore_array_tokens);
  EXPECT_NE(underscore_array_result, nullptr);
}

TEST_F(LiteralParsingTest, ObjectsWithNewLiterals) {
  // Object with character literal key/value
  auto char_object_tokens = create_tokens({{TokenType::LeftBrace, "{"},
                                           {TokenType::Character, "A"},
                                           {TokenType::Colon, ":"},
                                           {TokenType::Character, "B"},
                                           {TokenType::RightBrace, "}"}});
  auto char_object_result = safe_parse(char_object_tokens);
  EXPECT_NE(char_object_result, nullptr);

  // Object with None values
  auto none_object_tokens = create_tokens({{TokenType::LeftBrace, "{"},
                                           {TokenType::String, "\"key\""},
                                           {TokenType::Colon, ":"},
                                           {TokenType::None, "None"},
                                           {TokenType::RightBrace, "}"}});
  auto none_object_result = safe_parse(none_object_tokens);
  EXPECT_NE(none_object_result, nullptr);
}

} // namespace nugdev::test