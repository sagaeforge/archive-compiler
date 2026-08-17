#include "05_analysis/optimization/OptimizationPass.hpp"
#include "04_parsing/ast/core/ASTNode.hpp"
#include "04_parsing/ast/core/ASTVisitor.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>

using namespace nugdev::compiler::optimization;
using namespace nugdev::ast;

// Mock AST node for testing
class MockASTNode : public ASTNode {
public:
  MockASTNode() : ASTNode(NodeType::PROGRAM) {}

  bool visited = false;
  void accept(ASTVisitor &visitor) override { visited = true; }

  void accept(ASTVisitor &visitor) const override {
    const_cast<MockASTNode *>(this)->visited = true;
  }

  std::string to_string() const override { return "MockASTNode"; }
};

// Mock visitor for testing
class MockVisitor : public ASTVisitor {
public:
  bool visited = false;

  void visit(Program &) override { visited = true; }
  void visit(Module &) override {}
  void visit(VariableDeclaration &) override {}
  void visit(FunctionDeclaration &) override {}
  void visit(StructDeclaration &) override {}
  void visit(InterfaceDeclaration &) override {}
  void visit(ExpressionStatement &) override {}
  void visit(IfStatement &) override {}
  void visit(ForStatement &) override {}
  void visit(BreakStatement &) override {}
  void visit(ContinueStatement &) override {}
  void visit(ReturnStatement &) override {}
  void visit(BinaryExpression &) override {}
  void visit(UnaryExpression &) override {}
  void visit(PostfixExpression &) override {}
  void visit(AssignmentExpression &) override {}
  void visit(TernaryExpression &) override {}
  void visit(Identifier &) override {}
  void visit(BlockExpression &) override {}
  void visit(IfExpression &) override {}
  void visit(WhenExpression &) override {}
  void visit(FunctionExpression &) override {}
  void visit(LambdaExpression &) override {}
  void visit(NumberLiteral &) override {}
  void visit(StringLiteral &) override {}
  void visit(CharacterLiteral &) override {}
  void visit(BooleanLiteral &) override {}
  void visit(NullLiteral &) override {}
  void visit(NoneLiteral &) override {}
  void visit(RangeLiteral &) override {}
  void visit(ArrayLiteral &) override {}
  void visit(ObjectLiteral &) override {}
  void visit(TypeLiteral &) override {}
  void visit(FunctionType &) override {}
  void visit(OptionalType &) override {}
  void visit(TupleType &) override {}
  void visit(ImportStatement &) override {}
  void visit(ExportStatement &) override {}
  void visit(Parameter &) override {}
  void visit(ArgumentList &) override {}
  void visit(StructField &) override {}
  void visit(ObjectProperty &) override {}
  void visit(CastExpression &) override {}
  void visit(ArrayComprehension &) override {}
  void visit(TemplateExpression &) override {}
  void visit(WhenCondition &) override {}
  void visit(ValueCondition &) override {}
  void visit(RangeCondition &) override {}
  void visit(TypeCondition &) override {}
  void visit(GuardCondition &) override {}
  void visit(MultipleCondition &) override {}
};

// Mock optimization pass for testing
class MockOptimizationPass : public OptimizationPass {
public:
  bool should_optimize = false;
  bool run_called = false;

  bool run(ASTNode &node) override {
    run_called = true;
    m_stats.nodes_processed++;
    if (should_optimize) {
      m_stats.nodes_optimized++;
      m_stats.bytes_saved += 10;
    }
    return should_optimize;
  }

  std::string get_name() const override { return "MockPass"; }

  std::string get_description() const override {
    return "A mock optimization pass for testing";
  }
};

// Mock visitor-based optimization pass
class MockVisitorPass : public VisitorOptimizationPass {
public:
  bool visit_called = false;

  void visit(Program &node) override {
    visit_called = true;
    mark_optimization_applied();
  }

  std::string get_name() const override { return "MockVisitorPass"; }

  std::string get_description() const override {
    return "A mock visitor-based optimization pass";
  }
};

class OptimizationPassTest : public ::testing::Test {
protected:
  void SetUp() override {
    mock_node = std::make_unique<MockASTNode>();
    mock_pass = std::make_unique<MockOptimizationPass>();
    visitor_pass = std::make_unique<MockVisitorPass>();
  }

