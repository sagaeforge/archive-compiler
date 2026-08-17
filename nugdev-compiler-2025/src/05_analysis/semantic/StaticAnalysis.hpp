#pragma once

#include "04_parsing/ast/core/ASTNode.hpp"
#include "04_parsing/ast/expressions/Expressions.hpp"
#include "05_analysis/errors/AnalysisError.hpp"
#include "05_analysis/semantic/StrongTypeSystem.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 정적 분석 강화 시스템
 *
 * 컴파일 타임에 더 많은 에러를 잡아내는 정적 분석:
 * - 정적 단언(static assertions)
 * - 계약 기반 프로그래밍
 * - 불변성 검증
 * - 메모리 안전성 분석
 */
class EnhancedStaticAnalyzer {
public:
  explicit EnhancedStaticAnalyzer(const SymbolTable &symbol_table);

  /**
   * @brief 종합적인 정적 분석 수행
   */
  std::vector<AnalysisError> perform_static_analysis(ast::ASTNode &root);

  /**
   * @brief 계약 검증 (Contract Verification)
   */
  std::vector<AnalysisError> verify_contracts(ast::ASTNode &root);

  /**
   * @brief 불변성 검증 (Invariant Checking)
   */
  std::vector<AnalysisError> check_invariants(ast::ASTNode &root);

  /**
   * @brief 리소스 생명주기 분석
   */
  std::vector<AnalysisError> analyze_resource_lifetimes(ast::ASTNode &root);

private:
  const SymbolTable &m_symbol_table;
  std::unordered_map<const ast::ASTNode *, std::unique_ptr<StrongType>>
      m_type_cache;
};

/**
 * @brief 계약 기반 프로그래밍 지원
 */
class ContractChecker {
public:
  /**
   * @brief 함수 계약 정보
   */
  struct FunctionContract {
    std::vector<std::unique_ptr<ast::Expression>> preconditions;  // 사전 조건
    std::vector<std::unique_ptr<ast::Expression>> postconditions; // 사후 조건
    std::vector<std::unique_ptr<ast::Expression>> assertions;     // 중간 단언
  };

  /**
   * @brief 계약 검증
   */
  std::vector<AnalysisError>
  verify_function_contracts(const ast::FunctionDeclaration &function,
                            const FunctionContract &contract);

  /**
   * @brief 루프 불변식 검증
   */
  std::vector<AnalysisError> verify_loop_invariants(
      const ast::ForStatement &loop,
      const std::vector<std::unique_ptr<ast::Expression>> &invariants);

private:
  /**
   * @brief 조건 정적 검증
   */
  bool can_prove_condition(const ast::Expression &condition);

  /**
   * @brief Hoare 논리 기반 검증
   */
  bool verify_hoare_triple(const ast::Expression &precondition,
                           const ast::Statement &statement,
                           const ast::Expression &postcondition);
};

/**
 * @brief 값 범위 분석 (Value Range Analysis)
 */
class ValueRangeAnalyzer {
public:
  /**
   * @brief 값 범위 정보
   */
  struct ValueRange {
    std::optional<int64_t> min_value;
    std::optional<int64_t> max_value;
    bool is_constant = false;
    std::optional<int64_t> constant_value;

    bool contains(int64_t value) const;
    bool overlaps_with(const ValueRange &other) const;
    ValueRange intersect_with(const ValueRange &other) const;
    ValueRange union_with(const ValueRange &other) const;
  };

  /**
   * @brief 변수별 값 범위 분석
   */
  std::unordered_map<std::string, ValueRange>
  analyze_value_ranges(ast::ASTNode &root);

  /**
   * @brief 정수 오버플로우 감지
   */
  std::vector<AnalysisError> detect_integer_overflow(ast::ASTNode &root);

  /**
   * @brief 배열 경계 검사
   */
  std::vector<AnalysisError> check_array_bounds(ast::ASTNode &root);

private:
  std::unordered_map<std::string, ValueRange> m_value_ranges;

  /**
   * @brief 표현식의 값 범위 계산
   */
  ValueRange compute_expression_range(const ast::Expression &expr);

  /**
   * @brief 이항 연산의 결과 범위 계산
   */
  ValueRange compute_binary_operation_range(ast::BinaryExpression::Operator op,
                                            const ValueRange &left,
                                            const ValueRange &right);
};

/**
 * @brief 순수성 분석 (Purity Analysis)
 */
class PurityAnalyzer {
public:
  /**
   * @brief 함수 순수성 정보
   */
  struct PurityInfo {
    bool is_pure;  // 부작용이 없고 같은 입력에 같은 출력
    bool is_const; // 전역 상태를 읽기만 함
    bool is_total; // 모든 입력에 대해 종료됨
    std::unordered_set<std::string> modified_globals; // 수정하는 전역 변수들
  };

  /**
   * @brief 함수 순수성 분석
   */
  PurityInfo analyze_function_purity(const ast::FunctionDeclaration &function);

  /**
   * @brief 표현식 순수성 검사
   */
  bool is_pure_expression(const ast::Expression &expr);

  /**
   * @brief 부작용 감지
   */
  std::vector<std::string> detect_side_effects(const ast::ASTNode &node);

private:
  std::unordered_map<std::string, PurityInfo> m_function_purity_cache;

  /**
   * @brief 함수 호출 부작용 분석
   */
  bool has_side_effects_in_call(const ast::PostfixExpression &call);

  /**
   * @brief 전역 변수 접근 분석
   */
  std::unordered_set<std::string>
  analyze_global_access(const ast::ASTNode &node);
};

/**
 * @brief 소유권 및 차용 분석 (Ownership & Borrowing Analysis)
 */
