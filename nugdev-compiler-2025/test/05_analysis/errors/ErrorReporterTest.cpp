#include "05_analysis/errors/ErrorReporter.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <vector>

using namespace nugdev::compiler::analysis;

class ErrorReporterTest : public ::testing::Test {
protected:
  void SetUp() override {
    // 테스트용 에러들 생성
    type_error =
        AnalysisError(AnalysisErrorType::TYPE_MISMATCH,
                      "Type mismatch: expected 'number', got 'string'", 42, 15);
    type_error.set_severity(AnalysisError::Severity::ERROR);

    warning_error = AnalysisError(
        AnalysisErrorType::UNUSED_VARIABLE,
        "Variable 'unused_var' is declared but never used", 10, 5);
    warning_error.set_severity(AnalysisError::Severity::WARNING);

    fatal_error = AnalysisError(AnalysisErrorType::UNDEFINED_SYMBOL,
                                "Critical symbol resolution failure", 1, 1);
    fatal_error.set_severity(AnalysisError::Severity::FATAL);

    // 출력 스트림 초기화
    output_stream.str("");
    output_stream.clear();
  }

  AnalysisError type_error;
  AnalysisError warning_error;
  AnalysisError fatal_error;
  std::stringstream output_stream;
};

// =============================================================================
// Constructor and Basic Setup Tests
// =============================================================================

TEST_F(ErrorReporterTest, DefaultConstructor) {
  ErrorReporter reporter;

  EXPECT_EQ(reporter.get_error_count(), 0);
  EXPECT_EQ(reporter.get_warning_count(), 0);
  EXPECT_EQ(reporter.get_fatal_error_count(), 0);
  EXPECT_FALSE(reporter.has_errors());
  EXPECT_FALSE(reporter.has_fatal_errors());
}

TEST_F(ErrorReporterTest, ConstructorWithFormat) {
  ErrorReporter json_reporter(ErrorReporter::OutputFormat::JSON);
  ErrorReporter xml_reporter(ErrorReporter::OutputFormat::XML);
  ErrorReporter compiler_reporter(ErrorReporter::OutputFormat::COMPILER_STYLE);

  // 각 포맷이 올바르게 설정되었는지는 출력을 통해 확인할 수 있음
  EXPECT_EQ(json_reporter.get_error_count(), 0);
  EXPECT_EQ(xml_reporter.get_error_count(), 0);
  EXPECT_EQ(compiler_reporter.get_error_count(), 0);
}

// =============================================================================
// Error Addition and Counting Tests
// =============================================================================

TEST_F(ErrorReporterTest, AddSingleError) {
  ErrorReporter reporter;

  reporter.add_error(type_error);

  EXPECT_EQ(reporter.get_error_count(), 1);
  EXPECT_EQ(reporter.get_warning_count(), 0);
  EXPECT_EQ(reporter.get_fatal_error_count(), 0);
  EXPECT_TRUE(reporter.has_errors());
  EXPECT_FALSE(reporter.has_fatal_errors());
}

TEST_F(ErrorReporterTest, AddMultipleErrorTypes) {
  ErrorReporter reporter;

  reporter.add_error(type_error);
  reporter.add_error(warning_error);
  reporter.add_error(fatal_error);

  EXPECT_EQ(reporter.get_error_count(), 1);
  EXPECT_EQ(reporter.get_warning_count(), 1);
  EXPECT_EQ(reporter.get_fatal_error_count(), 1);
  EXPECT_TRUE(reporter.has_errors());
  EXPECT_TRUE(reporter.has_fatal_errors());
}

TEST_F(ErrorReporterTest, AddErrorsVector) {
  ErrorReporter reporter;

  std::vector<AnalysisError> errors = {type_error, warning_error, fatal_error};
  reporter.add_errors(errors);

  EXPECT_EQ(reporter.get_error_count(), 1);
  EXPECT_EQ(reporter.get_warning_count(), 1);
  EXPECT_EQ(reporter.get_fatal_error_count(), 1);
  EXPECT_TRUE(reporter.has_errors());
  EXPECT_TRUE(reporter.has_fatal_errors());
}

