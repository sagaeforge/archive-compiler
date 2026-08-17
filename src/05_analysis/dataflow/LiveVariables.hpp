#pragma once

#include "05_analysis/dataflow/DataFlowAnalysis.hpp"
#include "05_analysis/errors/AnalysisError.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace nugdev::compiler::analysis {

/**
 * @brief 활성 변수 분석 (Live Variables Analysis)
 *
 * 각 프로그램 지점에서 어떤 변수가 "활성"인지 분석:
 * - 변수가 미래에 사용될 가능성이 있는지 확인
 * - 죽은 변수 할당 감지
 * - 레지스터 할당 최적화 지원
 */
class LiveVariablesAnalysis : public DataFlowAnalysis<VariableSet> {
public:
  LiveVariablesAnalysis();

  /**
   * @brief 활성 변수 분석 실행
   */
  std::vector<AnalysisError>
  analyze_live_variables(const ControlFlowGraph &cfg);

  /**
   * @brief 특정 지점에서 활성 변수들 조회
   */
  std::vector<std::string>
  get_live_variables_at_point(BasicBlock::BlockId block_id,
                              bool at_entry = true) const;

  /**
   * @brief 죽은 변수 할당 검사
   */
  std::vector<AnalysisError> check_dead_assignments() const;

  /**
   * @brief 변수 간섭 그래프 구축 (레지스터 할당용)
   */
  std::unordered_map<std::string, std::unordered_set<std::string>>
  build_interference_graph() const;

  /**
   * @brief 변수 생존 정보 통계
   */
  struct LivenessStatistics {
    size_t total_variables = 0;
    size_t max_live_at_once = 0;
    double average_lifetime = 0.0;
    std::vector<std::string> never_live_variables;
    std::vector<std::string> always_live_variables;
  };

  LivenessStatistics get_liveness_statistics() const;

protected:
  // DataFlowAnalysis 인터페이스 구현
  VariableSet transfer(const BasicBlock &block,
                       const VariableSet &in_set) override;
  VariableSet meet(const std::vector<VariableSet> &sets) override;
  VariableSet get_initial_set() override;
  VariableSet get_boundary_set() override;

private:
  std::unique_ptr<DefUseExtractor> m_def_use_extractor;
  std::unordered_map<BasicBlock::BlockId, DefUseExtractor::DefUseInfo>
      m_block_def_use;
  std::vector<AnalysisError> m_analysis_errors;

  // 분석 캐시
  mutable std::unordered_map<std::string, std::unordered_set<std::string>>
      m_interference_cache;
  mutable bool m_interference_cached = false;

  /**
   * @brief 각 블록의 정의와 사용 정보 추출
   */
  void extract_def_use_info(const ControlFlowGraph &cfg);

  /**
   * @brief 죽은 할당 검사
   */
  void check_dead_assignments_in_block(
      const BasicBlock &block, const DefUseExtractor::DefUseInfo &def_use_info,
      const VariableSet &live_out);

  /**
   * @brief 블록에서 사용되는 변수들 (USE 집합)
   */
  VariableSet
  compute_use_set(const DefUseExtractor::DefUseInfo &def_use_info) const;

  /**
   * @brief 블록에서 정의되는 변수들 (DEF 집합)
   */
  VariableSet
  compute_def_set(const DefUseExtractor::DefUseInfo &def_use_info) const;

  /**
   * @brief 변수 간섭 검사
   */
  bool variables_interfere(const std::string &var1,
                           const std::string &var2) const;
};

/**
 * @brief 변수 간섭 그래프 (Interference Graph)
 *
 * 레지스터 할당을 위한 변수 간 간섭 관계 모델링
 */
class InterferenceGraph {
public:
  InterferenceGraph() = default;

  /**
   * @brief 변수 추가
   */
  void add_variable(const std::string &variable);

  /**
   * @brief 간섭 관계 추가
   */
  void add_interference(const std::string &var1, const std::string &var2);

