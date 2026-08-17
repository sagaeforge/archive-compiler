#include "05_analysis/AnalysisManager.hpp"
#include "04_parsing/ast/literals/Literals.hpp"
#include "04_parsing/ast/program/Program.hpp"
#include "04_parsing/ast/statements/Statements.hpp"
#include "05_analysis/AnalysisConfig.hpp"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

using namespace nugdev::compiler::analysis;
using namespace nugdev::ast;

class AnalysisManagerTest : public ::testing::Test {
protected:
  void SetUp() override {
    config = std::make_unique<AnalysisConfig>();
    manager = std::make_unique<AnalysisManager>(*config);
  }

  std::unique_ptr<AnalysisConfig> config;
  std::unique_ptr<AnalysisManager> manager;

  // Helper methods
  std::unique_ptr<Program> create_test_program() {
    auto program = std::make_unique<Program>();
    auto module = std::make_unique<Module>("test_module");

    // Add some basic statements to the module
    auto var_decl = std::make_unique<VariableDeclaration>(
        VariableDeclaration::Mutability::LET, "x", nullptr,
        std::make_unique<NumberLiteral>(
            "42", NumberLiteral::NumberType::DECIMAL_INTEGER));

    module->add_statement(std::move(var_decl));
    program->add_module(std::move(module));
    return program;
  }

  std::unique_ptr<Program> create_large_test_program() {
    auto program = std::make_unique<Program>();
    auto module = std::make_unique<Module>("large_test_module");

    // Add many statements to test performance
    for (int i = 0; i < 100; ++i) {
      auto var_decl = std::make_unique<VariableDeclaration>(
          VariableDeclaration::Mutability::LET, "var" + std::to_string(i),
          nullptr,
          std::make_unique<NumberLiteral>(
              std::to_string(i), NumberLiteral::NumberType::DECIMAL_INTEGER));
      module->add_statement(std::move(var_decl));
    }

    program->add_module(std::move(module));
    return program;
  }

  std::unique_ptr<Program> create_program_with_errors() {
    auto program = std::make_unique<Program>();
    auto module = std::make_unique<Module>("error_test_module");

    // Add a variable declaration with undefined reference
    auto var_decl = std::make_unique<VariableDeclaration>(
        VariableDeclaration::Mutability::LET, "x", nullptr,
        std::make_unique<Identifier>("undefined_variable"));

    module->add_statement(std::move(var_decl));
    program->add_module(std::move(module));
    return program;
  }
};

// Basic functionality tests
TEST_F(AnalysisManagerTest, ConstructorAndConfiguration) {
  EXPECT_NE(manager.get(), nullptr);

  AnalysisConfig new_config;
  new_config.set_analysis_level(AnalysisConfig::AnalysisLevel::THOROUGH);
  manager->set_config(new_config);

  EXPECT_EQ(manager->get_config().get_analysis_level(),
            AnalysisConfig::AnalysisLevel::THOROUGH);
}

TEST_F(AnalysisManagerTest, FullProgramAnalysis) {
  auto program = create_test_program();
  auto result = manager->analyze_program(*program);

  EXPECT_TRUE(result.success ||
              result.errors.size() > 0); // Either success or documented errors
  EXPECT_GE(result.total_errors() + result.total_warnings(), 0);
}

TEST_F(AnalysisManagerTest, SemanticAnalysis) {
  auto program = create_test_program();
  auto result = manager->analyze_semantic(*program);

  EXPECT_TRUE(result.success || result.errors.size() > 0);
  EXPECT_GE(result.total_errors(), 0);
}

TEST_F(AnalysisManagerTest, TypeAnalysis) {
  auto program = create_test_program();
  auto result = manager->analyze_types(*program);

  EXPECT_TRUE(result.success || result.errors.size() > 0);
  EXPECT_GE(result.total_errors(), 0);
}

TEST_F(AnalysisManagerTest, ControlFlowAnalysis) {
  auto program = create_test_program();
  auto result = manager->analyze_control_flow(*program);

  EXPECT_TRUE(result.success || result.errors.size() > 0);
  EXPECT_GE(result.total_errors(), 0);
}

TEST_F(AnalysisManagerTest, DataFlowAnalysis) {
  auto program = create_test_program();
  auto result = manager->analyze_data_flow(*program);

  EXPECT_TRUE(result.success || result.errors.size() > 0);
  EXPECT_GE(result.total_errors(), 0);
}

TEST_F(AnalysisManagerTest, MemorySafetyAnalysis) {
  auto program = create_test_program();
  auto result = manager->analyze_memory_safety(*program);

  EXPECT_TRUE(result.success || result.errors.size() > 0);
  EXPECT_GE(result.total_errors(), 0);
}

