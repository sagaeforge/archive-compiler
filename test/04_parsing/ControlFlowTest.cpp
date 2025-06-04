#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class ControlFlowTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }
};

// IfStatement Tests
TEST_F(ControlFlowTest, IfStatementBasic) {
  auto ifStmt = std::make_unique<IfStatement>();

  ASSERT_NE(ifStmt, nullptr);
  EXPECT_EQ(ifStmt->get_node_type(), NodeType::IF_STATEMENT);
  EXPECT_EQ(ifStmt->get_branches().size(), 0);
}

TEST_F(ControlFlowTest, IfStatementWithBranches) {
  auto ifStmt = std::make_unique<IfStatement>();

  // Add if branch
  auto condition = std::make_unique<BooleanLiteral>(true);
  auto block = std::make_unique<BlockExpression>();
  ifStmt->add_if_branch(std::move(condition), std::move(block));

  EXPECT_EQ(ifStmt->get_branches().size(), 1);
}

TEST_F(ControlFlowTest, IfStatementWithElseIf) {
  auto ifStmt = std::make_unique<IfStatement>();

  // Add if branch
  auto condition1 = std::make_unique<BooleanLiteral>(false);
  auto block1 = std::make_unique<BlockExpression>();
  ifStmt->add_if_branch(std::move(condition1), std::move(block1));

  // Add else if branch
  auto condition2 = std::make_unique<BooleanLiteral>(true);
  auto block2 = std::make_unique<BlockExpression>();
  ifStmt->add_else_if_branch(std::move(condition2), std::move(block2));

  EXPECT_EQ(ifStmt->get_branches().size(), 2);
}

TEST_F(ControlFlowTest, IfStatementWithElse) {
  auto ifStmt = std::make_unique<IfStatement>();

  // Add if branch
  auto condition = std::make_unique<BooleanLiteral>(false);
  auto block1 = std::make_unique<BlockExpression>();
  ifStmt->add_if_branch(std::move(condition), std::move(block1));

  // Add else branch (condition is nullptr)
  auto block2 = std::make_unique<BlockExpression>();
  ifStmt->add_else_branch(std::move(block2));

  EXPECT_EQ(ifStmt->get_branches().size(), 2);
  EXPECT_EQ(ifStmt->get_branches()[1].condition,
            nullptr); // else branch has no condition
}

// ForStatement Tests
TEST_F(ControlFlowTest, ForStatementInfinite) {
  auto forStmt =
      std::make_unique<ForStatement>(ForStatement::ForType::INFINITE);

  ASSERT_NE(forStmt, nullptr);
  EXPECT_EQ(forStmt->get_node_type(), NodeType::FOR_STATEMENT);
  EXPECT_EQ(forStmt->get_for_type(), ForStatement::ForType::INFINITE);
  EXPECT_EQ(forStmt->get_for_type_string(), "infinite");
}

TEST_F(ControlFlowTest, ForStatementWhile) {
  auto forStmt = std::make_unique<ForStatement>(ForStatement::ForType::WHILE);

  auto condition = std::make_unique<BooleanLiteral>(true);
  forStmt->set_condition(std::move(condition));

  EXPECT_EQ(forStmt->get_for_type(), ForStatement::ForType::WHILE);
  EXPECT_EQ(forStmt->get_for_type_string(), "while");
  EXPECT_NE(forStmt->get_condition(), nullptr);
}

TEST_F(ControlFlowTest, ForStatementCStyle) {
  auto forStmt = std::make_unique<ForStatement>(ForStatement::ForType::C_STYLE);

  // Set init statement
  auto initType = std::make_unique<SimpleType>("int");
  auto initStmt = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "i", std::move(initType));
  forStmt->set_init_statement(std::move(initStmt));

  // Set condition
  auto condition = std::make_unique<BooleanLiteral>(true);
  forStmt->set_condition(std::move(condition));

  // Set increment
  auto increment = std::make_unique<Identifier>("i");
  forStmt->set_increment_expression(std::move(increment));

  EXPECT_EQ(forStmt->get_for_type(), ForStatement::ForType::C_STYLE);
  EXPECT_EQ(forStmt->get_for_type_string(), "c-style");
  EXPECT_NE(forStmt->get_init_statement(), nullptr);
  EXPECT_NE(forStmt->get_condition(), nullptr);
  EXPECT_NE(forStmt->get_increment_expression(), nullptr);
}

TEST_F(ControlFlowTest, ForStatementForIn) {
  auto forStmt = std::make_unique<ForStatement>(ForStatement::ForType::FOR_IN);

  forStmt->set_iterator_variable("item");
  auto iterable = std::make_unique<Identifier>("collection");
  forStmt->set_iterable_expression(std::move(iterable));

  EXPECT_EQ(forStmt->get_for_type(), ForStatement::ForType::FOR_IN);
  EXPECT_EQ(forStmt->get_for_type_string(), "for-in");
  EXPECT_EQ(forStmt->get_iterator_variable(), "item");
  EXPECT_NE(forStmt->get_iterable_expression(), nullptr);
}

