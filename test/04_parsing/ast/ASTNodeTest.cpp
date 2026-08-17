#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class ASTNodeTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }
};

TEST_F(ASTNodeTest, NodeTypeTest) {
  auto identifier = std::make_unique<Identifier>("testVar");

  EXPECT_EQ(identifier->get_node_type(), NodeType::IDENTIFIER);
  EXPECT_EQ(identifier->get_name(), "testVar");
}

TEST_F(ASTNodeTest, NodeCastingTest) {
  auto identifier = std::make_unique<Identifier>("testVar");
  ASTNode *baseNode = identifier.get();

  // Test is<T>() method
  EXPECT_TRUE(baseNode->is<Identifier>());
  EXPECT_TRUE(baseNode->is<Expression>());
  EXPECT_FALSE(baseNode->is<NumberLiteral>());
  EXPECT_FALSE(baseNode->is<Statement>());

  // Test as<T>() method
  auto *identPtr = baseNode->as<Identifier>();
  EXPECT_NE(identPtr, nullptr);
  EXPECT_EQ(identPtr->get_name(), "testVar");

  auto *numberPtr = baseNode->as<NumberLiteral>();
  EXPECT_EQ(numberPtr, nullptr);
}

TEST_F(ASTNodeTest, ToStringTest) {
  auto identifier = std::make_unique<Identifier>("myVariable");
  EXPECT_EQ(identifier->to_string(), "Identifier(myVariable)");

  auto number = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  EXPECT_EQ(number->to_string(), "NumberLiteral(42)");
}