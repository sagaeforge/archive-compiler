#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class StatementsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }
};

// VariableDeclaration Tests
TEST_F(StatementsTest, VariableDeclarationLet) {
  auto type = std::make_unique<SimpleType>("int");
  auto varDecl = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "x", std::move(type));

  ASSERT_NE(varDecl, nullptr);
  EXPECT_EQ(varDecl->get_node_type(), NodeType::VARIABLE_DECLARATION);
  EXPECT_EQ(varDecl->get_mutability(), VariableDeclaration::Mutability::LET);
  EXPECT_EQ(varDecl->get_variable_name(), "x");
  EXPECT_FALSE(varDecl->has_initializer());
}

TEST_F(StatementsTest, VariableDeclarationWithInitializer) {
  auto type = std::make_unique<SimpleType>("int");
  auto varDecl = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "x", std::move(type));

  auto initializer = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  varDecl->set_initializer(std::move(initializer));

  EXPECT_TRUE(varDecl->has_initializer());
  EXPECT_EQ(varDecl->get_initializer()->get_node_type(),
            NodeType::NUMBER_LITERAL);
}

// FunctionDeclaration Tests
TEST_F(StatementsTest, FunctionDeclarationBasic) {
  std::vector<std::unique_ptr<Parameter>> params;
  auto returnType = std::make_unique<SimpleType>("void");
  auto funcDecl = std::make_unique<FunctionDeclaration>(
      "myFunction", std::move(params), std::move(returnType));

  ASSERT_NE(funcDecl, nullptr);
  EXPECT_EQ(funcDecl->get_node_type(), NodeType::FUNCTION_DECLARATION);
  EXPECT_EQ(funcDecl->get_function_name(), "myFunction");
  EXPECT_EQ(funcDecl->get_parameters().size(), 0);
}

// ExpressionStatement Tests
TEST_F(StatementsTest, ExpressionStatementBasic) {
  auto expr = std::make_unique<Identifier>("someFunction");
  auto exprStmt = std::make_unique<ExpressionStatement>(std::move(expr));

  ASSERT_NE(exprStmt, nullptr);
  EXPECT_EQ(exprStmt->get_node_type(), NodeType::EXPRESSION_STATEMENT);
  EXPECT_EQ(exprStmt->get_expression().get_node_type(), NodeType::IDENTIFIER);
}

// Visitor Pattern Tests
TEST_F(StatementsTest, StatementsAcceptVisitor) {
  class TestVisitor : public DefaultASTVisitor {
  public:
    int variableDeclarationCount = 0;
    int functionDeclarationCount = 0;
    int expressionStatementCount = 0;

    void visit(VariableDeclaration &) override { variableDeclarationCount++; }
    void visit(FunctionDeclaration &) override { functionDeclarationCount++; }
    void visit(ExpressionStatement &) override { expressionStatementCount++; }
  };

  TestVisitor visitor;

  // Test VariableDeclaration
  auto type = std::make_unique<SimpleType>("int");
  auto varDecl = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "x", std::move(type));
  varDecl->accept(visitor);

  // Test FunctionDeclaration
  std::vector<std::unique_ptr<Parameter>> params;
  auto returnType = std::make_unique<SimpleType>("void");
  auto funcDecl = std::make_unique<FunctionDeclaration>(
      "test", std::move(params), std::move(returnType));
  funcDecl->accept(visitor);

  // Test ExpressionStatement
  auto expr = std::make_unique<Identifier>("variable");
  auto exprStmt = std::make_unique<ExpressionStatement>(std::move(expr));
  exprStmt->accept(visitor);

  EXPECT_EQ(visitor.variableDeclarationCount, 1);
  EXPECT_EQ(visitor.functionDeclarationCount, 1);
  EXPECT_EQ(visitor.expressionStatementCount, 1);
}