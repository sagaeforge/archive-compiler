#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class ComplexExpressionsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }
};

// BlockExpression Tests
TEST_F(ComplexExpressionsTest, BlockExpressionEmpty) {
  auto block = std::make_unique<BlockExpression>();

  ASSERT_NE(block, nullptr);
  EXPECT_EQ(block->get_node_type(), NodeType::BLOCK_EXPRESSION);
  EXPECT_EQ(block->get_statements().size(), 0);
  EXPECT_TRUE(block->is_empty());
}

TEST_F(ComplexExpressionsTest, BlockExpressionWithStatements) {
  auto block = std::make_unique<BlockExpression>();

  // Add a variable declaration
  auto type = std::make_unique<SimpleType>("int");
  auto varDecl = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "x", std::move(type));

  block->add_statement(std::move(varDecl));

  EXPECT_EQ(block->get_statements().size(), 1);
  EXPECT_FALSE(block->is_empty());
}

TEST_F(ComplexExpressionsTest, BlockExpressionWithReturnValue) {
  auto block = std::make_unique<BlockExpression>();

  auto returnValue = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  block->set_result_expression(std::move(returnValue));

  EXPECT_TRUE(block->has_result_expression());
  EXPECT_EQ(block->get_result_expression()->get_node_type(),
            NodeType::NUMBER_LITERAL);
}

// IfExpression Tests
TEST_F(ComplexExpressionsTest, IfExpressionBasic) {
  auto condition = std::make_unique<BooleanLiteral>(true);
  auto thenBlock = std::make_unique<BlockExpression>();

  auto ifExpr = std::make_unique<IfExpression>(std::move(condition),
                                               std::move(thenBlock));

  ASSERT_NE(ifExpr, nullptr);
  EXPECT_EQ(ifExpr->get_node_type(), NodeType::IF_EXPRESSION);
  EXPECT_EQ(ifExpr->get_expression_type(), "if");
  EXPECT_FALSE(ifExpr->has_else_block());
}

TEST_F(ComplexExpressionsTest, IfExpressionWithElse) {
  auto condition = std::make_unique<BooleanLiteral>(false);
  auto thenBlock = std::make_unique<BlockExpression>();
  auto elseBlock = std::make_unique<BlockExpression>();

  auto ifExpr = std::make_unique<IfExpression>(std::move(condition),
                                               std::move(thenBlock));
  ifExpr->set_else_block(std::move(elseBlock));

  EXPECT_TRUE(ifExpr->has_else_block());
  EXPECT_NE(ifExpr->get_else_block(), nullptr);
}

TEST_F(ComplexExpressionsTest, IfExpressionConditionAccess) {
  auto condition = std::make_unique<Identifier>("isValid");
  auto thenBlock = std::make_unique<BlockExpression>();

  auto ifExpr = std::make_unique<IfExpression>(std::move(condition),
                                               std::move(thenBlock));

  EXPECT_EQ(ifExpr->get_condition().get_node_type(), NodeType::IDENTIFIER);
}

// WhenExpression Tests
TEST_F(ComplexExpressionsTest, WhenExpressionBasic) {
  auto scrutinee = std::make_unique<Identifier>("value");
  auto whenExpr = std::make_unique<WhenExpression>(std::move(scrutinee));

  ASSERT_NE(whenExpr, nullptr);
  EXPECT_EQ(whenExpr->get_node_type(), NodeType::WHEN_EXPRESSION);
  EXPECT_EQ(whenExpr->get_expression_type(), "when");
  EXPECT_EQ(whenExpr->get_branches().size(), 0);
}