TEST_F(ErrorReporterTest, ClearErrors) {
  ErrorReporter reporter;

  reporter.add_error(type_error);
  reporter.add_error(warning_error);

  EXPECT_EQ(reporter.get_error_count(), 1);
  EXPECT_EQ(reporter.get_warning_count(), 1);

  reporter.clear_errors();

  EXPECT_EQ(reporter.get_error_count(), 0);
  EXPECT_EQ(reporter.get_warning_count(), 0);
  EXPECT_EQ(reporter.get_fatal_error_count(), 0);
  EXPECT_FALSE(reporter.has_errors());
  EXPECT_FALSE(reporter.has_fatal_errors());
}

// =============================================================================
// Error Grouping Tests
// =============================================================================

TEST_F(ErrorReporterTest, GetErrorsByType) {
  ErrorReporter reporter;

  // 같은 타입의 에러 여러 개 추가
  AnalysisError type_error2(AnalysisErrorType::TYPE_MISMATCH,
                            "Another type mismatch", 50, 20);

  reporter.add_error(type_error);
  reporter.add_error(type_error2);
  reporter.add_error(warning_error);

  auto errors_by_type = reporter.get_errors_by_type();

  EXPECT_EQ(errors_by_type.size(), 2); // TYPE_MISMATCH, UNUSED_VARIABLE
  EXPECT_EQ(errors_by_type[AnalysisErrorType::TYPE_MISMATCH].size(), 2);
  EXPECT_EQ(errors_by_type[AnalysisErrorType::UNUSED_VARIABLE].size(), 1);
}

TEST_F(ErrorReporterTest, GetErrorsBySeverity) {
  ErrorReporter reporter;

  reporter.add_error(type_error);
  reporter.add_error(warning_error);
  reporter.add_error(fatal_error);

  auto errors_by_severity = reporter.get_errors_by_severity();

  EXPECT_EQ(errors_by_severity.size(), 3); // ERROR, WARNING, FATAL
  EXPECT_EQ(errors_by_severity[AnalysisError::Severity::ERROR].size(), 1);
  EXPECT_EQ(errors_by_severity[AnalysisError::Severity::WARNING].size(), 1);
  EXPECT_EQ(errors_by_severity[AnalysisError::Severity::FATAL].size(), 1);
}

// =============================================================================
// Output Format Tests
// =============================================================================

TEST_F(ErrorReporterTest, HumanReadableOutput) {
  ErrorReporter reporter(ErrorReporter::OutputFormat::HUMAN_READABLE);
  reporter.set_output_stream(output_stream);

  reporter.add_error(type_error);
  reporter.report_all_errors();

  std::string output = output_stream.str();

  EXPECT_NE(output.find("TYPE_MISMATCH"), std::string::npos);
  EXPECT_NE(output.find("Type mismatch"), std::string::npos);
  EXPECT_NE(output.find("42"), std::string::npos);
  EXPECT_NE(output.find("15"), std::string::npos);
}

TEST_F(ErrorReporterTest, JSONOutput) {
  ErrorReporter reporter(ErrorReporter::OutputFormat::JSON);
  reporter.set_output_stream(output_stream);

  reporter.add_error(type_error);
  reporter.report_all_errors();

  std::string output = output_stream.str();

  // JSON 형식 확인
  EXPECT_NE(output.find("{"), std::string::npos);
  EXPECT_NE(output.find("}"), std::string::npos);
  EXPECT_NE(output.find("\"type\""), std::string::npos);
  EXPECT_NE(output.find("\"message\""), std::string::npos);
  EXPECT_NE(output.find("\"line\""), std::string::npos);
  EXPECT_NE(output.find("\"column\""), std::string::npos);
}

TEST_F(ErrorReporterTest, XMLOutput) {
  ErrorReporter reporter(ErrorReporter::OutputFormat::XML);
  reporter.set_output_stream(output_stream);

  reporter.add_error(type_error);
  reporter.report_all_errors();

  std::string output = output_stream.str();

  // XML 형식 확인
  EXPECT_NE(output.find("<"), std::string::npos);
  EXPECT_NE(output.find(">"), std::string::npos);
  EXPECT_NE(output.find("</"), std::string::npos);
}

