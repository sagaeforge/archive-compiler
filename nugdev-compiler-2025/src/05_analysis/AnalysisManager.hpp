#pragma once

#include "05_analysis/AnalysisConfig.hpp"
#include "05_analysis/errors/AnalysisError.hpp"
#include "05_analysis/errors/ErrorReporter.hpp"
#include "05_analysis/semantic/SymbolTable.hpp"

// Forward declarations for all analyzers
namespace nugdev::compiler::analysis {
class SemanticAnalyzer;
class TypeChecker;
class ControlFlowAnalyzer;
class CompileTimeEvaluator;
class StrongTypeSystem;
class MetaProgrammingEngine;
class StaticAnalyzer;
class EBNFComplianceAnalyzer;
class CompileTimeCache;
} // namespace nugdev::compiler::analysis

namespace nugdev::compiler::optimization {
class OptimizationManager;
}

namespace nugdev::compiler::dataflow {
class DataFlowAnalysisManager;
}

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 전체 분석 시스템의 중앙 매니저
 *
 * 모든 분석기들을 조율하고 관리하여 일관된 분석 결과 제공
 */
class AnalysisManager {
public:
  /**
   * @brief 분석 결과 종합 리포트
   */
  struct AnalysisResult {
    bool success = true;
    std::vector<AnalysisError> errors;
    std::vector<AnalysisError> warnings;
    std::vector<AnalysisError> suggestions;
    SymbolTable symbol_table;
    AnalysisStatistics statistics;

    // 분석 단계별 결과
    bool semantic_analysis_passed = false;
    bool type_checking_passed = false;
    bool control_flow_analysis_passed = false;
    bool memory_safety_passed = false;
    bool ebnf_compliance_passed = false;

    size_t total_errors() const { return errors.size(); }
    size_t total_warnings() const { return warnings.size(); }
    bool has_fatal_errors() const;
    std::string generate_summary() const;
  };

  /**
   * @brief 분석 진행 상황 콜백
   */
  using ProgressCallback =
      std::function<void(const std::string &stage, double progress)>;

public:
  explicit AnalysisManager(const AnalysisConfig &config = AnalysisConfig{});
  ~AnalysisManager();

  /**
   * @brief 전체 분석 실행 (메인 인터페이스)
   */
  AnalysisResult analyze_program(ast::Program &program);

  /**
   * @brief 단계별 분석 실행
   */
  AnalysisResult analyze_semantic(ast::Program &program);
  AnalysisResult analyze_types(ast::Program &program);
  AnalysisResult analyze_control_flow(ast::Program &program);
  AnalysisResult analyze_data_flow(ast::Program &program);
  AnalysisResult analyze_memory_safety(ast::Program &program);
  AnalysisResult analyze_performance(ast::Program &program);
  AnalysisResult verify_ebnf_compliance(ast::Program &program);

  /**
   * @brief 병렬 분석 실행
   */
  AnalysisResult analyze_parallel(ast::Program &program,
                                  size_t num_threads = 0);

  /**
   * @brief 점진적 분석 (IDE용)
   */
  AnalysisResult
  analyze_incremental(ast::Module &module,
                      const std::vector<std::string> &changed_files);

  // 설정 관리
  void set_config(const AnalysisConfig &config);
  const AnalysisConfig &get_config() const { return m_config; }

  // 진행 상황 모니터링
  void set_progress_callback(ProgressCallback callback) {
    m_progress_callback = callback;
  }

  // 분석기 개별 접근
  SemanticAnalyzer &get_semantic_analyzer();
  TypeChecker &get_type_checker();
  ControlFlowAnalyzer &get_control_flow_analyzer();

  // 캐시 관리
  void clear_all_caches();
  void warm_up_caches(const std::vector<ast::Program *> &programs);

  // 통계 및 성능
  const AnalysisStatistics &get_statistics() const;
  void reset_statistics();

private:
  AnalysisConfig m_config;
  AnalysisStatistics m_statistics;
  ProgressCallback m_progress_callback;

  // 모든 분석기들
  std::unique_ptr<SemanticAnalyzer> m_semantic_analyzer;
  std::unique_ptr<TypeChecker> m_type_checker;
  std::unique_ptr<ControlFlowAnalyzer> m_control_flow_analyzer;
  std::unique_ptr<CompileTimeEvaluator> m_compile_time_evaluator;
  std::unique_ptr<MetaProgrammingEngine> m_meta_programming_engine;
  std::unique_ptr<EBNFComplianceAnalyzer> m_ebnf_compliance_analyzer;
  std::unique_ptr<CompileTimeCache> m_compile_time_cache;

  std::unique_ptr<optimization::OptimizationManager> m_optimization_manager;
  std::unique_ptr<dataflow::DataFlowAnalysisManager> m_dataflow_manager;

  // 에러 관리
  std::unique_ptr<ErrorReporter> m_error_reporter;

  // 초기화 및 정리
  void initialize_analyzers();
  void configure_analyzers();
  void cleanup_analyzers();

