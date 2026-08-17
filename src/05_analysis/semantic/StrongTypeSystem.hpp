#pragma once

#include "04_parsing/ast/expressions/Expressions.hpp"
#include "04_parsing/ast/types/Types.hpp"
#include "05_analysis/errors/AnalysisError.hpp"
#include "05_analysis/semantic/SymbolTable.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace nugdev::compiler::analysis {

/**
 * @brief 강화된 타입 정보
 */
class StrongType {
public:
  enum class Kind {
    PRIMITIVE,  // 기본 타입 (number, string, boolean, character) - EBNF 준수
    ARRAY,      // 배열 타입
    OBJECT,     // 객체/구조체 타입
    FUNCTION,   // 함수 타입 - EBNF: function_type
    OPTIONAL,   // 옵셔널 타입 (T?) - EBNF: optional_type
    TUPLE,      // 튜플 타입 - EBNF: tuple_type
    UNION,      // 유니온 타입 (T | U)
    NEVER,      // Never 타입 (값이 없음)
    NULL_TYPE,  // Null 타입 - EBNF: null_literal
    NONE_TYPE,  // None 타입 - EBNF: none_literal
    RANGE_TYPE, // Range 타입 - EBNF: range_literal
    UNKNOWN,    // Unknown 타입
    GENERIC,    // 제네릭 타입 매개변수
    CONSTRAINED // 제약이 있는 타입
  };

  enum class Nullability {
    NON_NULL, // null이 될 수 없음
    NULLABLE, // null이 될 수 있음
    UNKNOWN   // null 가능성 불명
  };

  StrongType(Kind kind, const std::string &name);

  // 기본 속성
  Kind get_kind() const { return m_kind; }
  const std::string &get_name() const { return m_name; }
  Nullability get_nullability() const { return m_nullability; }

  void set_nullability(Nullability nullability) { m_nullability = nullability; }

  // 타입 관계
  bool is_assignable_from(const StrongType &other) const;
  bool is_convertible_to(const StrongType &other) const;
  bool is_subtype_of(const StrongType &other) const;
  bool has_common_type_with(const StrongType &other) const;

  // 특수 타입 검사
  bool is_primitive() const { return m_kind == Kind::PRIMITIVE; }
  bool is_nullable() const { return m_nullability == Nullability::NULLABLE; }
  bool is_numeric() const;
  bool is_integral() const;
  bool is_floating_point() const;
  bool is_collection() const;

  // 제네릭 타입 지원
  void add_type_parameter(const std::string &name,
                          std::unique_ptr<StrongType> bound = nullptr);
  const std::unordered_map<std::string, std::unique_ptr<StrongType>> &
  get_type_parameters() const {
    return m_type_parameters;
  }

  // 타입 제약
  void add_constraint(std::unique_ptr<StrongType> constraint);
  const std::vector<std::unique_ptr<StrongType>> &get_constraints() const {
    return m_constraints;
  }

  // 타입 정보
  size_t get_size_in_bytes() const;
  size_t get_alignment() const;

  std::string to_string() const;

private:
  Kind m_kind;
  std::string m_name;
  Nullability m_nullability = Nullability::NON_NULL;

  // 제네릭 타입 매개변수
  std::unordered_map<std::string, std::unique_ptr<StrongType>>
      m_type_parameters;

  // 타입 제약
  std::vector<std::unique_ptr<StrongType>> m_constraints;

  // 캐시된 속성들
  mutable std::optional<size_t> m_cached_size;
  mutable std::optional<size_t> m_cached_alignment;
};

/**
 * @brief 향상된 타입 검사기
 */
class EnhancedTypeChecker : public ast::DefaultASTVisitor {
public:
  explicit EnhancedTypeChecker(const SymbolTable &symbol_table);

  /**
   * @brief 강화된 타입 검사 수행
   */
  std::vector<AnalysisError> check_types_enhanced(ast::ASTNode &root);

