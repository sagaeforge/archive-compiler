#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

// Test visitor class that counts nodes
class CountingVisitor : public DefaultASTVisitor {
public:
  int totalNodes = 0;
  int literalNodes = 0;
  int expressionNodes = 0;
  int identifierNodes = 0;

  // Literal visitors
  void visit(NumberLiteral &node [[maybe_unused]]) override {
    totalNodes++;
    literalNodes++;
  }

  void visit(StringLiteral &node [[maybe_unused]]) override {
    totalNodes++;
    literalNodes++;
  }

  void visit(BooleanLiteral &node [[maybe_unused]]) override {
    totalNodes++;
    literalNodes++;
  }

  void visit(CharacterLiteral &node [[maybe_unused]]) override {
    totalNodes++;
    literalNodes++;
  }

  void visit(NullLiteral &node [[maybe_unused]]) override {
    totalNodes++;
    literalNodes++;
  }

  void visit(NoneLiteral &node [[maybe_unused]]) override {
    totalNodes++;
    literalNodes++;
  }

  void visit(ArrayLiteral &node) override {
    totalNodes++;
    literalNodes++;
    for (const auto &element : node.get_elements()) {
      element->accept(*this);
    }
  }

  void visit(ObjectLiteral &node) override {
    totalNodes++;
    literalNodes++;
    for (const auto &property : node.get_properties()) {
      property->accept(*this);
    }
  }

  void visit(ObjectProperty &node) override {
    totalNodes++;
    if (node.get_key()) {
      node.get_key()->accept(*this);
    }
    if (node.get_value()) {
      node.get_value()->accept(*this);
    }
  }

  void visit(RangeLiteral &node) override {
    totalNodes++;
    literalNodes++;
    node.get_start().accept(*this);
    if (node.has_end()) {
      node.get_end()->accept(*this);
    }
  }

  // Expression visitors
  void visit(Identifier &node [[maybe_unused]]) override {
    totalNodes++;
    expressionNodes++;
    identifierNodes++;
  }

  void visit(BinaryExpression &node) override {
    totalNodes++;
    expressionNodes++;
    node.get_left().accept(*this);
    node.get_right().accept(*this);
  }

  void visit(UnaryExpression &node) override {
    totalNodes++;
    expressionNodes++;
    node.get_operand().accept(*this);
  }

  void visit(PostfixExpression &node) override {
    totalNodes++;
    expressionNodes++;
    node.get_operand().accept(*this);

    // Visit additional expressions based on operator type
    if (node.get_index_expression()) {
      node.get_index_expression()->accept(*this);
    }
    for (const auto &arg : node.get_arguments()) {
      arg->accept(*this);
    }
  }

  void visit(TernaryExpression &node) override {
    totalNodes++;
    expressionNodes++;
    node.get_condition().accept(*this);
    node.get_true_expression().accept(*this);
    node.get_false_expression().accept(*this);
  }

  void visit(AssignmentExpression &node) override {
    totalNodes++;
    expressionNodes++;
    node.get_left().accept(*this);
    node.get_right().accept(*this);
  }

  void reset() {
    totalNodes = 0;
    literalNodes = 0;
    expressionNodes = 0;
    identifierNodes = 0;
  }
};

class VisitorTest : public ::testing::Test {
protected:
  void SetUp() override { visitor.reset(); }

  CountingVisitor visitor;
};

TEST_F(VisitorTest, VisitNumberLiteral) {
  auto number = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  number->accept(visitor);

  EXPECT_EQ(visitor.totalNodes, 1);
  EXPECT_EQ(visitor.literalNodes, 1);
  EXPECT_EQ(visitor.expressionNodes, 0);
}

TEST_F(VisitorTest, VisitStringLiteral) {
  auto str = std::make_unique<StringLiteral>("Hello",
                                             StringLiteral::StringType::SIMPLE);
  str->accept(visitor);

  EXPECT_EQ(visitor.totalNodes, 1);
  EXPECT_EQ(visitor.literalNodes, 1);
}

TEST_F(VisitorTest, VisitBooleanLiteral) {
  auto boolean = std::make_unique<BooleanLiteral>(true);
  boolean->accept(visitor);

  EXPECT_EQ(visitor.totalNodes, 1);
  EXPECT_EQ(visitor.literalNodes, 1);
}

TEST_F(VisitorTest, VisitIdentifier) {
  auto ident = std::make_unique<Identifier>("variable");
  ident->accept(visitor);

  EXPECT_EQ(visitor.totalNodes, 1);
  EXPECT_EQ(visitor.expressionNodes, 1);
  EXPECT_EQ(visitor.identifierNodes, 1);
}

