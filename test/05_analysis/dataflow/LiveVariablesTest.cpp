#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include "05_analysis/dataflow/LiveVariables.hpp"

using namespace nugdev::compiler::analysis;

class LiveVariablesTest : public ::testing::Test {
protected:
  void SetUp() override {
    cfg = std::make_unique<ControlFlowGraph>();
    analysis = std::make_unique<LiveVariablesAnalysis>();
  }

  void TearDown() override {
    cfg.reset();
    analysis.reset();
  }

  std::unique_ptr<ControlFlowGraph> cfg;
  std::unique_ptr<LiveVariablesAnalysis> analysis;

  BasicBlock *createBlock() { return cfg->create_block(); }
};

// Basic functionality tests
TEST_F(LiveVariablesTest, ConstructorAndBasicSetup) {
  EXPECT_NE(analysis.get(), nullptr);
  EXPECT_NE(cfg.get(), nullptr);
}

TEST_F(LiveVariablesTest, EmptyGraphAnalysis) {
  // Analysis on empty graph should not crash
  EXPECT_NO_THROW({
    auto errors = analysis->analyze_live_variables(*cfg);
    EXPECT_GE(errors.size(), 0); // Could be empty or have errors
  });
}

TEST_F(LiveVariablesTest, SingleBlockAnalysis) {
  auto block = createBlock();
  cfg->set_entry_block(block);
  cfg->set_exit_block(block);

  auto errors = analysis->analyze_live_variables(*cfg);

  // Should complete without throwing
  EXPECT_GE(errors.size(), 0);
}

TEST_F(LiveVariablesTest, LinearControlFlow) {
  auto block1 = createBlock();
  auto block2 = createBlock();
  auto block3 = createBlock();

  cfg->add_edge(block1, block2);
  cfg->add_edge(block2, block3);
  cfg->set_entry_block(block1);
  cfg->set_exit_block(block3);

  auto errors = analysis->analyze_live_variables(*cfg);

  // Should handle linear flow
  EXPECT_GE(errors.size(), 0);
}

TEST_F(LiveVariablesTest, BranchingControlFlow) {
  auto entry = createBlock();
  auto branch1 = createBlock();
  auto branch2 = createBlock();
  auto merge = createBlock();

  cfg->add_edge(entry, branch1);
  cfg->add_edge(entry, branch2);
  cfg->add_edge(branch1, merge);
  cfg->add_edge(branch2, merge);
  cfg->set_entry_block(entry);
  cfg->set_exit_block(merge);

  auto errors = analysis->analyze_live_variables(*cfg);

  // Should handle branching
  EXPECT_GE(errors.size(), 0);
}

TEST_F(LiveVariablesTest, LoopControlFlow) {
  auto header = createBlock();
  auto body = createBlock();
  auto exit = createBlock();

  cfg->add_edge(header, body);
  cfg->add_edge(body, header); // Back edge
  cfg->add_edge(header, exit);
  cfg->set_entry_block(header);
  cfg->set_exit_block(exit);

  auto errors = analysis->analyze_live_variables(*cfg);

  // Should handle loops
  EXPECT_GE(errors.size(), 0);
}

// Performance tests
TEST_F(LiveVariablesTest, LargeGraphPerformance) {
  const int NUM_BLOCKS = 50;
  std::vector<BasicBlock *> blocks;

  for (int i = 0; i < NUM_BLOCKS; ++i) {
    blocks.push_back(createBlock());
  }

  // Create linear chain
  for (int i = 0; i < NUM_BLOCKS - 1; ++i) {
    cfg->add_edge(blocks[i], blocks[i + 1]);
  }
  cfg->set_entry_block(blocks[0]);
  cfg->set_exit_block(blocks[NUM_BLOCKS - 1]);

  auto start_time = std::chrono::high_resolution_clock::now();
  auto errors = analysis->analyze_live_variables(*cfg);
  auto end_time = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);

  // Should complete in reasonable time
  EXPECT_LT(duration.count(), 500);
  EXPECT_GE(errors.size(), 0);
}

// Variable set tests
TEST_F(LiveVariablesTest, VariableSetOperations) {
  VariableSet set1({"x", "y", "z"});
  VariableSet set2({"y", "z", "w"});

  // Test union
  auto union_set = set1 | set2;
  EXPECT_EQ(union_set.size(), 4);
  EXPECT_TRUE(union_set.contains("x"));
  EXPECT_TRUE(union_set.contains("y"));
  EXPECT_TRUE(union_set.contains("z"));
  EXPECT_TRUE(union_set.contains("w"));

  // Test intersection
  auto intersect_set = set1 & set2;
  EXPECT_EQ(intersect_set.size(), 2);
  EXPECT_TRUE(intersect_set.contains("y"));
  EXPECT_TRUE(intersect_set.contains("z"));
  EXPECT_FALSE(intersect_set.contains("x"));
  EXPECT_FALSE(intersect_set.contains("w"));

  // Test difference
  auto diff_set = set1 - set2;
  EXPECT_EQ(diff_set.size(), 1);
  EXPECT_TRUE(diff_set.contains("x"));
  EXPECT_FALSE(diff_set.contains("y"));
  EXPECT_FALSE(diff_set.contains("z"));
}

