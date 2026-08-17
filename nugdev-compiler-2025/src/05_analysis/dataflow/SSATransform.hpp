#pragma once

#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include "05_analysis/dataflow/DataFlowAnalysis.hpp"
#include "05_analysis/optimization/ConstantFolder.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::analysis {

using ConstantValue = optimization::ConstantValue;

/**
 * @brief SSA (Static Single Assignment) 형태 변환
 *
 * 각 변수가 정확히 한 번만 할당되는 형태로 변환:
 * - φ-함수 삽입
 * - 변수 이름 변경
 * - 지배 경계(Dominance Frontier) 계산
 */
class SSATransformer {
public:
  // φ-함수 관리 (public으로 이동)
  struct PhiFunction {
    std::string target_variable;
    std::vector<std::pair<std::string, BasicBlock *>>
        operands; // (변수명, 선행 블록)

    PhiFunction(const std::string &target) : target_variable(target) {}
  };

  explicit SSATransformer();

  /**
   * @brief CFG를 SSA 형태로 변환
   */
  std::unique_ptr<ControlFlowGraph>
  transform_to_ssa(const ControlFlowGraph &cfg);

  /**
   * @brief SSA에서 일반 형태로 역변환
   */
  std::unique_ptr<ControlFlowGraph>
  transform_from_ssa(const ControlFlowGraph &ssa_cfg);

private:
  std::unordered_map<BasicBlock *, std::vector<PhiFunction>> m_phi_functions;

  /**
   * @brief 지배 경계 계산
   */
  void compute_dominance_frontiers(const ControlFlowGraph &cfg);
  std::unordered_map<BasicBlock *, std::unordered_set<BasicBlock *>>
      m_dominance_frontiers;

  /**
   * @brief φ-함수 삽입 위치 결정
   */
  void place_phi_functions(const ControlFlowGraph &cfg);

  /**
   * @brief 변수 이름 변경 (Variable Renaming)
   */
  void rename_variables(ControlFlowGraph &cfg);

  /**
   * @brief 지배 트리를 이용한 재귀적 이름 변경
   */
  void rename_variables_recursive(
      BasicBlock *block,
      std::unordered_map<std::string, std::vector<std::string>> &rename_stacks);

  /**
   * @brief 새로운 변수 이름 생성
   */
  std::string generate_new_variable_name(const std::string &base_name);
  std::unordered_map<std::string, size_t> m_variable_counters;

  /**
   * @brief 변수 정의 수집
   */
  std::unordered_set<std::string>
  collect_defined_variables(const ControlFlowGraph &cfg);

  /**
   * @brief 각 변수의 정의 블록들 수집
   */
  std::unordered_map<std::string, std::unordered_set<BasicBlock *>>
  collect_definition_blocks(const ControlFlowGraph &cfg);
};

/**
 * @brief SSA 기반 최적화들
 */
class SSAOptimizer {
public:
  explicit SSAOptimizer(const ControlFlowGraph &ssa_cfg);

  /**
   * @brief 복사 전파 (Copy Propagation)
   */
  bool run_copy_propagation();

  /**
   * @brief 상수 전파 (Constant Propagation)
   */
  bool run_constant_propagation();

  /**
   * @brief 죽은 코드 제거 (Dead Code Elimination) - SSA 기반
   */
  bool run_dead_code_elimination();

  /**
   * @brief Global Value Numbering
   */
  bool run_global_value_numbering();

  /**
   * @brief Sparse Conditional Constant Propagation
   */
  bool run_sparse_conditional_constant_propagation();

private:
  const ControlFlowGraph &m_ssa_cfg;

  /**
   * @brief 복사 명령 감지 (x = y 형태)
   */
  bool is_copy_instruction(const ast::ASTNode &instruction, std::string &target,
                           std::string &source);

  /**
   * @brief 상수 할당 감지 (x = 5 형태)
   */
  bool is_constant_assignment(const ast::ASTNode &instruction,
                              std::string &target, ConstantValue &value);

  /**
   * @brief 변수 사용 지점 교체
   */
  void replace_variable_uses(const std::string &old_var,
                             const std::string &new_var);

  /**
   * @brief Value Numbering을 위한 표현식 해시
   */
  size_t compute_value_number(const ast::Expression &expr);
  std::unordered_map<size_t, std::string> m_value_to_variable;
};

/**
 * @brief Sparse Conditional Constant Propagation 구현
 */
class SCCPAnalysis {
public:
  enum class LatticeValue {
    TOP,      // 초기 상태 (아직 모름)
    CONSTANT, // 상수 값
    BOTTOM    // 여러 값 가능 (비상수)
  };

  struct LatticeElement {
    LatticeValue state = LatticeValue::TOP;
    ConstantValue constant_value;

    bool is_constant() const { return state == LatticeValue::CONSTANT; }
    bool is_bottom() const { return state == LatticeValue::BOTTOM; }
  };

  explicit SCCPAnalysis(const ControlFlowGraph &ssa_cfg);

  /**
   * @brief SCCP 분석 실행
   */
  void analyze();

  /**
   * @brief 변수의 상수 값 조회
   */
  std::optional<ConstantValue>
  get_constant_value(const std::string &variable) const;

  /**
   * @brief 도달 가능한 블록들 조회
   */
  std::unordered_set<BasicBlock *> get_reachable_blocks() const;

private:
  const ControlFlowGraph &m_ssa_cfg;
  std::unordered_map<std::string, LatticeElement> m_variable_lattice;
  std::unordered_set<BasicBlock *> m_reachable_blocks;

  // 워크리스트 알고리즘
  std::vector<BasicBlock *> m_block_worklist;
  std::vector<std::pair<BasicBlock *, BasicBlock *>> m_edge_worklist;

  /**
   * @brief Lattice 값 병합
   */
  LatticeElement meet(const LatticeElement &a, const LatticeElement &b);

  /**
   * @brief 명령어 해석
   */
  void interpret_instruction(const ast::ASTNode &instruction);

  /**
   * @brief 조건문 해석
   */
  void interpret_conditional(const ast::Expression &condition,
                             BasicBlock *true_block, BasicBlock *false_block);
};

/**
 * @brief φ-함수 제거를 위한 병렬 복사 해결
 */
class ParallelCopyResolver {
public:
  struct CopyInstruction {
    std::string target;
    std::string source;
  };

  /**
   * @brief φ-함수를 병렬 복사로 변환
   */
  static std::vector<CopyInstruction>
  resolve_phi_function(const SSATransformer::PhiFunction &phi,
                       BasicBlock *predecessor);

  /**
   * @brief 병렬 복사를 순차 복사로 변환
   */
  static std::vector<CopyInstruction>
  resolve_parallel_copies(const std::vector<CopyInstruction> &parallel_copies);

private:
  /**
   * @brief 복사 간섭 그래프 구축
   */
  static std::unordered_map<std::string, std::unordered_set<std::string>>
  build_interference_graph(const std::vector<CopyInstruction> &copies);

  /**
   * @brief 순환 의존성 해결
   */
  static void break_cycles(std::vector<CopyInstruction> &copies);
};

} // namespace nugdev::compiler::analysis