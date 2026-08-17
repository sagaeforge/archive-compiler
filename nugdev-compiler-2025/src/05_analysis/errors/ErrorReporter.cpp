#include "05_analysis/errors/ErrorReporter.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace nugdev::compiler::analysis {

ErrorReporter::ErrorReporter(OutputFormat format)
    : m_format(format), m_output_stream(&std::cout) {}

void ErrorReporter::add_error(const AnalysisError &error) {
  m_errors.push_back(error);

  if (m_error_callback) {
    m_error_callback(error);
  }
}

void ErrorReporter::add_errors(const std::vector<AnalysisError> &errors) {
  for (const auto &error : errors) {
    add_error(error);
  }
}

void ErrorReporter::clear_errors() { m_errors.clear(); }

size_t ErrorReporter::get_error_count() const {
  return std::count_if(
      m_errors.begin(), m_errors.end(), [](const AnalysisError &error) {
        return error.get_severity() == AnalysisError::Severity::ERROR ||
               error.get_severity() == AnalysisError::Severity::FATAL;
      });
}

size_t ErrorReporter::get_warning_count() const {
  return std::count_if(
      m_errors.begin(), m_errors.end(), [](const AnalysisError &error) {
        return error.get_severity() == AnalysisError::Severity::WARNING;
      });
}

size_t ErrorReporter::get_fatal_error_count() const {
  return std::count_if(
      m_errors.begin(), m_errors.end(), [](const AnalysisError &error) {
        return error.get_severity() == AnalysisError::Severity::FATAL;
      });
}

bool ErrorReporter::has_errors() const { return get_error_count() > 0; }

bool ErrorReporter::has_fatal_errors() const {
  return get_fatal_error_count() > 0;
}

std::unordered_map<AnalysisErrorType, std::vector<AnalysisError>>
ErrorReporter::get_errors_by_type() const {
  std::unordered_map<AnalysisErrorType, std::vector<AnalysisError>> result;

  for (const auto &error : m_errors) {
    result[error.get_type()].push_back(error);
  }

  return result;
}

std::unordered_map<AnalysisError::Severity, std::vector<AnalysisError>>
ErrorReporter::get_errors_by_severity() const {
  std::unordered_map<AnalysisError::Severity, std::vector<AnalysisError>>
      result;

  for (const auto &error : m_errors) {
    result[error.get_severity()].push_back(error);
  }

  return result;
}

void ErrorReporter::report_all_errors() {
  auto filtered_errors = filter_errors();

  switch (m_format) {
  case OutputFormat::HUMAN_READABLE:
    report_human_readable(filtered_errors);
    break;
  case OutputFormat::JSON:
    report_json(filtered_errors);
    break;
  case OutputFormat::XML:
    report_xml(filtered_errors);
    break;
  case OutputFormat::COMPILER_STYLE:
    report_compiler_style(filtered_errors);
    break;
  case OutputFormat::MINIMAL:
    report_minimal(filtered_errors);
    break;
  default:
    report_human_readable(filtered_errors);
    break;
  }
}

void ErrorReporter::report_errors_by_type(AnalysisErrorType type) {
  std::vector<AnalysisError> filtered_errors;

  for (const auto &error : m_errors) {
    if (error.get_type() == type && should_include_error(error)) {
      filtered_errors.push_back(error);
    }
  }

  switch (m_format) {
  case OutputFormat::HUMAN_READABLE:
    report_human_readable(filtered_errors);
    break;
  case OutputFormat::JSON:
    report_json(filtered_errors);
    break;
  default:
    report_human_readable(filtered_errors);
    break;
  }
}

void ErrorReporter::report_summary() {
  *m_output_stream << "\n=== Analysis Summary ===\n";
  *m_output_stream << "Total issues: " << m_errors.size() << "\n";
  *m_output_stream << "Errors: " << get_error_count() << "\n";
  *m_output_stream << "Warnings: " << get_warning_count() << "\n";
  *m_output_stream << "Fatal errors: " << get_fatal_error_count() << "\n";

  if (has_fatal_errors()) {
    *m_output_stream
        << "\n⚠️  Fatal errors detected - compilation cannot continue\n";
  } else if (has_errors()) {
    *m_output_stream << "\n❌ Compilation failed with errors\n";
  } else if (get_warning_count() > 0) {
    *m_output_stream << "\n⚠️  Compilation succeeded with warnings\n";
  } else {
    *m_output_stream << "\n✅ Compilation succeeded\n";
  }
}

void ErrorReporter::report_for_ide() {
  set_output_format(OutputFormat::JSON);
  report_all_errors();
}

void ErrorReporter::report_statistics() {
  ErrorStatistics stats(m_errors);
  *m_output_stream << stats.generate_summary();
}

std::vector<AnalysisError> ErrorReporter::filter_errors() const {
  std::vector<AnalysisError> filtered;
  std::unordered_map<AnalysisErrorType, size_t> type_counts;

  for (const auto &error : m_errors) {
    if (should_include_error(error)) {
      // 타입별 최대 에러 수 확인
      if (type_counts[error.get_type()] <
          m_filter_options.max_errors_per_type) {
        filtered.push_back(error);
        type_counts[error.get_type()]++;
      }
    }
  }

  return filtered;
}