TEST_F(ErrorReporterTest, CompilerStyleOutput) {
  ErrorReporter reporter(ErrorReporter::OutputFormat::COMPILER_STYLE);
  reporter.set_output_stream(output_stream);

  reporter.add_error(type_error);
  reporter.report_all_errors();

  std::string output = output_stream.str();

  // GCC/Clang 스타일 확인 (line:column: severity: message)
  EXPECT_NE(output.find("42:15:"), std::string::npos);
  EXPECT_NE(output.find("error:"), std::string::npos);
}

TEST_F(ErrorReporterTest, MinimalOutput) {
  ErrorReporter reporter(ErrorReporter::OutputFormat::MINIMAL);
  reporter.set_output_stream(output_stream);

  reporter.add_error(type_error);
  reporter.report_all_errors();

  std::string output = output_stream.str();

  // 최소한의 정보만 포함되어야 함
  EXPECT_FALSE(output.empty());
  // 최소 형식이므로 자세한 검증은 생략
}

// =============================================================================
// Filter Tests
// =============================================================================

TEST_F(ErrorReporterTest, FilterByIncludedTypes) {
  ErrorReporter reporter;

  ErrorReporter::FilterOptions filter;
  filter.included_types = {AnalysisErrorType::TYPE_MISMATCH};
  reporter.set_filter_options(filter);

  reporter.add_error(type_error);
  reporter.add_error(warning_error);
  reporter.set_output_stream(output_stream);

  reporter.report_all_errors();

  std::string output = output_stream.str();

  // TYPE_MISMATCH는 포함되어야 함
  EXPECT_NE(output.find("TYPE_MISMATCH"), std::string::npos);
  // UNUSED_VARIABLE은 제외되어야 함
  EXPECT_EQ(output.find("UNUSED_VARIABLE"), std::string::npos);
}

TEST_F(ErrorReporterTest, FilterByExcludedTypes) {
  ErrorReporter reporter;

  ErrorReporter::FilterOptions filter;
  filter.excluded_types = {AnalysisErrorType::UNUSED_VARIABLE};
  reporter.set_filter_options(filter);

  reporter.add_error(type_error);
  reporter.add_error(warning_error);
  reporter.set_output_stream(output_stream);

  reporter.report_all_errors();

  std::string output = output_stream.str();

  // TYPE_MISMATCH는 포함되어야 함
  EXPECT_NE(output.find("TYPE_MISMATCH"), std::string::npos);
  // UNUSED_VARIABLE은 제외되어야 함
  EXPECT_EQ(output.find("UNUSED_VARIABLE"), std::string::npos);
}

TEST_F(ErrorReporterTest, FilterBySeverity) {
  ErrorReporter reporter;

  ErrorReporter::FilterOptions filter;
  filter.show_warnings = false;
  filter.show_errors = true;
  filter.show_fatal_errors = true;
  reporter.set_filter_options(filter);

  reporter.add_error(type_error);
  reporter.add_error(warning_error);
  reporter.add_error(fatal_error);
  reporter.set_output_stream(output_stream);

  reporter.report_all_errors();

  std::string output = output_stream.str();

  // ERROR와 FATAL은 포함되어야 함
  EXPECT_NE(output.find("TYPE_MISMATCH"), std::string::npos);
  EXPECT_NE(output.find("UNDEFINED_SYMBOL"), std::string::npos);
  // WARNING은 제외되어야 함
  EXPECT_EQ(output.find("UNUSED_VARIABLE"), std::string::npos);
}

TEST_F(ErrorReporterTest, FilterMaxErrorsPerType) {
  ErrorReporter reporter;

  ErrorReporter::FilterOptions filter;
  filter.max_errors_per_type = 1;
  reporter.set_filter_options(filter);

  // 같은 타입의 에러 3개 추가
  AnalysisError type_error2(AnalysisErrorType::TYPE_MISMATCH, "Error 2", 2, 1);
  AnalysisError type_error3(AnalysisErrorType::TYPE_MISMATCH, "Error 3", 3, 1);

  reporter.add_error(type_error);
  reporter.add_error(type_error2);
  reporter.add_error(type_error3);

  // 내부적으로 최대 1개만 처리되는지 확인하려면
  // report 후 출력에서 확인하거나 별도 API가 필요
  EXPECT_EQ(reporter.get_error_count(), 3); // 추가는 모두 됨
}

