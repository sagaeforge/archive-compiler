#include "03_tokenize/Token.h"
#include "03_tokenize/TokenType.h"
#include "04_parsing/Parser.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace nugdev::compiler::parsing;
using namespace nugdev::compiler::tokenize;
using namespace nugdev::ast;

namespace nugdev::test {

/**
 * @brief Base test class for Parser tests with common utilities
 */
class ParserBaseTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Initialize any common test setup here
  }

  void TearDown() override {
    // Clean up any common test resources here
  }

  /**
   * @brief Helper function to create tokens from specifications
   */
  std::vector<Token> create_tokens(
      const std::vector<std::pair<TokenType, std::string>> &token_specs) {
    std::vector<Token> tokens;
    for (const auto &spec : token_specs) {
      tokens.emplace_back(spec.first, nugdev::lib::String(spec.second));
    }
    tokens.emplace_back(TokenType::EOF_TOKEN, nugdev::lib::String(""));
    return tokens;
  }

  /**
   * @brief Helper function to create a simple number token
   */
  std::vector<Token> create_number_tokens(const std::string &value) {
    return create_tokens({{TokenType::Number, value}});
  }

  /**
   * @brief Helper function to create a simple identifier token
   */
  std::vector<Token> create_identifier_tokens(const std::string &name) {
    return create_tokens({{TokenType::Identifier, name}});
  }

  /**
   * @brief Helper function to create a simple string token
   */
  std::vector<Token> create_string_tokens(const std::string &value) {
    return create_tokens({{TokenType::String, "\"" + value + "\""}});
  }

  /**
   * @brief Helper function to safely parse and catch exceptions
   */
  std::unique_ptr<Program> safe_parse(const std::vector<Token> &tokens) {
    try {
      Parser parser(tokens);
      return parser.parse();
    } catch (const Parser::ParseException &e) {
      ADD_FAILURE() << "Parse error: " << e.what();
      return nullptr;
    }
  }

  /**
   * @brief Helper function to expect parse failure
   */
  void expect_parse_failure(const std::vector<Token> &tokens) {
    Parser parser(tokens);
    EXPECT_THROW(parser.parse(), Parser::ParseException);
  }

  /**
   * @brief Helper function to check if parsing was successful
   */
  void expect_parse_success(const std::vector<Token> &tokens) {
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr);
  }
};

// ==================== Basic Parser Functionality Tests ====================

/**
 * @brief Test basic parser construction and initialization
 */
TEST_F(ParserBaseTest, ParserConstruction) {
  auto tokens = create_number_tokens("42");
  Parser parser(tokens);

  // Parser should be constructed successfully
  EXPECT_NO_THROW({
    auto result = parser.parse();
    EXPECT_NE(result, nullptr);
  });
}

/**
 * @brief Test empty token sequence parsing
 */
TEST_F(ParserBaseTest, EmptyTokenSequence) {
  std::vector<Token> tokens = {
      Token(TokenType::EOF_TOKEN, nugdev::lib::String(""))};
  Parser parser(tokens);

  auto result = parser.parse();
  EXPECT_NE(result, nullptr);
  // Empty program should have zero modules
  EXPECT_EQ(result->get_modules().size(), 0);
}

/**
 * @brief Test single token parsing
 */
TEST_F(ParserBaseTest, SingleTokenParsing) {
  // Test single number
  expect_parse_success(create_number_tokens("42"));

  // Test single identifier
  expect_parse_success(create_identifier_tokens("variable"));

  // Test single string
  expect_parse_success(create_string_tokens("hello"));

  // Test single boolean
  expect_parse_success(create_tokens({{TokenType::True, "true"}}));
  expect_parse_success(create_tokens({{TokenType::False, "false"}}));

  // Test null literal
  expect_parse_success(create_tokens({{TokenType::Null, "null"}}));
}

/**
 * @brief Test basic error recovery and exception handling
 */
