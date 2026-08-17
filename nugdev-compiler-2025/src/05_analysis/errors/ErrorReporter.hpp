#pragma once

#include "05_analysis/errors/AnalysisError.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 분석 에러 리포터
 *
 * 분석 단계에서 발생한 에러들을 수집, 분류, 출력하는 기능:
 * - 에러 수집 및 분류
 * - 다양한 출력 형식 지원
 * - 에러 필터링 및 정렬
 * - IDE 통합을 위한 구조화된 출력
 */
class ErrorReporter {
public:
  /**
   * @brief 출력 형식 옵션
   */
  enum class OutputFormat {
    HUMAN_READABLE, // 사람이 읽기 쉬운 형식
    JSON,           // JSON 형식 (IDE 통합용)
    XML,            // XML 형식
    COMPILER_STYLE, // GCC/Clang 스타일
    MINIMAL         // 최소한의 정보만
  };

  /**
   * @brief 에러 필터 옵션
   */
  struct FilterOptions {
    bool show_warnings;
    bool show_errors;
    bool show_fatal_errors;
    std::vector<AnalysisErrorType> included_types;
    std::vector<AnalysisErrorType> excluded_types;
    size_t max_errors_per_type;

    FilterOptions()
        : show_warnings(true), show_errors(true), show_fatal_errors(true),
          max_errors_per_type(SIZE_MAX) {}
  };

  explicit ErrorReporter(OutputFormat format = OutputFormat::HUMAN_READABLE);

  // 에러 수집
  void add_error(const AnalysisError &error);
  void add_errors(const std::vector<AnalysisError> &errors);
  void clear_errors();

  // 에러 통계
  size_t get_error_count() const;
  size_t get_warning_count() const;
  size_t get_fatal_error_count() const;
  bool has_errors() const;
  bool has_fatal_errors() const;

  // 에러 분류
  std::unordered_map<AnalysisErrorType, std::vector<AnalysisError>>
  get_errors_by_type() const;

  std::unordered_map<AnalysisError::Severity, std::vector<AnalysisError>>
  get_errors_by_severity() const;

  // 출력 설정
  void set_output_format(OutputFormat format) { m_format = format; }
  void set_filter_options(const FilterOptions &options) {
    m_filter_options = options;
  }
  void set_output_stream(std::ostream &stream) { m_output_stream = &stream; }

  // 에러 출력
  void report_all_errors();
  void report_errors_by_type(AnalysisErrorType type);
  void report_summary();

  // 특수 출력 모드
  void report_for_ide();    // IDE 통합용 JSON 출력
  void report_statistics(); // 통계 정보만 출력

  // 콜백 등록 (커스텀 처리용)
  using ErrorCallback = std::function<void(const AnalysisError &)>;
  void set_error_callback(ErrorCallback callback) {
    m_error_callback = callback;
  }

private:
  std::vector<AnalysisError> m_errors;
  OutputFormat m_format;
  FilterOptions m_filter_options;
  std::ostream *m_output_stream;
  ErrorCallback m_error_callback;

  // 필터링
  std::vector<AnalysisError> filter_errors() const;
  bool should_include_error(const AnalysisError &error) const;

  // 출력 형식별 구현
  void report_human_readable(const std::vector<AnalysisError> &errors);
  void report_json(const std::vector<AnalysisError> &errors);
  void report_xml(const std::vector<AnalysisError> &errors);
  void report_compiler_style(const std::vector<AnalysisError> &errors);
  void report_minimal(const std::vector<AnalysisError> &errors);

  // 헬퍼 메서드들
  std::string format_error_human_readable(const AnalysisError &error) const;
  std::string format_error_compiler_style(const AnalysisError &error) const;
  std::string escape_json_string(const std::string &str) const;
  std::string escape_xml_string(const std::string &str) const;
  std::string severity_to_string(AnalysisError::Severity severity) const;
  std::string get_color_code(AnalysisError::Severity severity) const;
  bool is_terminal_output() const;
};

/**
 * @brief 에러 통계 수집기
 */
class ErrorStatistics {
public:
  explicit ErrorStatistics(const std::vector<AnalysisError> &errors);

