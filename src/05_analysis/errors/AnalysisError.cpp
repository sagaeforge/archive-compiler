#include "05_analysis/errors/AnalysisError.hpp"
#include <sstream>

namespace nugdev::compiler::analysis {

AnalysisError::AnalysisError(AnalysisErrorType type, const std::string &message,
                             size_t line, size_t column)
    : m_type(type), m_message(message), m_line(line), m_column(column) {

  // 에러 타입에 따른 기본 심각도 설정
  switch (type) {
  case AnalysisErrorType::TYPE_MISMATCH:
  case AnalysisErrorType::UNDEFINED_SYMBOL:
  case AnalysisErrorType::REDEFINED_SYMBOL:
  case AnalysisErrorType::INVALID_OPERATION:
  case AnalysisErrorType::INVALID_RETURN_TYPE:
  case AnalysisErrorType::INVALID_FUNCTION_CALL:
  case AnalysisErrorType::SCOPE_ERROR:
  case AnalysisErrorType::CONTROL_FLOW_ERROR:
    m_severity = Severity::ERROR;
    break;

  case AnalysisErrorType::INCOMPATIBLE_ASSIGNMENT:
    m_severity = Severity::ERROR;
    break;

  case AnalysisErrorType::UNREACHABLE_CODE:
  case AnalysisErrorType::UNUSED_VARIABLE:
    m_severity = Severity::WARNING;
    break;

  case AnalysisErrorType::UNINITIALIZED_VARIABLE:
    m_severity = Severity::ERROR;
    break;

  default:
    m_severity = Severity::ERROR;
    break;
  }
}

std::string AnalysisError::to_string() const {
  std::ostringstream ss;

  // 심각도 표시
  switch (m_severity) {
  case Severity::WARNING:
    ss << "[WARNING] ";
    break;
  case Severity::ERROR:
    ss << "[ERROR] ";
    break;
  case Severity::FATAL:
    ss << "[FATAL] ";
    break;
  }

  // 에러 타입 표시
  ss << get_type_string() << ": ";

  // 메시지
  ss << m_message;

  // 위치 정보 (있는 경우)
  if (m_line > 0) {
    ss << " (line " << m_line;
    if (m_column > 0) {
      ss << ", column " << m_column;
    }
    ss << ")";
  }

  return ss.str();
}

std::string AnalysisError::get_type_string() const {
  switch (m_type) {
  case AnalysisErrorType::TYPE_MISMATCH:
    return "Type Mismatch";
  case AnalysisErrorType::UNDEFINED_SYMBOL:
    return "Undefined Symbol";
  case AnalysisErrorType::REDEFINED_SYMBOL:
    return "Redefined Symbol";
  case AnalysisErrorType::INCOMPATIBLE_ASSIGNMENT:
    return "Incompatible Assignment";
  case AnalysisErrorType::INVALID_OPERATION:
    return "Invalid Operation";
  case AnalysisErrorType::UNREACHABLE_CODE:
    return "Unreachable Code";
  case AnalysisErrorType::UNUSED_VARIABLE:
    return "Unused Variable";
  case AnalysisErrorType::UNINITIALIZED_VARIABLE:
    return "Uninitialized Variable";
  case AnalysisErrorType::INVALID_RETURN_TYPE:
    return "Invalid Return Type";
  case AnalysisErrorType::INVALID_FUNCTION_CALL:
    return "Invalid Function Call";
  case AnalysisErrorType::SCOPE_ERROR:
    return "Scope Error";
  case AnalysisErrorType::CONTROL_FLOW_ERROR:
    return "Control Flow Error";
  default:
    return "Unknown Error";
  }
}

} // namespace nugdev::compiler::analysis