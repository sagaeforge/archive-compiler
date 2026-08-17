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
 * @brief Test class for Type parsing functionality
 */
class TypeParsingTest : public ::testing::Test {
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

// ==================== Simple Type Literals ====================

TEST_F(TypeParsingTest, SimpleTypeLiterals) {
  // Test let x: Int
  auto int_type_tokens = create_tokens({{TokenType::Let, "let"},
                                        {TokenType::Identifier, "x"},
                                        {TokenType::Colon, ":"},
                                        {TokenType::Identifier, "Int"}});

  auto result = safe_parse(int_type_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(TypeParsingTest, BuiltinTypes) {
  std::vector<std::string> builtin_types = {"Int",  "Float", "String", "Bool",
                                            "Void", "Any",   "Never"};

  for (const auto &type : builtin_types) {
    auto tokens = create_tokens({{TokenType::Let, "let"},
                                 {TokenType::Identifier, "x"},
                                 {TokenType::Colon, ":"},
                                 {TokenType::Identifier, type}});

    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse type: " << type;
  }
}

// ==================== Optional Types ====================

TEST_F(TypeParsingTest, OptionalTypes) {
  // Test let x: Int?
  auto optional_int_tokens = create_tokens({{TokenType::Let, "let"},
                                            {TokenType::Identifier, "x"},
                                            {TokenType::Colon, ":"},
                                            {TokenType::Identifier, "Int"},
                                            {TokenType::Question, "?"}});

  auto result = safe_parse(optional_int_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(TypeParsingTest, DoubleOptionalTypes) {
  // Test let x: String??
  auto double_optional_tokens =
      create_tokens({{TokenType::Let, "let"},
                     {TokenType::Identifier, "x"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "String"},
                     {TokenType::Question, "?"},
                     {TokenType::Question, "?"}});

  auto result = safe_parse(double_optional_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Function Types ====================

TEST_F(TypeParsingTest, SimpleFunctionType) {
  // Test let f: (Int) -> String
  auto simple_func_type_tokens =
      create_tokens({{TokenType::Let, "let"},
                     {TokenType::Identifier, "f"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "Int"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "String"}});

  auto result = safe_parse(simple_func_type_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(TypeParsingTest, MultiParameterFunctionType) {
  // Test let f: (Int, String, Bool) -> Void
  auto multi_param_func_type_tokens =
      create_tokens({{TokenType::Let, "let"},
                     {TokenType::Identifier, "f"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "Int"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "String"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "Bool"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Void"}});

  auto result = safe_parse(multi_param_func_type_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(TypeParsingTest, NoParameterFunctionType) {
  // Test let f: () -> Int
  auto no_param_func_type_tokens =
      create_tokens({{TokenType::Let, "let"},
                     {TokenType::Identifier, "f"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Int"}});

  auto result = safe_parse(no_param_func_type_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(TypeParsingTest, NestedFunctionTypes) {
  // Test let f: ((Int) -> String) -> Bool
  auto nested_func_type_tokens =
      create_tokens({{TokenType::Let, "let"},
                     {TokenType::Identifier, "f"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "Int"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Bool"}});

  auto result = safe_parse(nested_func_type_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Function Types with Optional Parameters
// ====================

TEST_F(TypeParsingTest, FunctionTypeWithOptionalParameters) {
  // Test let f: (Int?, String) -> Bool?
  auto optional_param_func_type_tokens =
      create_tokens({{TokenType::Let, "let"},
                     {TokenType::Identifier, "f"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "Int"},
                     {TokenType::Question, "?"},
                     {TokenType::Comma, ","},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Bool"},
                     {TokenType::Question, "?"}});

  auto result = safe_parse(optional_param_func_type_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Complex Type Combinations ====================

TEST_F(TypeParsingTest, ComplexTypeCombinations) {
  // Test let complex: ((Int?, String) -> Bool)? = null
  auto complex_type_tokens = create_tokens({{TokenType::Let, "let"},
                                            {TokenType::Identifier, "complex"},
                                            {TokenType::Colon, ":"},
                                            {TokenType::LeftParen, "("},
                                            {TokenType::LeftParen, "("},
                                            {TokenType::Identifier, "Int"},
                                            {TokenType::Question, "?"},
                                            {TokenType::Comma, ","},
                                            {TokenType::Identifier, "String"},
                                            {TokenType::RightParen, ")"},
                                            {TokenType::Arrow, "->"},
                                            {TokenType::Identifier, "Bool"},
                                            {TokenType::RightParen, ")"},
                                            {TokenType::Question, "?"},
                                            {TokenType::Assign, "="},
                                            {TokenType::Null, "null"}});

  auto result = safe_parse(complex_type_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Error Cases ====================

TEST_F(TypeParsingTest, TypeErrorCases) {
  // Invalid type tokens that absolutely cannot be types
  auto semicolon_type_tokens = create_tokens({{TokenType::Let, "let"},
                                              {TokenType::Identifier, "x"},
                                              {TokenType::Colon, ":"},
                                              {TokenType::Semicolon, ";"}});
  expect_parse_failure(semicolon_type_tokens);

  auto comma_type_tokens = create_tokens({{TokenType::Let, "let"},
                                          {TokenType::Identifier, "x"},
                                          {TokenType::Colon, ":"},
                                          {TokenType::Comma, ","}});
  expect_parse_failure(comma_type_tokens);

  auto brace_type_tokens = create_tokens({{TokenType::Let, "let"},
                                          {TokenType::Identifier, "x"},
                                          {TokenType::Colon, ":"},
                                          {TokenType::RightBrace, "}"}});
  expect_parse_failure(brace_type_tokens);

  // Missing return type in function type (arrow with EOF)
  auto missing_return_type_tokens = create_tokens({{TokenType::Let, "let"},
                                                   {TokenType::Identifier, "f"},
                                                   {TokenType::Colon, ":"},
                                                   {TokenType::LeftParen, "("},
                                                   {TokenType::RightParen, ")"},
                                                   {TokenType::Arrow, "->"}});
  expect_parse_failure(missing_return_type_tokens);

  // Invalid function type with missing closing paren
  auto unclosed_func_type_tokens =
      create_tokens({{TokenType::Let, "let"},
                     {TokenType::Identifier, "f"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "Int"}});
  expect_parse_failure(unclosed_func_type_tokens);
}

// ==================== Function Declaration Type Tests ====================

TEST_F(TypeParsingTest, FunctionDeclarationTypes) {
  // Test fun add(let a: Int, let b: Int): Int = a + b
  auto func_decl_tokens = create_tokens({{TokenType::Function, "fun"},
                                         {TokenType::Identifier, "add"},
                                         {TokenType::LeftParen, "("},
                                         {TokenType::Let, "let"},
                                         {TokenType::Identifier, "a"},
                                         {TokenType::Colon, ":"},
                                         {TokenType::Identifier, "Int"},
                                         {TokenType::Comma, ","},
                                         {TokenType::Let, "let"},
                                         {TokenType::Identifier, "b"},
                                         {TokenType::Colon, ":"},
                                         {TokenType::Identifier, "Int"},
                                         {TokenType::RightParen, ")"},
                                         {TokenType::Colon, ":"},
                                         {TokenType::Identifier, "Int"},
                                         {TokenType::Assign, "="},
                                         {TokenType::Identifier, "a"},
                                         {TokenType::Plus, "+"},
                                         {TokenType::Identifier, "b"}});

  auto result = safe_parse(func_decl_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(TypeParsingTest, FunctionDeclarationWithOptionalTypes) {
  // Test fun process(let data: String?, let callback: (String) -> Void): Bool?
  auto func_optional_tokens =
      create_tokens({{TokenType::Function, "fun"},
                     {TokenType::Identifier, "process"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Let, "let"},
                     {TokenType::Identifier, "data"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "String"},
                     {TokenType::Question, "?"},
                     {TokenType::Comma, ","},
                     {TokenType::Let, "let"},
                     {TokenType::Identifier, "callback"},
                     {TokenType::Colon, ":"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Arrow, "->"},
                     {TokenType::Identifier, "Void"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Bool"},
                     {TokenType::Question, "?"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Return, "return"},
                     {TokenType::True, "true"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(func_optional_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

} // namespace nugdev::test