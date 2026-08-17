#include "05_analysis/AnalysisConfig.hpp"
#include <gtest/gtest.h>
#include <sstream>

using namespace nugdev::compiler::analysis;

class AnalysisConfigTest : public ::testing::Test {
protected:
  void SetUp() override { config = std::make_unique<AnalysisConfig>(); }

  void TearDown() override { config.reset(); }

  std::unique_ptr<AnalysisConfig> config;
};

// 기본 설정 테스트
TEST_F(AnalysisConfigTest, DefaultConfiguration) {
  EXPECT_EQ(config->get_analysis_level(),
            AnalysisConfig::AnalysisLevel::STANDARD);
  EXPECT_EQ(config->get_type_safety_level(),
            AnalysisConfig::TypeSafetyLevel::STANDARD);
  EXPECT_EQ(config->get_optimization_level(),
            AnalysisConfig::OptimizationLevel::BASIC);
}

// 분석 레벨 설정 테스트
TEST_F(AnalysisConfigTest, SetAnalysisLevel) {
  config->set_analysis_level(AnalysisConfig::AnalysisLevel::THOROUGH);
  EXPECT_EQ(config->get_analysis_level(),
            AnalysisConfig::AnalysisLevel::THOROUGH);

  config->set_analysis_level(AnalysisConfig::AnalysisLevel::MINIMAL);
  EXPECT_EQ(config->get_analysis_level(),
            AnalysisConfig::AnalysisLevel::MINIMAL);
}

// 타입 안전성 레벨 설정 테스트
TEST_F(AnalysisConfigTest, SetTypeSafetyLevel) {
  config->set_type_safety_level(AnalysisConfig::TypeSafetyLevel::STRICT);
  EXPECT_EQ(config->get_type_safety_level(),
            AnalysisConfig::TypeSafetyLevel::STRICT);

  config->set_type_safety_level(AnalysisConfig::TypeSafetyLevel::ZERO_COST);
  EXPECT_EQ(config->get_type_safety_level(),
            AnalysisConfig::TypeSafetyLevel::ZERO_COST);
}

// 최적화 레벨 설정 테스트
TEST_F(AnalysisConfigTest, SetOptimizationLevel) {
  config->set_optimization_level(AnalysisConfig::OptimizationLevel::AGGRESSIVE);
  EXPECT_EQ(config->get_optimization_level(),
            AnalysisConfig::OptimizationLevel::AGGRESSIVE);

  config->set_optimization_level(AnalysisConfig::OptimizationLevel::NONE);
  EXPECT_EQ(config->get_optimization_level(),
            AnalysisConfig::OptimizationLevel::NONE);
}

// 개발용 프리셋 테스트
TEST_F(AnalysisConfigTest, DevelopmentPreset) {
  auto dev_config = AnalysisConfig::create_development_preset();

  EXPECT_EQ(dev_config.get_analysis_level(),
            AnalysisConfig::AnalysisLevel::STANDARD);
  EXPECT_EQ(dev_config.get_type_safety_level(),
            AnalysisConfig::TypeSafetyLevel::STANDARD);
  EXPECT_EQ(dev_config.get_optimization_level(),
            AnalysisConfig::OptimizationLevel::BASIC);

  const auto &error_config = dev_config.get_error_reporting_config();
  EXPECT_TRUE(error_config.show_warnings);
  EXPECT_TRUE(error_config.show_suggestions);
  EXPECT_TRUE(error_config.colorize_output);
  EXPECT_EQ(error_config.max_errors_per_file, 50u);
}

// 프로덕션용 프리셋 테스트
TEST_F(AnalysisConfigTest, ProductionPreset) {
  auto prod_config = AnalysisConfig::create_production_preset();

  EXPECT_EQ(prod_config.get_analysis_level(),
            AnalysisConfig::AnalysisLevel::THOROUGH);
  EXPECT_EQ(prod_config.get_type_safety_level(),
            AnalysisConfig::TypeSafetyLevel::STRICT);
  EXPECT_EQ(prod_config.get_optimization_level(),
            AnalysisConfig::OptimizationLevel::AGGRESSIVE);

  const auto &memory_config = prod_config.get_memory_safety_policy();
  EXPECT_TRUE(memory_config.enforce_null_safety);
  EXPECT_TRUE(memory_config.enforce_bounds_checking);
  EXPECT_FALSE(memory_config.allow_unsafe_operations);
  EXPECT_TRUE(memory_config.detect_memory_leaks);
}

