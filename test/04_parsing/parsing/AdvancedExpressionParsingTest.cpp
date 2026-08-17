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
 * @brief Test class for Advanced Expression parsing functionality
 */
class AdvancedExpressionParsingTest : public ::testing::Test {
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

// ==================== Function Expressions ====================

TEST_F(AdvancedExpressionParsingTest, SimpleFunctionExpression) {
  // Test fun(let x: Int): Int = x + 1
  auto simple_func_tokens = create_tokens({{TokenType::Function, "fun"},
                                           {TokenType::LeftParen, "("},
                                           {TokenType::Let, "let"},
                                           {TokenType::Identifier, "x"},
                                           {TokenType::Colon, ":"},
                                           {TokenType::Identifier, "Int"},
                                           {TokenType::RightParen, ")"},
                                           {TokenType::Colon, ":"},
                                           {TokenType::Identifier, "Int"},
                                           {TokenType::Assign, "="},
                                           {TokenType::Identifier, "x"},
                                           {TokenType::Plus, "+"},
                                           {TokenType::Number, "1"}});

  auto result = safe_parse(simple_func_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, FunctionExpressionWithBlockBody) {
  // Test fun(let x: Int): Int { return x + 1; }
  auto block_func_tokens = create_tokens({{TokenType::Function, "fun"},
                                          {TokenType::LeftParen, "("},
                                          {TokenType::Let, "let"},
                                          {TokenType::Identifier, "x"},
                                          {TokenType::Colon, ":"},
                                          {TokenType::Identifier, "Int"},
                                          {TokenType::RightParen, ")"},
                                          {TokenType::Colon, ":"},
                                          {TokenType::Identifier, "Int"},
                                          {TokenType::LeftBrace, "{"},
                                          {TokenType::Return, "return"},
                                          {TokenType::Identifier, "x"},
                                          {TokenType::Plus, "+"},
                                          {TokenType::Number, "1"},
                                          {TokenType::Semicolon, ";"},
                                          {TokenType::RightBrace, "}"}});

  auto result = safe_parse(block_func_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, FunctionExpressionNoParameters) {
  // Test fun(): String = "hello"
  auto no_param_func_tokens = create_tokens({{TokenType::Function, "fun"},
                                             {TokenType::LeftParen, "("},
                                             {TokenType::RightParen, ")"},
                                             {TokenType::Colon, ":"},
                                             {TokenType::Identifier, "String"},
                                             {TokenType::Assign, "="},
                                             {TokenType::String, "\"hello\""}});

  auto result = safe_parse(no_param_func_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, FunctionExpressionMultipleParameters) {
  // Test fun(let a: Int, let b: String, let c: Bool): Void = doSomething()
  auto multi_param_func_tokens =
      create_tokens({{TokenType::Function, "fun"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Let, "let"},
                     {TokenType::Identifier, "a"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::Comma, ","},
                     {TokenType::Let, "let"},
                     {TokenType::Identifier, "b"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "String"},
                     {TokenType::Comma, ","},
                     {TokenType::Let, "let"},
                     {TokenType::Identifier, "c"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Bool"},
                     {TokenType::RightParen, ")"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Void"},
                     {TokenType::Assign, "="},
                     {TokenType::Identifier, "doSomething"},
                     {TokenType::LeftParen, "("},
                     {TokenType::RightParen, ")"}});

  auto result = safe_parse(multi_param_func_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Lambda Expressions ====================

TEST_F(AdvancedExpressionParsingTest, SimpleLambdaExpression) {
  // Test (let x: Int) => x * 2
  auto simple_lambda_tokens = create_tokens({{TokenType::LeftParen, "("},
                                             {TokenType::Let, "let"},
                                             {TokenType::Identifier, "x"},
                                             {TokenType::Colon, ":"},
                                             {TokenType::Identifier, "Int"},
                                             {TokenType::RightParen, ")"},
                                             {TokenType::FatArrow, "=>"},
                                             {TokenType::Identifier, "x"},
                                             {TokenType::Asterisk, "*"},
                                             {TokenType::Number, "2"}});

  auto result = safe_parse(simple_lambda_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, LambdaExpressionWithBlockBody) {
  // Test (let x: Int) => { let doubled = x * 2; return doubled; }
  auto block_lambda_tokens = create_tokens({{TokenType::LeftParen, "("},
                                            {TokenType::Let, "let"},
                                            {TokenType::Identifier, "x"},
                                            {TokenType::Colon, ":"},
                                            {TokenType::Identifier, "Int"},
                                            {TokenType::RightParen, ")"},
                                            {TokenType::FatArrow, "=>"},
                                            {TokenType::LeftBrace, "{"},
                                            {TokenType::Let, "let"},
                                            {TokenType::Identifier, "doubled"},
                                            {TokenType::Assign, "="},
                                            {TokenType::Identifier, "x"},
                                            {TokenType::Asterisk, "*"},
                                            {TokenType::Number, "2"},
                                            {TokenType::Semicolon, ";"},
                                            {TokenType::Return, "return"},
                                            {TokenType::Identifier, "doubled"},
                                            {TokenType::Semicolon, ";"},
                                            {TokenType::RightBrace, "}"}});

  auto result = safe_parse(block_lambda_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, LambdaExpressionNoParameters) {
  // Test () => 42
  auto no_param_lambda_tokens = create_tokens({{TokenType::LeftParen, "("},
                                               {TokenType::RightParen, ")"},
                                               {TokenType::FatArrow, "=>"},
                                               {TokenType::Number, "42"}});

  auto result = safe_parse(no_param_lambda_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Block Expressions ====================

TEST_F(AdvancedExpressionParsingTest, SimpleBlockExpression) {
  // Test { 42 }
  auto simple_block_tokens = create_tokens({{TokenType::LeftBrace, "{"},
                                            {TokenType::Number, "42"},
                                            {TokenType::RightBrace, "}"}});

  auto result = safe_parse(simple_block_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, BlockExpressionWithStatements) {
  // Test { let x = 10; let y = 20; x + y }
  auto block_with_statements_tokens =
      create_tokens({{TokenType::LeftBrace, "{"},
                     {TokenType::Let, "let"},
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
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(block_with_statements_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, EmptyBlockExpression) {
  // Test {}
  auto empty_block_tokens = create_tokens(
      {{TokenType::LeftBrace, "{"}, {TokenType::RightBrace, "}"}});

  auto result = safe_parse(empty_block_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, NestedBlockExpressions) {
  // Test { { 1 + 2 } + { 3 + 4 } }
  auto nested_block_tokens = create_tokens({{TokenType::LeftBrace, "{"},
                                            {TokenType::LeftBrace, "{"},
                                            {TokenType::Number, "1"},
                                            {TokenType::Plus, "+"},
                                            {TokenType::Number, "2"},
                                            {TokenType::RightBrace, "}"},
                                            {TokenType::Plus, "+"},
                                            {TokenType::LeftBrace, "{"},
                                            {TokenType::Number, "3"},
                                            {TokenType::Plus, "+"},
                                            {TokenType::Number, "4"},
                                            {TokenType::RightBrace, "}"},
                                            {TokenType::RightBrace, "}"}});

  auto result = safe_parse(nested_block_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== If Expressions ====================

TEST_F(AdvancedExpressionParsingTest, SimpleIfExpression) {
  // Test if (true) { 42 }
  auto simple_if_tokens = create_tokens({{TokenType::If, "if"},
                                         {TokenType::LeftParen, "("},
                                         {TokenType::True, "true"},
                                         {TokenType::RightParen, ")"},
                                         {TokenType::LeftBrace, "{"},
                                         {TokenType::Number, "42"},
                                         {TokenType::RightBrace, "}"}});

  auto result = safe_parse(simple_if_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, IfElseExpression) {
  // Test if (condition) { 1 } else { 2 }
  auto if_else_tokens = create_tokens({{TokenType::If, "if"},
                                       {TokenType::LeftParen, "("},
                                       {TokenType::Identifier, "condition"},
                                       {TokenType::RightParen, ")"},
                                       {TokenType::LeftBrace, "{"},
                                       {TokenType::Number, "1"},
                                       {TokenType::RightBrace, "}"},
                                       {TokenType::Else, "else"},
                                       {TokenType::LeftBrace, "{"},
                                       {TokenType::Number, "2"},
                                       {TokenType::RightBrace, "}"}});

  auto result = safe_parse(if_else_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, NestedIfExpressions) {
  // Test if (a) { if (b) { 1 } else { 2 } } else { 3 }
  auto nested_if_tokens =
      create_tokens({{TokenType::If, "if"},        {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "a"}, {TokenType::RightParen, ")"},
                     {TokenType::LeftBrace, "{"},  {TokenType::If, "if"},
                     {TokenType::LeftParen, "("},  {TokenType::Identifier, "b"},
                     {TokenType::RightParen, ")"}, {TokenType::LeftBrace, "{"},
                     {TokenType::Number, "1"},     {TokenType::RightBrace, "}"},
                     {TokenType::Else, "else"},    {TokenType::LeftBrace, "{"},
                     {TokenType::Number, "2"},     {TokenType::RightBrace, "}"},
                     {TokenType::RightBrace, "}"}, {TokenType::Else, "else"},
                     {TokenType::LeftBrace, "{"},  {TokenType::Number, "3"},
                     {TokenType::RightBrace, "}"}});

  auto result = safe_parse(nested_if_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== When Expressions ====================

TEST_F(AdvancedExpressionParsingTest, SimpleWhenExpression) {
  // Test when (x) { 1 -> "one", 2 -> "two", else -> "other" }
  auto simple_when_tokens = create_tokens({{TokenType::When, "when"},
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
                                           {TokenType::Else, "else"},
                                           {TokenType::Arrow, "->"},
                                           {TokenType::String, "\"other\""},
                                           {TokenType::RightBrace, "}"}});

  auto result = safe_parse(simple_when_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, WhenExpressionWithRangeConditions) {
  // Test when (value) { value in 1..10 -> "small", value in 11..100 ->
  // "medium", else -> "large" }
  auto range_when_tokens = create_tokens({{TokenType::When, "when"},
                                          {TokenType::LeftParen, "("},
                                          {TokenType::Identifier, "value"},
                                          {TokenType::RightParen, ")"},
                                          {TokenType::LeftBrace, "{"},
                                          {TokenType::Identifier, "value"},
                                          {TokenType::In, "in"},
                                          {TokenType::Number, "1"},
                                          {TokenType::Range, ".."},
                                          {TokenType::Number, "10"},
                                          {TokenType::Arrow, "->"},
                                          {TokenType::String, "\"small\""},
                                          {TokenType::Comma, ","},
                                          {TokenType::Identifier, "value"},
                                          {TokenType::In, "in"},
                                          {TokenType::Number, "11"},
                                          {TokenType::Range, ".."},
                                          {TokenType::Number, "100"},
                                          {TokenType::Arrow, "->"},
                                          {TokenType::String, "\"medium\""},
                                          {TokenType::Comma, ","},
                                          {TokenType::Else, "else"},
                                          {TokenType::Arrow, "->"},
                                          {TokenType::String, "\"large\""},
                                          {TokenType::RightBrace, "}"}});

  auto result = safe_parse(range_when_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, WhenExpressionWithTypeConditions) {
  // Test when (obj) { obj is Int -> "integer", obj is String -> "string", else
  // -> "other" }
  auto type_when_tokens = create_tokens(
      {{TokenType::When, "when"},      {TokenType::LeftParen, "("},
       {TokenType::Identifier, "obj"}, {TokenType::RightParen, ")"},
       {TokenType::LeftBrace, "{"},    {TokenType::Identifier, "obj"},
       {TokenType::Is, "is"},          {TokenType::Identifier, "Int"},
       {TokenType::Arrow, "->"},       {TokenType::String, "\"integer\""},
       {TokenType::Comma, ","},        {TokenType::Identifier, "obj"},
       {TokenType::Is, "is"},          {TokenType::Identifier, "String"},
       {TokenType::Arrow, "->"},       {TokenType::String, "\"string\""},
       {TokenType::Comma, ","},        {TokenType::Else, "else"},
       {TokenType::Arrow, "->"},       {TokenType::String, "\"other\""},
       {TokenType::RightBrace, "}"}});

  auto result = safe_parse(type_when_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

TEST_F(AdvancedExpressionParsingTest, WhenExpressionWithGuardConditions) {
  // Test when (x) { x if x > 0 -> "positive", x if x < 0 -> "negative", else ->
  // "zero" }
  auto guard_when_tokens = create_tokens(
      {{TokenType::When, "when"},     {TokenType::LeftParen, "("},
       {TokenType::Identifier, "x"},  {TokenType::RightParen, ")"},
       {TokenType::LeftBrace, "{"},   {TokenType::Identifier, "x"},
       {TokenType::If, "if"},         {TokenType::Identifier, "x"},
       {TokenType::GreaterThan, ">"}, {TokenType::Number, "0"},
       {TokenType::Arrow, "->"},      {TokenType::String, "\"positive\""},
       {TokenType::Comma, ","},       {TokenType::Identifier, "x"},
       {TokenType::If, "if"},         {TokenType::Identifier, "x"},
       {TokenType::LessThan, "<"},    {TokenType::Number, "0"},
       {TokenType::Arrow, "->"},      {TokenType::String, "\"negative\""},
       {TokenType::Comma, ","},       {TokenType::Else, "else"},
       {TokenType::Arrow, "->"},      {TokenType::String, "\"zero\""},
       {TokenType::RightBrace, "}"}});

  auto result = safe_parse(guard_when_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Complex Expression Combinations ====================

TEST_F(AdvancedExpressionParsingTest, ComplexExpressionCombinations) {
  // Test function expression assigned to variable
  // let f = fun(let x: Int): Int = { if (x > 0) { x * 2 } else { -x } }
  auto complex_tokens = create_tokens(
      {{TokenType::Let, "let"},        {TokenType::Identifier, "f"},
       {TokenType::Assign, "="},       {TokenType::Function, "fun"},
       {TokenType::LeftParen, "("},    {TokenType::Let, "let"},
       {TokenType::Identifier, "x"},   {TokenType::Colon, ":"},
       {TokenType::Identifier, "Int"}, {TokenType::RightParen, ")"},
       {TokenType::Colon, ":"},        {TokenType::Identifier, "Int"},
       {TokenType::Assign, "="},       {TokenType::LeftBrace, "{"},
       {TokenType::If, "if"},          {TokenType::LeftParen, "("},
       {TokenType::Identifier, "x"},   {TokenType::GreaterThan, ">"},
       {TokenType::Number, "0"},       {TokenType::RightParen, ")"},
       {TokenType::LeftBrace, "{"},    {TokenType::Identifier, "x"},
       {TokenType::Asterisk, "*"},     {TokenType::Number, "2"},
       {TokenType::RightBrace, "}"},   {TokenType::Else, "else"},
       {TokenType::LeftBrace, "{"},    {TokenType::Minus, "-"},
       {TokenType::Identifier, "x"},   {TokenType::RightBrace, "}"},
       {TokenType::RightBrace, "}"}});

  auto result = safe_parse(complex_tokens);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->get_modules().size(), 1);
}

// ==================== Error Cases ====================

TEST_F(AdvancedExpressionParsingTest, FunctionExpressionErrorCases) {
  // Missing return type
  auto missing_return_type_tokens =
      create_tokens({{TokenType::Function, "fun"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Let, "let"},
                     {TokenType::Identifier, "x"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::RightParen, ")"},
                     // Missing colon and return type
                     {TokenType::Assign, "="},
                     {TokenType::Identifier, "x"}});
  expect_parse_failure(missing_return_type_tokens);

  // Missing function body
  auto missing_body_tokens = create_tokens({
      {TokenType::Function, "fun"},
      {TokenType::LeftParen, "("},
      {TokenType::RightParen, ")"},
      {TokenType::Colon, ":"},
      {TokenType::Identifier, "Void"} // Missing body
  });
  expect_parse_failure(missing_body_tokens);
}

TEST_F(AdvancedExpressionParsingTest, LambdaExpressionErrorCases) {
  // Missing fat arrow
  auto missing_arrow_tokens = create_tokens({{TokenType::LeftParen, "("},
                                             {TokenType::Let, "let"},
                                             {TokenType::Identifier, "x"},
                                             {TokenType::Colon, ":"},
                                             {TokenType::Identifier, "Int"},
                                             {TokenType::RightParen, ")"},
                                             // Missing fat arrow
                                             {TokenType::Identifier, "x"}});
  expect_parse_failure(missing_arrow_tokens);

  // Missing lambda body
  auto missing_lambda_body_tokens = create_tokens({
      {TokenType::LeftParen, "("},
      {TokenType::RightParen, ")"},
      {TokenType::FatArrow, "=>"} // Missing body
  });
  expect_parse_failure(missing_lambda_body_tokens);
}

TEST_F(AdvancedExpressionParsingTest, WhenExpressionErrorCases) {
  // Missing arrow in when branch
  auto missing_when_arrow_tokens =
      create_tokens({{TokenType::When, "when"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Identifier, "x"},
                     {TokenType::RightParen, ")"},
                     {TokenType::LeftBrace, "{"},
                     {TokenType::Number, "1"},
                     // Missing arrow
                     {TokenType::String, "\"one\""},
                     {TokenType::RightBrace, "}"}});
  expect_parse_failure(missing_when_arrow_tokens);

  // Unmatched braces in when expression
  auto unmatched_when_braces_tokens = create_tokens({
      {TokenType::When, "when"},
      {TokenType::LeftParen, "("},
      {TokenType::Identifier, "x"},
      {TokenType::RightParen, ")"},
      {TokenType::LeftBrace, "{"},
      {TokenType::Number, "1"},
      {TokenType::Arrow, "->"},
      {TokenType::String, "\"one\""} // Missing closing brace
  });
  expect_parse_failure(unmatched_when_braces_tokens);
}

// ==================== Grammar-Complete Lambda Tests ====================

TEST_F(AdvancedExpressionParsingTest, LambdaParameterTypesRequired) {
  // Grammar requires type annotations for lambda parameters to distinguish from
  // function calls

  // Test (let x: Int) => x * 2
  auto simple_lambda_tokens = create_tokens({{TokenType::LeftParen, "("},
                                             {TokenType::Let, "let"},
                                             {TokenType::Identifier, "x"},
                                             {TokenType::Colon, ":"},
                                             {TokenType::Identifier, "Int"},
                                             {TokenType::RightParen, ")"},
                                             {TokenType::FatArrow, "=>"},
                                             {TokenType::Identifier, "x"},
                                             {TokenType::Asterisk, "*"},
                                             {TokenType::Number, "2"}});
  auto result = safe_parse(simple_lambda_tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(AdvancedExpressionParsingTest, LambdaWithMutableParameters) {
  // Test (mut x: Int, let y: String) => x + y.length
  auto mutable_lambda_tokens =
      create_tokens({{TokenType::LeftParen, "("},
                     {TokenType::Mut, "mut"},
                     {TokenType::Identifier, "x"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::Comma, ","},
                     {TokenType::Let, "let"},
                     {TokenType::Identifier, "y"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "String"},
                     {TokenType::RightParen, ")"},
                     {TokenType::FatArrow, "=>"},
                     {TokenType::Identifier, "x"},
                     {TokenType::Plus, "+"},
                     {TokenType::Identifier, "y"},
                     {TokenType::Dot, "."},
                     {TokenType::Identifier, "length"}});
  auto result = safe_parse(mutable_lambda_tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(AdvancedExpressionParsingTest, LambdaWithDefaultParameters) {
  // Test (let x: Int = 5, let y: Int = 10) => x + y
  auto default_params_tokens = create_tokens({{TokenType::LeftParen, "("},
                                              {TokenType::Let, "let"},
                                              {TokenType::Identifier, "x"},
                                              {TokenType::Colon, ":"},
                                              {TokenType::Identifier, "Int"},
                                              {TokenType::Assign, "="},
                                              {TokenType::Number, "5"},
                                              {TokenType::Comma, ","},
                                              {TokenType::Let, "let"},
                                              {TokenType::Identifier, "y"},
                                              {TokenType::Colon, ":"},
                                              {TokenType::Identifier, "Int"},
                                              {TokenType::Assign, "="},
                                              {TokenType::Number, "10"},
                                              {TokenType::RightParen, ")"},
                                              {TokenType::FatArrow, "=>"},
                                              {TokenType::Identifier, "x"},
                                              {TokenType::Plus, "+"},
                                              {TokenType::Identifier, "y"}});
  auto result = safe_parse(default_params_tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(AdvancedExpressionParsingTest, LambdaWithComplexTypes) {
  // Test (let f: (Int) -> String, let arr: Array<Int>) => f(arr[0])
  auto complex_types_tokens = create_tokens(
      {{TokenType::LeftParen, "("},       {TokenType::Let, "let"},
       {TokenType::Identifier, "f"},      {TokenType::Colon, ":"},
       {TokenType::LeftParen, "("},       {TokenType::Identifier, "Int"},
       {TokenType::RightParen, ")"},      {TokenType::Arrow, "->"},
       {TokenType::Identifier, "String"}, {TokenType::Comma, ","},
       {TokenType::Let, "let"},           {TokenType::Identifier, "arr"},
       {TokenType::Colon, ":"},           {TokenType::Identifier, "Array"},
       {TokenType::RightParen, ")"},      {TokenType::FatArrow, "=>"},
       {TokenType::Identifier, "f"},      {TokenType::LeftParen, "("},
       {TokenType::Identifier, "arr"},    {TokenType::LeftBracket, "["},
       {TokenType::Number, "0"},          {TokenType::RightBracket, "]"},
       {TokenType::RightParen, ")"}});
  auto result = safe_parse(complex_types_tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(AdvancedExpressionParsingTest, LabeledLambdaExpressions) {
  // Test label@ (let x: Int) => x * 2
  auto labeled_lambda_tokens =
      create_tokens({{TokenType::Identifier, "myLabel"},
                     {TokenType::At, "@"},
                     {TokenType::LeftParen, "("},
                     {TokenType::Let, "let"},
                     {TokenType::Identifier, "x"},
                     {TokenType::Colon, ":"},
                     {TokenType::Identifier, "Int"},
                     {TokenType::RightParen, ")"},
                     {TokenType::FatArrow, "=>"},
                     {TokenType::Identifier, "x"},
                     {TokenType::Asterisk, "*"},
                     {TokenType::Number, "2"}});
  auto result = safe_parse(labeled_lambda_tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Grammar-Complete Function Expression Tests
// ====================

TEST_F(AdvancedExpressionParsingTest, FunctionExpressionWithSingleExpression) {
  // Test fun(let x: Int): Int = x * 2
  auto func_expr_tokens = create_tokens({{TokenType::Function, "fun"},
                                         {TokenType::LeftParen, "("},
                                         {TokenType::Let, "let"},
                                         {TokenType::Identifier, "x"},
                                         {TokenType::Colon, ":"},
                                         {TokenType::Identifier, "Int"},
                                         {TokenType::RightParen, ")"},
                                         {TokenType::Colon, ":"},
                                         {TokenType::Identifier, "Int"},
                                         {TokenType::Assign, "="},
                                         {TokenType::Identifier, "x"},
                                         {TokenType::Asterisk, "*"},
                                         {TokenType::Number, "2"}});
  auto result = safe_parse(func_expr_tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(AdvancedExpressionParsingTest, LabeledFunctionExpression) {
  // Test label@ fun(let x: Int): Int = x * 2
  auto labeled_func_tokens = create_tokens({{TokenType::Identifier, "myFunc"},
                                            {TokenType::At, "@"},
                                            {TokenType::Function, "fun"},
                                            {TokenType::LeftParen, "("},
                                            {TokenType::Let, "let"},
                                            {TokenType::Identifier, "x"},
                                            {TokenType::Colon, ":"},
                                            {TokenType::Identifier, "Int"},
                                            {TokenType::RightParen, ")"},
                                            {TokenType::Colon, ":"},
                                            {TokenType::Identifier, "Int"},
                                            {TokenType::Assign, "="},
                                            {TokenType::Identifier, "x"},
                                            {TokenType::Asterisk, "*"},
                                            {TokenType::Number, "2"}});
  auto result = safe_parse(labeled_func_tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Lambda vs Function Call Disambiguation
// ====================

TEST_F(AdvancedExpressionParsingTest, LambdaVsFunctionCallDisambiguation) {
  // According to grammar: Lambda parameters MUST have type annotations
  // to distinguish from function calls

  // This should be parsed as function call: func(x, y)
  auto function_call_tokens = create_tokens({{TokenType::Identifier, "func"},
                                             {TokenType::LeftParen, "("},
                                             {TokenType::Identifier, "x"},
                                             {TokenType::Comma, ","},
                                             {TokenType::Identifier, "y"},
                                             {TokenType::RightParen, ")"}});
  auto func_call_result = safe_parse(function_call_tokens);
  EXPECT_NE(func_call_result, nullptr);

  // This should be parsed as lambda: (let x: Int, let y: Int) => x + y
  auto lambda_tokens = create_tokens({{TokenType::LeftParen, "("},
                                      {TokenType::Let, "let"},
                                      {TokenType::Identifier, "x"},
                                      {TokenType::Colon, ":"},
                                      {TokenType::Identifier, "Int"},
                                      {TokenType::Comma, ","},
                                      {TokenType::Let, "let"},
                                      {TokenType::Identifier, "y"},
                                      {TokenType::Colon, ":"},
                                      {TokenType::Identifier, "Int"},
                                      {TokenType::RightParen, ")"},
                                      {TokenType::FatArrow, "=>"},
                                      {TokenType::Identifier, "x"},
                                      {TokenType::Plus, "+"},
                                      {TokenType::Identifier, "y"}});
  auto lambda_result = safe_parse(lambda_tokens);
  EXPECT_NE(lambda_result, nullptr);
}

// ==================== Lambda Error Cases ====================

TEST_F(AdvancedExpressionParsingTest, LambdaErrorCases) {
  // Lambda without type annotations should fail according to grammar
  auto no_types_lambda = create_tokens({{TokenType::LeftParen, "("},
                                        {TokenType::Identifier, "x"},
                                        {TokenType::Comma, ","},
                                        {TokenType::Identifier, "y"},
                                        {TokenType::RightParen, ")"},
                                        {TokenType::FatArrow, "=>"},
                                        {TokenType::Identifier, "x"},
                                        {TokenType::Plus, "+"},
                                        {TokenType::Identifier, "y"}});
  expect_parse_failure(no_types_lambda);

  // Missing fat arrow
  auto missing_arrow = create_tokens({{TokenType::LeftParen, "("},
                                      {TokenType::Let, "let"},
                                      {TokenType::Identifier, "x"},
                                      {TokenType::Colon, ":"},
                                      {TokenType::Identifier, "Int"},
                                      {TokenType::RightParen, ")"},
                                      {TokenType::Identifier, "x"}});
  expect_parse_failure(missing_arrow);

  // Missing expression body
  auto missing_body = create_tokens({{TokenType::LeftParen, "("},
                                     {TokenType::Let, "let"},
                                     {TokenType::Identifier, "x"},
                                     {TokenType::Colon, ":"},
                                     {TokenType::Identifier, "Int"},
                                     {TokenType::RightParen, ")"},
                                     {TokenType::FatArrow, "=>"}});
  expect_parse_failure(missing_body);
}

} // namespace nugdev::test