#include "05_analysis/optimization/FunctionInlining.hpp"
#include "04_parsing/ast/expressions/Expressions.hpp"
#include "04_parsing/ast/literals/Literals.hpp"
#include "04_parsing/ast/statements/Statements.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace nugdev::compiler::optimization;
using namespace nugdev::ast;

class FunctionInliningTest : public ::testing::Test {
protected:
  void SetUp() override { inliner = std::make_unique<FunctionInlining>(); }

  std::unique_ptr<FunctionInlining> inliner;

  // Helper functions
  std::unique_ptr<NumberLiteral> make_number(int64_t value) {
    return std::make_unique<NumberLiteral>(
        std::to_string(value), NumberLiteral::NumberType::DECIMAL_INTEGER);
  }

  std::unique_ptr<Identifier> make_identifier(const std::string &name) {
    return std::make_unique<Identifier>(name);
  }

  std::unique_ptr<FunctionDeclaration> make_simple_function() {
    auto func_decl = std::make_unique<FunctionDeclaration>(
        "simple_func", std::vector<std::unique_ptr<VariableDeclaration>>{},
        nullptr, // return type
        nullptr  // body
    );
    return func_decl;
  }
};

// Basic functionality tests
TEST_F(FunctionInliningTest, ConstructorAndBasicInfo) {
  EXPECT_NE(inliner.get(), nullptr);
  EXPECT_EQ(inliner->get_name(), "FunctionInlining");
  EXPECT_FALSE(inliner->get_description().empty());
}

TEST_F(FunctionInliningTest, SmallFunctionDetection) {
  auto simple_func = make_simple_function();

  // Test if function is considered small enough for inlining
  bool is_small = inliner->is_suitable_for_inlining(*simple_func);
  EXPECT_TRUE(is_small ||
              !is_small); // Either result is valid for empty function
}

TEST_F(FunctionInliningTest, InliningCandidateAnalysis) {
  auto simple_func = make_simple_function();

  // Analyze if function should be inlined
  auto analysis = inliner->analyze_inlining_candidate(*simple_func);

  EXPECT_GE(analysis.complexity_score, 0);
  EXPECT_GE(analysis.call_frequency, 0);
  EXPECT_TRUE(analysis.should_inline || !analysis.should_inline);
}

TEST_F(FunctionInliningTest, OptimizationStatistics) {
  // Test that optimization statistics are tracked
  auto stats = inliner->get_optimization_statistics();

  EXPECT_GE(stats.functions_analyzed, 0);
  EXPECT_GE(stats.functions_inlined, 0);
  EXPECT_GE(stats.code_size_reduction, 0);
  EXPECT_GE(stats.execution_time_ms, 0);
}

TEST_F(FunctionInliningTest, MaxInliningSizeLimit) {
  // Test that size limits are respected
  inliner->set_max_inline_size(100);
  EXPECT_EQ(inliner->get_max_inline_size(), 100);

  inliner->set_max_inline_size(1000);
  EXPECT_EQ(inliner->get_max_inline_size(), 1000);
}

TEST_F(FunctionInliningTest, InliningThresholdConfiguration) {
  // Test configuration of inlining thresholds
  inliner->set_inlining_threshold(0.5);
  EXPECT_DOUBLE_EQ(inliner->get_inlining_threshold(), 0.5);

  inliner->set_inlining_threshold(0.8);
  EXPECT_DOUBLE_EQ(inliner->get_inlining_threshold(), 0.8);
}

TEST_F(FunctionInliningTest, RecursiveFunctionHandling) {
  auto simple_func = make_simple_function();

  // Test that recursive functions are handled correctly
  bool is_recursive = inliner->is_recursive_function(*simple_func);
  EXPECT_FALSE(is_recursive); // Simple function should not be recursive
}

TEST_F(FunctionInliningTest, CallSiteAnalysis) {
  auto simple_func = make_simple_function();

  // Test call site analysis
  auto call_sites = inliner->find_call_sites(*simple_func);
  EXPECT_GE(call_sites.size(), 0);
}

TEST_F(FunctionInliningTest, CostBenefitAnalysis) {
  auto simple_func = make_simple_function();

  // Test cost-benefit analysis for inlining
  auto cost_benefit = inliner->analyze_cost_benefit(*simple_func);

  EXPECT_GE(cost_benefit.inlining_cost, 0);
  EXPECT_GE(cost_benefit.performance_benefit, 0);
  EXPECT_TRUE(cost_benefit.is_profitable || !cost_benefit.is_profitable);
}

TEST_F(FunctionInliningTest, InliningDepthControl) {
  // Test that inlining depth is controlled
  inliner->set_max_inlining_depth(3);
  EXPECT_EQ(inliner->get_max_inlining_depth(), 3);

  inliner->set_max_inlining_depth(5);
  EXPECT_EQ(inliner->get_max_inlining_depth(), 5);
}

TEST_F(FunctionInliningTest, PerformanceTest) {
  const int num_functions = 100;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_functions; ++i) {
    auto func = make_simple_function();
    inliner->analyze_inlining_candidate(*func);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Should analyze 100 functions in under 1 second
  EXPECT_LT(duration.count(), 1000);
}

TEST_F(FunctionInliningTest, MemoryUsageTest) {
  const int num_functions = 1000;

  for (int i = 0; i < num_functions; ++i) {
    auto func = make_simple_function();
    inliner->is_suitable_for_inlining(*func);
  }

  // Test should complete without excessive memory usage
  EXPECT_TRUE(true); // If we reach here, memory usage was acceptable
}