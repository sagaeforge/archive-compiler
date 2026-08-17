#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 전체 분석 시스템의 중앙 설정 관리
 *
 * 모든 분석기들의 설정을 통합 관리하여 일관성 있는 분석 수행
 */
class AnalysisConfig {
public:
  /**
   * @brief 전체 분석 레벨
   */
  enum class AnalysisLevel {
    MINIMAL,  // 최소한의 에러 검사만
    STANDARD, // 표준 분석 (권장)
    THOROUGH, // 철저한 분석
    RESEARCH  // 실험적 분석 포함
  };

  /**
   * @brief 타입 안전성 정책
   */
  enum class TypeSafetyLevel {
    PERMISSIVE, // 관대한 타입 검사
    STANDARD,   // 표준 타입 검사
    STRICT,     // 엄격한 타입 검사
    ZERO_COST   // Zero-cost 추상화 검증
  };

  /**
   * @brief 최적화 공격성 레벨
   */
  enum class OptimizationLevel {
    NONE,        // 최적화 없음
    BASIC,       // 기본 최적화
    AGGRESSIVE,  // 공격적 최적화
    EXPERIMENTAL // 실험적 최적화
  };

  /**
   * @brief 메모리 안전성 정책
   */
  struct MemorySafetyPolicy {
    bool enforce_null_safety = true;
    bool enforce_bounds_checking = true;
    bool enforce_ownership_rules = true;
    bool allow_unsafe_operations = false;
    bool detect_memory_leaks = true;
  };

  /**
   * @brief 성능 분석 설정
   */
  struct PerformanceAnalysisConfig {
    bool analyze_complexity = true;
    bool detect_inefficiencies = true;
    bool suggest_optimizations = true;
    size_t max_recursion_depth = 1000;
    double performance_threshold = 0.1; // 10% 성능 저하 임계값
  };

  /**
   * @brief 컴파일 타임 분석 설정
   */
  struct CompileTimeConfig {
    bool enable_constexpr_evaluation = true;
    bool enable_template_metaprogramming = true;
    bool enable_static_assertions = true;
    bool cache_compile_time_results = true;
    size_t max_cache_size = 10000;
    bool aggressive_constant_folding = true;
  };

  /**
   * @brief 에러 보고 설정
   */
  struct ErrorReportingConfig {
    bool show_warnings = true;
    bool show_suggestions = true;
    bool show_performance_hints = true;
    size_t max_errors_per_file = 100;
    bool stop_on_first_error = false;
    bool colorize_output = true;
  };

  /**
   * @brief EBNF 호환성 검사 설정
   */
  struct EBNFComplianceConfig {
    bool enforce_strict_grammar = true;
    bool check_operator_precedence = true;
    bool validate_reserved_keywords = true;
    bool detect_missing_features = true;
    bool suggest_ebnf_fixes = true;
  };

public:
  AnalysisConfig() = default;

  // 전역 설정
  void set_analysis_level(AnalysisLevel level);
  void set_type_safety_level(TypeSafetyLevel level);
  void set_optimization_level(OptimizationLevel level);

  AnalysisLevel get_analysis_level() const { return m_analysis_level; }
  TypeSafetyLevel get_type_safety_level() const { return m_type_safety_level; }
  OptimizationLevel get_optimization_level() const {
    return m_optimization_level;
  }

  // 세부 정책 설정
  MemorySafetyPolicy &get_memory_safety_policy() { return m_memory_safety; }
  const MemorySafetyPolicy &get_memory_safety_policy() const {
    return m_memory_safety;
  }

  PerformanceAnalysisConfig &get_performance_config() {
    return m_performance_config;
  }
  const PerformanceAnalysisConfig &get_performance_config() const {
    return m_performance_config;
  }

  CompileTimeConfig &get_compile_time_config() { return m_compile_time_config; }
  const CompileTimeConfig &get_compile_time_config() const {
    return m_compile_time_config;
  }

