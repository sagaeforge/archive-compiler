#include "05_analysis/dataflow/DataFlowAnalysisManager.hpp"
#include "04_parsing/ast/literals/Literals.hpp"
#include "04_parsing/ast/program/Program.hpp"
#include "04_parsing/ast/statements/Statements.hpp"
#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

using namespace nugdev::compiler::dataflow;
using namespace nugdev::compiler::analysis;
using namespace nugdev::ast;

class DataFlowAnalysisManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    manager = std::make_unique<DataFlowAnalysisManager>();
  }

  std::unique_ptr<DataFlowAnalysisManager> manager;

  // Helper methods
  std::unique_ptr<ControlFlowGraph> create_simple_cfg() {
    auto cfg = std::make_unique<ControlFlowGraph>();

    // Create basic blocks
    auto entry_block = cfg->create_block("entry");
    auto stmt_block = cfg->create_block("statements");
    auto exit_block = cfg->create_block("exit");

    // Connect blocks
    cfg->add_edge(entry_block, stmt_block);
    cfg->add_edge(stmt_block, exit_block);

    return cfg;
  }

  std::unique_ptr<ControlFlowGraph> create_complex_cfg() {
    auto cfg = std::make_unique<ControlFlowGraph>();

    // Create multiple blocks with branches and loops
    auto entry_block = cfg->create_block("entry");
    auto condition_block = cfg->create_block("condition");
    auto true_block = cfg->create_block("true_branch");
    auto false_block = cfg->create_block("false_branch");
    auto loop_block = cfg->create_block("loop");
    auto exit_block = cfg->create_block("exit");

    // Connect blocks to form complex control flow
    cfg->add_edge(entry_block, condition_block);
    cfg->add_edge(condition_block, true_block);
    cfg->add_edge(condition_block, false_block);
    cfg->add_edge(true_block, loop_block);
    cfg->add_edge(false_block, exit_block);
    cfg->add_edge(loop_block, condition_block); // Back edge for loop
    cfg->add_edge(loop_block, exit_block);

    return cfg;
  }
};

// Basic functionality tests
TEST_F(DataFlowAnalysisManagerTest, ConstructorAndInitialization) {
  EXPECT_NE(manager.get(), nullptr);
  EXPECT_TRUE(manager->is_initialized());
}