  /**
   * @brief 간섭 관계 확인
   */
  bool interferes(const std::string &var1, const std::string &var2) const;

  /**
   * @brief 변수의 차수 (인접한 변수 수)
   */
  size_t get_degree(const std::string &variable) const;

  /**
   * @brief 모든 변수 조회
   */
  std::vector<std::string> get_all_variables() const;

  /**
   * @brief 인접 변수들 조회
   */
  std::vector<std::string> get_neighbors(const std::string &variable) const;

  /**
   * @brief 그래프 색칠 (단순한 탐욕 알고리즘)
   */
  std::unordered_map<std::string, size_t> color_graph(size_t max_colors) const;

  /**
   * @brief 최소 색칠 수 추정
   */
  size_t estimate_chromatic_number() const;

  /**
   * @brief 그래프 통계
   */
  struct GraphStatistics {
    size_t vertex_count = 0;
    size_t edge_count = 0;
    size_t max_degree = 0;
    double average_degree = 0.0;
    size_t clique_size = 0; // 최대 클리크 크기 추정
  };

  GraphStatistics get_statistics() const;

  /**
   * @brief DOT 형식으로 출력 (시각화용)
   */
  std::string to_dot_format() const;

private:
  std::unordered_map<std::string, std::unordered_set<std::string>>
      m_adjacency_list;

  // 간단한 클리크 찾기 (완전하지 않음)
  size_t find_max_clique_size() const;
};

/**
 * @brief 레지스터 할당 지원 도구
 */
class RegisterAllocationHelper {
public:
  explicit RegisterAllocationHelper(const LiveVariablesAnalysis &lv_analysis);

  /**
   * @brief 레지스터 할당 제안
   */
  struct AllocationSuggestion {
    std::unordered_map<std::string, size_t> variable_to_register;
    size_t registers_needed;
    std::vector<std::string> spilled_variables; // 메모리로 이동해야 할 변수들
  };

  AllocationSuggestion suggest_allocation(size_t available_registers) const;

  /**
   * @brief 스필 비용이 높은 변수들 찾기
   */
  std::vector<std::string> find_high_spill_cost_variables() const;

  /**
   * @brief 레지스터 압력 분석
   */
  struct RegisterPressure {
    BasicBlock::BlockId block_id;
    size_t max_simultaneous_live;
    std::vector<std::string> pressure_points; // 압력이 높은 지점의 변수들
  };

  std::vector<RegisterPressure>
  analyze_register_pressure(const ControlFlowGraph &cfg) const;

private:
  const LiveVariablesAnalysis &m_lv_analysis;
  mutable std::unique_ptr<InterferenceGraph> m_interference_graph;

  void build_interference_graph() const;
  double calculate_spill_cost(const std::string &variable) const;
};

/**
 * @brief 변수 생존 구간 최적화
 */
class LifetimeOptimizer {
public:
  explicit LifetimeOptimizer(const LiveVariablesAnalysis &lv_analysis);

  /**
   * @brief 생존 구간 분할 기회 찾기
   */
  struct SplitOpportunity {
    std::string variable;
    BasicBlock::BlockId split_point;
    std::string reason;
    double benefit_score;
  };

  std::vector<SplitOpportunity> find_split_opportunities() const;

  /**
   * @brief 변수 병합 기회 찾기
   */
  struct MergeOpportunity {
    std::string variable1;
    std::string variable2;
    std::string merged_name;
    double benefit_score;
  };

  std::vector<MergeOpportunity> find_merge_opportunities() const;

  /**
   * @brief 생존 구간 단축 제안
   */
  std::vector<std::string> suggest_lifetime_reduction() const;

private:
  const LiveVariablesAnalysis &m_lv_analysis;

  bool can_variables_be_merged(const std::string &var1,
                               const std::string &var2) const;

  double calculate_merge_benefit(const std::string &var1,
                                 const std::string &var2) const;
};

} // namespace nugdev::compiler::analysis