TEST_F(ControlFlowTest, ForStatementWithBody) {
  auto forStmt =
      std::make_unique<ForStatement>(ForStatement::ForType::INFINITE);

  auto body = std::make_unique<BlockExpression>();
  forStmt->set_body(std::move(body));

  EXPECT_EQ(forStmt->get_body().get_node_type(), NodeType::BLOCK_EXPRESSION);
}

// BreakStatement Tests
TEST_F(ControlFlowTest, BreakStatementBasic) {
  auto breakStmt = std::make_unique<BreakStatement>();

  ASSERT_NE(breakStmt, nullptr);
  EXPECT_EQ(breakStmt->get_node_type(), NodeType::BREAK_STATEMENT);
  EXPECT_FALSE(breakStmt->has_target_label());
  EXPECT_EQ(breakStmt->to_string(), "BreakStatement()");
}

TEST_F(ControlFlowTest, BreakStatementWithLabel) {
  auto breakStmt = std::make_unique<BreakStatement>("outerLoop");

  EXPECT_TRUE(breakStmt->has_target_label());
  EXPECT_EQ(breakStmt->get_target_label(), "outerLoop");
  EXPECT_EQ(breakStmt->to_string(), "BreakStatement(@outerLoop)");
}

// ContinueStatement Tests
TEST_F(ControlFlowTest, ContinueStatementBasic) {
  auto continueStmt = std::make_unique<ContinueStatement>();

  ASSERT_NE(continueStmt, nullptr);
  EXPECT_EQ(continueStmt->get_node_type(), NodeType::CONTINUE_STATEMENT);
  EXPECT_FALSE(continueStmt->has_target_label());
  EXPECT_EQ(continueStmt->to_string(), "ContinueStatement()");
}

TEST_F(ControlFlowTest, ContinueStatementWithLabel) {
  auto continueStmt = std::make_unique<ContinueStatement>("innerLoop");

  EXPECT_TRUE(continueStmt->has_target_label());
  EXPECT_EQ(continueStmt->get_target_label(), "innerLoop");
  EXPECT_EQ(continueStmt->to_string(), "ContinueStatement(@innerLoop)");
}

// ReturnStatement Tests
TEST_F(ControlFlowTest, ReturnStatementBasic) {
  auto returnStmt = std::make_unique<ReturnStatement>();

  ASSERT_NE(returnStmt, nullptr);
  EXPECT_EQ(returnStmt->get_node_type(), NodeType::RETURN_STATEMENT);
  EXPECT_FALSE(returnStmt->has_return_value());
  EXPECT_FALSE(returnStmt->has_target_label());
}

TEST_F(ControlFlowTest, ReturnStatementWithValue) {
  auto returnValue = std::make_unique<NumberLiteral>(
      "42", NumberLiteral::NumberType::DECIMAL_INTEGER);
  auto returnStmt = std::make_unique<ReturnStatement>(std::move(returnValue));

  EXPECT_TRUE(returnStmt->has_return_value());
  EXPECT_EQ(returnStmt->get_return_value()->get_node_type(),
            NodeType::NUMBER_LITERAL);
}

TEST_F(ControlFlowTest, ReturnStatementWithLabel) {
  auto returnValue = std::make_unique<StringLiteral>(
      "success", StringLiteral::StringType::SIMPLE);
  auto returnStmt = std::make_unique<ReturnStatement>(std::move(returnValue),
                                                      "functionLabel");

  EXPECT_TRUE(returnStmt->has_return_value());
  EXPECT_TRUE(returnStmt->has_target_label());
  EXPECT_EQ(returnStmt->get_target_label(), "functionLabel");
}

// Visitor Pattern Tests
TEST_F(ControlFlowTest, ControlFlowAcceptVisitor) {
  class TestVisitor : public DefaultASTVisitor {
  public:
    int ifStatementCount = 0;
    int forStatementCount = 0;
    int breakStatementCount = 0;
    int continueStatementCount = 0;
    int returnStatementCount = 0;

    void visit(IfStatement &) override { ifStatementCount++; }
    void visit(ForStatement &) override { forStatementCount++; }
    void visit(BreakStatement &) override { breakStatementCount++; }
    void visit(ContinueStatement &) override { continueStatementCount++; }
    void visit(ReturnStatement &) override { returnStatementCount++; }
  };

  TestVisitor visitor;

  // Test IfStatement
  auto ifStmt = std::make_unique<IfStatement>();
  ifStmt->accept(visitor);

  // Test ForStatement
  auto forStmt =
      std::make_unique<ForStatement>(ForStatement::ForType::INFINITE);
  forStmt->accept(visitor);

  // Test BreakStatement
  auto breakStmt = std::make_unique<BreakStatement>();
  breakStmt->accept(visitor);

  // Test ContinueStatement
  auto continueStmt = std::make_unique<ContinueStatement>();
  continueStmt->accept(visitor);

  // Test ReturnStatement
  auto returnStmt = std::make_unique<ReturnStatement>();
  returnStmt->accept(visitor);

  EXPECT_EQ(visitor.ifStatementCount, 1);
  EXPECT_EQ(visitor.forStatementCount, 1);
  EXPECT_EQ(visitor.breakStatementCount, 1);
  EXPECT_EQ(visitor.continueStatementCount, 1);
  EXPECT_EQ(visitor.returnStatementCount, 1);
}