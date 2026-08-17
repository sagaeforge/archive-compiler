#pragma once

#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include "05_analysis/dataflow/DataFlowAnalysis.hpp"
#include "05_analysis/optimization/FunctionInlining.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 함수 간 데이터 흐름 분석
 *
 * 함수 경계를 넘나드는 데이터 흐름을 추적:
 * - 함수 호출을 통한 데이터 전파
 * - 글로벌 변수 효과 분석
 * - 부작용(Side Effect) 분석
 * - 함수 요약(Function Summary) 생성
 */
class InterproceduralDataFlowAnalysis {
public:
  /**
   * @brief 함수 요약 정보
   */
  struct FunctionSummary {
    std::unordered_set<std::string> reads;    // 읽는 전역 변수들
    std::unordered_set<std::string> writes;   // 쓰는 전역 변수들
    std::unordered_set<std::string> modifies; // 포인터로 수정하는 변수들
    bool has_side_effects;                    // 부작용 여부
    bool is_pure;                             // 순수 함수 여부
    bool terminates;                          // 종료 보장 여부

    FunctionSummary()
        : has_side_effects(false), is_pure(true), terminates(true) {}
  };

  explicit InterproceduralDataFlowAnalysis(
      const optimization::CallGraph &call_graph);

  /**
   * @brief 함수 간 분석 실행
   */
  void analyze();

  /**
   * @brief 함수 요약 정보 조회
   */
  const FunctionSummary *
  get_function_summary(const std::string &function_name) const;

  /**
   * @brief 순수 함수들 조회
   */
  std::vector<std::string> get_pure_functions() const;

  /**
   * @brief 부작용이 있는 함수들 조회
   */
  std::vector<std::string> get_functions_with_side_effects() const;

  /**
   * @brief 함수 호출 효과 분석
   */
  std::unordered_set<std::string>
  analyze_call_effects(const ast::PostfixExpression &call);

private:
  const optimization::CallGraph &m_call_graph;
  std::unordered_map<std::string, FunctionSummary> m_function_summaries;

  /**
   * @brief 함수별 요약 정보 구축
   */
  void build_function_summaries();

  /**
   * @brief 재귀적 함수 요약 정보 해결
   */
  void resolve_recursive_summaries();

  /**
   * @brief 함수의 직접적 효과 분석
   */
  FunctionSummary
  analyze_direct_effects(const ast::FunctionDeclaration &function);

  /**
   * @brief 함수 호출을 통한 간접적 효과 분석
   */
  void propagate_indirect_effects();
};

/**
 * @brief 포인터 분석 (Points-to Analysis) - Context-Sensitive
 *
 * 호출 컨텍스트를 고려한 정밀한 포인터 분석
 */
class ContextSensitivePointsToAnalysis {
public:
  /**
   * @brief 호출 컨텍스트
   */
  struct CallContext {
    std::vector<const ast::PostfixExpression *> call_stack;
    size_t context_depth;

    bool operator==(const CallContext &other) const {
      return call_stack == other.call_stack;
    }
  };

  /**
   * @brief 컨텍스트별 Points-to 정보
   */
  struct ContextualPointsToInfo {
    CallContext context;
    std::unordered_map<std::string, std::unordered_set<std::string>>
        points_to_map;
  };

  explicit ContextSensitivePointsToAnalysis(
      const optimization::CallGraph &call_graph, size_t max_context_depth = 3);

  /**
   * @brief 컨텍스트 민감 분석 실행
   */
  void analyze();

  /**
   * @brief 특정 컨텍스트에서의 Points-to 정보 조회
   */
  std::optional<std::unordered_set<std::string>>
  get_points_to_set(const std::string &variable,
                    const CallContext &context) const;

  /**
   * @brief 별명 쿼리 (컨텍스트 고려)
   */
  bool may_alias(const std::string &var1, const std::string &var2,
                 const CallContext &context) const;

private:
  const optimization::CallGraph &m_call_graph;
  size_t m_max_context_depth;
  std::vector<ContextualPointsToInfo> m_contextual_info;

  /**
   * @brief 호출 컨텍스트 생성
   */
  CallContext create_call_context(
      const std::vector<const ast::PostfixExpression *> &call_stack);

  /**
   * @brief 컨텍스트별 분석 수행
   */
  void analyze_with_context(const CallContext &context);
};

/**
 * @brief 모듈화된 분석 (Modular Analysis)
 *
 * 각 모듈을 독립적으로 분석하고 인터페이스를 통해 연결
 */
