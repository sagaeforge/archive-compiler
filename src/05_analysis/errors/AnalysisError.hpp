#pragma once

#include <memory>
#include <string>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 분석 단계에서 발생하는 에러의 종류
 */
enum class AnalysisErrorType {
  TYPE_MISMATCH,
  UNDEFINED_SYMBOL,
  REDEFINED_SYMBOL,
  INCOMPATIBLE_ASSIGNMENT,
  INVALID_OPERATION,
  UNREACHABLE_CODE,
  UNUSED_VARIABLE,
  UNINITIALIZED_VARIABLE,
  INVALID_RETURN_TYPE,
  INVALID_FUNCTION_CALL,
  SCOPE_ERROR,
  CONTROL_FLOW_ERROR
};

/**
 * @brief 분석 에러 정보를 담는 클래스
 */
class AnalysisError {
public:
  AnalysisError(AnalysisErrorType type, const std::string &message,
                size_t line = 0, size_t column = 0);

  // Getters
  AnalysisErrorType get_type() const { return m_type; }
  const std::string &get_message() const { return m_message; }
  size_t get_line() const { return m_line; }
  size_t get_column() const { return m_column; }

  // 에러 레벨 (경고, 에러, 치명적 에러)
  enum class Severity { WARNING, ERROR, FATAL };

  Severity get_severity() const { return m_severity; }
  void set_severity(Severity severity) { m_severity = severity; }

  // 문자열 표현
  std::string to_string() const;
  std::string get_type_string() const;

private:
  AnalysisErrorType m_type;
  std::string m_message;
  size_t m_line;
  size_t m_column;
  Severity m_severity;
};

} // namespace nugdev::compiler::analysis