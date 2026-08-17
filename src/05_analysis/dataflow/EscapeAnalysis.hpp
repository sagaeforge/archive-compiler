#pragma once

#include "04_parsing/ast/expressions/Expressions.hpp"
#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include "05_analysis/dataflow/AliasAnalysis.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief Escape Analysis - 객체가 할당된 스코프를 벗어나는지 분석
 *
 * 스택 할당 최적화, 동기화 제거, GC 최적화에 활용:
 * - 지역 할당 가능 객체 식별
 * - 스레드 로컬 객체 식별
 * - 불필요한 동기화 제거
 * - 메모리 할당 최적화
 */
class EscapeAnalyzer {
public:
  /**
   * @brief 객체의 Escape 상태
   */
  enum class EscapeState {
    NO_ESCAPE,     // 할당된 스코프를 벗어나지 않음
    ARG_ESCAPE,    // 함수 인자로만 전달됨 (호출자로 escape)
    GLOBAL_ESCAPE, // 전역적으로 escape (다른 스레드에서 접근 가능)
    UNKNOWN        // 분석 불가능
  };

  /**
   * @brief Escape 분석 결과
   */
  struct EscapeInfo {
    EscapeState state;
    std::string reason;                              // Escape 이유
    std::vector<const ast::ASTNode *> escape_points; // Escape 발생 지점들
    bool thread_local;                               // 스레드 로컬 여부
    bool can_stack_allocate;                         // 스택 할당 가능 여부
  };

  explicit EscapeAnalyzer(const ControlFlowGraph &cfg,
                          const AndersonPointsToAnalysis &alias_analysis);

  /**
   * @brief Escape 분석 실행
   */
  void analyze();

  /**
   * @brief 변수의 Escape 정보 조회
   */
  std::optional<EscapeInfo> get_escape_info(const std::string &variable) const;

  /**
   * @brief 스택 할당 가능한 객체들 조회
   */
  std::vector<std::string> get_stack_allocatable_objects() const;

  /**
   * @brief 스레드 로컬 객체들 조회
   */
  std::vector<std::string> get_thread_local_objects() const;

  /**
   * @brief 동기화가 불필요한 객체들 조회
   */
  std::vector<std::string> get_synchronization_free_objects() const;

private:
  const ControlFlowGraph &m_cfg;
  const AndersonPointsToAnalysis &m_alias_analysis;
  std::unordered_map<std::string, EscapeInfo> m_escape_info;

  /**
   * @brief 함수별 Escape 분석
   */
  void analyze_function_escapes(const ast::FunctionDeclaration &function);

  /**
   * @brief 표현식에서 Escape 패턴 감지
   */
  void analyze_expression_escape(const ast::Expression &expr);

  /**
   * @brief Return문을 통한 Escape 분석
   */
  bool escapes_through_return(const std::string &variable,
                              const ast::FunctionDeclaration &function);

  /**
   * @brief 함수 인자를 통한 Escape 분석
   */
  bool escapes_through_parameter(const std::string &variable,
                                 const ast::PostfixExpression &call);

  /**
   * @brief 전역 변수 할당을 통한 Escape 분석
   */
  bool escapes_through_global_assignment(const std::string &variable);

  /**
   * @brief 다른 스레드로의 Escape 분석
   */
  bool escapes_to_other_thread(const std::string &variable);

  /**
   * @brief 스택 할당 가능성 결정
   */
  bool can_be_stack_allocated(const EscapeInfo &info) const;
};

/**
 * @brief Scalar Replacement of Aggregates (SROA)
 *
 * 구조체/배열을 개별 스칼라 변수로 분해하여 최적화
 */
class ScalarReplacementAnalyzer {
public:
  struct ReplacementInfo {
    std::string original_variable;
    std::vector<std::string> scalar_fields; // 분해된 필드들
    bool profitable_to_replace;
    double estimated_benefit;
  };

  explicit ScalarReplacementAnalyzer(const EscapeAnalyzer &escape_analyzer);

  /**
   * @brief SROA 분석 실행
   */
  std::vector<ReplacementInfo> analyze_scalar_replacement_opportunities();

  /**
   * @brief 구조체 분해 가능성 확인
   */
  bool can_replace_aggregate(const std::string &variable) const;

private:
  const EscapeAnalyzer &m_escape_analyzer;

  /**
   * @brief 필드별 사용 패턴 분석
   */
  void analyze_field_usage_patterns(const std::string &variable,
                                    ReplacementInfo &info);

  /**
   * @brief 분해 이익 계산
   */
  double calculate_replacement_benefit(const ReplacementInfo &info);
};

/**
 * @brief 메모리 할당 최적화기
 */
class AllocationOptimizer {
public:
  /**
   * @brief 최적화 기회
   */
  struct OptimizationOpportunity {
    enum class Type {
      STACK_ALLOCATION, // 힙 -> 스택 할당
      ELIMINATION,      // 할당 제거 (인라이닝 등)
      POOLING,          // 객체 풀링
      BULK_ALLOCATION   // 벌크 할당
    };

    Type type;
    std::string variable;
    std::string description;
    double estimated_speedup;
    size_t memory_saved;
  };

  explicit AllocationOptimizer(const EscapeAnalyzer &escape_analyzer,
                               const ScalarReplacementAnalyzer &sroa_analyzer);

  /**
   * @brief 할당 최적화 기회 찾기
   */
  std::vector<OptimizationOpportunity> find_optimization_opportunities();

  /**
   * @brief 스택 할당 변환 추천
   */
  std::vector<std::string> recommend_stack_allocation();

  /**
   * @brief 불필요한 할당 제거 추천
   */
  std::vector<std::string> recommend_allocation_elimination();

private:
  const EscapeAnalyzer &m_escape_analyzer;
  const ScalarReplacementAnalyzer &m_sroa_analyzer;

  /**
   * @brief 할당 사이트 분석
   */
  void analyze_allocation_sites();

  /**
   * @brief 객체 생명주기 분석
   */
  void analyze_object_lifetimes();
};

/**
 * @brief 동기화 최적화 분석기
 */
class SynchronizationOptimizer {
public:
  /**
   * @brief 불필요한 동기화 감지
   */
  struct UnnecessarySync {
    const ast::ASTNode *sync_point;
    std::string reason;
    double performance_impact;
  };

  explicit SynchronizationOptimizer(const EscapeAnalyzer &escape_analyzer);

  /**
   * @brief 불필요한 동기화 찾기
   */
  std::vector<UnnecessarySync> find_unnecessary_synchronization();

  /**
   * @brief 락 병합 기회 찾기
   */
  std::vector<std::pair<const ast::ASTNode *, const ast::ASTNode *>>
  find_lock_coarsening_opportunities();

  /**
   * @brief 락 제거 기회 찾기
   */
  std::vector<const ast::ASTNode *> find_lock_elimination_opportunities();

private:
  const EscapeAnalyzer &m_escape_analyzer;

  /**
   * @brief 스레드 로컬 접근 확인
   */
  bool is_thread_local_access(const ast::Expression &expr);

  /**
   * @brief 경쟁 조건 가능성 분석
   */
  bool has_race_condition_risk(const ast::Expression &expr);
};

} // namespace nugdev::compiler::analysis