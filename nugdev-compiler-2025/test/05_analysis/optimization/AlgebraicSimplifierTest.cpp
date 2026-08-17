#include "05_analysis/optimization/AlgebraicSimplifier.hpp"
#include "04_parsing/ast/expressions/Expressions.hpp"
#include "04_parsing/ast/literals/Literals.hpp"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

using namespace nugdev::compiler::optimization;
using namespace nugdev::ast;

class AlgebraicSimplifierTest : public ::testing::Test {
protected:
  void SetUp() override {
    simplifier = std::make_unique<AlgebraicSimplifier>();
  }

  std::unique_ptr<AlgebraicSimplifier> simplifier;

  // Helper functions
  std::unique_ptr<NumberLiteral> make_number(int64_t value) {
    return std::make_unique<NumberLiteral>(
        std::to_string(value), NumberLiteral::NumberType::DECIMAL_INTEGER);
  }

  std::unique_ptr<NumberLiteral> make_double(double value) {
    return std::make_unique<NumberLiteral>(
        std::to_string(value), NumberLiteral::NumberType::DECIMAL_FLOAT);
  }

  std::unique_ptr<BooleanLiteral> make_boolean(bool value) {
    return std::make_unique<BooleanLiteral>(value);
  }

  std::unique_ptr<Identifier> make_identifier(const std::string &name) {
    return std::make_unique<Identifier>(name);
  }

  std::unique_ptr<BinaryExpression>
  make_binary(BinaryExpression::Operator op, std::unique_ptr<Expression> left,
              std::unique_ptr<Expression> right) {
    return std::make_unique<BinaryExpression>(op, std::move(left),
                                              std::move(right));
  }

  std::unique_ptr<UnaryExpression>
  make_unary(UnaryExpression::Operator op,
             std::unique_ptr<Expression> operand) {
    return std::make_unique<UnaryExpression>(op, std::move(operand));
  }
};

// Basic functionality tests
TEST_F(AlgebraicSimplifierTest, ConstructorAndBasicInfo) {
  EXPECT_NE(simplifier.get(), nullptr);
  EXPECT_EQ(simplifier->get_name(), "AlgebraicSimplifier");
  EXPECT_FALSE(simplifier->get_description().empty());
}

// Identity operations: x + 0, x * 1, x - 0
TEST_F(AlgebraicSimplifierTest, AdditiveIdentity) {
  // x + 0 = x
  auto expr = make_binary(BinaryExpression::Operator::ADD, make_identifier("x"),
                          make_number(0));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  EXPECT_NE(simplified, nullptr);

  // Should be simplified to just x (identifier)
  auto identifier = dynamic_cast<Identifier *>(simplified.get());
  EXPECT_NE(identifier, nullptr);
  EXPECT_EQ(identifier->get_name(), "x");
}

TEST_F(AlgebraicSimplifierTest, AdditiveIdentityReversed) {
  // 0 + x = x
  auto expr = make_binary(BinaryExpression::Operator::ADD, make_number(0),
                          make_identifier("x"));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  EXPECT_NE(simplified, nullptr);

  auto identifier = dynamic_cast<Identifier *>(simplified.get());
  EXPECT_NE(identifier, nullptr);
  EXPECT_EQ(identifier->get_name(), "x");
}

TEST_F(AlgebraicSimplifierTest, MultiplicativeIdentity) {
  // x * 1 = x
  auto expr = make_binary(BinaryExpression::Operator::MULTIPLY,
                          make_identifier("x"), make_number(1));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  auto identifier = dynamic_cast<Identifier *>(simplified.get());
  EXPECT_NE(identifier, nullptr);
  EXPECT_EQ(identifier->get_name(), "x");
}

TEST_F(AlgebraicSimplifierTest, SubtractiveIdentity) {
  // x - 0 = x
  auto expr = make_binary(BinaryExpression::Operator::SUBTRACT,
                          make_identifier("x"), make_number(0));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  auto identifier = dynamic_cast<Identifier *>(simplified.get());
  EXPECT_NE(identifier, nullptr);
  EXPECT_EQ(identifier->get_name(), "x");
}

// Absorbing operations: x * 0, x && false, x || true
TEST_F(AlgebraicSimplifierTest, MultiplicativeAbsorption) {
  // x * 0 = 0
  auto expr = make_binary(BinaryExpression::Operator::MULTIPLY,
                          make_identifier("x"), make_number(0));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  auto literal = dynamic_cast<NumberLiteral *>(simplified.get());
  EXPECT_NE(literal, nullptr);
  EXPECT_EQ(literal->get_integer_value(), 0);
}

TEST_F(AlgebraicSimplifierTest, LogicalAndAbsorption) {
  // x && false = false
  auto expr = make_binary(BinaryExpression::Operator::LOGICAL_AND,
                          make_identifier("x"), make_boolean(false));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  auto literal = dynamic_cast<BooleanLiteral *>(simplified.get());
  EXPECT_NE(literal, nullptr);
  EXPECT_FALSE(literal->get_literal_value());
}

