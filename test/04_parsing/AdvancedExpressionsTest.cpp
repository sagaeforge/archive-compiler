#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class AdvancedExpressionsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }
};

// CastExpression Tests
TEST_F(AdvancedExpressionsTest, CastExpressionUnsafe) {
  auto value = std::make_unique<Identifier>("someValue");
  auto targetType = std::make_unique<SimpleType>("int");
  auto castExpr =
      std::make_unique<CastExpression>(std::move(value), std::move(targetType),
                                       CastExpression::CastType::UNSAFE);

  ASSERT_NE(castExpr, nullptr);
  EXPECT_EQ(castExpr->get_node_type(), NodeType::CAST_EXPRESSION);
  EXPECT_EQ(castExpr->get_expression_type(), "cast");
  EXPECT_EQ(castExpr->get_cast_type(), CastExpression::CastType::UNSAFE);
  EXPECT_FALSE(castExpr->is_safe_cast());
  EXPECT_EQ(castExpr->to_string(),
            "CastExpression(Identifier(someValue) as SimpleType(int))");
}

TEST_F(AdvancedExpressionsTest, CastExpressionSafe) {
  auto value = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto targetType = std::make_unique<SimpleType>("float");
  auto castExpr = std::make_unique<CastExpression>(
      std::move(value), std::move(targetType), CastExpression::CastType::SAFE);

  EXPECT_EQ(castExpr->get_cast_type(), CastExpression::CastType::SAFE);
  EXPECT_TRUE(castExpr->is_safe_cast());
  EXPECT_EQ(castExpr->to_string(),
            "CastExpression(NumberLiteral(42) as? SimpleType(float))");
}

TEST_F(AdvancedExpressionsTest, CastExpressionComplexTypes) {
  auto value = std::make_unique<Identifier>("obj");
  auto targetType =
      std::make_unique<OptionalType>(std::make_unique<SimpleType>("String"));
  auto castExpr = std::make_unique<CastExpression>(
      std::move(value), std::move(targetType), CastExpression::CastType::SAFE);

  EXPECT_EQ(castExpr->get_expression().get_node_type(), NodeType::IDENTIFIER);
  EXPECT_EQ(castExpr->get_target_type().get_node_type(),
            NodeType::OPTIONAL_TYPE);
}

// ArrayComprehension Tests
TEST_F(AdvancedExpressionsTest, ArrayComprehensionBasic) {
  auto elementExpr = std::make_unique<Identifier>("x");
  auto iterableExpr = std::make_unique<Identifier>("numbers");
  auto comprehension = std::make_unique<ArrayComprehension>(
      std::move(elementExpr), "x", std::move(iterableExpr));

  ASSERT_NE(comprehension, nullptr);
  EXPECT_EQ(comprehension->get_node_type(), NodeType::ARRAY_COMPREHENSION);
  EXPECT_EQ(comprehension->get_expression_type(), "array_comprehension");
  EXPECT_EQ(comprehension->get_iterator_variable(), "x");
  EXPECT_FALSE(comprehension->has_filter());
  EXPECT_EQ(comprehension->get_filter_expression(), nullptr);
}

