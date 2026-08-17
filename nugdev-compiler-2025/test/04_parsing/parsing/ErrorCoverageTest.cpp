#include "03_tokenize/Token.h"
#include "03_tokenize/TokenType.h"
#include "04_parsing/Parser.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace nugdev::compiler::parsing;
using namespace nugdev::compiler::tokenize;
using namespace nugdev::ast;

namespace nugdev::test {

/**
 * @brief Comprehensive error coverage test class for Parser
 */
class ErrorCoverageTest : public ::testing::Test {
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

  void expect_parse_failure(const std::vector<Token> &tokens) {
    Parser parser(tokens);
    EXPECT_THROW(parser.parse(), Parser::ParseException);
  }
};

// ==================== Token Management Errors ====================

TEST_F(ErrorCoverageTest, ConsumeErrors) {
  // Test invalid statement start that should trigger error
  auto invalid_start_tokens = create_tokens({{TokenType::Plus, "+"}});
  Parser parser(invalid_start_tokens);

  // This should trigger an error when trying to parse '+' as a statement start
  EXPECT_THROW(parser.parse(), Parser::ParseException);
}

TEST_F(ErrorCoverageTest, UnexpectedEOF) {
  // Test unexpected EOF in various contexts
  std::vector<std::vector<std::pair<TokenType, std::string>>> test_cases = {
      // Incomplete variable declaration
      {{TokenType::Let, "let"}},

      // Incomplete function declaration
      {{TokenType::Function, "fun"}, {TokenType::Identifier, "test"}},

      // Incomplete expression
      {{TokenType::Number, "1"}, {TokenType::Plus, "+"}},

      // Incomplete array
      {{TokenType::LeftBracket, "["}},

      // Incomplete object
      {{TokenType::LeftBrace, "{"}},

      // Incomplete parentheses
      {{TokenType::LeftParen, "("}},

      // Incomplete function call
      {{TokenType::Identifier, "func"}, {TokenType::LeftParen, "("}},

      // Incomplete assignment
      {{TokenType::Identifier, "x"}, {TokenType::Assign, "="}},

      // Incomplete type annotation
      {{TokenType::Let, "let"},
       {TokenType::Identifier, "x"},
       {TokenType::Colon, ":"}}};

  for (const auto &test_case : test_cases) {
    expect_parse_failure(create_tokens(test_case));
  }
}

// ==================== Invalid Token Sequences ====================

TEST_F(ErrorCoverageTest, InvalidStartTokens) {
  // Test invalid tokens at start of statements
  std::vector<TokenType> invalid_start_tokens = {
      TokenType::Plus,         TokenType::Asterisk,    TokenType::Slash,
      TokenType::Percent,      TokenType::Equal,       TokenType::NotEqual,
      TokenType::LessThan,     TokenType::GreaterThan, TokenType::Colon,
      TokenType::Semicolon,    TokenType::Comma,       TokenType::RightParen,
      TokenType::RightBracket, TokenType::RightBrace};

  for (const auto &token_type : invalid_start_tokens) {
    auto tokens = create_tokens({{token_type, "invalid"}});
    expect_parse_failure(tokens);
  }
}

TEST_F(ErrorCoverageTest, InvalidOperatorSequences) {
  // Test invalid operator combinations that should definitely fail
  std::vector<std::vector<std::pair<TokenType, std::string>>>
      invalid_sequences = {
          // Tokens that absolutely cannot start statements
          {{TokenType::Semicolon, ";"}, {TokenType::Number, "42"}},
          {{TokenType::Comma, ","}, {TokenType::Number, "42"}},
          {{TokenType::RightParen, ")"}, {TokenType::Number, "42"}},
          {{TokenType::RightBracket, "]"}, {TokenType::Number, "42"}},
          {{TokenType::RightBrace, "}"}, {TokenType::Number, "42"}},

          // Missing operand (incomplete binary expressions at EOF)
          {{TokenType::Number, "1"}, {TokenType::Plus, "+"}},
          {{TokenType::Number, "1"}, {TokenType::Asterisk, "*"}},
          {{TokenType::Number, "1"}, {TokenType::Equal, "=="}},

          // Invalid tokens alone
          {{TokenType::Semicolon, ";"}},
          {{TokenType::Comma, ","}},
          {{TokenType::RightParen, ")"}},

          // Multiple invalid tokens
          {{TokenType::Semicolon, ";"}, {TokenType::Comma, ","}},
          {{TokenType::RightParen, ")"}, {TokenType::RightBracket, "]"}}};

  for (const auto &sequence : invalid_sequences) {
    expect_parse_failure(create_tokens(sequence));
  }
}

// ==================== Bracket/Brace Mismatches ====================

TEST_F(ErrorCoverageTest, BracketMismatches) {
  std::vector<std::vector<std::pair<TokenType, std::string>>> mismatch_cases = {
      // Array with wrong closing
      {{TokenType::LeftBracket, "["},
       {TokenType::Number, "1"},
       {TokenType::RightBrace, "}"}},

      // Object with wrong closing
      {{TokenType::LeftBrace, "{"},
       {TokenType::Identifier, "key"},
       {TokenType::RightBracket, "]"}},

      // Parentheses with wrong closing
      {{TokenType::LeftParen, "("},
       {TokenType::Number, "1"},
       {TokenType::RightBracket, "]"}},

      // Nested mismatches
      {{TokenType::LeftBracket, "["},
       {TokenType::LeftBrace, "{"},
       {TokenType::RightBracket, "]"}},

      // Extra closing brackets
      {{TokenType::Number, "42"}, {TokenType::RightBracket, "]"}},
      {{TokenType::Number, "42"}, {TokenType::RightBrace, "}"}},
      {{TokenType::Number, "42"}, {TokenType::RightParen, ")"}}};

  for (const auto &case_ : mismatch_cases) {
    expect_parse_failure(create_tokens(case_));
  }
}

// ==================== Type System Errors ====================

TEST_F(ErrorCoverageTest, TypeSystemErrors) {
  std::vector<std::vector<std::pair<TokenType, std::string>>> type_errors = {
      // Invalid type in variable declaration
      {{TokenType::Let, "let"},
       {TokenType::Identifier, "x"},
       {TokenType::Colon, ":"},
       {TokenType::String, "\"InvalidType\""}},

      // Missing type in function parameter
      {{TokenType::Function, "fun"},
       {TokenType::Identifier, "test"},
       {TokenType::LeftParen, "("},
       {TokenType::Let, "let"},
       {TokenType::Identifier, "param"},
       {TokenType::RightParen, ")"}},

      // Invalid function type syntax
      {{TokenType::Let, "let"},
       {TokenType::Identifier, "f"},
       {TokenType::Colon, ":"},
       {TokenType::LeftParen, "("},
       {TokenType::Arrow, "->"},
       {TokenType::Identifier, "Int"}},

      // Malformed optional type
      {{TokenType::Let, "let"},
       {TokenType::Identifier, "x"},
       {TokenType::Colon, ":"},
       {TokenType::Question, "?"},
       {TokenType::Identifier, "Int"}},

      // Invalid tuple type
      {{TokenType::Let, "let"},
       {TokenType::Identifier, "x"},
       {TokenType::Colon, ":"},
       {TokenType::LeftParen, "("},
       {TokenType::Comma, ","},
       {TokenType::RightParen, ")"}}};

  for (const auto &error_case : type_errors) {
    expect_parse_failure(create_tokens(error_case));
  }
}

// ==================== Control Flow Errors ====================

TEST_F(ErrorCoverageTest, ControlFlowErrors) {
  std::vector<std::vector<std::pair<TokenType, std::string>>>
      control_flow_errors = {
          // If with missing condition
          {{TokenType::If, "if"}, {TokenType::LeftBrace, "{"}},

          // For with invalid syntax
          {{TokenType::For, "for"}, {TokenType::LeftBrace, "{"}},

          // When with missing condition
          {{TokenType::When, "when"}, {TokenType::LeftBrace, "{"}},

          // Break with invalid token after
          {{TokenType::Break, "break"}, {TokenType::Plus, "+"}},

          // Continue with invalid token after
          {{TokenType::Continue, "continue"}, {TokenType::Asterisk, "*"}},

          // Return with invalid token after @
          {{TokenType::Return, "return"},
           {TokenType::At, "@"},
           {TokenType::Plus, "+"}}};

  for (const auto &error_case : control_flow_errors) {
    expect_parse_failure(create_tokens(error_case));
  }
}

// ==================== Function Definition Errors ====================

TEST_F(ErrorCoverageTest, FunctionDefinitionErrors) {
  std::vector<std::vector<std::pair<TokenType, std::string>>> function_errors =
      {// Function without name
       {{TokenType::Function, "fun"}, {TokenType::LeftParen, "("}},

       // Function without parameters closing
       {{TokenType::Function, "fun"},
        {TokenType::Identifier, "test"},
        {TokenType::LeftParen, "("},
        {TokenType::Let, "let"},
        {TokenType::Identifier, "x"}},

       // Function without return type
       {{TokenType::Function, "fun"},
        {TokenType::Identifier, "test"},
        {TokenType::LeftParen, "("},
        {TokenType::RightParen, ")"},
        {TokenType::LeftBrace, "{"}},

       // Function with invalid parameter
       {{TokenType::Function, "fun"},
        {TokenType::Identifier, "test"},
        {TokenType::LeftParen, "("},
        {TokenType::Number, "42"},
        {TokenType::RightParen, ")"}},

       // Function expression without parameters
       {{TokenType::Function, "fun"},
        {TokenType::Colon, ":"},
        {TokenType::Identifier, "Int"}},

       // Lambda without arrow
       {{TokenType::LeftParen, "("},
        {TokenType::Let, "let"},
        {TokenType::Identifier, "x"},
        {TokenType::Colon, ":"},
        {TokenType::Identifier, "Int"},
        {TokenType::RightParen, ")"},
        {TokenType::Number, "42"}}};

  for (const auto &error_case : function_errors) {
    expect_parse_failure(create_tokens(error_case));
  }
}

// ==================== Struct/Interface Errors ====================

TEST_F(ErrorCoverageTest, StructInterfaceErrors) {
  std::vector<std::vector<std::pair<TokenType, std::string>>> struct_errors = {
      // Struct without name
      {{TokenType::Struct, "struct"}, {TokenType::LeftBrace, "{"}},

      // Struct without closing brace
      {{TokenType::Struct, "struct"},
       {TokenType::Identifier, "Test"},
       {TokenType::LeftBrace, "{"},
       {TokenType::Identifier, "field"},
       {TokenType::Colon, ":"},
       {TokenType::Identifier, "Int"}},

      // Struct field without type
      {{TokenType::Struct, "struct"},
       {TokenType::Identifier, "Test"},
       {TokenType::LeftBrace, "{"},
       {TokenType::Identifier, "field"},
       {TokenType::RightBrace, "}"}},

      // Interface without name
      {{TokenType::Interface, "interface"}, {TokenType::LeftBrace, "{"}},

      // Interface member without type
      {{TokenType::Interface, "interface"},
       {TokenType::Identifier, "Test"},
       {TokenType::LeftBrace, "{"},
       {TokenType::Identifier, "member"},
       {TokenType::RightBrace, "}"}}};

  for (const auto &error_case : struct_errors) {
    expect_parse_failure(create_tokens(error_case));
  }
}

// ==================== Complex Nested Errors ====================

TEST_F(ErrorCoverageTest, ComplexNestedErrors) {
  // Test complex scenarios with multiple levels of nesting that could fail
  std::vector<std::vector<std::pair<TokenType, std::string>>> complex_errors = {
      // Nested function call with missing arguments
      {{TokenType::Identifier, "outer"},
       {TokenType::LeftParen, "("},
       {TokenType::Identifier, "inner"},
       {TokenType::LeftParen, "("},
       {TokenType::Comma, ","},
       {TokenType::RightParen, ")"},
       {TokenType::RightParen, ")"}},

      // Nested array with object error
      {{TokenType::LeftBracket, "["},
       {TokenType::LeftBrace, "{"},
       {TokenType::Identifier, "key"},
       {TokenType::LeftBracket, "["},
       {TokenType::Number, "1"},
       {TokenType::RightBrace, "}"}},

      // Complex expression with type error
      {{TokenType::Identifier, "obj"},
       {TokenType::Dot, "."},
       {TokenType::Identifier, "method"},
       {TokenType::LeftParen, "("},
       {TokenType::As, "as"},
       {TokenType::RightParen, ")"}},

      // Deeply nested control flow error
      {{TokenType::If, "if"},
       {TokenType::LeftParen, "("},
       {TokenType::True, "true"},
       {TokenType::RightParen, ")"},
       {TokenType::LeftBrace, "{"},
       {TokenType::For, "for"},
       {TokenType::LeftParen, "("},
       {TokenType::When, "when"}}};

  for (const auto &error_case : complex_errors) {
    expect_parse_failure(create_tokens(error_case));
  }
}

// ==================== Edge Case Identifier Errors ====================

TEST_F(ErrorCoverageTest, IdentifierErrors) {
  // Test cases where identifiers are expected but not found
  std::vector<std::vector<std::pair<TokenType, std::string>>>
      identifier_errors = {// Member access without member name
                           {{TokenType::Identifier, "obj"},
                            {TokenType::Dot, "."},
                            {TokenType::Number, "42"}},

                           // Label without identifier
                           {{TokenType::Break, "break"},
                            {TokenType::At, "@"},
                            {TokenType::Number, "123"}},

                           // Import as without identifier
                           {{TokenType::Import, "import"},
                            {TokenType::String, "\"module\""},
                            {TokenType::As, "as"},
                            {TokenType::String, "\"invalid\""}},

                           // Computed property without expression
                           {{TokenType::LeftBrace, "{"},
                            {TokenType::LeftBracket, "["},
                            {TokenType::RightBracket, "]"},
                            {TokenType::Colon, ":"},
                            {TokenType::Number, "42"}}};

  for (const auto &error_case : identifier_errors) {
    expect_parse_failure(create_tokens(error_case));
  }
}

} // namespace nugdev::test