class ModularAnalysis {
public:
  /**
   * @brief 모듈 인터페이스 정보
   */
  struct ModuleInterface {
    std::string module_name;
    std::unordered_set<std::string> exported_functions;
    std::unordered_set<std::string> imported_functions;
    std::unordered_map<std::string,
                       InterproceduralDataFlowAnalysis::FunctionSummary>
        function_contracts;

    ModuleInterface(const std::string &name) : module_name(name) {}
  };

  explicit ModularAnalysis();

  /**
   * @brief 모듈 인터페이스 등록
   */
  void register_module_interface(std::unique_ptr<ModuleInterface> interface);

  /**
   * @brief 모듈 간 분석 수행
   */
  void analyze_inter_module_dependencies();

  /**
   * @brief 모듈별 분석 결과 조회
   */
  const ModuleInterface *
  get_module_interface(const std::string &module_name) const;

  /**
   * @brief 전체 프로그램 분석 결과 생성
   */
  InterproceduralDataFlowAnalysis::FunctionSummary
  merge_module_summaries(const std::string &function_name) const;

private:
  std::unordered_map<std::string, std::unique_ptr<ModuleInterface>>
      m_module_interfaces;

  /**
   * @brief 모듈 의존성 그래프 구축
   */
  void build_dependency_graph();

  /**
   * @brief 순환 의존성 감지
   */
  std::vector<std::vector<std::string>> detect_circular_dependencies();
};

/**
 * @brief 병렬 분석 최적화
 *
 * 함수 간 분석을 병렬로 수행하여 성능 향상
 */
class ParallelInterproceduralAnalysis {
public:
  explicit ParallelInterproceduralAnalysis(
      const optimization::CallGraph &call_graph);

  /**
   * @brief 병렬 분석 실행
   */
  void analyze_parallel(size_t num_threads = 0); // 0 = 하드웨어 동시성

  /**
   * @brief 병렬화 가능한 함수 그룹 식별
   */
  std::vector<std::vector<std::string>> identify_parallelizable_groups();

private:
  const optimization::CallGraph &m_call_graph;

  /**
   * @brief 함수 의존성 분석
   */
  void analyze_function_dependencies();

  /**
   * @brief 작업 분할
   */
  std::vector<std::vector<std::string>> partition_work(size_t num_partitions);
};

/**
 * @brief 함수 간 최적화 기회 탐지
 */
class InterproceduralOptimizationFinder {
public:
  /**
   * @brief 최적화 기회 종류
   */
  enum class OptimizationType {
    CONSTANT_PROPAGATION,      // 상수 전파
    DEAD_ARGUMENT_ELIMINATION, // 죽은 인자 제거
    FUNCTION_SPECIALIZATION,   // 함수 특화
    TAIL_CALL_OPTIMIZATION,    // 꼬리 호출 최적화
    CROSS_MODULE_INLINING      // 모듈 간 인라이닝
  };

  /**
   * @brief 최적화 기회 정보
   */
  struct OptimizationOpportunity {
    OptimizationType type;
    std::string function_name;
    std::string description;
    double estimated_benefit;
    std::vector<std::string> affected_functions;
  };

  explicit InterproceduralOptimizationFinder(
      const InterproceduralDataFlowAnalysis &ipda);

  /**
   * @brief 최적화 기회 탐지
   */
  std::vector<OptimizationOpportunity> find_optimization_opportunities();

  /**
   * @brief 함수 특화 후보 찾기
   */
  std::vector<std::string> find_specialization_candidates();

  /**
   * @brief 꼬리 호출 최적화 후보 찾기
   */
  std::vector<const ast::PostfixExpression *> find_tail_call_candidates();

private:
  const InterproceduralDataFlowAnalysis &m_ipda;

  /**
   * @brief 상수 인자 패턴 분석
   */
  void analyze_constant_argument_patterns();

  /**
   * @brief 사용되지 않는 인자 찾기
   */
  std::vector<std::string>
  find_unused_parameters(const std::string &function_name);
};

} // namespace nugdev::compiler::analysis

// CallContext를 위한 해시 특수화
namespace std {
template <>
struct hash<
    nugdev::compiler::analysis::ContextSensitivePointsToAnalysis::CallContext> {
  size_t
  operator()(const nugdev::compiler::analysis::
                 ContextSensitivePointsToAnalysis::CallContext &ctx) const {
    size_t result = 0;
    for (const auto *call : ctx.call_stack) {
      result ^= hash<const void *>{}(call) + 0x9e3779b9 + (result << 6) +
                (result >> 2);
    }
    return result;
  }
};
} // namespace std