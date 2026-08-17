#pragma once

#include "04_parsing/ast/core/ASTNode.hpp"
#include "04_parsing/ast/expressions/Expressions.hpp"
#include "05_analysis/semantic/CompileTimeEvaluator.hpp"
#include "05_analysis/semantic/StrongTypeSystem.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 컴파일 타임 메타프로그래밍 지원
 *
 * 템플릿, 매크로, 컴파일 타임 반사(reflection) 등을 지원:
 * - 템플릿 메타프로그래밍
 * - constexpr 계산
 * - 컴파일 타임 조건부 컴파일
 * - 타입 조작
 */
class MetaProgrammingEngine {
public:
  explicit MetaProgrammingEngine(const CompileTimeEvaluator &evaluator);

  /**
   * @brief 템플릿 인스턴스화
   */
  struct TemplateInstantiation {
    std::string template_name;
    std::vector<CompileTimeValue> arguments;
    std::unique_ptr<ast::ASTNode> instantiated_code;
  };

  /**
   * @brief 템플릿 특수화
   */
  std::vector<TemplateInstantiation> instantiate_templates(
      ast::ASTNode &root,
      const std::unordered_map<std::string, std::vector<CompileTimeValue>>
          &template_args);

  /**
   * @brief 컴파일 타임 조건부 컴파일
   */
  std::unique_ptr<ast::ASTNode> evaluate_conditional_compilation(
      const ast::Expression &condition,
      std::unique_ptr<ast::ASTNode> then_branch,
      std::unique_ptr<ast::ASTNode> else_branch = nullptr);

  /**
   * @brief 코드 생성
   */
  std::unique_ptr<ast::ASTNode>
  generate_code(const std::string &template_name,
                const std::vector<CompileTimeValue> &arguments);

private:
  const CompileTimeEvaluator &m_evaluator;
  std::unordered_map<std::string, std::unique_ptr<ast::ASTNode>> m_templates;
};

/**
 * @brief 컴파일 타임 반사(Reflection)
 */
class CompileTimeReflection {
public:
  /**
   * @brief 타입 정보
   */
  struct TypeInfo {
    std::string name;
    size_t size;
    size_t alignment;
    std::vector<std::string> member_names;
    std::vector<std::unique_ptr<StrongType>> member_types;
    bool is_pod; // Plain Old Data
    bool is_trivial;
    bool is_standard_layout;
  };

  /**
   * @brief 함수 정보
   */
  struct FunctionInfo {
    std::string name;
    std::unique_ptr<StrongType> return_type;
    std::vector<std::unique_ptr<StrongType>> parameter_types;
    bool is_constexpr;
    bool is_noexcept;
    bool is_pure;
  };

  /**
   * @brief 타입 정보 추출
   */
  static TypeInfo extract_type_info(const StrongType &type);

  /**
   * @brief 함수 정보 추출
   */
  static FunctionInfo
  extract_function_info(const ast::FunctionDeclaration &function);

  /**
   * @brief 컴파일 타임 타입 검사
   */
  static bool is_same_type(const StrongType &type1, const StrongType &type2);
  static bool is_base_of(const StrongType &base, const StrongType &derived);
  static bool is_convertible(const StrongType &from, const StrongType &to);

  /**
   * @brief 타입 특성 (Type Traits)
   */
  static bool is_integral(const StrongType &type);
  static bool is_floating_point(const StrongType &type);
  static bool is_pointer(const StrongType &type);
  static bool is_reference(const StrongType &type);
  static bool is_array(const StrongType &type);
  static bool is_function(const StrongType &type);

private:
  static void analyze_type_layout(const StrongType &type, TypeInfo &info);
};

/**
 * @brief 컴파일 타임 어서션
 */
class StaticAssertions {
public:
  /**
   * @brief static_assert 구현
   */
  static bool evaluate_static_assert(const CompileTimeValue &condition,
                                     const std::string &message);

  /**
   * @brief 타입 어서션
   */
  static bool assert_is_type(const StrongType &type,
                             const std::string &expected_type);
  static bool assert_is_convertible(const StrongType &from,
                                    const StrongType &to);
  static bool assert_size_equals(const StrongType &type, size_t expected_size);

  /**
   * @brief 컴파일 타임 제약 검사
   */
  static std::vector<AnalysisError> validate_constraints(
      ast::ASTNode &root,
      const std::vector<std::pair<CompileTimeValue, std::string>> &assertions);
};

/**
 * @brief 컴파일 타임 루프 언롤링
 */
class CompileTimeLoopUnroller {
public:
  /**
   * @brief 컴파일 타임에 결정 가능한 루프 언롤
   */
  std::unique_ptr<ast::ASTNode>
  unroll_compile_time_loop(const ast::ForStatement &loop,
                           const CompileTimeEvaluator &evaluator);