TEST_F(AdvancedExpressionsTest, ArrayComprehensionWithFilter) {
  // [x * 2 for x in numbers if x > 5]
  auto left = std::make_unique<Identifier>("x");
  auto right = std::make_unique<NumberLiteral>(
      "2", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto elementExpr = std::make_unique<BinaryExpression>(
      BinaryExpression::Operator::MULTIPLY, std::move(left), std::move(right));

  auto iterableExpr = std::make_unique<Identifier>("numbers");

  auto filterLeft = std::make_unique<Identifier>("x");
  auto filterRight = std::make_unique<NumberLiteral>(
      "5", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto filterExpr = std::make_unique<BinaryExpression>(
      BinaryExpression::Operator::GREATER_THAN, std::move(filterLeft),
      std::move(filterRight));

  auto comprehension = std::make_unique<ArrayComprehension>(
      std::move(elementExpr), "x", std::move(iterableExpr),
      std::move(filterExpr));

  EXPECT_TRUE(comprehension->has_filter());
  EXPECT_NE(comprehension->get_filter_expression(), nullptr);
  EXPECT_EQ(comprehension->get_filter_expression()->get_node_type(),
            NodeType::BINARY_EXPRESSION);
}

TEST_F(AdvancedExpressionsTest, ArrayComprehensionToString) {
  auto elementExpr = std::make_unique<Identifier>("item");
  auto iterableExpr = std::make_unique<Identifier>("collection");
  auto comprehension = std::make_unique<ArrayComprehension>(
      std::move(elementExpr), "item", std::move(iterableExpr));

  EXPECT_EQ(comprehension->to_string(), "ArrayComprehension([Identifier(item) "
                                        "for item in Identifier(collection)])");
}

TEST_F(AdvancedExpressionsTest, ArrayComprehensionToStringWithFilter) {
  auto elementExpr = std::make_unique<Identifier>("n");
  auto iterableExpr = std::make_unique<Identifier>("data");
  auto filterExpr = std::make_unique<BooleanLiteral>(true);
  auto comprehension = std::make_unique<ArrayComprehension>(
      std::move(elementExpr), "n", std::move(iterableExpr),
      std::move(filterExpr));

  EXPECT_EQ(comprehension->to_string(),
            "ArrayComprehension([Identifier(n) for n in Identifier(data) if "
            "BooleanLiteral(true)])");
}

TEST_F(AdvancedExpressionsTest, ArrayComprehensionProperties) {
  auto elementExpr = std::make_unique<StringLiteral>(
      "hello", StringLiteral::StringType::SIMPLE);
  auto iterableExpr = std::make_unique<Identifier>("items");
  auto comprehension = std::make_unique<ArrayComprehension>(
      std::move(elementExpr), "i", std::move(iterableExpr));

  EXPECT_EQ(comprehension->get_element_expression().get_node_type(),
            NodeType::STRING_LITERAL);
  EXPECT_EQ(comprehension->get_iterable_expression().get_node_type(),
            NodeType::IDENTIFIER);
  EXPECT_EQ(comprehension->get_iterator_variable(), "i");
}

// TemplateExpression Tests
TEST_F(AdvancedExpressionsTest, TemplateExpressionBasic) {
  auto innerExpr = std::make_unique<Identifier>("name");
  auto templateExpr =
      std::make_unique<TemplateExpression>(std::move(innerExpr));

  ASSERT_NE(templateExpr, nullptr);
  EXPECT_EQ(templateExpr->get_node_type(), NodeType::TEMPLATE_EXPRESSION);
  EXPECT_EQ(templateExpr->get_expression_type(), "template");
  EXPECT_EQ(templateExpr->to_string(),
            "TemplateExpression(${Identifier(name)})");
}

TEST_F(AdvancedExpressionsTest, TemplateExpressionWithExpression) {
  auto left = std::make_unique<Identifier>("count");
  auto right = std::make_unique<NumberLiteral>(
      "1", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto addition = std::make_unique<BinaryExpression>(
      BinaryExpression::Operator::ADD, std::move(left), std::move(right));

  auto templateExpr = std::make_unique<TemplateExpression>(std::move(addition));

  EXPECT_EQ(templateExpr->get_expression().get_node_type(),
            NodeType::BINARY_EXPRESSION);
}

TEST_F(AdvancedExpressionsTest, TemplateExpressionWithFunctionCall) {
  auto funcName = std::make_unique<Identifier>("format");

  // Create arguments vector instead of ArgumentList
  std::vector<std::unique_ptr<Expression>> args;
  args.push_back(std::make_unique<StringLiteral>(
      "%.2f", StringLiteral::StringType::SIMPLE));

  auto funcCall = std::make_unique<PostfixExpression>(
      PostfixExpression::OperatorType::FUNCTION_CALL, std::move(funcName));
  funcCall->set_arguments(std::move(args));

  auto templateExpr = std::make_unique<TemplateExpression>(std::move(funcCall));

  EXPECT_EQ(templateExpr->get_expression().get_node_type(),
            NodeType::POSTFIX_EXPRESSION);
}

// Complex Combinations
TEST_F(AdvancedExpressionsTest, CastInArrayComprehension) {
  // [x as int for x in strings]
  auto value = std::make_unique<Identifier>("x");
  auto targetType = std::make_unique<SimpleType>("int");
  auto castExpr =
      std::make_unique<CastExpression>(std::move(value), std::move(targetType),
                                       CastExpression::CastType::UNSAFE);

  auto iterableExpr = std::make_unique<Identifier>("strings");
  auto comprehension = std::make_unique<ArrayComprehension>(
      std::move(castExpr), "x", std::move(iterableExpr));

  EXPECT_EQ(comprehension->get_element_expression().get_node_type(),
            NodeType::CAST_EXPRESSION);
}

TEST_F(AdvancedExpressionsTest, TemplateInArrayComprehension) {
  // [`Hello ${name}` for name in names]
  auto nameExpr = std::make_unique<Identifier>("name");
  auto templateExpr = std::make_unique<TemplateExpression>(std::move(nameExpr));

  auto iterableExpr = std::make_unique<Identifier>("names");
  auto comprehension = std::make_unique<ArrayComprehension>(
      std::move(templateExpr), "name", std::move(iterableExpr));

  EXPECT_EQ(comprehension->get_element_expression().get_node_type(),
            NodeType::TEMPLATE_EXPRESSION);
}

// Visitor Pattern Tests
TEST_F(AdvancedExpressionsTest, AdvancedExpressionsAcceptVisitor) {
  class TestVisitor : public DefaultASTVisitor {
  public:
    int castExpressionCount = 0;
    int arrayComprehensionCount = 0;
    int templateExpressionCount = 0;

    void visit(CastExpression &) override { castExpressionCount++; }
    void visit(ArrayComprehension &) override { arrayComprehensionCount++; }
    void visit(TemplateExpression &) override { templateExpressionCount++; }
  };

  TestVisitor visitor;

  // Test CastExpression
  auto value = std::make_unique<Identifier>("x");
  auto targetType = std::make_unique<SimpleType>("string");
  auto castExpr = std::make_unique<CastExpression>(
      std::move(value), std::move(targetType), CastExpression::CastType::SAFE);
  castExpr->accept(visitor);

  // Test ArrayComprehension
  auto elementExpr = std::make_unique<Identifier>("item");
  auto iterableExpr = std::make_unique<Identifier>("list");
  auto comprehension = std::make_unique<ArrayComprehension>(
      std::move(elementExpr), "item", std::move(iterableExpr));
  comprehension->accept(visitor);

  // Test TemplateExpression
  auto innerExpr = std::make_unique<Identifier>("value");
  auto templateExpr =
      std::make_unique<TemplateExpression>(std::move(innerExpr));
  templateExpr->accept(visitor);

  EXPECT_EQ(visitor.castExpressionCount, 1);
  EXPECT_EQ(visitor.arrayComprehensionCount, 1);
  EXPECT_EQ(visitor.templateExpressionCount, 1);
}

// Edge Cases
TEST_F(AdvancedExpressionsTest, EmptyIteratorVariable) {
  auto elementExpr = std::make_unique<NumberLiteral>(
      "1", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto iterableExpr = std::make_unique<Identifier>("range");
  auto comprehension = std::make_unique<ArrayComprehension>(
      std::move(elementExpr), "", std::move(iterableExpr));

  EXPECT_EQ(comprehension->get_iterator_variable(), "");
}

TEST_F(AdvancedExpressionsTest, NestedTemplateExpressions) {
  auto innerExpr = std::make_unique<Identifier>("inner");
  auto innerTemplate =
      std::make_unique<TemplateExpression>(std::move(innerExpr));
  auto outerTemplate =
      std::make_unique<TemplateExpression>(std::move(innerTemplate));

  EXPECT_EQ(outerTemplate->get_expression().get_node_type(),
            NodeType::TEMPLATE_EXPRESSION);
}