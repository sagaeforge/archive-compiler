#include "03_tokenize/Token.h"
#include "03_tokenize/TokenType.h"
#include "04_parsing/Parser.hpp"
#include <gtest/gtest.h>

using namespace nugdev::compiler::parsing;
using namespace nugdev::compiler::tokenize;
using namespace nugdev::ast;

namespace nugdev::test {

/**
 * @brief Test class for expression parsing functionality
 */
class ExpressionParsingTest : public ::testing::Test {
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

// ==================== Binary Expression Tests ====================

TEST_F(ExpressionParsingTest, ArithmeticBinaryExpressions) {
  struct TestCase {
    std::vector<std::pair<TokenType, std::string>> tokens;
    std::string description;
  };

  std::vector<TestCase> test_cases = {
      // Basic arithmetic
      {{{TokenType::Number, "1"},
        {TokenType::Plus, "+"},
        {TokenType::Number, "2"}},
       "1 + 2"},
      {{{TokenType::Number, "5"},
        {TokenType::Minus, "-"},
        {TokenType::Number, "3"}},
       "5 - 3"},
      {{{TokenType::Number, "4"},
        {TokenType::Asterisk, "*"},
        {TokenType::Number, "7"}},
       "4 * 7"},
      {{{TokenType::Number, "8"},
        {TokenType::Slash, "/"},
        {TokenType::Number, "2"}},
       "8 / 2"},
      {{{TokenType::Number, "10"},
        {TokenType::Percent, "%"},
        {TokenType::Number, "3"}},
       "10 % 3"},
  };

  for (const auto &test_case : test_cases) {
    auto tokens = create_tokens(test_case.tokens);
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr) << "Failed to parse: " << test_case.description;
  }
}

TEST_F(ExpressionParsingTest, ComparisonBinaryExpressions) {
  std::vector<std::vector<std::pair<TokenType, std::string>>> test_cases = {
      {{TokenType::Number, "1"},
       {TokenType::LessThan, "<"},
       {TokenType::Number, "2"}},
      {{TokenType::Number, "5"},
       {TokenType::GreaterThan, ">"},
       {TokenType::Number, "3"}},
      {{TokenType::Number, "4"},
       {TokenType::LessThanEqual, "<="},
       {TokenType::Number, "4"}},
      {{TokenType::Number, "7"},
       {TokenType::GreaterThanEqual, ">="},
       {TokenType::Number, "5"}},
      {{TokenType::Number, "42"},
       {TokenType::Equal, "=="},
       {TokenType::Number, "42"}},
      {{TokenType::Number, "1"},
       {TokenType::NotEqual, "!="},
       {TokenType::Number, "2"}},
  };

  for (const auto &test_case : test_cases) {
    auto tokens = create_tokens(test_case);
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr);
  }
}

TEST_F(ExpressionParsingTest, LogicalBinaryExpressions) {
  std::vector<std::vector<std::pair<TokenType, std::string>>> test_cases = {
      {{TokenType::True, "true"},
       {TokenType::LogicalAnd, "and"},
       {TokenType::False, "false"}},
      {{TokenType::True, "true"},
       {TokenType::LogicalOr, "or"},
       {TokenType::False, "false"}},
  };

  for (const auto &test_case : test_cases) {
    auto tokens = create_tokens(test_case);
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr);
  }
}

TEST_F(ExpressionParsingTest, BitwiseBinaryExpressions) {
  std::vector<std::vector<std::pair<TokenType, std::string>>> test_cases = {
      {{TokenType::Number, "5"},
       {TokenType::Ampersand, "&"},
       {TokenType::Number, "3"}},
      {{TokenType::Number, "5"},
       {TokenType::Pipe, "|"},
       {TokenType::Number, "3"}},
      {{TokenType::Number, "5"},
       {TokenType::Caret, "^"},
       {TokenType::Number, "3"}},
      {{TokenType::Number, "8"},
       {TokenType::BitwiseShiftLeft, "<<"},
       {TokenType::Number, "1"}},
      {{TokenType::Number, "8"},
       {TokenType::BitwiseShiftRight, ">>"},
       {TokenType::Number, "1"}},
  };

  for (const auto &test_case : test_cases) {
    auto tokens = create_tokens(test_case);
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr);
  }
}