class OwnershipAnalyzer {
public:
  /**
   * @brief 소유권 상태
   */
  enum class OwnershipState {
    OWNED,        // 소유됨
    BORROWED,     // 차용됨 (읽기 전용)
    MUT_BORROWED, // 가변 차용됨
    MOVED,        // 이동됨 (더 이상 사용 불가)
    UNINITIALIZED // 초기화되지 않음
  };

  /**
   * @brief 리소스 정보
   */
  struct ResourceInfo {
    std::string name;
    OwnershipState state;
    std::optional<std::string> borrowed_from; // 차용 소스
    size_t lifetime_scope_depth;
  };

  /**
   * @brief 소유권 분석 수행
   */
  std::vector<AnalysisError> analyze_ownership(ast::ASTNode &root);

  /**
   * @brief 차용 검사
   */
  std::vector<AnalysisError> check_borrowing_rules(ast::ASTNode &root);

  /**
   * @brief 이동 의미론 검증
   */
  std::vector<AnalysisError> verify_move_semantics(ast::ASTNode &root);

private:
  std::unordered_map<std::string, ResourceInfo> m_resource_states;

  /**
   * @brief 소유권 전이 추적
   */
  void track_ownership_transfer(const std::string &from, const std::string &to);

  /**
   * @brief 생명주기 충돌 검사
   */
  bool check_lifetime_conflict(const std::string &resource1,
                               const std::string &resource2);
};

/**
 * @brief 불변성 검사기 (Immutability Checker)
 */
class ImmutabilityChecker {
public:
  /**
   * @brief 불변성 위반 검사
   */
  std::vector<AnalysisError> check_immutability_violations(ast::ASTNode &root);

  /**
   * @brief 깊은 불변성 검사
   */
  std::vector<AnalysisError> check_deep_immutability(ast::ASTNode &root);

  /**
   * @brief 함수형 프로그래밍 규칙 검증
   */
  std::vector<AnalysisError> verify_functional_constraints(ast::ASTNode &root);

private:
  /**
   * @brief 변수 변경 시도 감지
   */
  bool is_mutation_attempt(const ast::AssignmentExpression &assignment);

  /**
   * @brief 간접 변경 감지 (포인터, 참조를 통한)
   */
  bool is_indirect_mutation(const ast::Expression &expr);
};

/**
 * @brief 동시성 안전성 분석
 */
class ConcurrencySafetyAnalyzer {
public:
  /**
   * @brief 경쟁 조건 감지
   */
  std::vector<AnalysisError> detect_race_conditions(ast::ASTNode &root);

  /**
   * @brief 데드락 가능성 분석
   */
  std::vector<AnalysisError> analyze_deadlock_potential(ast::ASTNode &root);

  /**
   * @brief 스레드 안전성 검증
   */
  std::vector<AnalysisError> verify_thread_safety(ast::ASTNode &root);

private:
  /**
   * @brief 공유 리소스 식별
   */
  std::unordered_set<std::string> identify_shared_resources(ast::ASTNode &root);

  /**
   * @brief 동기화 패턴 분석
   */
  void analyze_synchronization_patterns(ast::ASTNode &root);
};

/**
 * @brief 성능 분석기
 */
class PerformanceAnalyzer {
public:
  /**
   * @brief 성능 잠재적 문제점
   */
  struct PerformanceIssue {
    enum class Type {
      INEFFICIENT_ALGORITHM,
      UNNECESSARY_ALLOCATION,
      POOR_CACHE_LOCALITY,
      EXCESSIVE_RECURSION,
      SUBOPTIMAL_DATA_STRUCTURE
    };

    Type type;
    const ast::ASTNode *location;
    std::string description;
    std::string suggestion;
    double severity_score;
  };

  /**
   * @brief 성능 문제 분석
   */
  std::vector<PerformanceIssue> analyze_performance_issues(ast::ASTNode &root);

  /**
   * @brief 복잡도 분석
   */
  struct ComplexityInfo {
    std::string time_complexity;
    std::string space_complexity;
    bool is_optimal;
  };

  ComplexityInfo
  analyze_algorithm_complexity(const ast::FunctionDeclaration &function);

private:
  /**
   * @brief 루프 복잡도 분석
   */
  ComplexityInfo analyze_loop_complexity(const ast::ForStatement &loop);

  /**
   * @brief 메모리 할당 패턴 분석
   */
  std::vector<PerformanceIssue> analyze_allocation_patterns(ast::ASTNode &root);
};

/**
 * @brief 컴파일 타임 최적화 분석
 */
class CompileTimeOptimizationAnalyzer {
public:
  /**
   * @brief 컴파일 타임 최적화 기회
   */
  struct OptimizationOpportunity {
    enum class Type {
      CONSTANT_PROPAGATION,
      DEAD_CODE_ELIMINATION,
      FUNCTION_INLINING,
      LOOP_UNROLLING,
      TEMPLATE_SPECIALIZATION
    };

    Type type;
    const ast::ASTNode *node;
    std::string description;
    double expected_speedup;
    size_t confidence_percentage;
  };

  /**
   * @brief 최적화 기회 분석
   */
  std::vector<OptimizationOpportunity>
  analyze_optimization_opportunities(ast::ASTNode &root);

  /**
   * @brief Zero-cost 추상화 검증
   */
  bool verify_zero_cost_abstractions(ast::ASTNode &root);

private:
  /**
   * @brief 인라이닝 후보 분석
   */
  std::vector<OptimizationOpportunity>
  analyze_inlining_candidates(ast::ASTNode &root);

  /**
   * @brief 템플릿 특수화 기회 분석
   */
  std::vector<OptimizationOpportunity>
  analyze_specialization_opportunities(ast::ASTNode &root);
};

} // namespace nugdev::compiler::analysis