// 연구용 프리셋 테스트
TEST_F(AnalysisConfigTest, ResearchPreset) {
  auto research_config = AnalysisConfig::create_research_preset();

  EXPECT_EQ(research_config.get_analysis_level(),
            AnalysisConfig::AnalysisLevel::RESEARCH);
  EXPECT_EQ(research_config.get_type_safety_level(),
            AnalysisConfig::TypeSafetyLevel::ZERO_COST);
  EXPECT_EQ(research_config.get_optimization_level(),
            AnalysisConfig::OptimizationLevel::EXPERIMENTAL);

  const auto &compile_config = research_config.get_compile_time_config();
  EXPECT_TRUE(compile_config.enable_template_metaprogramming);
  EXPECT_TRUE(compile_config.enable_static_assertions);
  EXPECT_TRUE(compile_config.aggressive_constant_folding);
  EXPECT_EQ(compile_config.max_cache_size, 20000u);
}

// IDE용 프리셋 테스트
TEST_F(AnalysisConfigTest, IDEPreset) {
  auto ide_config = AnalysisConfig::create_ide_preset();

  EXPECT_EQ(ide_config.get_analysis_level(),
            AnalysisConfig::AnalysisLevel::STANDARD);

  const auto &error_config = ide_config.get_error_reporting_config();
  EXPECT_TRUE(error_config.show_warnings);
  EXPECT_TRUE(error_config.show_suggestions);
  EXPECT_TRUE(error_config.show_performance_hints);
  EXPECT_FALSE(error_config.stop_on_first_error);
  EXPECT_FALSE(error_config.colorize_output); // IDE가 색상 처리
}

// 설정 검증 테스트
TEST_F(AnalysisConfigTest, ValidateConfig) {
  // 유효한 설정
  auto errors = config->validate_config();
  EXPECT_TRUE(errors.empty());

  // 무효한 조합: ZERO_COST + NONE optimization
  config->set_type_safety_level(AnalysisConfig::TypeSafetyLevel::ZERO_COST);
  config->set_optimization_level(AnalysisConfig::OptimizationLevel::NONE);

  errors = config->validate_config();
  EXPECT_FALSE(errors.empty());
  EXPECT_TRUE(
      std::any_of(errors.begin(), errors.end(), [](const std::string &error) {
        return error.find("Invalid combination") != std::string::npos;
      }));
}

// JSON 직렬화 테스트
TEST_F(AnalysisConfigTest, JSONSerialization) {
  config->set_analysis_level(AnalysisConfig::AnalysisLevel::THOROUGH);
  config->set_type_safety_level(AnalysisConfig::TypeSafetyLevel::STRICT);

  std::string json = config->to_json();

  // JSON에 필수 필드가 포함되어 있는지 확인
  EXPECT_NE(json.find("analysis_level"), std::string::npos);
  EXPECT_NE(json.find("type_safety_level"), std::string::npos);
  EXPECT_NE(json.find("optimization_level"), std::string::npos);
  EXPECT_NE(json.find("memory_safety"), std::string::npos);
  EXPECT_NE(json.find("error_reporting"), std::string::npos);
}

// 메모리 안전성 정책 테스트
TEST_F(AnalysisConfigTest, MemorySafetyPolicy) {
  auto &policy = config->get_memory_safety_policy();

  // 기본값 확인
  EXPECT_TRUE(policy.enforce_null_safety);
  EXPECT_TRUE(policy.enforce_bounds_checking);
  EXPECT_TRUE(policy.enforce_ownership_rules);
  EXPECT_FALSE(policy.allow_unsafe_operations);
  EXPECT_TRUE(policy.detect_memory_leaks);

  // 값 변경
  policy.allow_unsafe_operations = true;
  policy.detect_memory_leaks = false;

  EXPECT_TRUE(policy.allow_unsafe_operations);
  EXPECT_FALSE(policy.detect_memory_leaks);
}

// 성능 분석 설정 테스트
TEST_F(AnalysisConfigTest, PerformanceAnalysisConfig) {
  auto &perf_config = config->get_performance_config();

  // 기본값 확인
  EXPECT_TRUE(perf_config.analyze_complexity);
  EXPECT_TRUE(perf_config.detect_inefficiencies);
  EXPECT_TRUE(perf_config.suggest_optimizations);
  EXPECT_EQ(perf_config.max_recursion_depth, 1000u);
  EXPECT_DOUBLE_EQ(perf_config.performance_threshold, 0.1);

  // 값 변경
  perf_config.max_recursion_depth = 2000;
  perf_config.performance_threshold = 0.05;

  EXPECT_EQ(perf_config.max_recursion_depth, 2000u);
  EXPECT_DOUBLE_EQ(perf_config.performance_threshold, 0.05);
}