  /**
   * @brief 널 안전성 검사
   */
  std::vector<AnalysisError> check_null_safety(ast::ASTNode &root);

  /**
   * @brief 경계 검사
   */
  std::vector<AnalysisError> check_bounds_safety(ast::ASTNode &root);

  /**
   * @brief 메모리 안전성 검사
   */
  std::vector<AnalysisError> check_memory_safety(ast::ASTNode &root);

  // AST 방문자 메서드들
  void visit(ast::BinaryExpression &node) override;
  void visit(ast::UnaryExpression &node) override;
  void visit(ast::PostfixExpression &node) override;
  void visit(ast::AssignmentExpression &node) override;
  void visit(ast::Identifier &node) override;

private:
  const SymbolTable &m_symbol_table;
  std::vector<AnalysisError> m_errors;
  std::unordered_map<const ast::ASTNode *, std::unique_ptr<StrongType>>
      m_node_types;

  /**
   * @brief 표현식의 타입 추론
   */
  std::unique_ptr<StrongType> infer_type(const ast::Expression &expr);

  /**
   * @brief 널 포인터 역참조 검사
   */
  bool check_null_dereference(const ast::Expression &expr);

  /**
   * @brief 배열 경계 검사
   */
  bool check_array_bounds(const ast::PostfixExpression &array_access);

  /**
   * @brief 정수 오버플로우 검사
   */
  bool check_integer_overflow(const ast::BinaryExpression &expr);

  /**
   * @brief 타입 안전 캐스팅 검사
   */
  bool check_safe_casting(const StrongType &from, const StrongType &to);
};

/**
 * @brief 제네릭 타입 인스턴스화
 */
class GenericTypeInstantiator {
public:
  /**
   * @brief 제네릭 타입을 구체 타입으로 인스턴스화
   */
  std::unique_ptr<StrongType>
  instantiate(const StrongType &generic_type,
              const std::unordered_map<std::string, std::unique_ptr<StrongType>>
                  &type_args);

  /**
   * @brief 타입 매개변수 추론
   */
  std::unordered_map<std::string, std::unique_ptr<StrongType>>
  infer_type_arguments(
      const StrongType &generic_type,
      const std::vector<std::unique_ptr<StrongType>> &argument_types);

  /**
   * @brief 제약 검사
   */
  bool check_constraints(
      const std::unordered_map<std::string, std::unique_ptr<StrongType>>
          &type_args,
      const StrongType &generic_type);

private:
  /**
   * @brief 타입 치환
   */
  std::unique_ptr<StrongType> substitute_type_parameters(
      const StrongType &type,
      const std::unordered_map<std::string, std::unique_ptr<StrongType>>
          &substitutions);
};

/**
 * @brief 타입 추론 엔진
 */
class TypeInferenceEngine {
public:
  explicit TypeInferenceEngine(const SymbolTable &symbol_table);

  /**
   * @brief Hindley-Milner 스타일 타입 추론
   */
  std::unique_ptr<StrongType> infer_type(const ast::Expression &expr);

  /**
   * @brief 타입 변수 생성
   */
  std::unique_ptr<StrongType>
  create_type_variable(const std::string &name = "");

  /**
   * @brief 타입 단일화 (Unification)
   */
  bool unify(const StrongType &type1, const StrongType &type2);

  /**
   * @brief 타입 일반화 (Generalization)
   */
  std::unique_ptr<StrongType> generalize(const StrongType &type);

private:
  const SymbolTable &m_symbol_table;
  std::unordered_map<std::string, std::unique_ptr<StrongType>>
      m_type_substitutions;
  size_t m_type_var_counter = 0;

  /**
   * @brief 타입 치환 적용
   */
  std::unique_ptr<StrongType> apply_substitution(const StrongType &type);

  /**
   * @brief 자유 타입 변수 수집
   */
  std::unordered_set<std::string>
  get_free_type_variables(const StrongType &type);