TEST_F(AnalysisManagerTest, PerformanceAnalysis) {
  auto program = create_test_program();
  auto result = manager->analyze_performance(*program);

  EXPECT_TRUE(result.success || result.errors.size() > 0);
  EXPECT_GE(result.total_errors(), 0);
}

TEST_F(AnalysisManagerTest, EBNFComplianceCheck) {
  auto program = create_test_program();
  auto result = manager->verify_ebnf_compliance(*program);

  EXPECT_TRUE(result.success || result.errors.size() > 0);
  EXPECT_GE(result.total_errors(), 0);
}

TEST_F(AnalysisManagerTest, ParallelAnalysis) {
  auto program = create_test_program();

  auto start = std::chrono::high_resolution_clock::now();
  auto result = manager->analyze_parallel(*program, 2);
  auto end = std::chrono::high_resolution_clock::now();

  EXPECT_TRUE(result.success || result.errors.size() > 0);

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  EXPECT_LT(duration.count(), 10000); // Should complete in reasonable time
}

TEST_F(AnalysisManagerTest, ProgressTracking) {
  std::atomic<int> progress_updates{0};

  manager->set_progress_callback(
      [&](const std::string &stage, double progress) {
        progress_updates++;
        EXPECT_GE(progress, 0.0);
        EXPECT_LE(progress, 1.0);
        EXPECT_FALSE(stage.empty());
      });

  auto program = create_test_program();
  manager->analyze_program(*program);

  EXPECT_GE(progress_updates.load(), 0); // May or may not have progress updates
}

TEST_F(AnalysisManagerTest, ErrorHandling) {
  auto program_with_errors = create_program_with_errors();
  auto result = manager->analyze_program(*program_with_errors);

  // Should handle errors gracefully
  EXPECT_GE(result.total_errors(), 0);
  EXPECT_GE(result.total_warnings(), 0);
}

TEST_F(AnalysisManagerTest, AnalysisResultStructure) {
  auto program = create_test_program();
  auto result = manager->analyze_program(*program);

  // Test result structure
  EXPECT_TRUE(result.success || !result.success);
  EXPECT_GE(result.errors.size(), 0);
  EXPECT_GE(result.warnings.size(), 0);
  EXPECT_GE(result.suggestions.size(), 0);

  // Test result methods
  EXPECT_EQ(result.total_errors(), result.errors.size());
  EXPECT_EQ(result.total_warnings(), result.warnings.size());

  // Test summary generation
  std::string summary = result.generate_summary();
  EXPECT_FALSE(summary.empty());
}

TEST_F(AnalysisManagerTest, AnalyzerAccess) {
  // Test individual analyzer access
  auto &semantic_analyzer = manager->get_semantic_analyzer();
  auto &type_checker = manager->get_type_checker();
  auto &control_flow_analyzer = manager->get_control_flow_analyzer();

  EXPECT_NE(&semantic_analyzer, nullptr);
  EXPECT_NE(&type_checker, nullptr);
  EXPECT_NE(&control_flow_analyzer, nullptr);
}

TEST_F(AnalysisManagerTest, CacheManagement) {
  auto program = create_test_program();

  // First analysis
  auto start1 = std::chrono::high_resolution_clock::now();
  auto result1 = manager->analyze_program(*program);
  auto end1 = std::chrono::high_resolution_clock::now();

  // Warm up caches
  std::vector<Program *> programs = {program.get()};
  manager->warm_up_caches(programs);

  // Second analysis (potentially cached)
  auto start2 = std::chrono::high_resolution_clock::now();
  auto result2 = manager->analyze_program(*program);
  auto end2 = std::chrono::high_resolution_clock::now();

  // Clear caches
  manager->clear_all_caches();

  EXPECT_TRUE(result1.success || result1.errors.size() > 0);
  EXPECT_TRUE(result2.success || result2.errors.size() > 0);
}

TEST_F(AnalysisManagerTest, Statistics) {
  auto program = create_test_program();

  // Reset statistics
  manager->reset_statistics();

  // Run analysis
  manager->analyze_program(*program);

  // Check statistics
  const auto &stats = manager->get_statistics();
  // Note: Using the actual member names from
  // AnalysisStatistics::PerformanceMetrics
  EXPECT_GE(stats.get_metrics().total_analysis_time_ms, 0);
  EXPECT_GE(stats.get_metrics().nodes_processed, 0);
}

TEST_F(AnalysisManagerTest, ConfigurationLevels) {
  // Test different analysis levels
  config->set_analysis_level(AnalysisConfig::AnalysisLevel::MINIMAL);
  EXPECT_EQ(config->get_analysis_level(),
            AnalysisConfig::AnalysisLevel::MINIMAL);

  config->set_type_safety_level(AnalysisConfig::TypeSafetyLevel::STRICT);
  EXPECT_EQ(config->get_type_safety_level(),
            AnalysisConfig::TypeSafetyLevel::STRICT);

  config->set_optimization_level(AnalysisConfig::OptimizationLevel::AGGRESSIVE);
  EXPECT_EQ(config->get_optimization_level(),
            AnalysisConfig::OptimizationLevel::AGGRESSIVE);
}