TEST_F(ParserBaseTest, BasicErrorHandling) {
  // Test incomplete expression
  auto incomplete_tokens = create_tokens({
      {TokenType::Number, "1"}, {TokenType::Plus, "+"} // Missing right operand
  });
  expect_parse_failure(incomplete_tokens);

  // Test unmatched parentheses
  auto unmatched_tokens = create_tokens({
      {TokenType::LeftParen, "("}, {TokenType::Number, "42"}
      // Missing closing parenthesis
  });
  expect_parse_failure(unmatched_tokens);

  // Test invalid token sequence
  auto invalid_tokens = create_tokens({{TokenType::Plus, "+"},
                                       {TokenType::Asterisk, "*"},
                                       {TokenType::Number, "42"}});
  expect_parse_failure(invalid_tokens);
}

/**
 * @brief Test parser with various token sequences
 */
TEST_F(ParserBaseTest, TokenSequenceHandling) {
  // Test multiple expressions
  auto multi_expr_tokens = create_tokens({{TokenType::Number, "1"},
                                          {TokenType::Semicolon, ";"},
                                          {TokenType::Number, "2"},
                                          {TokenType::Semicolon, ";"},
                                          {TokenType::Number, "3"}});
  expect_parse_success(multi_expr_tokens);

  // Test mixed expression types
  auto mixed_tokens = create_tokens({{TokenType::True, "true"},
                                     {TokenType::Semicolon, ";"},
                                     {TokenType::String, "\"hello\""},
                                     {TokenType::Semicolon, ";"},
                                     {TokenType::Number, "42"}});
  expect_parse_success(mixed_tokens);
}

/**
 * @brief Test parser performance with reasonable token count
 */
TEST_F(ParserBaseTest, ReasonablePerformance) {
  // Create a moderate-sized token sequence
  std::vector<std::pair<TokenType, std::string>> token_specs;
  for (int i = 0; i < 100; ++i) {
    token_specs.push_back({TokenType::Number, std::to_string(i)});
    if (i < 99) {
      token_specs.push_back({TokenType::Semicolon, ";"});
    }
  }

  auto tokens = create_tokens(token_specs);

  auto start = std::chrono::high_resolution_clock::now();
  expect_parse_success(tokens);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  EXPECT_LT(duration.count(), 1000)
      << "Parsing took too long: " << duration.count() << "ms";
}

// ==================== Label Parsing Tests ====================