  ErrorReportingConfig &get_error_reporting_config() {
    return m_error_reporting;
  }
  const ErrorReportingConfig &get_error_reporting_config() const {
    return m_error_reporting;
  }

  EBNFComplianceConfig &get_ebnf_compliance_config() {
    return m_ebnf_compliance;
  }
  const EBNFComplianceConfig &get_ebnf_compliance_config() const {
    return m_ebnf_compliance;
  }

  // 프리셋 설정
  static AnalysisConfig create_development_preset();
  static AnalysisConfig create_production_preset();
  static AnalysisConfig create_research_preset();
  static AnalysisConfig create_ide_preset();

  // 설정 유효성 검사
  std::vector<std::string> validate_config() const;

  // 설정 직렬화
  std::string to_json() const;
  bool from_json(const std::string &json);

  // 환경변수에서 설정 로드
  void load_from_environment();

  // 파일에서 설정 로드
  bool load_from_file(const std::string &config_file);
  bool save_to_file(const std::string &config_file) const;

private:
  AnalysisLevel m_analysis_level = AnalysisLevel::STANDARD;
  TypeSafetyLevel m_type_safety_level = TypeSafetyLevel::STANDARD;
  OptimizationLevel m_optimization_level = OptimizationLevel::BASIC;

  MemorySafetyPolicy m_memory_safety;
  PerformanceAnalysisConfig m_performance_config;
  CompileTimeConfig m_compile_time_config;
  ErrorReportingConfig m_error_reporting;
  EBNFComplianceConfig m_ebnf_compliance;

  // 설정 검증 헬퍼
  bool is_level_combination_valid() const;
  void apply_level_constraints();
};

/**
 * @brief 분석기별 개별 설정 관리
 */
class AnalyzerSpecificConfig {
public:
  // 개별 분석기 활성화/비활성화
  struct AnalyzerFlags {
    bool enable_semantic_analysis = true;
    bool enable_type_checking = true;
    bool enable_control_flow_analysis = true;
    bool enable_data_flow_analysis = true;
    bool enable_memory_safety_analysis = true;
    bool enable_performance_analysis = true;
    bool enable_compile_time_evaluation = true;
    bool enable_ebnf_compliance_check = true;
    bool enable_static_analysis = true;
    bool enable_meta_programming = true;
  };

  AnalyzerFlags &get_analyzer_flags() { return m_analyzer_flags; }
  const AnalyzerFlags &get_analyzer_flags() const { return m_analyzer_flags; }

  // 분석기별 세부 설정
  void configure_analyzer(
      const std::string &analyzer_name,
      const std::unordered_map<std::string, std::string> &options);

  std::unordered_map<std::string, std::string>
  get_analyzer_config(const std::string &analyzer_name) const;

private:
  AnalyzerFlags m_analyzer_flags;
  std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
      m_analyzer_configs;
};

/**
 * @brief 런타임 분석 통계 수집
 */
class AnalysisStatistics {
public:
  struct PerformanceMetrics {
    size_t total_analysis_time_ms = 0;
    size_t semantic_analysis_time_ms = 0;
    size_t type_checking_time_ms = 0;
    size_t optimization_time_ms = 0;
    size_t nodes_processed = 0;
    size_t errors_found = 0;
    size_t warnings_generated = 0;
    size_t optimizations_applied = 0;
  };

  void record_analysis_start();
  void record_analysis_end();
  void record_analyzer_time(const std::string &analyzer_name, size_t time_ms);
  void increment_nodes_processed(size_t count = 1);
  void increment_errors_found(size_t count = 1);
  void increment_warnings_generated(size_t count = 1);
  void increment_optimizations_applied(size_t count = 1);

  const PerformanceMetrics &get_metrics() const { return m_metrics; }

  std::string generate_performance_report() const;
  void reset_statistics();

private:
  PerformanceMetrics m_metrics;
  std::chrono::steady_clock::time_point m_analysis_start_time;
  std::unordered_map<std::string, size_t> m_analyzer_times;
};

} // namespace nugdev::compiler::analysis