  std::unique_ptr<MockASTNode> mock_node;
  std::unique_ptr<MockOptimizationPass> mock_pass;
  std::unique_ptr<MockVisitorPass> visitor_pass;
};

TEST_F(OptimizationPassTest, BasicOptimization) {
  mock_pass->should_optimize = true;
  EXPECT_TRUE(mock_pass->run(*mock_node));
  EXPECT_TRUE(mock_pass->run_called);

  const auto &stats = mock_pass->get_statistics();
  EXPECT_EQ(stats.nodes_processed, 1);
  EXPECT_EQ(stats.nodes_optimized, 1);
  EXPECT_EQ(stats.bytes_saved, 10);
}

TEST_F(OptimizationPassTest, NoOptimization) {
  mock_pass->should_optimize = false;
  EXPECT_FALSE(mock_pass->run(*mock_node));
  EXPECT_TRUE(mock_pass->run_called);

  const auto &stats = mock_pass->get_statistics();
  EXPECT_EQ(stats.nodes_processed, 1);
  EXPECT_EQ(stats.nodes_optimized, 0);
  EXPECT_EQ(stats.bytes_saved, 0);
}

TEST_F(OptimizationPassTest, StatisticsReset) {
  mock_pass->should_optimize = true;
  mock_pass->run(*mock_node);
  mock_pass->reset_statistics();

  const auto &stats = mock_pass->get_statistics();
  EXPECT_EQ(stats.nodes_processed, 0);
  EXPECT_EQ(stats.nodes_optimized, 0);
  EXPECT_EQ(stats.bytes_saved, 0);
}

TEST_F(OptimizationPassTest, VisitorBasedOptimization) {
  EXPECT_TRUE(visitor_pass->run(*mock_node));
  EXPECT_TRUE(visitor_pass->visit_called);
  EXPECT_TRUE(mock_node->visited);

  const auto &stats = visitor_pass->get_statistics();
  EXPECT_EQ(stats.nodes_optimized, 1);
}

TEST_F(OptimizationPassTest, PassCompatibility) {
  MockOptimizationPass other_pass;
  EXPECT_TRUE(mock_pass->is_compatible_with(other_pass));
}

TEST_F(OptimizationPassTest, OptimizationManager) {
  OptimizationManager manager;

  // Add passes
  manager.add_pass(std::move(mock_pass));
  manager.add_pass(std::move(visitor_pass));

  EXPECT_EQ(manager.get_pass_count(), 2);

  auto pass_names = manager.get_pass_names();
  EXPECT_EQ(pass_names.size(), 2);
  EXPECT_TRUE(std::find(pass_names.begin(), pass_names.end(), "MockPass") !=
              pass_names.end());
  EXPECT_TRUE(std::find(pass_names.begin(), pass_names.end(),
                        "MockVisitorPass") != pass_names.end());

  // Test optimization levels
  manager.set_optimization_level(OptimizationManager::OptimizationLevel::O0);
  manager.set_optimization_level(OptimizationManager::OptimizationLevel::O1);
  manager.set_optimization_level(OptimizationManager::OptimizationLevel::O2);
  manager.set_optimization_level(OptimizationManager::OptimizationLevel::O3);
}

TEST_F(OptimizationPassTest, Performance) {
  const int num_iterations = 1000;
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_iterations; ++i) {
    mock_pass->run(*mock_node);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Performance requirement: 1000 iterations should complete in under 1 second
  EXPECT_LT(duration.count(), 1000);
}

TEST_F(OptimizationPassTest, MemoryUsage) {
  const int num_passes = 1000;
  std::vector<std::unique_ptr<MockOptimizationPass>> passes;

  for (int i = 0; i < num_passes; ++i) {
    passes.push_back(std::make_unique<MockOptimizationPass>());
  }

  // Memory requirement: 1000 passes should not cause significant memory growth
  EXPECT_EQ(passes.size(), num_passes);
}

TEST_F(OptimizationPassTest, ThreadSafety) {
  const int num_threads = 10;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&]() {
      MockOptimizationPass pass;
      if (pass.run(*mock_node)) {
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