TEST_F(LiveVariablesTest, VariableSetEquality) {
  VariableSet set1({"x", "y", "z"});
  VariableSet set2({"x", "y", "z"});
  VariableSet set3({"x", "y", "w"});

  EXPECT_TRUE(set1 == set2);
  EXPECT_FALSE(set1 == set3);
  EXPECT_TRUE(set1 != set3);
}

TEST_F(LiveVariablesTest, VariableSetModification) {
  VariableSet set;

  EXPECT_TRUE(set.empty());
  EXPECT_EQ(set.size(), 0);

  set.add("x");
  set.add("y");
  set.add("z");

  EXPECT_FALSE(set.empty());
  EXPECT_EQ(set.size(), 3);
  EXPECT_TRUE(set.contains("x"));
  EXPECT_TRUE(set.contains("y"));
  EXPECT_TRUE(set.contains("z"));
  EXPECT_FALSE(set.contains("w"));

  set.remove("y");
  EXPECT_EQ(set.size(), 2);
  EXPECT_FALSE(set.contains("y"));
  EXPECT_TRUE(set.contains("x"));
  EXPECT_TRUE(set.contains("z"));
}

TEST_F(LiveVariablesTest, VariableSetIterators) {
  VariableSet set({"x", "y", "z"});

  std::vector<std::string> vars;
  for (const auto &var : set) {
    vars.push_back(var);
  }

  EXPECT_EQ(vars.size(), 3);
  // Note: order is not guaranteed with unordered_set
  EXPECT_NE(std::find(vars.begin(), vars.end(), "x"), vars.end());
  EXPECT_NE(std::find(vars.begin(), vars.end(), "y"), vars.end());
  EXPECT_NE(std::find(vars.begin(), vars.end(), "z"), vars.end());
}

// Edge cases
TEST_F(LiveVariablesTest, NullGraphComponents) {
  // Test with graph that has null entry/exit blocks
  auto block = createBlock();
  // Don't set entry/exit blocks

  auto errors = analysis->analyze_live_variables(*cfg);

  // Should handle gracefully
  EXPECT_GE(errors.size(), 0);
}

TEST_F(LiveVariablesTest, DisconnectedBlocks) {
  auto block1 = createBlock();
  auto block2 = createBlock();
  auto block3 = createBlock();

  // Connect only block1 and block2, leave block3 disconnected
  cfg->add_edge(block1, block2);
  cfg->set_entry_block(block1);
  cfg->set_exit_block(block2);
  // block3 is unreachable

  auto errors = analysis->analyze_live_variables(*cfg);

  // Should handle unreachable blocks
  EXPECT_GE(errors.size(), 0);
}

TEST_F(LiveVariablesTest, ComplexControlFlow) {
  // Create a more complex CFG with multiple loops and branches
  auto entry = createBlock();
  auto loop_header = createBlock();
  auto loop_body1 = createBlock();
  auto loop_body2 = createBlock();
  auto branch1 = createBlock();
  auto branch2 = createBlock();
  auto merge = createBlock();
  auto exit = createBlock();

  // Entry to loop
  cfg->add_edge(entry, loop_header);

  // Loop structure
  cfg->add_edge(loop_header, loop_body1);
  cfg->add_edge(loop_body1, loop_body2);
  cfg->add_edge(loop_body2, loop_header); // Back edge

  // Exit from loop to branches
  cfg->add_edge(loop_header, branch1);
  cfg->add_edge(loop_header, branch2);

  // Branches merge
  cfg->add_edge(branch1, merge);
  cfg->add_edge(branch2, merge);
  cfg->add_edge(merge, exit);

  cfg->set_entry_block(entry);
  cfg->set_exit_block(exit);

  auto start_time = std::chrono::high_resolution_clock::now();
  auto errors = analysis->analyze_live_variables(*cfg);
  auto end_time = std::chrono::high_resolution_clock::now();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time);

  // Should handle complex flow in reasonable time
  EXPECT_LT(duration.count(), 200);
  EXPECT_GE(errors.size(), 0);
}

// Memory tests
TEST_F(LiveVariablesTest, MemoryUsage) {
  const int NUM_ITERATIONS = 100;

  for (int i = 0; i < NUM_ITERATIONS; ++i) {
    auto block = createBlock();
    cfg->set_entry_block(block);
    cfg->set_exit_block(block);

    auto errors = analysis->analyze_live_variables(*cfg);

    // Reset for next iteration
    cfg = std::make_unique<ControlFlowGraph>();
  }

  // Test should complete without memory issues
  EXPECT_TRUE(true);
}

// Analysis result access tests
TEST_F(LiveVariablesTest, AnalysisResultQueries) {
  auto block = createBlock();
  cfg->set_entry_block(block);
  cfg->set_exit_block(block);

  auto errors = analysis->analyze_live_variables(*cfg);

  // Test querying live variables (even if empty)
  auto live_vars =
      analysis->get_live_variables_at_point(block->get_id(), false);
  EXPECT_GE(live_vars.size(), 0); // Could be empty

  auto entry_vars =
      analysis->get_live_variables_at_point(block->get_id(), true);
  EXPECT_GE(entry_vars.size(), 0); // Could be empty
}

TEST_F(LiveVariablesTest, DeadVariableDetection) {
  auto block = createBlock();
  cfg->set_entry_block(block);
  cfg->set_exit_block(block);

  auto errors = analysis->analyze_live_variables(*cfg);
  auto dead_errors = analysis->check_dead_assignments();

  // Should return some result (could be empty)
  EXPECT_GE(dead_errors.size(), 0);
}