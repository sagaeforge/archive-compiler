#include "03_tokenize/Token.h"
#include "03_tokenize/TokenType.h"
#include "04_parsing/Parser.hpp"
#include <gtest/gtest.h>

using namespace nugdev::compiler::parsing;
using namespace nugdev::compiler::tokenize;
using namespace nugdev::ast;

namespace nugdev::test {

/**
 * @brief Test class for statement parsing functionality
 */
class StatementParsingTest : public ::testing::Test {
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

// ==================== Variable Declaration Tests ====================

TEST_F(StatementParsingTest, LetDeclarations) {
  // Test: let x = 42
  auto tokens = create_tokens({{TokenType::Let, "let"},
                               {TokenType::Identifier, "x"},
                               {TokenType::Assign, "="},
                               {TokenType::Number, "42"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(StatementParsingTest, MutDeclarations) {
  // Test: mut y = "hello"
  auto tokens = create_tokens({{TokenType::Mut, "mut"},
                               {TokenType::Identifier, "y"},
                               {TokenType::Assign, "="},
                               {TokenType::String, "\"hello\""}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(StatementParsingTest, TypedVariableDeclarations) {
  // Test: let x: int = 42
  auto tokens = create_tokens({{TokenType::Let, "let"},
                               {TokenType::Identifier, "x"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "int"},
                               {TokenType::Assign, "="},
                               {TokenType::Number, "42"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test: mut name: string = "John"
  auto tokens2 = create_tokens({{TokenType::Mut, "mut"},
                                {TokenType::Identifier, "name"},
                                {TokenType::Colon, ":"},
                                {TokenType::Identifier, "string"},
                                {TokenType::Assign, "="},
                                {TokenType::String, "\"John\""}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

TEST_F(StatementParsingTest, OptionalTypeDeclarations) {
  // Test: let x: int?
  auto tokens = create_tokens({{TokenType::Let, "let"},
                               {TokenType::Identifier, "x"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "int"},
                               {TokenType::Question, "?"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test: mut y: string? = null
  auto tokens2 = create_tokens({{TokenType::Mut, "mut"},
                                {TokenType::Identifier, "y"},
                                {TokenType::Colon, ":"},
                                {TokenType::Identifier, "string"},
                                {TokenType::Question, "?"},
                                {TokenType::Assign, "="},
                                {TokenType::Null, "null"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

TEST_F(StatementParsingTest, VariableDeclarationWithoutInitializer) {
  // Test: let x: int (type without initializer)
  auto tokens = create_tokens({{TokenType::Let, "let"},
                               {TokenType::Identifier, "x"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "int"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(StatementParsingTest, ComplexVariableDeclarations) {
  // Test: let result: bool = x > 0 and y < 10
  auto tokens = create_tokens({{TokenType::Let, "let"},
                               {TokenType::Identifier, "result"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "bool"},
                               {TokenType::Assign, "="},
                               {TokenType::Identifier, "x"},
                               {TokenType::GreaterThan, ">"},
                               {TokenType::Number, "0"},
                               {TokenType::LogicalAnd, "and"},
                               {TokenType::Identifier, "y"},
                               {TokenType::LessThan, "<"},
                               {TokenType::Number, "10"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Function Declaration Tests ====================

TEST_F(StatementParsingTest, SimpleFunctionDeclarations) {
  // Test: fun greet(): void { }
  auto tokens = create_tokens({{TokenType::Function, "fun"},
                               {TokenType::Identifier, "greet"},
                               {TokenType::LeftParen, "("},
                               {TokenType::RightParen, ")"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "void"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(StatementParsingTest, FunctionWithParameters) {
  // Test: fun add(let x: int, let y: int): int { }
  auto tokens = create_tokens({{TokenType::Function, "fun"},
                               {TokenType::Identifier, "add"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Let, "let"},
                               {TokenType::Identifier, "x"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "int"},
                               {TokenType::Comma, ","},
                               {TokenType::Let, "let"},
                               {TokenType::Identifier, "y"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "int"},
                               {TokenType::RightParen, ")"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "int"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(StatementParsingTest, FunctionWithMutableParameters) {
  // Test: fun modify(mut arr: Array<int>): void { }
  auto tokens = create_tokens({{TokenType::Function, "fun"},
                               {TokenType::Identifier, "modify"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Mut, "mut"},
                               {TokenType::Identifier, "arr"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "Array"},
                               {TokenType::RightParen, ")"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "void"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(StatementParsingTest, FunctionWithDefaultParameters) {
  // Test: fun greet(let name: string = "World"): string { }
  auto tokens = create_tokens({{TokenType::Function, "fun"},
                               {TokenType::Identifier, "greet"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Let, "let"},
                               {TokenType::Identifier, "name"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "string"},
                               {TokenType::Assign, "="},
                               {TokenType::String, "\"World\""},
                               {TokenType::RightParen, ")"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "string"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(StatementParsingTest, FunctionWithExpressionBody) {
  // Test: fun double(let x: int): int = x * 2
  auto tokens = create_tokens({{TokenType::Function, "fun"},
                               {TokenType::Identifier, "double"},
                               {TokenType::LeftParen, "("},
                               {TokenType::Let, "let"},
                               {TokenType::Identifier, "x"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "int"},
                               {TokenType::RightParen, ")"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "int"},
                               {TokenType::Assign, "="},
                               {TokenType::Identifier, "x"},
                               {TokenType::Asterisk, "*"},
                               {TokenType::Number, "2"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(StatementParsingTest, FunctionWithBlockBody) {
  // Test: fun calculate(): int { return 42; }
  auto tokens = create_tokens({{TokenType::Function, "fun"},
                               {TokenType::Identifier, "calculate"},
                               {TokenType::LeftParen, "("},
                               {TokenType::RightParen, ")"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "int"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::Return, "return"},
                               {TokenType::Number, "42"},
                               {TokenType::Semicolon, ";"},
                               {TokenType::RightBrace, "}"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Expression Statement Tests ====================

TEST_F(StatementParsingTest, SimpleExpressionStatements) {
  // Test: 42;
  auto tokens =
      create_tokens({{TokenType::Number, "42"}, {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test: "hello world";
  auto tokens2 = create_tokens(
      {{TokenType::String, "\"hello world\""}, {TokenType::Semicolon, ";"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

TEST_F(StatementParsingTest, FunctionCallStatements) {
  // Test: doSomething();
  auto tokens = create_tokens({{TokenType::Identifier, "doSomething"},
                               {TokenType::LeftParen, "("},
                               {TokenType::RightParen, ")"},
                               {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test: process(x, y, z);
  auto tokens2 = create_tokens({{TokenType::Identifier, "process"},
                                {TokenType::LeftParen, "("},
                                {TokenType::Identifier, "x"},
                                {TokenType::Comma, ","},
                                {TokenType::Identifier, "y"},
                                {TokenType::Comma, ","},
                                {TokenType::Identifier, "z"},
                                {TokenType::RightParen, ")"},
                                {TokenType::Semicolon, ";"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

TEST_F(StatementParsingTest, AssignmentStatements) {
  // Test: x = 42;
  auto tokens = create_tokens({{TokenType::Identifier, "x"},
                               {TokenType::Assign, "="},
                               {TokenType::Number, "42"},
                               {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test: arr[i] = value;
  auto tokens2 = create_tokens({{TokenType::Identifier, "arr"},
                                {TokenType::LeftBracket, "["},
                                {TokenType::Identifier, "i"},
                                {TokenType::RightBracket, "]"},
                                {TokenType::Assign, "="},
                                {TokenType::Identifier, "value"},
                                {TokenType::Semicolon, ";"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

TEST_F(StatementParsingTest, ComplexExpressionStatements) {
  // Test: x + y * z;
  auto tokens = create_tokens({{TokenType::Identifier, "x"},
                               {TokenType::Plus, "+"},
                               {TokenType::Identifier, "y"},
                               {TokenType::Asterisk, "*"},
                               {TokenType::Identifier, "z"},
                               {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test: obj.method().property;
  auto tokens2 = create_tokens({{TokenType::Identifier, "obj"},
                                {TokenType::Dot, "."},
                                {TokenType::Identifier, "method"},
                                {TokenType::LeftParen, "("},
                                {TokenType::RightParen, ")"},
                                {TokenType::Dot, "."},
                                {TokenType::Identifier, "property"},
                                {TokenType::Semicolon, ";"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

// ==================== Multiple Statement Tests ====================

TEST_F(StatementParsingTest, MultipleStatements) {
  // Test multiple statements in sequence
  auto tokens = create_tokens({{TokenType::Let, "let"},
                               {TokenType::Identifier, "x"},
                               {TokenType::Assign, "="},
                               {TokenType::Number, "10"},
                               {TokenType::Semicolon, ";"},
                               {TokenType::Let, "let"},
                               {TokenType::Identifier, "y"},
                               {TokenType::Assign, "="},
                               {TokenType::Number, "20"},
                               {TokenType::Semicolon, ";"},
                               {TokenType::Identifier, "x"},
                               {TokenType::Plus, "+"},
                               {TokenType::Identifier, "y"},
                               {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(StatementParsingTest, MixedStatementTypes) {
  // Test mix of variable declarations, function declarations, and expressions
  auto tokens = create_tokens({{TokenType::Let, "let"},
                               {TokenType::Identifier, "counter"},
                               {TokenType::Assign, "="},
                               {TokenType::Number, "0"},
                               {TokenType::Semicolon, ";"},
                               {TokenType::Function, "fun"},
                               {TokenType::Identifier, "increment"},
                               {TokenType::LeftParen, "("},
                               {TokenType::RightParen, ")"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "void"},
                               {TokenType::LeftBrace, "{"},
                               {TokenType::RightBrace, "}"},
                               {TokenType::Identifier, "increment"},
                               {TokenType::LeftParen, "("},
                               {TokenType::RightParen, ")"},
                               {TokenType::Semicolon, ";"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Error Cases ====================

TEST_F(StatementParsingTest, VariableDeclarationErrors) {
  // Missing identifier after let
  expect_parse_failure(create_tokens({{TokenType::Let, "let"},
                                      {TokenType::Assign, "="},
                                      {TokenType::Number, "42"}}));

  // Invalid syntax: just identifier without let/mut
  expect_parse_failure(create_tokens({{TokenType::Identifier, "x"},
                                      {TokenType::Colon, ":"},
                                      {TokenType::Identifier, "int"}}));

  // Missing type and initializer after identifier
  expect_parse_failure(
      create_tokens({{TokenType::Let, "let"}, {TokenType::Identifier, "x"}}));
}

TEST_F(StatementParsingTest, FunctionDeclarationErrors) {
  // Missing function name
  expect_parse_failure(create_tokens({{TokenType::Function, "fun"},
                                      {TokenType::LeftParen, "("},
                                      {TokenType::RightParen, ")"},
                                      {TokenType::Colon, ":"},
                                      {TokenType::Identifier, "void"}}));

  // Missing return type
  expect_parse_failure(create_tokens({{TokenType::Function, "fun"},
                                      {TokenType::Identifier, "test"},
                                      {TokenType::LeftParen, "("},
                                      {TokenType::RightParen, ")"}}));

  // Unmatched parentheses in parameters
  expect_parse_failure(create_tokens({
      {TokenType::Function, "fun"},
      {TokenType::Identifier, "test"},
      {TokenType::LeftParen, "("},
      {TokenType::Let, "let"},
      {TokenType::Identifier, "x"},
      {TokenType::Colon, ":"},
      {TokenType::Identifier, "int"} // Missing RightParen
  }));
}

TEST_F(StatementParsingTest, SemicolonHandling) {
  // Test that semicolons are optional in some contexts
  auto tokens_no_semicolon = create_tokens({{TokenType::Number, "42"}});
  auto result = safe_parse(tokens_no_semicolon);
  EXPECT_NE(result, nullptr);

  // Test that semicolons work when present
  auto tokens_with_semicolon =
      create_tokens({{TokenType::Number, "42"}, {TokenType::Semicolon, ";"}});
  auto result2 = safe_parse(tokens_with_semicolon);
  EXPECT_NE(result2, nullptr);
}

} // namespace nugdev::test