bool ErrorReporter::should_include_error(const AnalysisError &error) const {
  // 심각도 필터링
  switch (error.get_severity()) {
  case AnalysisError::Severity::WARNING:
    if (!m_filter_options.show_warnings)
      return false;
    break;
  case AnalysisError::Severity::ERROR:
    if (!m_filter_options.show_errors)
      return false;
    break;
  case AnalysisError::Severity::FATAL:
    if (!m_filter_options.show_fatal_errors)
      return false;
    break;
  }

  // 타입 필터링
  auto error_type = error.get_type();

  // 제외 목록 확인
  for (auto excluded_type : m_filter_options.excluded_types) {
    if (error_type == excluded_type)
      return false;
  }

  // 포함 목록 확인 (비어있으면 모든 타입 허용)
  if (!m_filter_options.included_types.empty()) {
    bool found = false;
    for (auto included_type : m_filter_options.included_types) {
      if (error_type == included_type) {
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }

  return true;
}

void ErrorReporter::report_human_readable(
    const std::vector<AnalysisError> &errors) {
  if (errors.empty()) {
    *m_output_stream << "No errors found.\n";
    return;
  }

  *m_output_stream << "\n=== Analysis Results ===\n\n";

  for (const auto &error : errors) {
    *m_output_stream << format_error_human_readable(error) << "\n";
  }

  // 요약 정보
  *m_output_stream << "\n--- Summary ---\n";
  *m_output_stream << "Total issues: " << errors.size() << "\n";

  size_t error_count = 0, warning_count = 0, fatal_count = 0;
  for (const auto &error : errors) {
    switch (error.get_severity()) {
    case AnalysisError::Severity::ERROR:
      error_count++;
      break;
    case AnalysisError::Severity::WARNING:
      warning_count++;
      break;
    case AnalysisError::Severity::FATAL:
      fatal_count++;
      break;
    }
  }

  *m_output_stream << "Errors: " << error_count << "\n";
  *m_output_stream << "Warnings: " << warning_count << "\n";
  *m_output_stream << "Fatal: " << fatal_count << "\n";
}

void ErrorReporter::report_json(const std::vector<AnalysisError> &errors) {
  *m_output_stream << "{\n";
  *m_output_stream << "  \"errors\": [\n";

  for (size_t i = 0; i < errors.size(); ++i) {
    const auto &error = errors[i];
    *m_output_stream << "    {\n";
    *m_output_stream << "      \"type\": \""
                     << escape_json_string(error.get_type_string()) << "\",\n";
    *m_output_stream << "      \"severity\": \""
                     << severity_to_string(error.get_severity()) << "\",\n";
    *m_output_stream << "      \"message\": \""
                     << escape_json_string(error.get_message()) << "\",\n";
    *m_output_stream << "      \"line\": " << error.get_line() << ",\n";
    *m_output_stream << "      \"column\": " << error.get_column() << "\n";
    *m_output_stream << "    }" << (i < errors.size() - 1 ? "," : "") << "\n";
  }

  *m_output_stream << "  ],\n";
  *m_output_stream << "  \"summary\": {\n";
  *m_output_stream << "    \"total\": " << errors.size() << ",\n";
  *m_output_stream << "    \"errors\": " << get_error_count() << ",\n";
  *m_output_stream << "    \"warnings\": " << get_warning_count() << ",\n";
  *m_output_stream << "    \"fatal\": " << get_fatal_error_count() << "\n";
  *m_output_stream << "  }\n";
  *m_output_stream << "}\n";
}

void ErrorReporter::report_xml(const std::vector<AnalysisError> &errors) {
  *m_output_stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  *m_output_stream << "<analysis_results>\n";

  for (const auto &error : errors) {
    *m_output_stream << "  <error>\n";
    *m_output_stream << "    <type>"
                     << escape_xml_string(error.get_type_string())
                     << "</type>\n";
    *m_output_stream << "    <severity>"
                     << severity_to_string(error.get_severity())
                     << "</severity>\n";
    *m_output_stream << "    <message>"
                     << escape_xml_string(error.get_message())
                     << "</message>\n";
    *m_output_stream << "    <line>" << error.get_line() << "</line>\n";
    *m_output_stream << "    <column>" << error.get_column() << "</column>\n";
    *m_output_stream << "  </error>\n";
  }

  *m_output_stream << "</analysis_results>\n";
}

void ErrorReporter::report_compiler_style(
    const std::vector<AnalysisError> &errors) {
  for (const auto &error : errors) {
    *m_output_stream << format_error_compiler_style(error) << "\n";
  }
}

void ErrorReporter::report_minimal(const std::vector<AnalysisError> &errors) {
  for (const auto &error : errors) {
    *m_output_stream << error.get_line() << ":" << error.get_column() << ": "
                     << severity_to_string(error.get_severity()) << ": "
                     << error.get_message() << "\n";
  }
}

std::string
ErrorReporter::format_error_human_readable(const AnalysisError &error) const {
  std::ostringstream result;

  // 색상 코드 (터미널 지원시)
  if (is_terminal_output() && m_filter_options.colorize_output) {
    result << get_color_code(error.get_severity());
  }

  // 에러 타입 및 위치
  result << "[" << severity_to_string(error.get_severity()) << "] ";
  result << error.get_type_string() << " ";

  if (error.get_line() > 0) {
    result << "at line " << error.get_line();
    if (error.get_column() > 0) {
      result << ":" << error.get_column();
    }
    result << " ";
  }

  result << "- " << error.get_message();

  // 색상 리셋
  if (is_terminal_output() && m_filter_options.colorize_output) {
    result << "\033[0m"; // 색상 리셋
  }

  return result.str();
}

std::string
ErrorReporter::format_error_compiler_style(const AnalysisError &error) const {
  std::ostringstream result;

  // GCC/Clang 스타일: file:line:column: severity: message
  result << "file:" << error.get_line() << ":" << error.get_column() << ": ";
  result << severity_to_string(error.get_severity()) << ": ";
  result << error.get_message();

  return result.str();
}

std::string ErrorReporter::escape_json_string(const std::string &str) const {
  std::ostringstream result;
  for (char c : str) {
    switch (c) {
    case '"':
      result << "\\\"";
      break;
    case '\\':
      result << "\\\\";
      break;
    case '\n':
      result << "\\n";
      break;
    case '\t':
      result << "\\t";
      break;
    case '\r':
      result << "\\r";
      break;
    default:
      result << c;
      break;
    }
  }
  return result.str();
}

std::string ErrorReporter::escape_xml_string(const std::string &str) const {
  std::ostringstream result;
  for (char c : str) {
    switch (c) {
    case '<':
      result << "&lt;";
      break;
    case '>':
      result << "&gt;";
      break;
    case '&':
      result << "&amp;";
      break;
    case '"':
      result << "&quot;";
      break;
    case '\'':
      result << "&apos;";
      break;
    default:
      result << c;
      break;
    }
  }
  return result.str();
}

std::string
ErrorReporter::severity_to_string(AnalysisError::Severity severity) const {
  switch (severity) {
  case AnalysisError::Severity::WARNING:
    return "warning";
  case AnalysisError::Severity::ERROR:
    return "error";
  case AnalysisError::Severity::FATAL:
    return "fatal";
  default:
    return "unknown";
  }
}

std::string
ErrorReporter::get_color_code(AnalysisError::Severity severity) const {
  switch (severity) {
  case AnalysisError::Severity::WARNING:
    return "\033[33m"; // 노란색
  case AnalysisError::Severity::ERROR:
    return "\033[31m"; // 빨간색
  case AnalysisError::Severity::FATAL:
    return "\033[91m"; // 밝은 빨간색
  default:
    return "";
  }
}

bool ErrorReporter::is_terminal_output() const {
  // 간단한 터미널 확인 (실제로는 더 정교한 검사 필요)
  return m_output_stream == &std::cout || m_output_stream == &std::cerr;
}

// ErrorStatistics 구현
ErrorStatistics::ErrorStatistics(const std::vector<AnalysisError> &errors) {
  analyze_errors(errors);
}

void ErrorStatistics::analyze_errors(const std::vector<AnalysisError> &errors) {
  m_total_count = errors.size();

  for (const auto &error : errors) {
    switch (error.get_severity()) {
    case AnalysisError::Severity::ERROR:
      m_error_count++;
      break;
    case AnalysisError::Severity::WARNING:
      m_warning_count++;
      break;
    case AnalysisError::Severity::FATAL:
      m_fatal_count++;
      break;
    }

    m_type_counts[error.get_type()]++;
    m_line_counts[error.get_line()]++;
  }
}

std::unordered_map<AnalysisErrorType, size_t>
ErrorStatistics::get_type_distribution() const {
  return m_type_counts;
}

AnalysisErrorType ErrorStatistics::get_most_common_error_type() const {
  if (m_type_counts.empty()) {
    return AnalysisErrorType::TYPE_MISMATCH; // 기본값
  }

  auto max_it = std::max_element(
      m_type_counts.begin(), m_type_counts.end(),
      [](const auto &a, const auto &b) { return a.second < b.second; });

  return max_it->first;
}

std::string ErrorStatistics::generate_summary() const {
  std::ostringstream ss;
  ss << "\n=== Error Statistics ===\n";
  ss << "Total: " << m_total_count << "\n";
  ss << "Errors: " << m_error_count << "\n";
  ss << "Warnings: " << m_warning_count << "\n";
  ss << "Fatal: " << m_fatal_count << "\n";

  if (!m_type_counts.empty()) {
    ss << "\nMost common error: ";
    // 간단한 타입 이름 변환 (실제로는 더 정교한 변환 필요)
    ss << "Type " << static_cast<int>(get_most_common_error_type()) << "\n";
  }

  return ss.str();
}

} // namespace nugdev::compiler::analysis