TEST_F(ComplexExpressionsTest, WhenExpressionWithBranches) {
  auto scrutinee = std::make_unique<NumberLiteral>(
      "5", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto whenExpr = std::make_unique<WhenExpression>(std::move(scrutinee));

  // Add a branch
  auto condition =
      std::make_unique<ValueCondition>(std::make_unique<NumberLiteral>(
          "5", NumberLiteral::NumberType::DECIMAL_INTEGER));
  auto action = std::make_unique<BlockExpression>();
  whenExpr->add_branch(std::move(condition), std::move(action));

  EXPECT_EQ(whenExpr->get_branches().size(), 1);
}

TEST_F(ComplexExpressionsTest, WhenExpressionWithElse) {
  auto scrutinee = std::make_unique<Identifier>("x");
  auto whenExpr = std::make_unique<WhenExpression>(std::move(scrutinee));

  auto elseAction = std::make_unique<BlockExpression>();
  whenExpr->set_else_branch(std::move(elseAction));

  EXPECT_TRUE(whenExpr->has_else_branch());
  EXPECT_NE(whenExpr->get_else_branch(), nullptr);
}

// FunctionExpression Tests
TEST_F(ComplexExpressionsTest, FunctionExpressionBasic) {
  std::vector<std::unique_ptr<Parameter>> params;
  auto returnType = std::make_unique<SimpleType>("int");

  auto funcExpr = std::make_unique<FunctionExpression>(std::move(params),
                                                       std::move(returnType));

  ASSERT_NE(funcExpr, nullptr);
  EXPECT_EQ(funcExpr->get_node_type(), NodeType::FUNCTION_EXPRESSION);
  EXPECT_EQ(funcExpr->get_expression_type(), "function");
  EXPECT_EQ(funcExpr->get_parameters().size(), 0);
}

TEST_F(ComplexExpressionsTest, FunctionExpressionWithParameters) {
  std::vector<std::unique_ptr<Parameter>> params;

  // Add parameter
  auto paramType = std::make_unique<SimpleType>("int");
  params.push_back(std::make_unique<Parameter>(Parameter::Mutability::LET, "x",
                                               std::move(paramType)));

  auto returnType = std::make_unique<SimpleType>("bool");

  auto funcExpr = std::make_unique<FunctionExpression>(std::move(params),
                                                       std::move(returnType));

  EXPECT_EQ(funcExpr->get_parameters().size(), 1);
  EXPECT_EQ(funcExpr->get_parameters()[0]->get_parameter_name(), "x");
}

// LambdaExpression Tests
TEST_F(ComplexExpressionsTest, LambdaExpressionBasic) {
  std::vector<std::unique_ptr<Parameter>> params;

  auto lambda = std::make_unique<LambdaExpression>(std::move(params));

  ASSERT_NE(lambda, nullptr);
  EXPECT_EQ(lambda->get_node_type(), NodeType::LAMBDA_EXPRESSION);
  EXPECT_EQ(lambda->get_expression_type(), "lambda");
  EXPECT_EQ(lambda->get_parameters().size(), 0);
}

TEST_F(ComplexExpressionsTest, LambdaExpressionWithReturnType) {
  std::vector<std::unique_ptr<Parameter>> params;

  auto lambda = std::make_unique<LambdaExpression>(std::move(params));

  // Set return type
  auto returnType = std::make_unique<SimpleType>("int");
  lambda->set_return_type(std::move(returnType));

  EXPECT_TRUE(lambda->has_return_type());
}

// Visitor Pattern Tests
TEST_F(ComplexExpressionsTest, ComplexExpressionsAcceptVisitor) {
  class TestVisitor : public DefaultASTVisitor {
  public:
    int blockExpressionCount = 0;
    int ifExpressionCount = 0;
    int whenExpressionCount = 0;
    int functionExpressionCount = 0;
    int lambdaExpressionCount = 0;

    void visit(BlockExpression &) override { blockExpressionCount++; }
    void visit(IfExpression &) override { ifExpressionCount++; }
    void visit(WhenExpression &) override { whenExpressionCount++; }
    void visit(FunctionExpression &) override { functionExpressionCount++; }
    void visit(LambdaExpression &) override { lambdaExpressionCount++; }
  };

  TestVisitor visitor;

  // Test BlockExpression
  auto block = std::make_unique<BlockExpression>();
  block->accept(visitor);

  // Test IfExpression
  auto condition = std::make_unique<BooleanLiteral>(true);
  auto thenBlock = std::make_unique<BlockExpression>();
  auto ifExpr = std::make_unique<IfExpression>(std::move(condition),
                                               std::move(thenBlock));
  ifExpr->accept(visitor);

  // Test WhenExpression
  auto scrutinee = std::make_unique<Identifier>("x");
  auto whenExpr = std::make_unique<WhenExpression>(std::move(scrutinee));
  whenExpr->accept(visitor);

  EXPECT_EQ(visitor.blockExpressionCount, 1);
  EXPECT_EQ(visitor.ifExpressionCount, 1);
  EXPECT_EQ(visitor.whenExpressionCount, 1);
}