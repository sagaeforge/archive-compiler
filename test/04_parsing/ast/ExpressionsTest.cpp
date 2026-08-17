#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class ExpressionsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }
};

// Identifier Tests
TEST_F(ExpressionsTest, IdentifierTest) {
  auto ident = std::make_unique<Identifier>("myVariable");

  EXPECT_EQ(ident->get_name(), "myVariable");
  EXPECT_EQ(ident->get_node_type(), NodeType::IDENTIFIER);
  EXPECT_EQ(ident->to_string(), "Identifier(myVariable)");
}

// BinaryExpression Tests
TEST_F(ExpressionsTest, BinaryExpressionAddition) {
  auto left = std::make_unique<NumberLiteral>(
      "5", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto right = std::make_unique<NumberLiteral>(
      "3", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto binExpr = std::make_unique<BinaryExpression>(
      BinaryExpression::Operator::ADD, std::move(left), std::move(right));

  EXPECT_EQ(binExpr->get_operator(), BinaryExpression::Operator::ADD);
  EXPECT_EQ(binExpr->get_node_type(), NodeType::BINARY_EXPRESSION);
  EXPECT_TRUE(binExpr->is_arithmetic_operator());
}

TEST_F(ExpressionsTest, BinaryExpressionComparison) {
  auto left = std::make_unique<Identifier>("x");
  auto right = std::make_unique<NumberLiteral>(
      "10", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto binExpr = std::make_unique<BinaryExpression>(
      BinaryExpression::Operator::LESS_THAN, std::move(left), std::move(right));

  EXPECT_EQ(binExpr->get_operator(), BinaryExpression::Operator::LESS_THAN);
  EXPECT_TRUE(binExpr->is_comparison_operator());
}

TEST_F(ExpressionsTest, BinaryExpressionLogical) {
  auto left = std::make_unique<BooleanLiteral>(true);
  auto right = std::make_unique<BooleanLiteral>(false);
  auto binExpr = std::make_unique<BinaryExpression>(
      BinaryExpression::Operator::LOGICAL_AND, std::move(left),
      std::move(right));

  EXPECT_EQ(binExpr->get_operator(), BinaryExpression::Operator::LOGICAL_AND);
  EXPECT_TRUE(binExpr->is_logical_operator());
}

// UnaryExpression Tests
TEST_F(ExpressionsTest, UnaryExpressionNegation) {
  auto operand = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto unaryExpr = std::make_unique<UnaryExpression>(
      UnaryExpression::Operator::MINUS, std::move(operand));

  EXPECT_EQ(unaryExpr->get_operator(), UnaryExpression::Operator::MINUS);
  EXPECT_EQ(unaryExpr->get_node_type(), NodeType::UNARY_EXPRESSION);
}

TEST_F(ExpressionsTest, UnaryExpressionLogicalNot) {
  auto operand = std::make_unique<BooleanLiteral>(true);
  auto unaryExpr = std::make_unique<UnaryExpression>(
      UnaryExpression::Operator::LOGICAL_NOT, std::move(operand));

  EXPECT_EQ(unaryExpr->get_operator(), UnaryExpression::Operator::LOGICAL_NOT);
}

TEST_F(ExpressionsTest, UnaryExpressionPreIncrement) {
  auto operand = std::make_unique<Identifier>("counter");
  auto unaryExpr = std::make_unique<UnaryExpression>(
      UnaryExpression::Operator::PRE_INCREMENT, std::move(operand));

  EXPECT_EQ(unaryExpr->get_operator(),
            UnaryExpression::Operator::PRE_INCREMENT);
}

// PostfixExpression Tests
TEST_F(ExpressionsTest, PostfixExpressionPostIncrement) {
  auto operand = std::make_unique<Identifier>("counter");
  auto postfixExpr = std::make_unique<PostfixExpression>(
      PostfixExpression::OperatorType::POST_INCREMENT, std::move(operand));

  EXPECT_EQ(postfixExpr->get_operator_type(),
            PostfixExpression::OperatorType::POST_INCREMENT);
  EXPECT_EQ(postfixExpr->get_node_type(), NodeType::POSTFIX_EXPRESSION);
}

TEST_F(ExpressionsTest, PostfixExpressionMemberAccess) {
  auto object = std::make_unique<Identifier>("obj");
  auto memberAccess = std::make_unique<PostfixExpression>(
      PostfixExpression::OperatorType::MEMBER_ACCESS, std::move(object));

  memberAccess->set_member_name("property");

  EXPECT_EQ(memberAccess->get_operator_type(),
            PostfixExpression::OperatorType::MEMBER_ACCESS);
  EXPECT_EQ(memberAccess->get_member_name(), "property");
}

TEST_F(ExpressionsTest, PostfixExpressionArrayAccess) {
  auto object = std::make_unique<Identifier>("array");
  auto arrayAccess = std::make_unique<PostfixExpression>(
      PostfixExpression::OperatorType::ARRAY_ACCESS, std::move(object));

  auto index = std::make_unique<NumberLiteral>(
      "0", NumberLiteral::NumberType::DECIMAL_INTEGER);
  arrayAccess->set_index_expression(std::move(index));

  EXPECT_EQ(arrayAccess->get_operator_type(),
            PostfixExpression::OperatorType::ARRAY_ACCESS);
  EXPECT_NE(arrayAccess->get_index_expression(), nullptr);
}

// AssignmentExpression Tests
TEST_F(ExpressionsTest, AssignmentExpressionSimple) {
  auto target = std::make_unique<Identifier>("x");
  auto value = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto assignment = std::make_unique<AssignmentExpression>(
      BinaryExpression::Operator::ASSIGN, std::move(target), std::move(value));

  EXPECT_EQ(assignment->get_operator(), BinaryExpression::Operator::ASSIGN);
  EXPECT_EQ(assignment->get_node_type(), NodeType::ASSIGNMENT_EXPRESSION);
}

TEST_F(ExpressionsTest, AssignmentExpressionCompound) {
  auto target = std::make_unique<Identifier>("x");
  auto value = std::make_unique<NumberLiteral>(
      "5", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto assignment = std::make_unique<AssignmentExpression>(
      BinaryExpression::Operator::ADD_ASSIGN, std::move(target),
      std::move(value));

  EXPECT_EQ(assignment->get_operator(), BinaryExpression::Operator::ADD_ASSIGN);
}

// TernaryExpression Tests
TEST_F(ExpressionsTest, TernaryExpression) {
  auto condition = std::make_unique<BooleanLiteral>(true);
  auto trueExpr = std::make_unique<NumberLiteral>(
      "1", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto falseExpr = std::make_unique<NumberLiteral>(
      "0", NumberLiteral::NumberType::DECIMAL_INTEGER);

  auto ternary = std::make_unique<TernaryExpression>(
      std::move(condition), std::move(trueExpr), std::move(falseExpr));

  EXPECT_EQ(ternary->get_node_type(), NodeType::TERNARY_EXPRESSION);
}