  /**
   * @brief 템플릿 재귀를 통한 루프 언롤
   */
  std::unique_ptr<ast::ASTNode>
  unroll_template_recursion(const std::string &template_name,
                            const CompileTimeValue &count,
                            const std::vector<CompileTimeValue> &initial_args);

private:
  /**
   * @brief 루프 바운드 분석
   */
  std::optional<std::pair<int64_t, int64_t>>
  analyze_loop_bounds(const ast::ForStatement &loop,
                      const CompileTimeEvaluator &evaluator);

  /**
   * @brief 루프 바디 복제
   */
  std::unique_ptr<ast::ASTNode>
  clone_loop_body(const ast::Statement &body, const std::string &induction_var,
                  const CompileTimeValue &iteration_value);
};

/**
 * @brief SFINAE (Substitution Failure Is Not An Error) 지원
 */
class SFINAEHandler {
public:
  /**
   * @brief 템플릿 대체 실패 처리
   */
  struct SubstitutionResult {
    bool success;
    std::unique_ptr<StrongType> result_type;
    std::string failure_reason;
  };

  /**
   * @brief 타입 대체 시도
   */
  SubstitutionResult attempt_substitution(
      const StrongType &template_type,
      const std::unordered_map<std::string, std::unique_ptr<StrongType>>
          &substitutions);

  /**
   * @brief 함수 오버로드 해결
   */
  std::vector<const ast::FunctionDeclaration *> resolve_overloads(
      const std::string &function_name,
      const std::vector<std::unique_ptr<StrongType>> &argument_types,
      const std::vector<const ast::FunctionDeclaration *> &candidates);

private:
  /**
   * @brief 대체 가능성 검사
   */
  bool is_substitution_valid(const StrongType &template_param,
                             const StrongType &substitute_type);

  /**
   * @brief 오버로드 순위 계산
   */
  int calculate_overload_rank(
      const ast::FunctionDeclaration &function,
      const std::vector<std::unique_ptr<StrongType>> &argument_types);
};

/**
 * @brief 컴파일 타임 문자열 처리
 */
class CompileTimeStringProcessor {
public:
  /**
   * @brief 문자열 리터럴 조작
   */
  static CompileTimeValue
  concatenate_strings(const std::vector<CompileTimeValue> &strings);

  static CompileTimeValue
  format_string(const std::string &format,
                const std::vector<CompileTimeValue> &arguments);

  /**
   * @brief 해시 계산
   */
  static CompileTimeValue compute_string_hash(const std::string &str);

  /**
   * @brief 문자열 변환
   */
  static CompileTimeValue to_upper_case(const std::string &str);
  static CompileTimeValue to_lower_case(const std::string &str);
  static CompileTimeValue substring(const std::string &str, size_t start,
                                    size_t length);

private:
  static std::string apply_format_specifier(const std::string &specifier,
                                            const CompileTimeValue &value);
};

/**
 * @brief 컴파일 타임 데이터 구조
 */
template <typename T> class CompileTimeArray {
public:
  constexpr CompileTimeArray(std::initializer_list<T> values)
      : m_data(values) {}

  constexpr size_t size() const { return m_data.size(); }
  constexpr const T &operator[](size_t index) const { return m_data[index]; }

  constexpr auto begin() const { return m_data.begin(); }
  constexpr auto end() const { return m_data.end(); }

private:
  std::vector<T> m_data;
};

/**
 * @brief 컴파일 타임 해시맵
 */
template <typename Key, typename Value> class CompileTimeMap {
public:
  constexpr CompileTimeMap(std::initializer_list<std::pair<Key, Value>> pairs)
      : m_data(pairs) {}

  constexpr std::optional<Value> get(const Key &key) const {
    for (const auto &pair : m_data) {
      if (pair.first == key) {
        return pair.second;
      }
    }
    return std::nullopt;
  }

  constexpr bool contains(const Key &key) const { return get(key).has_value(); }

private:
  std::vector<std::pair<Key, Value>> m_data;
};

/**
 * @brief 메타프로그래밍 유틸리티
 */
class MetaProgrammingUtils {
public:
  /**
   * @brief 컴파일 타임 팩토리얼
   */
  template <int N> static constexpr int factorial() {
    if constexpr (N <= 1) {
      return 1;
    } else {
      return N * factorial<N - 1>();
    }
  }

  /**
   * @brief 컴파일 타임 거듭제곱
   */
  template <int Base, int Exp> static constexpr int power() {
    if constexpr (Exp == 0) {
      return 1;
    } else {
      return Base * power<Base, Exp - 1>();
    }
  }

  /**
   * @brief 타입 리스트 조작
   */
  template <typename... Types> struct TypeList {
    static constexpr size_t size = sizeof...(Types);
  };

  template <typename T, typename... Types>
  static constexpr bool contains_type() {
    return ((std::is_same_v<T, Types>) || ...);
  }

  /**
   * @brief 컴파일 타임 정렬
   */
  template <int... Values> static constexpr auto sort_values() {
    std::array<int, sizeof...(Values)> arr{Values...};
    std::sort(arr.begin(), arr.end());
    return arr;
  }
};

} // namespace nugdev::compiler::analysis