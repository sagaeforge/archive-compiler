#include "05_analysis/AnalysisConfig.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace nugdev::compiler::analysis {

void AnalysisConfig::set_analysis_level(AnalysisLevel level) {
  m_analysis_level = level;
  apply_level_constraints();
}

void AnalysisConfig::set_type_safety_level(TypeSafetyLevel level) {
  m_type_safety_level = level;
  apply_level_constraints();
}

void AnalysisConfig::set_optimization_level(OptimizationLevel level) {
  m_optimization_level = level;
  apply_level_constraints();
}

AnalysisConfig AnalysisConfig::create_development_preset() {
  AnalysisConfig config;
  config.set_analysis_level(AnalysisLevel::STANDARD);
  config.set_type_safety_level(TypeSafetyLevel::STANDARD);
  config.set_optimization_level(OptimizationLevel::BASIC);

  // 개발용 설정
  auto &error_config = config.get_error_reporting_config();
  error_config.show_warnings = true;
  error_config.show_suggestions = true;
  error_config.colorize_output = true;
  error_config.max_errors_per_file = 50;

  auto &compile_config = config.get_compile_time_config();
  compile_config.enable_constexpr_evaluation = true;
  compile_config.cache_compile_time_results = true;
  compile_config.max_cache_size = 5000;

  return config;
}

AnalysisConfig AnalysisConfig::create_production_preset() {
  AnalysisConfig config;
  config.set_analysis_level(AnalysisLevel::THOROUGH);
  config.set_type_safety_level(TypeSafetyLevel::STRICT);
  config.set_optimization_level(OptimizationLevel::AGGRESSIVE);

  // 프로덕션용 설정
  auto &memory_config = config.get_memory_safety_policy();
  memory_config.enforce_null_safety = true;
  memory_config.enforce_bounds_checking = true;
  memory_config.allow_unsafe_operations = false;
  memory_config.detect_memory_leaks = true;

  auto &perf_config = config.get_performance_config();
  perf_config.analyze_complexity = true;
  perf_config.detect_inefficiencies = true;
  perf_config.suggest_optimizations = true;

  return config;
}

AnalysisConfig AnalysisConfig::create_research_preset() {
  AnalysisConfig config;
  config.set_analysis_level(AnalysisLevel::RESEARCH);
  config.set_type_safety_level(TypeSafetyLevel::ZERO_COST);
  config.set_optimization_level(OptimizationLevel::EXPERIMENTAL);

  // 연구용 설정 - 모든 기능 활성화
  auto &compile_config = config.get_compile_time_config();
  compile_config.enable_template_metaprogramming = true;
  compile_config.enable_static_assertions = true;
  compile_config.aggressive_constant_folding = true;
  compile_config.max_cache_size = 20000;

  auto &ebnf_config = config.get_ebnf_compliance_config();
  ebnf_config.enforce_strict_grammar = true;
  ebnf_config.detect_missing_features = true;
  ebnf_config.suggest_ebnf_fixes = true;

  return config;
}

AnalysisConfig AnalysisConfig::create_ide_preset() {
  AnalysisConfig config;
  config.set_analysis_level(AnalysisLevel::STANDARD);
  config.set_type_safety_level(TypeSafetyLevel::STANDARD);
  config.set_optimization_level(OptimizationLevel::BASIC);

  // IDE용 설정 - 실시간 분석에 최적화
  auto &error_config = config.get_error_reporting_config();
  error_config.show_warnings = true;
  error_config.show_suggestions = true;
  error_config.show_performance_hints = true;
  error_config.stop_on_first_error = false;
  error_config.colorize_output = false; // IDE가 색상 처리

  auto &compile_config = config.get_compile_time_config();
  compile_config.cache_compile_time_results = true;
  compile_config.max_cache_size = 10000;

  return config;
}

std::vector<std::string> AnalysisConfig::validate_config() const {
  std::vector<std::string> errors;

  if (!is_level_combination_valid()) {
    errors.push_back("Invalid combination of analysis levels");
  }

  if (m_compile_time_config.max_cache_size == 0) {
    errors.push_back("Compile time cache size cannot be zero");
  }

  if (m_performance_config.max_recursion_depth < 10) {
    errors.push_back("Max recursion depth too low (minimum 10)");
  }

  if (m_error_reporting.max_errors_per_file == 0) {
    errors.push_back("Max errors per file cannot be zero");
  }

  if (m_performance_config.performance_threshold < 0.0 ||
      m_performance_config.performance_threshold > 1.0) {
    errors.push_back("Performance threshold must be between 0.0 and 1.0");
  }

  return errors;
}