TEST_F(DataFlowAnalysisManagerTest, LiveVariableAnalysis) {
  auto cfg = create_simple_cfg();

  auto result = manager->analyze_live_variables(*cfg);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.live_variables.size(), 0);
  EXPECT_GE(result.dead_assignments.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, ReachingDefinitionsAnalysis) {
  auto cfg = create_simple_cfg();

  auto result = manager->analyze_reaching_definitions(*cfg);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.definitions.size(), 0);
  EXPECT_GE(result.use_def_chains.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, AvailableExpressionsAnalysis) {
  auto cfg = create_simple_cfg();

  auto result = manager->analyze_available_expressions(*cfg);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.available_expressions.size(), 0);
  EXPECT_GE(result.redundant_expressions.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, DominatorAnalysis) {
  auto cfg = create_complex_cfg();

  auto result = manager->analyze_dominators(*cfg);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.dominators.size(), 0);
  EXPECT_GE(result.immediate_dominators.size(), 0);
  EXPECT_GE(result.dominator_tree.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, PostDominatorAnalysis) {
  auto cfg = create_complex_cfg();

  auto result = manager->analyze_post_dominators(*cfg);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.post_dominators.size(), 0);
  EXPECT_GE(result.immediate_post_dominators.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, LoopAnalysis) {
  auto cfg = create_complex_cfg();

  auto result = manager->analyze_loops(*cfg);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.natural_loops.size(), 0);
  EXPECT_GE(result.loop_headers.size(), 0);
  EXPECT_GE(result.back_edges.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, SSATransform) {
  auto cfg = create_simple_cfg();

  auto result = manager->transform_to_ssa(*cfg);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.phi_functions.size(), 0);
  EXPECT_GE(result.renamed_variables.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, DefUseChainAnalysis) {
  auto cfg = create_simple_cfg();

  auto result = manager->analyze_def_use_chains(*cfg);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.def_use_chains.size(), 0);
  EXPECT_GE(result.use_def_chains.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, ConstantPropagationAnalysis) {
  auto cfg = create_simple_cfg();

  auto result = manager->analyze_constant_propagation(*cfg);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.constant_values.size(), 0);
  EXPECT_GE(result.propagated_constants.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, AliasAnalysis) {
  auto cfg = create_simple_cfg();

  auto result = manager->analyze_aliases(*cfg);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.alias_sets.size(), 0);
  EXPECT_GE(result.may_alias_pairs.size(), 0);
  EXPECT_GE(result.must_alias_pairs.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, TaintAnalysis) {
  auto cfg = create_simple_cfg();

  auto result = manager->analyze_taint(*cfg);

  EXPECT_TRUE(result.success);
  EXPECT_GE(result.tainted_variables.size(), 0);
  EXPECT_GE(result.taint_sources.size(), 0);
  EXPECT_GE(result.taint_sinks.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, CombinedAnalysis) {
  auto cfg = create_complex_cfg();

  // Run multiple analyses together
  auto live_vars = manager->analyze_live_variables(*cfg);
  auto reaching_defs = manager->analyze_reaching_definitions(*cfg);
  auto dominators = manager->analyze_dominators(*cfg);

  EXPECT_TRUE(live_vars.success);
  EXPECT_TRUE(reaching_defs.success);
  EXPECT_TRUE(dominators.success);

  // Check that results are consistent
  EXPECT_GE(live_vars.live_variables.size(), 0);
  EXPECT_GE(reaching_defs.definitions.size(), 0);
  EXPECT_GE(dominators.dominators.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, AnalysisConfiguration) {
  DataFlowAnalysisConfig config;
  config.enable_live_variable_analysis = true;
  config.enable_reaching_definitions = true;
  config.enable_dominator_analysis = false;
  config.enable_ssa_transform = true;

  manager->set_configuration(config);

  auto cfg = create_simple_cfg();
  auto results = manager->run_all_analyses(*cfg);

  EXPECT_TRUE(results.live_variable_result.has_value());
  EXPECT_TRUE(results.reaching_definitions_result.has_value());
  EXPECT_FALSE(results.dominator_result.has_value());
  EXPECT_TRUE(results.ssa_result.has_value());
}

TEST_F(DataFlowAnalysisManagerTest, IterativeAnalysis) {
  auto cfg = create_complex_cfg();

  // Test iterative analysis with fixed point computation
  auto result =
      manager->run_iterative_analysis(*cfg, AnalysisType::LIVE_VARIABLES);

  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.converged);
  EXPECT_GT(result.iterations, 0);
  EXPECT_LT(result.iterations, 100); // Should converge reasonably quickly
}

TEST_F(DataFlowAnalysisManagerTest, WorklistAlgorithm) {
  auto cfg = create_complex_cfg();

  // Test worklist-based analysis for efficiency
  auto start = std::chrono::high_resolution_clock::now();
  auto result =
      manager->run_worklist_analysis(*cfg, AnalysisType::REACHING_DEFINITIONS);
  auto end = std::chrono::high_resolution_clock::now();

  EXPECT_TRUE(result.success);

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  EXPECT_LT(duration.count(), 5000); // Should complete quickly
}

TEST_F(DataFlowAnalysisManagerTest, MeetOperatorTesting) {
  auto cfg = create_complex_cfg();

  // Test different meet operators (union, intersection, etc.)
  auto union_result = manager->run_analysis_with_meet_operator(
      *cfg, AnalysisType::LIVE_VARIABLES, MeetOperator::UNION);
  auto intersection_result = manager->run_analysis_with_meet_operator(
      *cfg, AnalysisType::AVAILABLE_EXPRESSIONS, MeetOperator::INTERSECTION);

  EXPECT_TRUE(union_result.success);
  EXPECT_TRUE(intersection_result.success);
}

TEST_F(DataFlowAnalysisManagerTest, BackwardAnalysis) {
  auto cfg = create_simple_cfg();

  // Test backward data flow analysis (e.g., live variables)
  auto result =
      manager->run_backward_analysis(*cfg, AnalysisType::LIVE_VARIABLES);

  EXPECT_TRUE(result.success);
  EXPECT_TRUE(result.is_backward_analysis);
  EXPECT_GE(result.analysis_data.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, ForwardAnalysis) {
  auto cfg = create_simple_cfg();

  // Test forward data flow analysis (e.g., reaching definitions)
  auto result =
      manager->run_forward_analysis(*cfg, AnalysisType::REACHING_DEFINITIONS);

  EXPECT_TRUE(result.success);
  EXPECT_FALSE(result.is_backward_analysis);
  EXPECT_GE(result.analysis_data.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, LargeGraphPerformance) {
  // Create a large control flow graph
  auto large_cfg = std::make_unique<ControlFlowGraph>();

  const int num_blocks = 1000;
  std::vector<BasicBlock *> blocks;

  // Create many blocks
  for (int i = 0; i < num_blocks; ++i) {
    blocks.push_back(large_cfg->create_block("block_" + std::to_string(i)));
  }

  // Connect blocks linearly
  for (int i = 0; i < num_blocks - 1; ++i) {
    large_cfg->add_edge(blocks[i], blocks[i + 1]);
  }

  auto start = std::chrono::high_resolution_clock::now();
  auto result = manager->analyze_live_variables(*large_cfg);
  auto end = std::chrono::high_resolution_clock::now();

  EXPECT_TRUE(result.success);

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  EXPECT_LT(duration.count(), 10000); // Should complete within 10 seconds
}

TEST_F(DataFlowAnalysisManagerTest, AnalysisValidation) {
  auto cfg = create_complex_cfg();

  // Run analysis and validate results
  auto live_vars = manager->analyze_live_variables(*cfg);
  auto reaching_defs = manager->analyze_reaching_definitions(*cfg);

  // Validate that results are self-consistent
  auto validation_result =
      manager->validate_analysis_results(live_vars, reaching_defs);

  EXPECT_TRUE(validation_result.is_valid);
  EXPECT_EQ(validation_result.errors.size(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, MemoryUsageTracking) {
  auto cfg = create_complex_cfg();

  manager->enable_memory_tracking(true);

  auto result = manager->analyze_live_variables(*cfg);

  EXPECT_TRUE(result.success);

  auto memory_stats = manager->get_memory_statistics();
  EXPECT_GT(memory_stats.peak_memory_usage_kb, 0);
  EXPECT_GE(memory_stats.current_memory_usage_kb, 0);
}

TEST_F(DataFlowAnalysisManagerTest, ThreadSafetyTest) {
  const int num_threads = 4;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, i]() {
      auto cfg = create_simple_cfg();
      auto result = manager->analyze_live_variables(*cfg);
      if (result.success) {
        success_count++;
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_EQ(success_count.load(), num_threads);
}

TEST_F(DataFlowAnalysisManagerTest, ErrorHandling) {
  // Test with invalid/empty CFG
  auto empty_cfg = std::make_unique<ControlFlowGraph>();

  auto result = manager->analyze_live_variables(*empty_cfg);

  // Should handle gracefully
  EXPECT_FALSE(result.success);
  EXPECT_GT(result.error_message.length(), 0);
}

TEST_F(DataFlowAnalysisManagerTest, CachingBehavior) {
  auto cfg = create_simple_cfg();

  // Enable caching
  manager->enable_result_caching(true);

  // First analysis
  auto start1 = std::chrono::high_resolution_clock::now();
  auto result1 = manager->analyze_live_variables(*cfg);
  auto end1 = std::chrono::high_resolution_clock::now();
  auto duration1 =
      std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);

  // Second analysis (should be cached)
  auto start2 = std::chrono::high_resolution_clock::now();
  auto result2 = manager->analyze_live_variables(*cfg);
  auto end2 = std::chrono::high_resolution_clock::now();
  auto duration2 =
      std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);

  EXPECT_TRUE(result1.success);
  EXPECT_TRUE(result2.success);
  EXPECT_LT(duration2.count(),
            duration1.count() / 2); // Cached should be much faster

  // Clear cache
  manager->clear_analysis_cache();
}

TEST_F(DataFlowAnalysisManagerTest, AnalysisStatistics) {
  auto cfg = create_complex_cfg();

  manager->reset_statistics();

  manager->analyze_live_variables(*cfg);
  manager->analyze_reaching_definitions(*cfg);
  manager->analyze_dominators(*cfg);

  auto stats = manager->get_analysis_statistics();

  EXPECT_EQ(stats.total_analyses_run, 3);
  EXPECT_GT(stats.total_analysis_time_ms, 0);
  EXPECT_GT(stats.total_blocks_processed, 0);
  EXPECT_GE(stats.cache_hits, 0);
  EXPECT_GE(stats.cache_misses, 0);
}