  // 기본 통계
  size_t get_total_count() const { return m_total_count; }
  size_t get_error_count() const { return m_error_count; }
  size_t get_warning_count() const { return m_warning_count; }
  size_t get_fatal_count() const { return m_fatal_count; }

  // 타입별 통계
  std::unordered_map<AnalysisErrorType, size_t> get_type_distribution() const;
  AnalysisErrorType get_most_common_error_type() const;

  // 위치별 통계
  std::unordered_map<size_t, size_t> get_line_distribution() const;
  std::vector<size_t> get_error_hotspots(size_t top_n = 5) const;

  // 요약 정보
  std::string generate_summary() const;
  std::string generate_detailed_report() const;

private:
  size_t m_total_count = 0;
  size_t m_error_count = 0;
  size_t m_warning_count = 0;
  size_t m_fatal_count = 0;

  std::unordered_map<AnalysisErrorType, size_t> m_type_counts;
  std::unordered_map<size_t, size_t> m_line_counts;

  void analyze_errors(const std::vector<AnalysisError> &errors);
};

/**
 * @brief 에러 그룹화 도구
 */
class ErrorGrouper {
public:
  /**
   * @brief 유사한 에러들을 그룹화
   */
  struct ErrorGroup {
    AnalysisErrorType type;
    std::string pattern; // 공통 패턴
    std::vector<AnalysisError> errors;
    size_t count;
  };

  static std::vector<ErrorGroup>
  group_similar_errors(const std::vector<AnalysisError> &errors);

  /**
   * @brief 연관된 에러들 찾기 (동일 변수, 동일 함수 등)
   */
  static std::vector<std::vector<AnalysisError>>
  find_related_errors(const std::vector<AnalysisError> &errors);

private:
  static bool are_errors_similar(const AnalysisError &e1,
                                 const AnalysisError &e2);
  static std::string extract_error_pattern(const AnalysisError &error);
};

/**
 * @brief 에러 제안 시스템
 */
class ErrorSuggestionSystem {
public:
  struct Suggestion {
    std::string description;
    std::string fix_hint;
    double confidence_score; // 0.0 - 1.0
  };

  /**
   * @brief 에러에 대한 해결 제안 생성
   */
  static std::vector<Suggestion>
  generate_suggestions(const AnalysisError &error);

  /**
   * @brief 일반적인 실수 패턴 감지
   */
  static std::vector<Suggestion>
  detect_common_mistakes(const std::vector<AnalysisError> &errors);

private:
  static std::vector<Suggestion>
  suggest_for_type_mismatch(const AnalysisError &error);
  static std::vector<Suggestion>
  suggest_for_undefined_symbol(const AnalysisError &error);
  static std::vector<Suggestion>
  suggest_for_unused_variable(const AnalysisError &error);
  static std::vector<Suggestion>
  suggest_for_uninitialized_variable(const AnalysisError &error);
};

/**
 * @brief 에러 리포터 빌더 (Fluent Interface)
 */
class ErrorReporterBuilder {
public:
  ErrorReporterBuilder &format(ErrorReporter::OutputFormat fmt);
  ErrorReporterBuilder &show_warnings(bool show = true);
  ErrorReporterBuilder &show_errors(bool show = true);
  ErrorReporterBuilder &show_fatal_errors(bool show = true);
  ErrorReporterBuilder &include_type(AnalysisErrorType type);
  ErrorReporterBuilder &exclude_type(AnalysisErrorType type);
  ErrorReporterBuilder &max_errors_per_type(size_t max_count);
  ErrorReporterBuilder &output_to(std::ostream &stream);
  ErrorReporterBuilder &with_callback(ErrorReporter::ErrorCallback callback);

  std::unique_ptr<ErrorReporter> build();

private:
  ErrorReporter::OutputFormat m_format =
      ErrorReporter::OutputFormat::HUMAN_READABLE;
  ErrorReporter::FilterOptions m_filter_options;
  std::ostream *m_output_stream = &std::cout;
  ErrorReporter::ErrorCallback m_callback;
};

} // namespace nugdev::compiler::analysis