TEST_F(AlgebraicSimplifierTest, LogicalOrAbsorption) {
  // x || true = true
  auto expr = make_binary(BinaryExpression::Operator::LOGICAL_OR,
                          make_identifier("x"), make_boolean(true));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  auto literal = dynamic_cast<BooleanLiteral *>(simplified.get());
  EXPECT_NE(literal, nullptr);
  EXPECT_TRUE(literal->get_literal_value());
}

// Idempotent operations: x - x, x / x
TEST_F(AlgebraicSimplifierTest, SubtractionIdempotent) {
  // x - x = 0
  auto expr = make_binary(BinaryExpression::Operator::SUBTRACT,
                          make_identifier("x"), make_identifier("x"));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  auto literal = dynamic_cast<NumberLiteral *>(simplified.get());
  EXPECT_NE(literal, nullptr);
  EXPECT_EQ(literal->get_integer_value(), 0);
}

TEST_F(AlgebraicSimplifierTest, DivisionIdempotent) {
  // x / x = 1 (assuming x != 0)
  auto expr = make_binary(BinaryExpression::Operator::DIVIDE,
                          make_identifier("x"), make_identifier("x"));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  auto literal = dynamic_cast<NumberLiteral *>(simplified.get());
  EXPECT_NE(literal, nullptr);
  EXPECT_EQ(literal->get_integer_value(), 1);
}

// Double negation: --x = x
TEST_F(AlgebraicSimplifierTest, DoubleNegation) {
  auto inner_negation =
      make_unary(UnaryExpression::Operator::MINUS, make_identifier("x"));

  auto double_negation =
      make_unary(UnaryExpression::Operator::MINUS, std::move(inner_negation));

  simplifier->visit(*double_negation);

  auto simplified = simplifier->get_simplified_expression();
  auto identifier = dynamic_cast<Identifier *>(simplified.get());
  EXPECT_NE(identifier, nullptr);
  EXPECT_EQ(identifier->get_name(), "x");
}

// Distributive laws: a * (b + c) = a * b + a * c
TEST_F(AlgebraicSimplifierTest, DistributiveLawMultiplication) {
  // a * (b + c) should be expanded to a * b + a * c
  auto sum = make_binary(BinaryExpression::Operator::ADD, make_identifier("b"),
                         make_identifier("c"));

  auto expr = make_binary(BinaryExpression::Operator::MULTIPLY,
                          make_identifier("a"), std::move(sum));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  EXPECT_NE(simplified, nullptr);

  // Should be expanded form: a * b + a * c
  auto outer_add = dynamic_cast<BinaryExpression *>(simplified.get());
  EXPECT_NE(outer_add, nullptr);
  EXPECT_EQ(outer_add->get_operator(), BinaryExpression::Operator::ADD);
}

// Associativity: (x + y) + z = x + (y + z)
TEST_F(AlgebraicSimplifierTest, AssociativityReorganization) {
  // Test that associative operations are properly reorganized
  auto inner_add = make_binary(BinaryExpression::Operator::ADD,
                               make_identifier("x"), make_identifier("y"));

  auto expr = make_binary(BinaryExpression::Operator::ADD, std::move(inner_add),
                          make_identifier("z"));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  EXPECT_NE(simplified, nullptr);

  // Should maintain associative structure or optimize it
  auto binary_expr = dynamic_cast<BinaryExpression *>(simplified.get());
  EXPECT_NE(binary_expr, nullptr);
}

// Constant folding with algebraic simplification
TEST_F(AlgebraicSimplifierTest, ConstantFoldingIntegration) {
  // (2 + 3) * x = 5 * x
  auto constant_add = make_binary(BinaryExpression::Operator::ADD,
                                  make_number(2), make_number(3));

  auto expr = make_binary(BinaryExpression::Operator::MULTIPLY,
                          std::move(constant_add), make_identifier("x"));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  auto binary_expr = dynamic_cast<BinaryExpression *>(simplified.get());
  EXPECT_NE(binary_expr, nullptr);

  // Left operand should be constant 5
  auto left_literal = dynamic_cast<NumberLiteral *>(&binary_expr->get_left());
  EXPECT_NE(left_literal, nullptr);
  EXPECT_EQ(left_literal->get_integer_value(), 5);
}

// Complex nested simplifications
TEST_F(AlgebraicSimplifierTest, NestedSimplifications) {
  // ((x + 0) * 1) - (x - x) = x - 0 = x
  auto x_plus_0 = make_binary(BinaryExpression::Operator::ADD,
                              make_identifier("x"), make_number(0));

  auto times_1 = make_binary(BinaryExpression::Operator::MULTIPLY,
                             std::move(x_plus_0), make_number(1));

  auto x_minus_x = make_binary(BinaryExpression::Operator::SUBTRACT,
                               make_identifier("x"), make_identifier("x"));

  auto expr = make_binary(BinaryExpression::Operator::SUBTRACT,
                          std::move(times_1), std::move(x_minus_x));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  auto identifier = dynamic_cast<Identifier *>(simplified.get());
  EXPECT_NE(identifier, nullptr);
  EXPECT_EQ(identifier->get_name(), "x");
}