TEST_F(ParserBaseTest, LabelParsing) {
  // Test break @loop
  auto break_with_label_tokens =
      create_tokens({{TokenType::Break, "break"},
                     {TokenType::At, "@"},
                     {TokenType::Identifier, "loop"}});

  auto result = safe_parse(break_with_label_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(ParserBaseTest, ContinueWithLabel) {
  // Test continue @outer
  auto continue_with_label_tokens =
      create_tokens({{TokenType::Continue, "continue"},
                     {TokenType::At, "@"},
                     {TokenType::Identifier, "outer"}});

  auto result = safe_parse(continue_with_label_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(ParserBaseTest, ReturnWithLabel) {
  // Test return @function 42
  auto return_with_label_tokens =
      create_tokens({{TokenType::Return, "return"},
                     {TokenType::At, "@"},
                     {TokenType::Identifier, "function"},
                     {TokenType::Number, "42"}});

  auto result = safe_parse(return_with_label_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Error Recovery Tests ====================

TEST_F(ParserBaseTest, ErrorRecoveryAfterInvalidStatement) {
  // Test error recovery: invalid statement followed by valid statement
  auto error_recovery_tokens = create_tokens(
      {{TokenType::Plus, "+"},      // Invalid statement start (operator)
       {TokenType::Semicolon, ";"}, // Should help synchronize
       {TokenType::Let, "let"},     // Valid statement
       {TokenType::Identifier, "x"},
       {TokenType::Assign, "="},
       {TokenType::Number, "42"}});

  // This should throw an exception because '+' cannot start a statement
  expect_parse_failure(error_recovery_tokens);
}

TEST_F(ParserBaseTest, ErrorRecoveryAfterInvalidExpression) {
  // Test error recovery in expression parsing
  auto invalid_expr_tokens = create_tokens(
      {{TokenType::Let, "let"},
       {TokenType::Identifier, "x"},
       {TokenType::Assign, "="},
       {TokenType::Plus, "+"}, // Invalid: operator without operand
       {TokenType::Semicolon, ";"}});

  expect_parse_failure(invalid_expr_tokens);
}

// ==================== Edge Cases ====================

TEST_F(ParserBaseTest, SingleIdentifierExpression) {
  // Test single identifier as expression statement
  auto single_id_tokens = create_tokens({{TokenType::Identifier, "variable"}});

  auto result = safe_parse(single_id_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(ParserBaseTest, MultipleStatementsWithSemicolons) {
  // Test multiple statements with semicolons
  auto multi_stmt_tokens = create_tokens({{TokenType::Let, "let"},
                                          {TokenType::Identifier, "a"},
                                          {TokenType::Assign, "="},
                                          {TokenType::Number, "1"},
                                          {TokenType::Semicolon, ";"},
                                          {TokenType::Let, "let"},
                                          {TokenType::Identifier, "b"},
                                          {TokenType::Assign, "="},
                                          {TokenType::Number, "2"},
                                          {TokenType::Semicolon, ";"},
                                          {TokenType::Identifier, "a"},
                                          {TokenType::Plus, "+"},
                                          {TokenType::Identifier, "b"}});

  auto result = safe_parse(multi_stmt_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Operator Conversion Tests ====================

TEST_F(ParserBaseTest, AllBinaryOperators) {
  // Test all binary operators to ensure token_to_binary_operator coverage
  std::vector<std::pair<TokenType, std::string>> binary_ops = {
      {TokenType::Plus, "+"},
      {TokenType::Minus, "-"},
      {TokenType::Asterisk, "*"},
      {TokenType::Slash, "/"},
      {TokenType::Percent, "%"},
      {TokenType::Equal, "=="},
      {TokenType::NotEqual, "!="},
      {TokenType::LessThan, "<"},
      {TokenType::GreaterThan, ">"},
      {TokenType::LessThanEqual, "<="},
      {TokenType::GreaterThanEqual, ">="},
      {TokenType::LogicalAnd, "and"},
      {TokenType::LogicalOr, "or"},
      {TokenType::Ampersand, "&"},
      {TokenType::Pipe, "|"},
      {TokenType::Caret, "^"},
      {TokenType::BitwiseShiftLeft, "<<"},
      {TokenType::BitwiseShiftRight, ">>"},
      {TokenType::Range, ".."}};

  for (const auto &op : binary_ops) {
    auto tokens =
        create_tokens({{TokenType::Number, "1"}, op, {TokenType::Number, "2"}});

    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr)
        << "Failed to parse binary operator: " << op.second;
  }
}

TEST_F(ParserBaseTest, AllUnaryOperators) {
  // Test all unary operators to ensure token_to_unary_operator coverage
  std::vector<std::pair<TokenType, std::string>> unary_ops = {
      {TokenType::Plus, "+"},        {TokenType::Minus, "-"},
      {TokenType::Exclamation, "!"}, {TokenType::LogicalNot, "not"},
      {TokenType::Tilde, "~"},       {TokenType::Increment, "++"},
      {TokenType::Decrement, "--"}};

  for (const auto &op : unary_ops) {
    auto tokens = create_tokens({op, {TokenType::Number, "42"}});

    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr)
        << "Failed to parse unary operator: " << op.second;
  }
}

TEST_F(ParserBaseTest, AllAssignmentOperators) {
  // Test all assignment operators to ensure token_to_assignment_operator
  // coverage
  std::vector<std::pair<TokenType, std::string>> assignment_ops = {
      {TokenType::Assign, "="},           {TokenType::PlusAssign, "+="},
      {TokenType::MinusAssign, "-="},     {TokenType::AsteriskAssign, "*="},
      {TokenType::SlashAssign, "/="},     {TokenType::PercentAssign, "%="},
      {TokenType::AmpersandAssign, "&="}, {TokenType::PipeAssign, "|="},
      {TokenType::CaretAssign, "^="}};

  for (const auto &op : assignment_ops) {
    auto tokens = create_tokens(
        {{TokenType::Identifier, "x"}, op, {TokenType::Number, "42"}});

    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr)
        << "Failed to parse assignment operator: " << op.second;
  }
}

} // namespace nugdev::test