TEST_F(VisitorTest, VisitBinaryExpression) {
  auto left = std::make_unique<NumberLiteral>(
      "5", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto right = std::make_unique<NumberLiteral>(
      "3", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto binExpr = std::make_unique<BinaryExpression>(
      BinaryExpression::Operator::ADD, std::move(left), std::move(right));

  binExpr->accept(visitor);

  // BinaryExpression + 2 NumberLiterals = 3 total nodes
  EXPECT_EQ(visitor.totalNodes, 3);
  EXPECT_EQ(visitor.expressionNodes, 1); // Only BinaryExpression
  EXPECT_EQ(visitor.literalNodes, 2);    // Two NumberLiterals
}

TEST_F(VisitorTest, VisitUnaryExpression) {
  auto operand = std::make_unique<Identifier>("variable");
  auto unaryExpr = std::make_unique<UnaryExpression>(
      UnaryExpression::Operator::MINUS, std::move(operand));

  unaryExpr->accept(visitor);

  // UnaryExpression + Identifier = 2 total nodes
  EXPECT_EQ(visitor.totalNodes, 2);
  EXPECT_EQ(visitor.expressionNodes, 2); // UnaryExpression + Identifier
  EXPECT_EQ(visitor.identifierNodes, 1);
}

TEST_F(VisitorTest, VisitArrayLiteral) {
  std::vector<std::unique_ptr<Expression>> elements;
  elements.push_back(std::make_unique<NumberLiteral>(
      "1", NumberLiteral::NumberType::DECIMAL_INTEGER));
  elements.push_back(std::make_unique<NumberLiteral>(
      "2", NumberLiteral::NumberType::DECIMAL_INTEGER));
  elements.push_back(std::make_unique<Identifier>("x"));

  auto array = std::make_unique<ArrayLiteral>(std::move(elements));
  array->accept(visitor);

  // ArrayLiteral + 2 NumberLiterals + 1 Identifier = 4 total nodes
  EXPECT_EQ(visitor.totalNodes, 4);
  EXPECT_EQ(visitor.literalNodes, 3);    // ArrayLiteral + 2 NumberLiterals
  EXPECT_EQ(visitor.expressionNodes, 1); // 1 Identifier
  EXPECT_EQ(visitor.identifierNodes, 1);
}

TEST_F(VisitorTest, VisitObjectLiteral) {
  std::vector<std::unique_ptr<ObjectProperty>> properties;

  auto key = std::make_unique<Identifier>("name");
  auto value = std::make_unique<StringLiteral>(
      "Alice", StringLiteral::StringType::SIMPLE);
  properties.push_back(std::make_unique<ObjectProperty>(
      ObjectProperty::PropertyType::NORMAL, std::move(key), std::move(value)));

  auto object = std::make_unique<ObjectLiteral>(std::move(properties));
  object->accept(visitor);

  // ObjectLiteral + ObjectProperty + Identifier + StringLiteral = 4 total nodes
  EXPECT_EQ(visitor.totalNodes, 4);
  EXPECT_EQ(visitor.literalNodes, 2);    // ObjectLiteral + StringLiteral
  EXPECT_EQ(visitor.expressionNodes, 1); // Identifier
  EXPECT_EQ(visitor.identifierNodes, 1);
}

TEST_F(VisitorTest, VisitComplexExpression) {
  // Create: (x + 5) > (y - 3)
  auto x = std::make_unique<Identifier>("x");
  auto five = std::make_unique<NumberLiteral>(
      "5", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto leftExpr = std::make_unique<BinaryExpression>(
      BinaryExpression::Operator::ADD, std::move(x), std::move(five));

  auto y = std::make_unique<Identifier>("y");
  auto three = std::make_unique<NumberLiteral>(
      "3", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto rightExpr = std::make_unique<BinaryExpression>(
      BinaryExpression::Operator::SUBTRACT, std::move(y), std::move(three));

  auto comparison = std::make_unique<BinaryExpression>(
      BinaryExpression::Operator::GREATER_THAN, std::move(leftExpr),
      std::move(rightExpr));

  comparison->accept(visitor);

  // 3 BinaryExpressions + 2 Identifiers + 2 NumberLiterals = 7 total nodes
  EXPECT_EQ(visitor.totalNodes, 7);
  EXPECT_EQ(visitor.expressionNodes, 5); // 3 BinaryExpressions + 2 Identifiers
  EXPECT_EQ(visitor.literalNodes, 2);    // 2 NumberLiterals
  EXPECT_EQ(visitor.identifierNodes, 2);
}

TEST_F(VisitorTest, VisitTernaryExpression) {
  auto condition = std::make_unique<BooleanLiteral>(true);
  auto trueExpr = std::make_unique<NumberLiteral>(
      "1", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto falseExpr = std::make_unique<NumberLiteral>(
      "0", NumberLiteral::NumberType::DECIMAL_INTEGER);

  auto ternary = std::make_unique<TernaryExpression>(
      std::move(condition), std::move(trueExpr), std::move(falseExpr));

  ternary->accept(visitor);

  // TernaryExpression + BooleanLiteral + 2 NumberLiterals = 4 total nodes
  EXPECT_EQ(visitor.totalNodes, 4);
  EXPECT_EQ(visitor.expressionNodes, 1); // TernaryExpression
  EXPECT_EQ(visitor.literalNodes, 3);    // 1 BooleanLiteral + 2 NumberLiterals
}