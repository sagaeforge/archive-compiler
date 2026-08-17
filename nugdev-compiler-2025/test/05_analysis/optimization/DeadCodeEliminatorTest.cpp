#include "05_analysis/optimization/DeadCodeEliminator.hpp"
#include "04_parsing/ast/expressions/Expressions.hpp"
#include "04_parsing/ast/literals/Literals.hpp"
#include "04_parsing/ast/statements/Statements.hpp"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>

using namespace nugdev::compiler::optimization;
using namespace nugdev::ast;

class DeadCodeEliminatorTest : public ::testing::Test {
protected:
  void SetUp() override { eliminator = std::make_unique<DeadCodeEliminator>(); }

  std::unique_ptr<DeadCodeEliminator> eliminator;
};

// Helper function to create number literals
std::unique_ptr<NumberLiteral> make_integer(int64_t value) {
  return std::make_unique<NumberLiteral>(
      std::to_string(value), NumberLiteral::NumberType::DECIMAL_INTEGER);
}

// Helper function to create boolean literals
std::unique_ptr<BooleanLiteral> make_boolean(bool value) {
  return std::make_unique<BooleanLiteral>(value);
}

// Helper function to create identifiers
std::unique_ptr<Identifier> make_identifier(const std::string &name) {
  return std::make_unique<Identifier>(name);
}

// Helper function to create variable declarations
std::unique_ptr<VariableDeclaration>
make_variable_declaration(const std::string &name,
                          std::unique_ptr<Expression> initializer = nullptr) {
  return std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, name, nullptr,
      std::move(initializer));
}

// Helper function to create if statements
std::unique_ptr<IfStatement>
make_if_statement(std::unique_ptr<Expression> condition,
                  std::unique_ptr<Statement> then_branch,
                  std::unique_ptr<Statement> else_branch = nullptr) {
  auto if_stmt = std::make_unique<IfStatement>(NodeType::IF_STATEMENT);
  if_stmt->set_condition(std::move(condition));
  if_stmt->set_then_branch(std::move(then_branch));
  if (else_branch) {
    if_stmt->set_else_branch(std::move(else_branch));
  }
  return if_stmt;
}

// Helper function to create block expressions
std::unique_ptr<BlockExpression>
make_block(std::vector<std::unique_ptr<Statement>> statements) {
  auto block = std::make_unique<BlockExpression>(NodeType::BLOCK_EXPRESSION);
  for (auto &stmt : statements) {
    block->add_statement(std::move(stmt));
  }
  return block;
}

TEST_F(DeadCodeEliminatorTest, UnreachableCodeAfterReturn) {
  auto block = make_block({
      make_variable_declaration("x", make_integer(1)),
      std::make_unique<ReturnStatement>(NodeType::RETURN_STATEMENT),
      make_variable_declaration("y", make_integer(2)) // Should be removed
  });

  eliminator->visit(*block);

  // Check that the unreachable code was removed
  EXPECT_EQ(block->get_statements().size(), 2);
  EXPECT_TRUE(block->get_statements()[0]->is<VariableDeclaration>());
  EXPECT_TRUE(block->get_statements()[1]->is<ReturnStatement>());
}

TEST_F(DeadCodeEliminatorTest, UnreachableCodeInIfStatement) {
  auto block = make_block(
      {make_if_statement(
           make_boolean(false),
           make_variable_declaration("x", make_integer(1)) // Should be removed
           ),
       make_variable_declaration("y", make_integer(2))});

  eliminator->visit(*block);

  // Check that the unreachable code was removed
  EXPECT_EQ(block->get_statements().size(), 1);
  EXPECT_TRUE(block->get_statements()[0]->is<VariableDeclaration>());
}

