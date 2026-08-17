#pragma once

#include "05_analysis/dataflow/DataFlowAnalysis.hpp"
#include "05_analysis/errors/AnalysisError.hpp"
#include <memory>
#include <unordered_map>

namespace nugdev::compiler::analysis {

/**
 * @brief 도달 정의 분석 (Reaching Definitions Analysis)
 *
 * 각 프로그램 지점에서 어떤 변수 정의가 도달할 수 있는지 분석:
 * - 변수 초기화 검사
 * - 사용 전 정의 검증
 * - 정의-사용 체인 구축
 */
class ReachingDefinitionsAnalysis : public DataFlowAnalysis<DefinitionSet> {
public:
  ReachingDefinitionsAnalysis();

  /**
   * @brief 도달 정의 분석 실행
   */
  std::vector<AnalysisError>
  analyze_reaching_definitions(const ControlFlowGraph &cfg);

  /**
   * @brief 특정 지점에서 변수에 대한 도달 정의들 조회
   */
  std::vector<VariableDefinition>
  get_reaching_definitions_for_variable(const std::string &variable,
                                        BasicBlock::BlockId block_id) const;

  /**
   * @brief 초기화되지 않은 변수 사용 검사
   */
  std::vector<AnalysisError> check_uninitialized_variables() const;

  /**
   * @brief 정의-사용 체인 구축
   */
  std::unordered_map<VariableDefinition, std::vector<VariableUse>,
                     VariableDefinition::Hash>
  build_def_use_chains() const;

protected:
  // DataFlowAnalysis 인터페이스 구현
  DefinitionSet transfer(const BasicBlock &block,
                         const DefinitionSet &in_set) override;
  DefinitionSet meet(const std::vector<DefinitionSet> &sets) override;
  DefinitionSet get_initial_set() override;
  DefinitionSet get_boundary_set() override;

private:
  std::unique_ptr<DefUseExtractor> m_def_use_extractor;
  std::unordered_map<BasicBlock::BlockId, DefUseExtractor::DefUseInfo>
      m_block_def_use;
  std::vector<AnalysisError> m_analysis_errors;

  /**
   * @brief 각 블록의 정의와 사용 정보 추출
   */
  void extract_def_use_info(const ControlFlowGraph &cfg);

  /**
   * @brief 변수가 정의되기 전에 사용되는지 검사
   */
  void
  check_use_before_definition(const BasicBlock &block,
                              const DefUseExtractor::DefUseInfo &def_use_info,
                              const DefinitionSet &reaching_defs);

  /**
   * @brief 정의가 특정 변수를 kills하는지 확인
   */
  DefinitionSet kill_definitions(const DefinitionSet &definitions,
                                 const std::string &variable) const;

  /**
   * @brief 새로운 정의들을 생성
   */
  DefinitionSet
  generate_definitions(const std::vector<VariableDefinition> &new_defs) const;
};

/**
 * @brief 정의-사용 체인 (Def-Use Chain) 관리
 */
class DefUseChain {
public:
  DefUseChain() = default;

  /**
   * @brief 정의에 사용 추가
   */
  void add_use_to_definition(const VariableDefinition &def,
                             const VariableUse &use);

  /**
   * @brief 사용에 정의 추가
   */
  void add_definition_to_use(const VariableUse &use,
                             const VariableDefinition &def);

  /**
   * @brief 정의에 대한 모든 사용 조회
   */
  std::vector<VariableUse>
  get_uses_for_definition(const VariableDefinition &def) const;

  /**
   * @brief 사용에 대한 모든 정의 조회
   */
  std::vector<VariableDefinition>
  get_definitions_for_use(const VariableUse &use) const;

  /**
   * @brief 사용되지 않는 정의들 찾기
   */
  std::vector<VariableDefinition> find_unused_definitions() const;

  /**
   * @brief 정의 없는 사용들 찾기 (초기화되지 않은 변수)
   */
  std::vector<VariableUse> find_undefined_uses() const;

  /**
   * @brief 체인 통계
   */
  struct Statistics {
    size_t total_definitions = 0;
    size_t total_uses = 0;
    size_t unused_definitions = 0;
    size_t undefined_uses = 0;
  };

  Statistics get_statistics() const;

private:
  // 정의 -> 사용들
  std::unordered_map<VariableDefinition, std::vector<VariableUse>,
                     VariableDefinition::Hash>
      m_def_to_uses;

  // 사용 -> 정의들
  std::unordered_map<VariableUse, std::vector<VariableDefinition>,
                     VariableUse::Hash>
      m_use_to_defs;
};

/**
 * @brief 변수 생존 범위 (Variable Lifetime) 분석
 */
class VariableLifetimeAnalyzer {
public:
  struct LifetimeInfo {
    VariableDefinition first_definition;
    VariableUse last_use;
    std::vector<BasicBlock::BlockId> live_blocks;
    bool is_live_at_entry = false;
    bool is_live_at_exit = false;
  };

  explicit VariableLifetimeAnalyzer(const DefUseChain &def_use_chain);

  /**
   * @brief 변수의 생존 범위 분석
   */
  std::unordered_map<std::string, LifetimeInfo>
  analyze_variable_lifetimes(const ControlFlowGraph &cfg);

  /**
   * @brief 짧은 생존 범위를 가진 변수들 찾기 (최적화 후보)
   */
  std::vector<std::string> find_short_lived_variables() const;

  /**
   * @brief 긴 생존 범위를 가진 변수들 찾기 (메모리 사용량 주의)
   */
  std::vector<std::string> find_long_lived_variables() const;

private:
  const DefUseChain &m_def_use_chain;
  std::unordered_map<std::string, LifetimeInfo> m_lifetime_info;

  void compute_live_blocks_for_variable(const std::string &variable,
                                        const ControlFlowGraph &cfg,
                                        LifetimeInfo &lifetime);
};

/**
 * @brief 초기화 검사기
 */
class InitializationChecker {
public:
  explicit InitializationChecker(
      const ReachingDefinitionsAnalysis &rd_analysis);

  /**
   * @brief 초기화 검사 수행
   */
  std::vector<AnalysisError> check_initialization(const ControlFlowGraph &cfg);

private:
  const ReachingDefinitionsAnalysis &m_rd_analysis;

  /**
   * @brief 변수가 모든 경로에서 초기화되었는지 확인
   */
  bool is_definitely_initialized(const std::string &variable,
                                 BasicBlock::BlockId block_id) const;

  /**
   * @brief 조건부 초기화 경고
   */
  std::vector<AnalysisError>
  check_conditional_initialization(const ControlFlowGraph &cfg) const;
};

} // namespace nugdev::compiler::analysis