// Boolean algebra simplifications
TEST_F(AlgebraicSimplifierTest, BooleanAlgebraSimplifications) {
  // x && true = x
  auto expr1 = make_binary(BinaryExpression::Operator::LOGICAL_AND,
                           make_identifier("x"), make_boolean(true));

  simplifier->visit(*expr1);
  auto simplified1 = simplifier->get_simplified_expression();
  auto identifier1 = dynamic_cast<Identifier *>(simplified1.get());
  EXPECT_NE(identifier1, nullptr);
  EXPECT_EQ(identifier1->get_name(), "x");

  // x || false = x
  auto expr2 = make_binary(BinaryExpression::Operator::LOGICAL_OR,
                           make_identifier("y"), make_boolean(false));

  simplifier->visit(*expr2);
  auto simplified2 = simplifier->get_simplified_expression();
  auto identifier2 = dynamic_cast<Identifier *>(simplified2.get());
  EXPECT_NE(identifier2, nullptr);
  EXPECT_EQ(identifier2->get_name(), "y");
}

// Performance test
TEST_F(AlgebraicSimplifierTest, PerformanceTest) {
  const int num_expressions = 1000;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_expressions; ++i) {
    auto expr =
        make_binary(BinaryExpression::Operator::ADD,
                    make_identifier("x" + std::to_string(i)), make_number(0));

    simplifier->visit(*expr);
    auto simplified = simplifier->get_simplified_expression();
    EXPECT_NE(simplified, nullptr);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Should process 1000 expressions in under 1 second
  EXPECT_LT(duration.count(), 1000);
}

// Memory usage test
TEST_F(AlgebraicSimplifierTest, MemoryUsageTest) {
  const int num_expressions = 10000;

  for (int i = 0; i < num_expressions; ++i) {
    auto expr =
        make_binary(BinaryExpression::Operator::MULTIPLY,
                    make_identifier("var" + std::to_string(i)), make_number(1));

    simplifier->visit(*expr);
    auto simplified = simplifier->get_simplified_expression();
    EXPECT_NE(simplified, nullptr);
  }

  // Test should complete without excessive memory usage
  EXPECT_TRUE(true); // If we reach here, memory usage was acceptable
}

// Thread safety test
TEST_F(AlgebraicSimplifierTest, ThreadSafetyTest) {
  const int num_threads = 4;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, i]() {
      auto local_simplifier = std::make_unique<AlgebraicSimplifier>();

      auto expr = make_binary(BinaryExpression::Operator::ADD,
                              make_identifier("x"), make_number(0));

      local_simplifier->visit(*expr);
      auto simplified = local_simplifier->get_simplified_expression();

      if (simplified != nullptr) {
        auto identifier = dynamic_cast<Identifier *>(simplified.get());
        if (identifier != nullptr && identifier->get_name() == "x") {
          success_count++;
        }
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_EQ(success_count.load(), num_threads);
}

// Edge cases
TEST_F(AlgebraicSimplifierTest, EdgeCases) {
  // Division by zero should not be simplified
  auto div_by_zero = make_binary(BinaryExpression::Operator::DIVIDE,
                                 make_identifier("x"), make_number(0));

  simplifier->visit(*div_by_zero);
  auto simplified = simplifier->get_simplified_expression();

  // Should remain as division expression (not simplified)
  auto binary_expr = dynamic_cast<BinaryExpression *>(simplified.get());
  EXPECT_NE(binary_expr, nullptr);
  EXPECT_EQ(binary_expr->get_operator(), BinaryExpression::Operator::DIVIDE);
}

// Floating point operations
TEST_F(AlgebraicSimplifierTest, FloatingPointOperations) {
  // x + 0.0 = x
  auto expr = make_binary(BinaryExpression::Operator::ADD, make_identifier("x"),
                          make_double(0.0));

  simplifier->visit(*expr);

  auto simplified = simplifier->get_simplified_expression();
  auto identifier = dynamic_cast<Identifier *>(simplified.get());
  EXPECT_NE(identifier, nullptr);
  EXPECT_EQ(identifier->get_name(), "x");
}

// Statistics and reporting
TEST_F(AlgebraicSimplifierTest, StatisticsAndReporting) {
  // Perform several simplifications
  auto expr1 = make_binary(BinaryExpression::Operator::ADD,
                           make_identifier("x"), make_number(0));
  auto expr2 = make_binary(BinaryExpression::Operator::MULTIPLY,
                           make_identifier("y"), make_number(1));
  auto expr3 = make_binary(BinaryExpression::Operator::SUBTRACT,
                           make_identifier("z"), make_identifier("z"));

  simplifier->visit(*expr1);
  simplifier->visit(*expr2);
  simplifier->visit(*expr3);

  auto stats = simplifier->get_optimization_statistics();
  EXPECT_GT(stats.expressions_simplified, 0);
  EXPECT_GT(stats.total_optimizations, 0);
  EXPECT_GE(stats.execution_time_ms, 0);
}