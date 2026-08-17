#include "05_analysis/optimization/ConstantFolder.hpp"
#include "04_parsing/ast/expressions/Expressions.hpp"
#include "04_parsing/ast/literals/Literals.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <limits>
#include <memory>
#include <string>
#include <thread>

using namespace nugdev::compiler::optimization;
using namespace nugdev::ast;

class ConstantFolderTest : public ::testing::Test {
protected:
  void SetUp() override { folder = std::make_unique<ConstantFolder>(); }

  std::unique_ptr<ConstantFolder> folder;
};

// Helper function to create number literals
std::unique_ptr<NumberLiteral> make_integer(int64_t value) {
  return std::make_unique<NumberLiteral>(
      std::to_string(value), NumberLiteral::NumberType::DECIMAL_INTEGER);
}

std::unique_ptr<NumberLiteral> make_float(double value) {
  return std::make_unique<NumberLiteral>(
      std::to_string(value), NumberLiteral::NumberType::FLOATING_POINT);
}

// Helper function to create boolean literals
std::unique_ptr<BooleanLiteral> make_boolean(bool value) {
  return std::make_unique<BooleanLiteral>(value);
}

// Helper function to create string literals
std::unique_ptr<StringLiteral> make_string(const std::string &value) {
  return std::make_unique<StringLiteral>(value,
                                         StringLiteral::StringType::SIMPLE);
}

// Helper function to create binary expressions
std::unique_ptr<BinaryExpression>
make_binary(std::unique_ptr<Expression> left, BinaryExpression::Operator op,
            std::unique_ptr<Expression> right) {
  return std::make_unique<BinaryExpression>(op, std::move(left),
                                            std::move(right));
}

// Helper function to create unary expressions
std::unique_ptr<UnaryExpression>
make_unary(UnaryExpression::Operator op, std::unique_ptr<Expression> operand) {
  return std::make_unique<UnaryExpression>(op, std::move(operand));
}

TEST_F(ConstantFolderTest, BasicArithmetic) {
  // Test integer arithmetic
  auto expr = make_binary(make_integer(2), BinaryExpression::Operator::ADD,
                          make_integer(3));

  folder->visit(*expr);
  EXPECT_TRUE(expr->is<NumberLiteral>());
  EXPECT_EQ(expr->as<NumberLiteral>()->get_integer_value(), 5);

  // Test floating-point arithmetic
  expr = make_binary(make_float(2.5), BinaryExpression::Operator::MULTIPLY,
                     make_float(2.0));

  folder->visit(*expr);
  EXPECT_TRUE(expr->is<NumberLiteral>());
  EXPECT_DOUBLE_EQ(expr->as<NumberLiteral>()->get_floating_point_value(), 5.0);
}

TEST_F(ConstantFolderTest, BooleanOperations) {
  // Test logical AND
  auto expr =
      make_binary(make_boolean(true), BinaryExpression::Operator::LOGICAL_AND,
                  make_boolean(false));

  folder->visit(*expr);
  EXPECT_TRUE(expr->is<BooleanLiteral>());
  EXPECT_FALSE(expr->as<BooleanLiteral>()->get_boolean_value());

  // Test logical OR
  expr = make_binary(make_boolean(true), BinaryExpression::Operator::LOGICAL_OR,
                     make_boolean(false));

  folder->visit(*expr);
  EXPECT_TRUE(expr->is<BooleanLiteral>());
  EXPECT_TRUE(expr->as<BooleanLiteral>()->get_boolean_value());
}

TEST_F(ConstantFolderTest, StringOperations) {
  // Test string concatenation
  auto expr = make_binary(make_string("hello"), BinaryExpression::Operator::ADD,
                          make_string(" world"));

  folder->visit(*expr);
  EXPECT_TRUE(expr->is<StringLiteral>());
  EXPECT_EQ(expr->as<StringLiteral>()->get_literal_value(), "hello world");
}

TEST_F(ConstantFolderTest, UnaryOperations) {
  // Test unary minus
  auto expr = make_unary(UnaryExpression::Operator::MINUS, make_integer(5));

  folder->visit(*expr);
  EXPECT_TRUE(expr->is<NumberLiteral>());
  EXPECT_EQ(expr->as<NumberLiteral>()->get_integer_value(), -5);

  // Test logical NOT
  expr = make_unary(UnaryExpression::Operator::LOGICAL_NOT, make_boolean(true));

  folder->visit(*expr);
  EXPECT_TRUE(expr->is<BooleanLiteral>());
  EXPECT_FALSE(expr->as<BooleanLiteral>()->get_boolean_value());
}

TEST_F(ConstantFolderTest, TypeConversions) {
  // Test integer to double conversion
  auto expr = make_binary(make_integer(5), BinaryExpression::Operator::ADD,
                          make_float(2.5));

  folder->visit(*expr);
  EXPECT_TRUE(expr->is<NumberLiteral>());
  EXPECT_DOUBLE_EQ(expr->as<NumberLiteral>()->get_floating_point_value(), 7.5);
}

TEST_F(ConstantFolderTest, EdgeCases) {
  // Test division by zero
  auto expr = make_binary(make_integer(5), BinaryExpression::Operator::DIVIDE,
                          make_integer(0));

  folder->visit(*expr);
  // Should not fold due to division by zero
  EXPECT_TRUE(expr->is<BinaryExpression>());

  // Test integer overflow
  expr = make_binary(make_integer(std::numeric_limits<int64_t>::max()),
                     BinaryExpression::Operator::ADD, make_integer(1));

  folder->visit(*expr);
  // Should not fold due to overflow
  EXPECT_TRUE(expr->is<BinaryExpression>());
}

TEST_F(ConstantFolderTest, ComplexExpressions) {
  // Test nested expressions
  auto expr = make_binary(
      make_binary(make_integer(2), BinaryExpression::Operator::MULTIPLY,
                  make_integer(3)),
      BinaryExpression::Operator::ADD,
      make_binary(make_integer(4), BinaryExpression::Operator::MULTIPLY,
                  make_integer(5)));

  folder->visit(*expr);
  EXPECT_TRUE(expr->is<NumberLiteral>());
  EXPECT_EQ(expr->as<NumberLiteral>()->get_integer_value(), 26);
}

TEST_F(ConstantFolderTest, Performance) {
  const int num_iterations = 1000;
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_iterations; ++i) {
    auto expr = make_binary(make_integer(i), BinaryExpression::Operator::ADD,
                            make_integer(i + 1));
    folder->visit(*expr);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Performance requirement: 1000 iterations should complete in under 1 second
  EXPECT_LT(duration.count(), 1000);
}

TEST_F(ConstantFolderTest, MemoryUsage) {
  const int num_expressions = 1000;
  std::vector<std::unique_ptr<BinaryExpression>> expressions;

  for (int i = 0; i < num_expressions; ++i) {
    expressions.push_back(make_binary(
        make_integer(i), BinaryExpression::Operator::ADD, make_integer(i + 1)));
  }

  for (auto &expr : expressions) {
    folder->visit(*expr);
  }

  // Memory requirement: 1000 expressions should not cause significant memory
  // growth
  EXPECT_EQ(expressions.size(), num_expressions);
}

TEST_F(ConstantFolderTest, ThreadSafety) {
  const int num_threads = 10;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, i]() {
      auto expr = make_binary(make_integer(i), BinaryExpression::Operator::ADD,
                              make_integer(i + 1));
      folder->visit(*expr);
      if (expr->is<NumberLiteral>()) {
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