  /**
   * @brief Most General Unifier 계산
   */
  bool compute_mgu(const StrongType &type1, const StrongType &type2);
};

/**
 * @brief 컴파일 타임 타입 안전성 보장
 */
class CompileTimeTypeSafety {
public:
  /**
   * @brief 컴파일 타임 타입 검증
   */
  struct TypeCheckResult {
    bool is_type_safe;
    std::vector<AnalysisError> type_errors;
    std::vector<AnalysisError> safety_warnings;
  };

  static TypeCheckResult verify_type_safety(ast::ASTNode &root,
                                            const SymbolTable &symbol_table);

  /**
   * @brief Zero-cost 추상화 검증
   */
  static bool verify_zero_cost_abstraction(const ast::ASTNode &node);

  /**
   * @brief 이동 의미론 검증
   */
  static std::vector<AnalysisError> verify_move_semantics(ast::ASTNode &root);

  /**
   * @brief RAII 패턴 검증
   */
  static std::vector<AnalysisError> verify_raii_patterns(ast::ASTNode &root);

private:
  /**
   * @brief 생명주기 분석
   */
  static void analyze_lifetimes(ast::ASTNode &root);

  /**
   * @brief 소유권 분석
   */
  static void analyze_ownership(ast::ASTNode &root);
};

/**
 * @brief 타입 기반 최적화 힌트
 */
class TypeBasedOptimizationHints {
public:
  struct OptimizationHint {
    enum class Type {
      INLINE_FUNCTION,        // 함수 인라이닝
      DEVIRTUALIZE_CALL,      // 가상 함수 호출 최적화
      ELIMINATE_BOUNDS_CHECK, // 경계 검사 제거
      OPTIMIZE_MEMORY_LAYOUT, // 메모리 레이아웃 최적화
      VECTORIZE_LOOP          // 루프 벡터화
    };

    Type type;
    const ast::ASTNode *location;
    std::string description;
    double confidence_score;
  };

  static std::vector<OptimizationHint> generate_hints(
      ast::ASTNode &root,
      const std::unordered_map<const ast::ASTNode *,
                               std::unique_ptr<StrongType>> &type_info);

private:
  static std::vector<OptimizationHint> analyze_function_calls(
      ast::ASTNode &root,
      const std::unordered_map<const ast::ASTNode *,
                               std::unique_ptr<StrongType>> &type_info);

  static std::vector<OptimizationHint> analyze_array_accesses(
      ast::ASTNode &root,
      const std::unordered_map<const ast::ASTNode *,
                               std::unique_ptr<StrongType>> &type_info);
};

/**
 * @brief 타입 안전성 정책 관리자
 */
class TypeSafetyPolicyManager {
public:
  enum class SafetyLevel {
    PERMISSIVE, // 관대한 검사
    STANDARD,   // 표준 검사
    STRICT,     // 엄격한 검사
    PARANOID    // 매우 엄격한 검사
  };

  struct SafetyPolicy {
    SafetyLevel level;
    bool enforce_null_safety;
    bool enforce_bounds_checking;
    bool enforce_integer_overflow_checking;
    bool enforce_memory_safety;
    bool allow_unsafe_operations;
  };

  explicit TypeSafetyPolicyManager(const SafetyPolicy &policy);

  /**
   * @brief 정책에 따른 타입 검사
   */
  std::vector<AnalysisError> enforce_policy(ast::ASTNode &root,
                                            const SymbolTable &symbol_table);

private:
  SafetyPolicy m_policy;

  std::vector<AnalysisError> check_null_safety_policy(ast::ASTNode &root);
  std::vector<AnalysisError> check_bounds_safety_policy(ast::ASTNode &root);
  std::vector<AnalysisError> check_memory_safety_policy(ast::ASTNode &root);
};

} // namespace nugdev::compiler::analysis