// =============================================================================
// Special Report Methods Tests
// =============================================================================

TEST_F(ErrorReporterTest, ReportErrorsByType) {
  ErrorReporter reporter;
  reporter.set_output_stream(output_stream);

  reporter.add_error(type_error);
  reporter.add_error(warning_error);

  reporter.report_errors_by_type(AnalysisErrorType::TYPE_MISMATCH);

  std::string output = output_stream.str();

  // TYPE_MISMATCH만 출력되어야 함
  EXPECT_NE(output.find("TYPE_MISMATCH"), std::string::npos);
  EXPECT_EQ(output.find("UNUSED_VARIABLE"), std::string::npos);
}

TEST_F(ErrorReporterTest, ReportSummary) {
  ErrorReporter reporter;
  reporter.set_output_stream(output_stream);

  reporter.add_error(type_error);
  reporter.add_error(warning_error);
  reporter.add_error(fatal_error);

  reporter.report_summary();

  std::string output = output_stream.str();

  // 요약 정보가 포함되어야 함
  EXPECT_NE(output.find("1"), std::string::npos); // 에러 개수
  EXPECT_FALSE(output.empty());
}

TEST_F(ErrorReporterTest, ReportForIDE) {
  ErrorReporter reporter;
  reporter.set_output_stream(output_stream);

  reporter.add_error(type_error);
  reporter.add_error(warning_error);

  reporter.report_for_ide();

  std::string output = output_stream.str();

  // IDE용 JSON 형식이어야 함
  EXPECT_NE(output.find("{"), std::string::npos);
  EXPECT_NE(output.find("}"), std::string::npos);
}

TEST_F(ErrorReporterTest, ReportStatistics) {
  ErrorReporter reporter;
  reporter.set_output_stream(output_stream);

  reporter.add_error(type_error);
  reporter.add_error(warning_error);
  reporter.add_error(fatal_error);

  reporter.report_statistics();

  std::string output = output_stream.str();

  // 통계 정보가 포함되어야 함
  EXPECT_FALSE(output.empty());
}

// =============================================================================
// Error Callback Tests
// =============================================================================

TEST_F(ErrorReporterTest, ErrorCallback) {
  ErrorReporter reporter;

  bool callback_called = false;
  AnalysisError received_error(AnalysisErrorType::SCOPE_ERROR, "dummy");

  reporter.set_error_callback([&](const AnalysisError &error) {
    callback_called = true;
    received_error = error;
  });

  reporter.add_error(type_error);

  EXPECT_TRUE(callback_called);
  EXPECT_EQ(received_error.get_type(), type_error.get_type());
  EXPECT_EQ(received_error.get_message(), type_error.get_message());
}

// =============================================================================
// Performance Tests
// =============================================================================