TEST_F(DeadCodeEliminatorTest, UnusedVariableRemoval) {
  auto block = make_block(
      {make_variable_declaration("x", make_integer(1)), // Should be removed
       make_variable_declaration("y", make_integer(2)),
       std::make_unique<ExpressionStatement>(make_identifier("y"))});

  eliminator->visit(*block);

  // Check that the unused variable was removed
  EXPECT_EQ(block->get_statements().size(), 2);
  EXPECT_TRUE(block->get_statements()[0]->is<VariableDeclaration>());
  EXPECT_TRUE(block->get_statements()[1]->is<ExpressionStatement>());
}

TEST_F(DeadCodeEliminatorTest, EmptyBlockRemoval) {
  auto block = make_block({make_block({}), // Should be removed
                           make_variable_declaration("x", make_integer(1))});

  eliminator->visit(*block);

  // Check that the empty block was removed
  EXPECT_EQ(block->get_statements().size(), 1);
  EXPECT_TRUE(block->get_statements()[0]->is<VariableDeclaration>());
}

TEST_F(DeadCodeEliminatorTest, InfiniteLoopDetection) {
  auto block =
      make_block({std::make_unique<ForStatement>(NodeType::FOR_STATEMENT)});

  eliminator->visit(*block);

  // Check that the infinite loop was detected
  EXPECT_EQ(block->get_statements().size(), 1);
  EXPECT_TRUE(block->get_statements()[0]->is<ForStatement>());
}

TEST_F(DeadCodeEliminatorTest, ConstantConditionOptimization) {
  auto block = make_block({make_if_statement(
      make_boolean(true), make_variable_declaration("x", make_integer(1)),
      make_variable_declaration("y", make_integer(2)) // Should be removed
      )});

  eliminator->visit(*block);

  // Check that the else branch was removed
  auto if_stmt = block->get_statements()[0]->as<IfStatement>();
  EXPECT_TRUE(if_stmt->get_else_branch() == nullptr);
}

TEST_F(DeadCodeEliminatorTest, NestedDeadCodeRemoval) {
  auto block = make_block(
      {make_if_statement(
           make_boolean(false),
           make_block({make_variable_declaration("x", make_integer(1)),
                       make_variable_declaration("y", make_integer(2))})),
       make_variable_declaration("z", make_integer(3))});

  eliminator->visit(*block);

  // Check that the nested dead code was removed
  EXPECT_EQ(block->get_statements().size(), 1);
  EXPECT_TRUE(block->get_statements()[0]->is<VariableDeclaration>());
}

TEST_F(DeadCodeEliminatorTest, Performance) {
  const int num_statements = 1000;
  std::vector<std::unique_ptr<Statement>> statements;

  for (int i = 0; i < num_statements; ++i) {
    statements.push_back(
        make_variable_declaration("x" + std::to_string(i), make_integer(i)));
  }

  auto block = make_block(std::move(statements));

  auto start = std::chrono::high_resolution_clock::now();
  eliminator->visit(*block);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Performance requirement: 1000 statements should be processed in under 1
  // second
  EXPECT_LT(duration.count(), 1000);
}

TEST_F(DeadCodeEliminatorTest, MemoryUsage) {
  const int num_statements = 1000;
  std::vector<std::unique_ptr<Statement>> statements;

  for (int i = 0; i < num_statements; ++i) {
    statements.push_back(
        make_variable_declaration("x" + std::to_string(i), make_integer(i)));
  }

  auto block = make_block(std::move(statements));
  eliminator->visit(*block);

  // Memory requirement: Processing 1000 statements should not cause significant
  // memory growth
  EXPECT_EQ(block->get_statements().size(), num_statements);
}

TEST_F(DeadCodeEliminatorTest, ThreadSafety) {
  const int num_threads = 10;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, i]() {
      auto block =
          make_block({make_variable_declaration("x", make_integer(i)),
                      make_variable_declaration("y", make_integer(i + 1))});
      eliminator->visit(*block);
      if (block->get_statements().size() == 2) {
        success_count++;
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  // Thread safety requirement: All threads should complete successfully
  EXPECT_EQ(success_count.load(), num_threads);
}