  // 분석 파이프라인
  AnalysisResult run_analysis_pipeline(ast::Program &program);
  void run_preprocessing_stage(ast::Program &program, AnalysisResult &result);
  void run_core_analysis_stage(ast::Program &program, AnalysisResult &result);
  void run_optimization_stage(ast::Program &program, AnalysisResult &result);
  void run_validation_stage(ast::Program &program, AnalysisResult &result);

  // 에러 처리 및 복구
  bool can_continue_after_errors(const AnalysisResult &result) const;
  void attempt_error_recovery(ast::Program &program, AnalysisResult &result);

  // 병렬 처리 지원
  std::vector<std::future<AnalysisResult>>
  schedule_parallel_analysis(ast::Program &program, size_t num_threads);

  AnalysisResult merge_parallel_results(std::vector<AnalysisResult> &&results);

  // 진행 상황 보고
  void report_progress(const std::string &stage, double progress);

  // 의존성 관리
  bool check_analyzer_dependencies() const;
  std::vector<std::string> get_analysis_order() const;
};

/**
 * @brief 분석 파이프라인 빌더
 *
 * 사용자 정의 분석 파이프라인 구성을 위한 빌더 패턴
 */
class AnalysisPipelineBuilder {
public:
  AnalysisPipelineBuilder() = default;

  // 분석 단계 추가
  AnalysisPipelineBuilder &add_semantic_analysis();
  AnalysisPipelineBuilder &add_type_checking();
  AnalysisPipelineBuilder &add_control_flow_analysis();
  AnalysisPipelineBuilder &add_data_flow_analysis();
  AnalysisPipelineBuilder &add_memory_safety_analysis();
  AnalysisPipelineBuilder &add_performance_analysis();
  AnalysisPipelineBuilder &add_ebnf_compliance_check();
  AnalysisPipelineBuilder &add_optimization();

  // 조건부 분석 추가
  AnalysisPipelineBuilder &
  add_if(bool condition,
         std::function<AnalysisPipelineBuilder &()> builder_func);

  // 병렬 실행 설정
  AnalysisPipelineBuilder &enable_parallel_execution(size_t num_threads = 0);

  // 에러 처리 정책
  AnalysisPipelineBuilder &continue_on_errors(bool continue_flag = true);
  AnalysisPipelineBuilder &fail_fast(bool fail_fast_flag = true);

  // 파이프라인 구축
  std::unique_ptr<AnalysisManager>
  build(const AnalysisConfig &config = AnalysisConfig{});

private:
  struct PipelineStage {
    std::string name;
    std::function<void(AnalysisManager &, ast::Program &,
                       AnalysisManager::AnalysisResult &)>
        executor;
    bool parallel_capable = false;
    std::vector<std::string> dependencies;
  };

  std::vector<PipelineStage> m_stages;
  bool m_parallel_execution = false;
  size_t m_num_threads = 0;
  bool m_continue_on_errors = true;
  bool m_fail_fast = false;
};

/**
 * @brief 분석 결과 비교 및 차이점 감지
 */
class AnalysisResultComparator {
public:
  struct ComparisonResult {
    bool are_equivalent = true;
    std::vector<std::string> differences;
    std::vector<AnalysisError> new_errors;
    std::vector<AnalysisError> resolved_errors;
    std::vector<AnalysisError> changed_errors;
  };

  static ComparisonResult
  compare_results(const AnalysisManager::AnalysisResult &old_result,
                  const AnalysisManager::AnalysisResult &new_result);

  static bool are_errors_equivalent(const AnalysisError &error1,
                                    const AnalysisError &error2);

  static std::string generate_diff_report(const ComparisonResult &comparison);
};

/**
 * @brief 분석 세션 관리 (IDE 통합용)
 */
class AnalysisSession {
public:
  explicit AnalysisSession(const AnalysisConfig &config);

  // 세션 생명주기
  void start_session();
  void end_session();
  bool is_active() const { return m_active; }

  // 파일별 분석 상태 추적
  void track_file(const std::string &file_path);
  void untrack_file(const std::string &file_path);
  void mark_file_changed(const std::string &file_path);

  // 점진적 분석
  AnalysisManager::AnalysisResult
  analyze_changes(const std::vector<std::string> &changed_files);

  // 실시간 분석 결과 조회
  std::vector<AnalysisError>
  get_errors_for_file(const std::string &file_path) const;
  std::vector<AnalysisError>
  get_warnings_for_file(const std::string &file_path) const;

  // 성능 최적화
  void enable_background_analysis(bool enable = true);
  void set_analysis_delay(std::chrono::milliseconds delay);

private:
  AnalysisConfig m_config;
  std::unique_ptr<AnalysisManager> m_manager;
  bool m_active = false;

  // 파일 추적
  std::unordered_map<std::string, AnalysisManager::AnalysisResult>
      m_file_results;
  std::unordered_set<std::string> m_tracked_files;
  std::unordered_set<std::string> m_changed_files;

  // 백그라운드 분석
  bool m_background_analysis_enabled = false;
  std::chrono::milliseconds m_analysis_delay{500};
  std::unique_ptr<std::thread> m_background_thread;

  void run_background_analysis();
};

} // namespace nugdev::compiler::analysis