TEST_F(ErrorReporterTest, PerformanceTestManyErrors) {
  ErrorReporter reporter;

  auto start = std::chrono::high_resolution_clock::now();

  // 1000개의 에러 추가
  for (size_t i = 0; i < 1000; ++i) {
    AnalysisError perf_error(static_cast<AnalysisErrorType>(i % 12),
                             "Performance test error " + std::to_string(i), i,
                             i % 100);
    reporter.add_error(perf_error);
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  EXPECT_EQ(reporter.get_error_count() + reporter.get_warning_count() +
                reporter.get_fatal_error_count(),
            1000);

  // 1000개 에러 추가가 100ms 미만이어야 함
  EXPECT_LT(duration.count(), 100)
      << "Error addition too slow: " << duration.count() << "ms";
}

TEST_F(ErrorReporterTest, PerformanceTestReporting) {
  ErrorReporter reporter;
  reporter.set_output_stream(output_stream);

  // 100개의 에러 추가
  for (size_t i = 0; i < 100; ++i) {
    AnalysisError perf_error(AnalysisErrorType::TYPE_MISMATCH,
                             "Performance test error " + std::to_string(i), i,
                             i % 50);
    reporter.add_error(perf_error);
  }

  auto start = std::chrono::high_resolution_clock::now();
  reporter.report_all_errors();
  auto end = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // 100개 에러 리포팅이 50ms 미만이어야 함
  EXPECT_LT(duration.count(), 50)
      << "Error reporting too slow: " << duration.count() << "ms";

  std::string output = output_stream.str();
  EXPECT_FALSE(output.empty());
}

// =============================================================================
// Error Statistics Tests
// =============================================================================

TEST_F(ErrorReporterTest, ErrorStatisticsBasic) {
  std::vector<AnalysisError> errors = {type_error, warning_error, fatal_error};
  ErrorStatistics stats(errors);

  EXPECT_EQ(stats.get_total_count(), 3);
  EXPECT_EQ(stats.get_error_count(), 1);
  EXPECT_EQ(stats.get_warning_count(), 1);
  EXPECT_EQ(stats.get_fatal_count(), 1);
}

TEST_F(ErrorReporterTest, ErrorStatisticsTypeDistribution) {
  std::vector<AnalysisError> errors = {type_error, warning_error, fatal_error};
  ErrorStatistics stats(errors);

  auto type_dist = stats.get_type_distribution();

  EXPECT_EQ(type_dist[AnalysisErrorType::TYPE_MISMATCH], 1);
  EXPECT_EQ(type_dist[AnalysisErrorType::UNUSED_VARIABLE], 1);
  EXPECT_EQ(type_dist[AnalysisErrorType::UNDEFINED_SYMBOL], 1);
}

TEST_F(ErrorReporterTest, ErrorStatisticsMostCommonType) {
  std::vector<AnalysisError> errors;

  // TYPE_MISMATCH 에러를 3개 추가
  for (int i = 0; i < 3; ++i) {
    AnalysisError error(AnalysisErrorType::TYPE_MISMATCH,
                        "Error " + std::to_string(i), i, i);
    errors.push_back(error);
  }

  // 다른 타입 에러 1개씩 추가
  errors.push_back(warning_error);
  errors.push_back(fatal_error);

  ErrorStatistics stats(errors);

  EXPECT_EQ(stats.get_most_common_error_type(),
            AnalysisErrorType::TYPE_MISMATCH);
}

TEST_F(ErrorReporterTest, ErrorStatisticsReports) {
  std::vector<AnalysisError> errors = {type_error, warning_error, fatal_error};
  ErrorStatistics stats(errors);

  std::string summary = stats.generate_summary();
  std::string detailed = stats.generate_detailed_report();

  EXPECT_FALSE(summary.empty());
  EXPECT_FALSE(detailed.empty());
  EXPECT_GT(detailed.length(), summary.length()); // 상세 보고서가 더 길어야 함
}

// =============================================================================
// Error Builder Pattern Tests
// =============================================================================

TEST_F(ErrorReporterTest, ErrorReporterBuilder) {
  auto reporter = ErrorReporterBuilder()
                      .format(ErrorReporter::OutputFormat::JSON)
                      .include_type(AnalysisErrorType::TYPE_MISMATCH)
                      .exclude_type(AnalysisErrorType::UNUSED_VARIABLE)
                      .max_errors_per_type(5)
                      .build();

  EXPECT_NE(reporter, nullptr);
  EXPECT_EQ(reporter->get_error_count(), 0);
}

// =============================================================================
// Edge Cases and Error Handling
// =============================================================================

TEST_F(ErrorReporterTest, ReportWithoutErrors) {
  ErrorReporter reporter;
  reporter.set_output_stream(output_stream);

  reporter.report_all_errors();

  std::string output = output_stream.str();

  // 에러가 없어도 출력이 있을 수 있음 (예: "No errors found")
  // 또는 빈 출력일 수도 있음
  // 어느 쪽이든 크래시하지 않아야 함
}

TEST_F(ErrorReporterTest, ChangeFormatAfterConstruction) {
  ErrorReporter reporter(ErrorReporter::OutputFormat::HUMAN_READABLE);
  reporter.set_output_stream(output_stream);

  reporter.add_error(type_error);

  // 포맷 변경
  reporter.set_output_format(ErrorReporter::OutputFormat::JSON);
  reporter.report_all_errors();

  std::string output = output_stream.str();

  // JSON 형식이어야 함
  EXPECT_NE(output.find("{"), std::string::npos);
}