bool AnalysisConfig::is_level_combination_valid() const {
  // Zero-cost 검증에는 최소한의 최적화가 필요
  if (m_type_safety_level == TypeSafetyLevel::ZERO_COST &&
      m_optimization_level == OptimizationLevel::NONE) {
    return false;
  }

  // 연구 레벨 분석에는 실험적 최적화가 권장됨
  if (m_analysis_level == AnalysisLevel::RESEARCH &&
      m_optimization_level == OptimizationLevel::NONE) {
    return false;
  }

  return true;
}

void AnalysisConfig::apply_level_constraints() {
  switch (m_analysis_level) {
  case AnalysisLevel::MINIMAL:
    m_error_reporting.max_errors_per_file =
        std::min(m_error_reporting.max_errors_per_file, size_t(10));
    break;

  case AnalysisLevel::RESEARCH:
    m_performance_config.analyze_complexity = true;
    m_performance_config.detect_inefficiencies = true;
    m_ebnf_compliance.enforce_strict_grammar = true;
    break;

  default:
    break;
  }

  // 타입 안전성 레벨에 따른 제약사항
  switch (m_type_safety_level) {
  case TypeSafetyLevel::STRICT:
    m_memory_safety.enforce_null_safety = true;
    m_memory_safety.enforce_bounds_checking = true;
    break;

  case TypeSafetyLevel::ZERO_COST:
    m_compile_time_config.aggressive_constant_folding = true;
    break;

  default:
    break;
  }
}

std::string AnalysisConfig::to_json() const {
  std::ostringstream json;
  json << "{\n";
  json << "  \"analysis_level\": " << static_cast<int>(m_analysis_level)
       << ",\n";
  json << "  \"type_safety_level\": " << static_cast<int>(m_type_safety_level)
       << ",\n";
  json << "  \"optimization_level\": " << static_cast<int>(m_optimization_level)
       << ",\n";

  json << "  \"memory_safety\": {\n";
  json << "    \"enforce_null_safety\": "
       << (m_memory_safety.enforce_null_safety ? "true" : "false") << ",\n";
  json << "    \"enforce_bounds_checking\": "
       << (m_memory_safety.enforce_bounds_checking ? "true" : "false") << ",\n";
  json << "    \"allow_unsafe_operations\": "
       << (m_memory_safety.allow_unsafe_operations ? "true" : "false") << "\n";
  json << "  },\n";

  json << "  \"error_reporting\": {\n";
  json << "    \"show_warnings\": "
       << (m_error_reporting.show_warnings ? "true" : "false") << ",\n";
  json << "    \"show_suggestions\": "
       << (m_error_reporting.show_suggestions ? "true" : "false") << ",\n";
  json << "    \"max_errors_per_file\": "
       << m_error_reporting.max_errors_per_file << "\n";
  json << "  }\n";

  json << "}";
  return json.str();
}

bool AnalysisConfig::from_json(const std::string &json) {
  // 간단한 JSON 파싱 (실제 프로젝트에서는 JSON 라이브러리 사용)
  // 여기서는 기본적인 구현만 제공
  try {
    // TODO: 실제 JSON 파싱 구현
    return true;
  } catch (...) {
    return false;
  }
}

void AnalysisConfig::load_from_environment() {
  const char *level_env = std::getenv("NUGDEV_ANALYSIS_LEVEL");
  if (level_env) {
    int level = std::atoi(level_env);
    if (level >= 0 && level <= 3) {
      m_analysis_level = static_cast<AnalysisLevel>(level);
    }
  }

  const char *type_safety_env = std::getenv("NUGDEV_TYPE_SAFETY");
  if (type_safety_env) {
    int safety = std::atoi(type_safety_env);
    if (safety >= 0 && safety <= 3) {
      m_type_safety_level = static_cast<TypeSafetyLevel>(safety);
    }
  }

  const char *optimization_env = std::getenv("NUGDEV_OPTIMIZATION");
  if (optimization_env) {
    int opt = std::atoi(optimization_env);
    if (opt >= 0 && opt <= 3) {
      m_optimization_level = static_cast<OptimizationLevel>(opt);
    }
  }

  const char *debug_env = std::getenv("NUGDEV_DEBUG");
  if (debug_env && std::string(debug_env) == "1") {
    m_error_reporting.colorize_output = true;
    m_error_reporting.show_suggestions = true;
  }
}

bool AnalysisConfig::load_from_file(const std::string &config_file) {
  std::ifstream file(config_file);
  if (!file.is_open()) {
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return from_json(buffer.str());
}

bool AnalysisConfig::save_to_file(const std::string &config_file) const {
  std::ofstream file(config_file);
  if (!file.is_open()) {
    return false;
  }

  file << to_json();
  return true;
}

} // namespace nugdev::compiler::analysis