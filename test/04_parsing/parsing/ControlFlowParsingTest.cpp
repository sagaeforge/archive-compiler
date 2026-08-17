#include "03_tokenize/Token.h"
#include "03_tokenize/TokenType.h"
#include "04_parsing/Parser.hpp"
#include <chrono>
#include <gtest/gtest.h>

using namespace nugdev::compiler::parsing;
using namespace nugdev::compiler::tokenize;
using namespace nugdev::ast;

namespace nugdev::test {

/**
 * @brief Test class for control flow parsing functionality
 */
class ControlFlowParsingTest : public ::testing::Test {
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

  void expect_parse_failure(const std::vector<Token> &tokens) {
    Parser parser(tokens);
    EXPECT_THROW(parser.parse(), Parser::ParseException);
  }
};

// ==================== If Statement Tests ====================

TEST_F(ControlFlowParsingTest, SimpleIfStatement) {
  // Test: if (condition) { statement; }
  auto tokens = create_tokens({{TokenType::If, "if"},
                               {TokenType::LeftParen, "("},
                               {TokenType::True, "true"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "42"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, IfStatementWithoutParentheses) {
  // Test: if condition { statement; }
  auto tokens = create_tokens({{TokenType::If, "if"},
                               {TokenType::True, "true"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "42"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, IfElseStatement) {
  // Test: if (condition) { ... } else { ... }
  auto tokens = create_tokens({{TokenType::If, "if"},
                               {TokenType::LeftParen, "("},
                               {TokenType::True, "true"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "1"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::Else, "else"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "2"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, IfElifElseStatement) {
  // Test: if (a) { ... } elif (b) { ... } else { ... }
  auto tokens = create_tokens({{TokenType::If, "if"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "a"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "1"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::Elif, "elif"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "b"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "2"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::Else, "else"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "3"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, NestedIfStatements) {
  // Test nested if statements
  auto tokens = create_tokens({{TokenType::If, "if"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "a"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::If, "if"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "b"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "42"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, IfWithComplexCondition) {
  // Test: if (x > 0 and y < 10) { ... }
  auto tokens = create_tokens({{TokenType::If, "if"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "x"},
                               {TokenType::GreaterThan, ">"},
                               {TokenType::Number, "0"},
                               {TokenType::LogicalAnd, "and"},
                               {TokenType::Identifier, "y"},
                               {TokenType::LessThan, "<"},
                               {TokenType::Number, "10"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::String, "\"valid\""},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== For Statement Tests ====================

TEST_F(ControlFlowParsingTest, InfiniteForLoop) {
  // Test: for { ... }
  auto tokens = create_tokens({{TokenType::For, "for"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "42"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, WhileStyleForLoop) {
  // Test: for (condition) { ... }
  auto tokens = create_tokens({{TokenType::For, "for"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "running"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Identifier, "doWork"},
                               {TokenType::LeftParen, "("},
                               {TokenType::RightParen, ")"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, ForInLoop) {
  // Test: for (item in collection) { ... }
  auto tokens = create_tokens({{TokenType::For, "for"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "item"},
                               {TokenType::In, "in"},
                               {TokenType::Identifier, "collection"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Identifier, "process"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "item"},
                               {TokenType::RightParen, ")"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, CStyleForLoop) {
  // Test: for (let i: int = 0; i < 10; i++) { ... }
  auto tokens = create_tokens(
      {{TokenType::For, "for"},      {TokenType::LeftParen, "("},
       {TokenType::Let, "let"},      {TokenType::Identifier, "i"},
       {TokenType::Colon, ":"},      {TokenType::Identifier, "int"},
       {TokenType::Assign, "="},     {TokenType::Number, "0"},
       {TokenType::Semicolon, ";"},  {TokenType::Identifier, "i"},
       {TokenType::LessThan, "<"},   {TokenType::Number, "10"},
       {TokenType::Semicolon, ";"},  {TokenType::Identifier, "i"},
       {TokenType::Increment, "++"}, {TokenType::RightParen, ")"},
       {TokenType::LeftBrace, "{"},  {TokenType::Identifier, "print"},
       {TokenType::LeftParen, "("},  {TokenType::Identifier, "i"},
       {TokenType::RightParen, ")"}, {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Break and Continue Tests ====================

TEST_F(ControlFlowParsingTest, BreakStatement) {
  // Test: break;
  auto tokens =
      create_tokens({{TokenType::Break, "break"}, {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, BreakWithLabel) {
  // Test: break @outer;
  auto tokens = create_tokens({{TokenType::Break, "break"},
                               {TokenType::At, "@"},
                               {TokenType::Identifier, "outer"},
                               {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, ContinueStatement) {
  // Test: continue;
  auto tokens = create_tokens(
      {{TokenType::Continue, "continue"}, {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, ContinueWithLabel) {
  // Test: continue @loop;
  auto tokens = create_tokens({{TokenType::Continue, "continue"},
                               {TokenType::At, "@"},
                               {TokenType::Identifier, "loop"},
                               {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Return Statement Tests ====================

TEST_F(ControlFlowParsingTest, ReturnWithoutValue) {
  // Test: return;
  auto tokens = create_tokens(
      {{TokenType::Return, "return"}, {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, ReturnWithValue) {
  // Test: return 42;
  auto tokens = create_tokens({{TokenType::Return, "return"},
                               {TokenType::Number, "42"},
                               {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, ReturnWithExpression) {
  // Test: return x + y * 2;
  auto tokens = create_tokens({{TokenType::Return, "return"},
                               {TokenType::Identifier, "x"},
                               {TokenType::Plus, "+"},
                               {TokenType::Identifier, "y"},
                               {TokenType::Asterisk, "*"},
                               {TokenType::Number, "2"},
                               {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, ReturnWithLabel) {
  // Test: return @function_name value;
  auto tokens = create_tokens({{TokenType::Return, "return"},
                               {TokenType::At, "@"},
                               {TokenType::Identifier, "function_name"},
                               {TokenType::Identifier, "value"},
                               {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== When Expression Tests ====================

TEST_F(ControlFlowParsingTest, SimpleWhenExpression) {
  // Test: when (value) { 1 -> "one", 2 -> "two", else -> "other" }
  auto tokens = create_tokens({{TokenType::When, "when"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "value"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "1"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"one\""},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "2"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"two\""},
                               {TokenType::Comma, ","},
                               {TokenType::Else, "else"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"other\""},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, WhenWithComplexExpressions) {
  // Test when with complex conditions and expressions
  auto tokens = create_tokens({{TokenType::When, "when"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "x"},
                               {TokenType::Plus, "+"},
                               {TokenType::Identifier, "y"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "0"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"zero\""},
                               {TokenType::Comma, ","},
                               {TokenType::Else, "else"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"nonzero\""},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, WhenWithTypeConditions) {
  // Test: when (obj) { value is string -> ..., value is int -> ... }
  auto tokens = create_tokens({{TokenType::When, "when"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "obj"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Identifier, "value"},
                               {TokenType::Is, "is"},
                               {TokenType::Identifier, "string"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"string_value\""},
                               {TokenType::Comma, ","},
                               {TokenType::Identifier, "value"},
                               {TokenType::Is, "is"},
                               {TokenType::Identifier, "int"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"int_value\""},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, WhenWithRangeConditions) {
  // Test: when (num) { value in 1..10 -> "small", value in 11..100 -> "medium"
  // }
  auto tokens = create_tokens(
      {{TokenType::When, "when"},      {TokenType::LeftParen, "("},
       {TokenType::Identifier, "num"}, {TokenType::RightParen, ")"},
       {TokenType::LeftBrace, "{"},    {TokenType::Identifier, "value"},
       {TokenType::In, "in"},          {TokenType::Number, "1"},
       {TokenType::Range, ".."},       {TokenType::Number, "10"},
       {TokenType::Arrow, "->"},       {TokenType::String, "\"small\""},
       {TokenType::Comma, ","},        {TokenType::Identifier, "value"},
       {TokenType::In, "in"},          {TokenType::Number, "11"},
       {TokenType::Range, ".."},       {TokenType::Number, "100"},
       {TokenType::Arrow, "->"},       {TokenType::String, "\"medium\""},
       {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Block and If Expression Tests ====================

TEST_F(ControlFlowParsingTest, SimpleBlockExpression) {
  // Simple block expression as statement: { 42 }
  auto tokens = create_tokens({{TokenType::LeftBrace, "{"},
                               {TokenType::Number, "42"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, BlockWithStatements) {
  // Block with multiple statements: { let x = 1; x + 2 }
  auto tokens = create_tokens({{TokenType::LeftBrace, "{"},
                               {TokenType::Let, "let"},
                               {TokenType::Identifier, "x"},
                               {TokenType::Assign, "="},
                               {TokenType::Number, "1"},
                               {TokenType::Semicolon, ";"},
                               {TokenType::Identifier, "x"},
                               {TokenType::Plus, "+"},
                               {TokenType::Number, "2"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, IfExpression) {
  // Test if expression: if (condition) { value1 } else { value2 }
  auto tokens = create_tokens({{TokenType::If, "if"},
                               {TokenType::LeftParen, "("},
                               {TokenType::True, "true"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "1"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::Else, "else"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "2"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, NestedIfExpression) {
  // Test nested if expressions
  auto tokens = create_tokens({{TokenType::If, "if"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "a"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::If, "if"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "b"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "1"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::Else, "else"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "2"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Complex Control Flow Tests ====================

TEST_F(ControlFlowParsingTest, NestedLoopsWithBreakContinue) {
  // Test nested loops with labeled break/continue
  auto tokens = create_tokens({{TokenType::For, "for"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "i"},
                               {TokenType::In, "in"},
                               {TokenType::Identifier, "range1"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::For, "for"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "j"},
                               {TokenType::In, "in"},
                               {TokenType::Identifier, "range2"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::If, "if"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "shouldBreak"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Break, "break"},
                               {TokenType::At, "@"},
                               {TokenType::Identifier, "outer"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Error Cases ====================

TEST_F(ControlFlowParsingTest, IfStatementErrors) {
  // Missing condition
  expect_parse_failure(create_tokens({{TokenType::If, "if"},
                                      {TokenType::LeftBrace, "{"},
                                      {TokenType::Number, "42"},
                                      {TokenType::RightBrace, "}"}}));

  // Missing body
  expect_parse_failure(create_tokens({{TokenType::If, "if"},
                                      {TokenType::LeftParen, "("},
                                      {TokenType::True, "true"},
                                      {TokenType::RightParen, ")"}}));

  // Unmatched braces
  expect_parse_failure(create_tokens({{TokenType::If, "if"},
                                      {TokenType::LeftParen, "("},
                                      {TokenType::True, "true"},
                                      {TokenType::RightParen, ")"},
                                      {TokenType::LeftBrace, "{"},
                                      {TokenType::Number, "42"}}));
}

TEST_F(ControlFlowParsingTest, ForStatementErrors) {
  // Invalid for syntax
  expect_parse_failure(create_tokens({{TokenType::For, "for"},
                                      {TokenType::LeftParen, "("},
                                      {TokenType::Number, "42"},
                                      {TokenType::RightParen, ")"}}));

  // Missing body
  expect_parse_failure(create_tokens({{TokenType::For, "for"},
                                      {TokenType::LeftParen, "("},
                                      {TokenType::Identifier, "condition"},
                                      {TokenType::RightParen, ")"}}));
}

TEST_F(ControlFlowParsingTest, WhenExpressionErrors) {
  // Missing condition
  expect_parse_failure(create_tokens({{TokenType::When, "when"},
                                      {TokenType::LeftBrace, "{"},
                                      {TokenType::Number, "1"},
                                      {TokenType::Arrow, "->"},
                                      {TokenType::String, "\"one\""},
                                      {TokenType::RightBrace, "}"}}));

  // Missing arrow
  expect_parse_failure(create_tokens({{TokenType::When, "when"},
                                      {TokenType::LeftParen, "("},
                                      {TokenType::Identifier, "value"},
                                      {TokenType::RightParen, ")"},
                                      {TokenType::LeftBrace, "{"},
                                      {TokenType::Number, "1"},
                                      {TokenType::String, "\"one\""},
                                      {TokenType::RightBrace, "}"}}));
}

// ==================== Grammar-Complete When Expression Tests
// ====================

TEST_F(ControlFlowParsingTest, WhenValueConditions) {
  // Test simple value conditions from grammar
  auto tokens = create_tokens({{TokenType::When, "when"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "x"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "1"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"one\""},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "2"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"two\""},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "3"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"three\""},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, WhenGuardConditions) {
  // Test guard conditions: value_condition if expression
  auto tokens = create_tokens({{TokenType::When, "when"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "x"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "1"},
                               {TokenType::If, "if"},
                               {TokenType::Identifier, "condition1"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"one with guard\""},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "2"},
                               {TokenType::If, "if"},
                               {TokenType::Identifier, "condition2"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"two with guard\""},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, WhenMultipleConditions) {
  // Test multiple conditions: value_condition, value_condition, ...
  auto tokens = create_tokens({{TokenType::When, "when"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "x"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Number, "1"},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "2"},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "3"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"small numbers\""},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "10"},
                               {TokenType::Comma, ","},
                               {TokenType::Number, "20"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"big numbers\""},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, WhenComplexGuardWithMultiple) {
  // Test complex guard with multiple conditions
  auto tokens = create_tokens({{TokenType::When, "when"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "status"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::String, "\"active\""},
                               {TokenType::Comma, ","},
                               {TokenType::String, "\"running\""},
                               {TokenType::If, "if"},
                               {TokenType::Identifier, "hasPermission"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"allowed\""},
                               {TokenType::Comma, ","},
                               {TokenType::String, "\"inactive\""},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"denied\""},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== When Expression vs Statement Distinction
// ====================

TEST_F(ControlFlowParsingTest, WhenAsStatement) {
  // when used as statement (no return value)
  auto tokens = create_tokens({{TokenType::When, "when"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "command"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::String, "\"save\""},
                               {TokenType::Arrow, "->"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Identifier, "save"},
                               {TokenType::LeftParen, "("},
                               {TokenType::RightParen, ")"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::Comma, ","},
                               {TokenType::String, "\"load\""},
                               {TokenType::Arrow, "->"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Identifier, "load"},
                               {TokenType::LeftParen, "("},
                               {TokenType::RightParen, ")"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, WhenAsExpression) {
  // when used as expression (returns value)
  auto tokens = create_tokens(
      {{TokenType::Let, "let"},      {TokenType::Identifier, "result"},
       {TokenType::Assign, "="},     {TokenType::When, "when"},
       {TokenType::LeftParen, "("},  {TokenType::Identifier, "grade"},
       {TokenType::RightParen, ")"}, {TokenType::LeftBrace, "{"},
       {TokenType::Number, "90"},    {TokenType::Range, ".."},
       {TokenType::Number, "100"},   {TokenType::Arrow, "->"},
       {TokenType::String, "\"A\""}, {TokenType::Comma, ","},
       {TokenType::Number, "80"},    {TokenType::Range, ".."},
       {TokenType::Number, "89"},    {TokenType::Arrow, "->"},
       {TokenType::String, "\"B\""}, {TokenType::Comma, ","},
       {TokenType::Else, "else"},    {TokenType::Arrow, "->"},
       {TokenType::String, "\"F\""}, {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Enhanced Range and Type Tests ====================

TEST_F(ControlFlowParsingTest, WhenWithCharacterRanges) {
  // Test character ranges: 'a'..'z'
  auto tokens = create_tokens(
      {{TokenType::When, "when"},       {TokenType::LeftParen, "("},
       {TokenType::Identifier, "char"}, {TokenType::RightParen, ")"},
       {TokenType::LeftBrace, "{"},     {TokenType::Identifier, "value"},
       {TokenType::In, "in"},           {TokenType::Character, "a"},
       {TokenType::Range, ".."},        {TokenType::Character, "z"},
       {TokenType::Arrow, "->"},        {TokenType::String, "\"lowercase\""},
       {TokenType::Comma, ","},         {TokenType::Identifier, "value"},
       {TokenType::In, "in"},           {TokenType::Character, "A"},
       {TokenType::Range, ".."},        {TokenType::Character, "Z"},
       {TokenType::Arrow, "->"},        {TokenType::String, "\"uppercase\""},
       {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ControlFlowParsingTest, WhenWithComplexTypeConditions) {
  // Test complex type conditions
  auto tokens = create_tokens({{TokenType::When, "when"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Identifier, "obj"},
                               {TokenType::RightParen, ")"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Identifier, "value"},
                               {TokenType::Is, "is"},
                               {TokenType::Identifier, "String"},
                               {TokenType::If, "if"},
                               {TokenType::Identifier, "value"},
                               {TokenType::Dot, "."},
                               {TokenType::Identifier, "length"},
                               {TokenType::GreaterThan, ">"},
                               {TokenType::Number, "0"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"non-empty string\""},
                               {TokenType::Comma, ","},
                               {TokenType::Identifier, "value"},
                               {TokenType::Is, "is"},
                               {TokenType::Identifier, "Number"},
                               {TokenType::Arrow, "->"},
                               {TokenType::String, "\"number\""},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== When Error Cases ====================

TEST_F(ControlFlowParsingTest, WhenComprehensiveErrorCases) {
  // Missing arrow after condition
  auto missing_arrow = create_tokens({{TokenType::When, "when"},
                                      {TokenType::LeftParen, "("},
                                      {TokenType::Identifier, "x"},
                                      {TokenType::RightParen, ")"},
                                      {TokenType::LeftBrace, "{"},
                                      {TokenType::Number, "1"},
                                      {TokenType::String, "\"one\""},
                                      {TokenType::RightBrace, "}"}});
  expect_parse_failure(missing_arrow);

  // Missing expression/statement after arrow
  auto missing_result = create_tokens({{TokenType::When, "when"},
                                       {TokenType::LeftParen, "("},
                                       {TokenType::Identifier, "x"},
                                       {TokenType::RightParen, ")"},
                                       {TokenType::LeftBrace, "{"},
                                       {TokenType::Number, "1"},
                                       {TokenType::Arrow, "->"},
                                       {TokenType::RightBrace, "}"}});
  expect_parse_failure(missing_result);

  // Invalid guard condition syntax
  auto invalid_guard = create_tokens({{TokenType::When, "when"},
                                      {TokenType::LeftParen, "("},
                                      {TokenType::Identifier, "x"},
                                      {TokenType::RightParen, ")"},
                                      {TokenType::LeftBrace, "{"},
                                      {TokenType::Number, "1"},
                                      {TokenType::If, "if"},
                                      {TokenType::Arrow, "->"},
                                      {TokenType::String, "\"invalid\""},
                                      {TokenType::RightBrace, "}"}});
  expect_parse_failure(invalid_guard);
}

} // namespace nugdev::test