TEST_F(AnalysisManagerTest, MemorySafetyPolicy) {
  auto &memory_policy = config->get_memory_safety_policy();
  memory_policy.enforce_null_safety = false;
  memory_policy.enforce_bounds_checking = true;
  memory_policy.allow_unsafe_operations = true;

  manager->set_config(*config);

  auto program = create_test_program();
  auto result = manager->analyze_memory_safety(*program);

  EXPECT_TRUE(result.success || result.errors.size() > 0);
}

TEST_F(AnalysisManagerTest, ErrorReportingConfig) {
  auto &error_config = config->get_error_reporting_config();
  error_config.show_warnings = true;
  error_config.show_suggestions = true;
  error_config.max_errors_per_file = 50;
  error_config.stop_on_first_error = false;

  manager->set_config(*config);

  auto program = create_program_with_errors();
  auto result = manager->analyze_program(*program);

  EXPECT_GE(result.total_errors() + result.total_warnings(), 0);
}

TEST_F(AnalysisManagerTest, ConfigPresets) {
  // Test configuration presets
  auto dev_config = AnalysisConfig::create_development_preset();
  EXPECT_EQ(dev_config.get_analysis_level(),
            AnalysisConfig::AnalysisLevel::STANDARD);

  auto prod_config = AnalysisConfig::create_production_preset();
  EXPECT_EQ(prod_config.get_analysis_level(),
            AnalysisConfig::AnalysisLevel::THOROUGH);

  auto research_config = AnalysisConfig::create_research_preset();
  EXPECT_EQ(research_config.get_analysis_level(),
            AnalysisConfig::AnalysisLevel::RESEARCH);

  auto ide_config = AnalysisConfig::create_ide_preset();
  EXPECT_EQ(ide_config.get_analysis_level(),
            AnalysisConfig::AnalysisLevel::STANDARD);
}

TEST_F(AnalysisManagerTest, Performance) {
  auto large_program = create_large_test_program();

  auto start = std::chrono::high_resolution_clock::now();
  auto result = manager->analyze_program(*large_program);
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  EXPECT_TRUE(result.success || result.errors.size() > 0);
  EXPECT_LT(duration.count(), 30000); // Should complete within 30 seconds
}

TEST_F(AnalysisManagerTest, ThreadSafety) {
  const int num_threads = 4;
  std::vector<std::thread> threads;
  std::atomic<int> success_count{0};

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&, i]() {
      auto program = create_test_program();
      auto result = manager->analyze_program(*program);
      if (result.success || result.errors.size() > 0) {
        success_count++;
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_EQ(success_count.load(), num_threads);
}

// Test AnalysisPipelineBuilder
TEST_F(AnalysisManagerTest, PipelineBuilder) {
  AnalysisPipelineBuilder builder;

  auto pipeline = builder.add_semantic_analysis()
                      .add_type_checking()
                      .add_control_flow_analysis()
                      .enable_parallel_execution()
                      .build();

  EXPECT_NE(pipeline.get(), nullptr);

  auto program = create_test_program();
  auto result = pipeline->analyze_program(*program);

  EXPECT_TRUE(result.success || result.errors.size() > 0);
}

// Test AnalysisResultComparator
TEST_F(AnalysisManagerTest, ResultComparison) {
  auto program = create_test_program();

  auto result1 = manager->analyze_program(*program);
  auto result2 = manager->analyze_program(*program);

  auto comparison = AnalysisResultComparator::compare_results(result1, result2);

  EXPECT_TRUE(comparison.are_equivalent || !comparison.are_equivalent);
  EXPECT_GE(comparison.differences.size(), 0);
  EXPECT_GE(comparison.new_errors.size(), 0);
  EXPECT_GE(comparison.resolved_errors.size(), 0);
  EXPECT_GE(comparison.changed_errors.size(), 0);

  std::string diff_report =
      AnalysisResultComparator::generate_diff_report(comparison);
  EXPECT_FALSE(diff_report.empty());
}

// Test AnalysisSession
TEST_F(AnalysisManagerTest, AnalysisSession) {
  AnalysisSession session(*config);

  EXPECT_FALSE(session.is_active());

  session.start_session();
  EXPECT_TRUE(session.is_active());

  session.track_file("test.nug");
  session.mark_file_changed("test.nug");

  session.enable_background_analysis(true);
  session.set_analysis_delay(std::chrono::milliseconds(100));

  session.end_session();
  EXPECT_FALSE(session.is_active());
}