// 컴파일 타임 설정 테스트
TEST_F(AnalysisConfigTest, CompileTimeConfig) {
  auto &compile_config = config->get_compile_time_config();

  // 기본값 확인
  EXPECT_TRUE(compile_config.enable_constexpr_evaluation);
  EXPECT_TRUE(compile_config.enable_template_metaprogramming);
  EXPECT_TRUE(compile_config.enable_static_assertions);
  EXPECT_TRUE(compile_config.cache_compile_time_results);
  EXPECT_EQ(compile_config.max_cache_size, 10000u);
  EXPECT_TRUE(compile_config.aggressive_constant_folding);

  // 값 변경
  compile_config.max_cache_size = 5000;
  compile_config.aggressive_constant_folding = false;

  EXPECT_EQ(compile_config.max_cache_size, 5000u);
  EXPECT_FALSE(compile_config.aggressive_constant_folding);
}

// 에러 보고 설정 테스트
TEST_F(AnalysisConfigTest, ErrorReportingConfig) {
  auto &error_config = config->get_error_reporting_config();

  // 기본값 확인
  EXPECT_TRUE(error_config.show_warnings);
  EXPECT_TRUE(error_config.show_suggestions);
  EXPECT_TRUE(error_config.show_performance_hints);
  EXPECT_EQ(error_config.max_errors_per_file, 100u);
  EXPECT_FALSE(error_config.stop_on_first_error);
  EXPECT_TRUE(error_config.colorize_output);

  // 값 변경
  error_config.max_errors_per_file = 50;
  error_config.stop_on_first_error = true;
  error_config.colorize_output = false;

  EXPECT_EQ(error_config.max_errors_per_file, 50u);
  EXPECT_TRUE(error_config.stop_on_first_error);
  EXPECT_FALSE(error_config.colorize_output);
}

// EBNF 호환성 설정 테스트
TEST_F(AnalysisConfigTest, EBNFComplianceConfig) {
  auto &ebnf_config = config->get_ebnf_compliance_config();

  // 기본값 확인
  EXPECT_TRUE(ebnf_config.enforce_strict_grammar);
  EXPECT_TRUE(ebnf_config.check_operator_precedence);
  EXPECT_TRUE(ebnf_config.validate_reserved_keywords);
  EXPECT_TRUE(ebnf_config.detect_missing_features);
  EXPECT_TRUE(ebnf_config.suggest_ebnf_fixes);

  // 값 변경
  ebnf_config.enforce_strict_grammar = false;
  ebnf_config.suggest_ebnf_fixes = false;

  EXPECT_FALSE(ebnf_config.enforce_strict_grammar);
  EXPECT_FALSE(ebnf_config.suggest_ebnf_fixes);
}

// 레벨 제약사항 적용 테스트
TEST_F(AnalysisConfigTest, ApplyLevelConstraints) {
  // MINIMAL 레벨 설정
  config->set_analysis_level(AnalysisConfig::AnalysisLevel::MINIMAL);

  const auto &error_config = config->get_error_reporting_config();
  EXPECT_LE(error_config.max_errors_per_file, 10u);

  // RESEARCH 레벨 설정
  config->set_analysis_level(AnalysisConfig::AnalysisLevel::RESEARCH);

  const auto &perf_config = config->get_performance_config();
  EXPECT_TRUE(perf_config.analyze_complexity);
  EXPECT_TRUE(perf_config.detect_inefficiencies);

  // STRICT 타입 안전성 설정
  config->set_type_safety_level(AnalysisConfig::TypeSafetyLevel::STRICT);

  const auto &memory_config = config->get_memory_safety_policy();
  EXPECT_TRUE(memory_config.enforce_null_safety);
  EXPECT_TRUE(memory_config.enforce_bounds_checking);
}

// 환경변수 로딩 테스트 (실제 환경변수는 설정하지 않고 함수 호출만 테스트)
TEST_F(AnalysisConfigTest, LoadFromEnvironment) {
  // 기본값 저장
  auto original_level = config->get_analysis_level();

  // 환경변수 로딩 시도 (실제 환경변수가 없어도 오류 없이 처리되어야 함)
  EXPECT_NO_THROW(config->load_from_environment());

  // 환경변수가 없으면 기본값 유지
  EXPECT_EQ(config->get_analysis_level(), original_level);
}