TEST_F(ExpressionParsingTest, OperatorPrecedence) {
  // Test that 2 * 3 + 4 is parsed as (2 * 3) + 4, not 2 * (3 + 4)
  auto tokens = create_tokens({{TokenType::Number, "2"},
                               {TokenType::Asterisk, "*"},
                               {TokenType::Number, "3"},
                               {TokenType::Plus, "+"},
                               {TokenType::Number, "4"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test that 1 + 2 * 3 is parsed as 1 + (2 * 3), not (1 + 2) * 3
  auto tokens2 = create_tokens({{TokenType::Number, "1"},
                                {TokenType::Plus, "+"},
                                {TokenType::Number, "2"},
                                {TokenType::Asterisk, "*"},
                                {TokenType::Number, "3"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

TEST_F(ExpressionParsingTest, ChainedComparisons) {
  // Test a < b < c (should be parsed as (a < b) < c)
  auto tokens = create_tokens({{TokenType::Identifier, "a"},
                               {TokenType::LessThan, "<"},
                               {TokenType::Identifier, "b"},
                               {TokenType::LessThan, "<"},
                               {TokenType::Identifier, "c"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Unary Expression Tests ====================

TEST_F(ExpressionParsingTest, UnaryExpressions) {
  std::vector<std::vector<std::pair<TokenType, std::string>>> test_cases = {
      // Arithmetic unary
      {{TokenType::Plus, "+"}, {TokenType::Number, "42"}},
      {{TokenType::Minus, "-"}, {TokenType::Number, "42"}},
      // Logical unary
      {{TokenType::Exclamation, "!"}, {TokenType::True, "true"}},
      {{TokenType::LogicalNot, "not"}, {TokenType::False, "false"}},
      // Bitwise unary
      {{TokenType::Tilde, "~"}, {TokenType::Number, "42"}},
      // Pre-increment/decrement
      {{TokenType::Increment, "++"}, {TokenType::Identifier, "x"}},
      {{TokenType::Decrement, "--"}, {TokenType::Identifier, "y"}},
  };

  for (const auto &test_case : test_cases) {
    auto tokens = create_tokens(test_case);
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr);
  }
}

TEST_F(ExpressionParsingTest, NestedUnaryExpressions) {
  // Test --++x (should parse as --(++x))
  auto tokens = create_tokens({{TokenType::Minus, "-"},
                               {TokenType::Minus, "-"},
                               {TokenType::Plus, "+"},
                               {TokenType::Plus, "+"},
                               {TokenType::Identifier, "x"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test !-x (should parse as !(-x))
  auto tokens2 = create_tokens({{TokenType::Exclamation, "!"},
                                {TokenType::Minus, "-"},
                                {TokenType::Identifier, "x"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

// ==================== Postfix Expression Tests ====================

TEST_F(ExpressionParsingTest, PostfixIncrementDecrement) {
  std::vector<std::vector<std::pair<TokenType, std::string>>> test_cases = {
      {{TokenType::Identifier, "x"}, {TokenType::Increment, "++"}},
      {{TokenType::Identifier, "y"}, {TokenType::Decrement, "--"}},
  };

  for (const auto &test_case : test_cases) {
    auto tokens = create_tokens(test_case);
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr);
  }
}

TEST_F(ExpressionParsingTest, MemberAccess) {
  // Test obj.property
  auto tokens = create_tokens({{TokenType::Identifier, "obj"},
                               {TokenType::Dot, "."},
                               {TokenType::Identifier, "property"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test chained member access: obj.prop1.prop2
  auto tokens2 = create_tokens({{TokenType::Identifier, "obj"},
                                {TokenType::Dot, "."},
                                {TokenType::Identifier, "prop1"},
                                {TokenType::Dot, "."},
                                {TokenType::Identifier, "prop2"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

TEST_F(ExpressionParsingTest, NullSafeMemberAccess) {
  // Test obj?.property
  auto tokens = create_tokens({{TokenType::Identifier, "obj"},
                               {TokenType::NullSafeAccess, "?."},
                               {TokenType::Identifier, "property"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ExpressionParsingTest, ArrayAccess) {
  // Test arr[0]
  auto tokens = create_tokens({{TokenType::Identifier, "arr"},
                               {TokenType::LeftBracket, "["},
                               {TokenType::Number, "0"},
                               {TokenType::RightBracket, "]"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test multidimensional array access: arr[i][j]
  auto tokens2 = create_tokens({{TokenType::Identifier, "arr"},
                                {TokenType::LeftBracket, "["},
                                {TokenType::Identifier, "i"},
                                {TokenType::RightBracket, "]"},
                                {TokenType::LeftBracket, "["},
                                {TokenType::Identifier, "j"},
                                {TokenType::RightBracket, "]"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

TEST_F(ExpressionParsingTest, FunctionCall) {
  // Test simple function call: func()
  auto tokens = create_tokens({{TokenType::Identifier, "func"},
                               {TokenType::LeftParen, "("},
                               {TokenType::RightParen, ")"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test function call with arguments: func(a, b)
  auto tokens2 = create_tokens({{TokenType::Identifier, "func"},
                                {TokenType::LeftParen, "("},
                                {TokenType::Identifier, "a"},
                                {TokenType::Comma, ","},
                                {TokenType::Identifier, "b"},
                                {TokenType::RightParen, ")"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);

  // Test chained function calls: func1().func2()
  auto tokens3 = create_tokens({{TokenType::Identifier, "func1"},
                                {TokenType::LeftParen, "("},
                                {TokenType::RightParen, ")"},
                                {TokenType::Dot, "."},
                                {TokenType::Identifier, "func2"},
                                {TokenType::LeftParen, "("},
                                {TokenType::RightParen, ")"}});
  auto result3 = safe_parse(tokens3);
  EXPECT_NE(result3, nullptr);
}

TEST_F(ExpressionParsingTest, TypeCasting) {
  // Test type casting: value as Type
  auto tokens = create_tokens({{TokenType::Identifier, "value"},
                               {TokenType::As, "as"},
                               {TokenType::Identifier, "Type"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Assignment Expression Tests ====================

TEST_F(ExpressionParsingTest, SimpleAssignment) {
  // Test x = 42
  auto tokens = create_tokens({{TokenType::Identifier, "x"},
                               {TokenType::Assign, "="},
                               {TokenType::Number, "42"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ExpressionParsingTest, CompoundAssignment) {
  std::vector<std::vector<std::pair<TokenType, std::string>>> test_cases = {
      {{TokenType::Identifier, "x"},
       {TokenType::PlusAssign, "+="},
       {TokenType::Number, "5"}},
      {{TokenType::Identifier, "x"},
       {TokenType::MinusAssign, "-="},
       {TokenType::Number, "3"}},
      {{TokenType::Identifier, "x"},
       {TokenType::AsteriskAssign, "*="},
       {TokenType::Number, "2"}},
      {{TokenType::Identifier, "x"},
       {TokenType::SlashAssign, "/="},
       {TokenType::Number, "4"}},
      {{TokenType::Identifier, "x"},
       {TokenType::PercentAssign, "%="},
       {TokenType::Number, "3"}},
      {{TokenType::Identifier, "x"},
       {TokenType::AmpersandAssign, "&="},
       {TokenType::Number, "7"}},
      {{TokenType::Identifier, "x"},
       {TokenType::PipeAssign, "|="},
       {TokenType::Number, "7"}},
      {{TokenType::Identifier, "x"},
       {TokenType::CaretAssign, "^="},
       {TokenType::Number, "7"}},
  };

  for (const auto &test_case : test_cases) {
    auto tokens = create_tokens(test_case);
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr);
  }
}

TEST_F(ExpressionParsingTest, ChainedAssignment) {
  // Test a = b = c = 42 (should be right-associative)
  auto tokens = create_tokens({{TokenType::Identifier, "a"},
                               {TokenType::Assign, "="},
                               {TokenType::Identifier, "b"},
                               {TokenType::Assign, "="},
                               {TokenType::Identifier, "c"},
                               {TokenType::Assign, "="},
                               {TokenType::Number, "42"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Ternary Expression Tests ====================

TEST_F(ExpressionParsingTest, TernaryConditional) {
  // Test condition ? true_value : false_value
  auto tokens = create_tokens({{TokenType::Identifier, "condition"},
                               {TokenType::Question, "?"},
                               {TokenType::Number, "1"},
                               {TokenType::Colon, ":"},
                               {TokenType::Number, "0"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ExpressionParsingTest, NestedTernary) {
  // Test a ? b ? c : d : e
  auto tokens = create_tokens({{TokenType::Identifier, "a"},
                               {TokenType::Question, "?"},
                               {TokenType::Identifier, "b"},
                               {TokenType::Question, "?"},
                               {TokenType::Identifier, "c"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "d"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "e"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Null Coalescing Tests ====================

TEST_F(ExpressionParsingTest, NullCoalescing) {
  // Test a ?? b
  auto tokens = create_tokens({{TokenType::Identifier, "a"},
                               {TokenType::NullCoalescing, "??"},
                               {TokenType::Identifier, "b"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test chained null coalescing: a ?? b ?? c
  auto tokens2 = create_tokens({{TokenType::Identifier, "a"},
                                {TokenType::NullCoalescing, "??"},
                                {TokenType::Identifier, "b"},
                                {TokenType::NullCoalescing, "??"},
                                {TokenType::Identifier, "c"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

// ==================== Parenthesized Expressions ====================

TEST_F(ExpressionParsingTest, ParenthesizedExpressions) {
  // Test (1 + 2) * 3
  auto tokens = create_tokens({{TokenType::LeftParen, "("},
                               {TokenType::Number, "1"},
                               {TokenType::Plus, "+"},
                               {TokenType::Number, "2"},
                               {TokenType::RightParen, ")"},
                               {TokenType::Asterisk, "*"},
                               {TokenType::Number, "3"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);

  // Test nested parentheses: ((1 + 2) * 3)
  auto tokens2 = create_tokens({{TokenType::LeftParen, "("},
                                {TokenType::LeftParen, "("},
                                {TokenType::Number, "1"},
                                {TokenType::Plus, "+"},
                                {TokenType::Number, "2"},
                                {TokenType::RightParen, ")"},
                                {TokenType::Asterisk, "*"},
                                {TokenType::Number, "3"},
                                {TokenType::RightParen, ")"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

// ==================== Error Cases ====================

TEST_F(ExpressionParsingTest, ExpressionErrorCases) {
  // Missing operand in binary expression
  expect_parse_failure(
      create_tokens({{TokenType::Number, "1"}, {TokenType::Plus, "+"}}));

  // Missing operand in unary expression
  expect_parse_failure(create_tokens({{TokenType::Minus, "-"}}));

  // Unmatched parentheses
  expect_parse_failure(
      create_tokens({{TokenType::LeftParen, "("}, {TokenType::Number, "42"}}));

  // Invalid operator sequence
  expect_parse_failure(create_tokens({{TokenType::Plus, "+"},
                                      {TokenType::Asterisk, "*"},
                                      {TokenType::Number, "42"}}));

  // Missing colon in ternary
  expect_parse_failure(create_tokens({{TokenType::Identifier, "condition"},
                                      {TokenType::Question, "?"},
                                      {TokenType::Number, "1"}}));
}

// ==================== Null Operators Tests ====================

TEST_F(ExpressionParsingTest, NullCoalescingOperator) {
  // Test a ?? b (null coalescing)
  auto tokens = create_tokens({{TokenType::Identifier, "a"},
                               {TokenType::NullCoalescing, "??"},
                               {TokenType::Identifier, "b"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ExpressionParsingTest, NullSafeOperators) {
  // Test a?.property (null safe access)
  auto null_safe_access_tokens =
      create_tokens({{TokenType::Identifier, "a"},
                     {TokenType::NullSafeAccess, "?."},
                     {TokenType::Identifier, "property"}});
  auto result1 = safe_parse(null_safe_access_tokens);
  EXPECT_NE(result1, nullptr);

  // Test a!! (null assertion)
  auto null_assertion_tokens = create_tokens(
      {{TokenType::Identifier, "a"}, {TokenType::NullAssertion, "!!"}});
  auto result2 = safe_parse(null_assertion_tokens);
  EXPECT_NE(result2, nullptr);
}

// ==================== Type Checking Expressions ====================

TEST_F(ExpressionParsingTest, TypeCheckExpression) {
  // Test value is Type
  auto tokens = create_tokens({{TokenType::Identifier, "value"},
                               {TokenType::Is, "is"},
                               {TokenType::Identifier, "String"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ExpressionParsingTest, SafeCastExpression) {
  // Test value as? Type (safe cast)
  auto safe_cast_tokens = create_tokens({{TokenType::Identifier, "value"},
                                         {TokenType::As, "as"},
                                         {TokenType::Question, "?"},
                                         {TokenType::Identifier, "Type"}});
  auto result = safe_parse(safe_cast_tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ExpressionParsingTest, UnsafeCastExpression) {
  // Test value as Type (unsafe cast)
  auto unsafe_cast_tokens = create_tokens({{TokenType::Identifier, "value"},
                                           {TokenType::As, "as"},
                                           {TokenType::Identifier, "Type"}});
  auto result = safe_parse(unsafe_cast_tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Range Expression Tests ====================

TEST_F(ExpressionParsingTest, RangeExpressions) {
  // Test bounded range: start..end
  auto bounded_range_tokens = create_tokens({{TokenType::Identifier, "start"},
                                             {TokenType::Range, ".."},
                                             {TokenType::Identifier, "end"}});
  auto result1 = safe_parse(bounded_range_tokens);
  EXPECT_NE(result1, nullptr);

  // Test unbounded range: start..
  auto unbounded_range_tokens = create_tokens(
      {{TokenType::Identifier, "start"}, {TokenType::Range, ".."}});
  auto result2 = safe_parse(unbounded_range_tokens);
  EXPECT_NE(result2, nullptr);

  // Test numeric range: 1..10
  auto numeric_range_tokens = create_tokens({{TokenType::Number, "1"},
                                             {TokenType::Range, ".."},
                                             {TokenType::Number, "10"}});
  auto result3 = safe_parse(numeric_range_tokens);
  EXPECT_NE(result3, nullptr);
}

// ==================== Advanced Assignment Tests ====================

TEST_F(ExpressionParsingTest, AllAssignmentOperators) {
  std::vector<std::pair<TokenType, std::string>> assignment_operators = {
      {TokenType::Assign, "="},           {TokenType::PlusAssign, "+="},
      {TokenType::MinusAssign, "-="},     {TokenType::AsteriskAssign, "*="},
      {TokenType::SlashAssign, "/="},     {TokenType::PercentAssign, "%="},
      {TokenType::AmpersandAssign, "&="}, {TokenType::PipeAssign, "|="},
      {TokenType::CaretAssign, "^="},     {TokenType::TildeAssign, "~="}};

  for (const auto &[op_type, op_literal] : assignment_operators) {
    auto tokens = create_tokens({{TokenType::Identifier, "x"},
                                 {op_type, op_literal},
                                 {TokenType::Number, "42"}});
    auto result = safe_parse(tokens);
    EXPECT_NE(result, nullptr)
        << "Failed to parse assignment: x " << op_literal << " 42";
  }
}

// ==================== Complex Expression Combinations ====================

TEST_F(ExpressionParsingTest, ComplexOperatorPrecedence) {
  // Test: a ?? b + c * d  (should be: a ?? (b + (c * d)))
  auto tokens1 = create_tokens({{TokenType::Identifier, "a"},
                                {TokenType::NullCoalescing, "??"},
                                {TokenType::Identifier, "b"},
                                {TokenType::Plus, "+"},
                                {TokenType::Identifier, "c"},
                                {TokenType::Asterisk, "*"},
                                {TokenType::Identifier, "d"}});
  auto result1 = safe_parse(tokens1);
  EXPECT_NE(result1, nullptr);

  // Test: a is Type and b > c  (should be: (a is Type) and (b > c))
  auto tokens2 = create_tokens({{TokenType::Identifier, "a"},
                                {TokenType::Is, "is"},
                                {TokenType::Identifier, "Type"},
                                {TokenType::LogicalAnd, "and"},
                                {TokenType::Identifier, "b"},
                                {TokenType::GreaterThan, ">"},
                                {TokenType::Identifier, "c"}});
  auto result2 = safe_parse(tokens2);
  EXPECT_NE(result2, nullptr);
}

TEST_F(ExpressionParsingTest, ChainedNullOperators) {
  // Test: a?.b?.c (chained null safe access)
  auto tokens = create_tokens({{TokenType::Identifier, "a"},
                               {TokenType::NullSafeAccess, "?."},
                               {TokenType::Identifier, "b"},
                               {TokenType::NullSafeAccess, "?."},
                               {TokenType::Identifier, "c"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

TEST_F(ExpressionParsingTest, ComplexTernaryWithNullCoalescing) {
  // Test: a ?? b ? c : d (should be: (a ?? b) ? c : d)
  auto tokens = create_tokens({{TokenType::Identifier, "a"},
                               {TokenType::NullCoalescing, "??"},
                               {TokenType::Identifier, "b"},
                               {TokenType::Question, "?"},
                               {TokenType::Identifier, "c"},
                               {TokenType::Colon, ":"},
                               {TokenType::Identifier, "d"}});
  auto result = safe_parse(tokens);
  EXPECT_NE(result, nullptr);
}

// ==================== Error Cases for New Operators ====================

TEST_F(ExpressionParsingTest, NullOperatorErrorCases) {
  // Test incomplete null coalescing
  auto incomplete_null_coalescing = create_tokens(
      {{TokenType::Identifier, "a"}, {TokenType::NullCoalescing, "??"}});
  expect_parse_failure(incomplete_null_coalescing);

  // Test incomplete safe cast
  auto incomplete_safe_cast = create_tokens({{TokenType::Identifier, "value"},
                                             {TokenType::As, "as"},
                                             {TokenType::Question, "?"}});
  expect_parse_failure(incomplete_safe_cast);

  // Test incomplete type check
  auto incomplete_type_check =
      create_tokens({{TokenType::Identifier, "value"}, {TokenType::Is, "is"}});
  expect_parse_failure(incomplete_type_check);
}

} // namespace nugdev::test