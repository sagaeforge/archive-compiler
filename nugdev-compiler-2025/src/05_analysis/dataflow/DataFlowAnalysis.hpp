#pragma once

#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 데이터 흐름 분석의 기본 인터페이스
 *
 * Kildall의 워크리스트 알고리즘을 기반으로 한
 * 일반적인 데이터 흐름 분석 프레임워크
 */
template <typename DataFlowSet> class DataFlowAnalysis {
public:
  enum class Direction {
    FORWARD, // 전진 분석 (예: reaching definitions)
    BACKWARD // 후진 분석 (예: live variables)
  };

  explicit DataFlowAnalysis(Direction dir) : m_direction(dir) {}
  virtual ~DataFlowAnalysis() = default;

  /**
   * @brief 데이터 흐름 분석 실행
   */
  void analyze(const ControlFlowGraph &cfg);

  /**
   * @brief 기본 블록의 입력 집합 가져오기
   */
  const DataFlowSet &get_in_set(BasicBlock::BlockId block_id) const;

  /**
   * @brief 기본 블록의 출력 집합 가져오기
   */
  const DataFlowSet &get_out_set(BasicBlock::BlockId block_id) const;

protected:
  /**
   * @brief 전이 함수 (transfer function)
   * 블록의 입력을 출력으로 변환
   */
  virtual DataFlowSet transfer(const BasicBlock &block,
                               const DataFlowSet &in_set) = 0;

  /**
   * @brief 합집합 연산 (meet operation)
   * 여러 입력을 하나로 결합
   */
  virtual DataFlowSet meet(const std::vector<DataFlowSet> &sets) = 0;

  /**
   * @brief 초기값 생성
   */
  virtual DataFlowSet get_initial_set() = 0;

  /**
   * @brief 경계 조건 (boundary condition)
   */
  virtual DataFlowSet get_boundary_set() = 0;

private:
  Direction m_direction;
  std::unordered_map<BasicBlock::BlockId, DataFlowSet> m_in_sets;
  std::unordered_map<BasicBlock::BlockId, DataFlowSet> m_out_sets;

  void initialize_sets(const ControlFlowGraph &cfg);
  void worklist_algorithm(const ControlFlowGraph &cfg);
  bool has_changed(const DataFlowSet &old_set,
                   const DataFlowSet &new_set) const;
};

/**
 * @brief 집합 기반 데이터 흐름 정보
 */
template <typename Element> class SetBasedDataFlowSet {
public:
  SetBasedDataFlowSet() = default;
  SetBasedDataFlowSet(const std::unordered_set<Element> &elements)
      : m_elements(elements) {}

  // 집합 연산
  void add(const Element &element) { m_elements.insert(element); }
  void remove(const Element &element) { m_elements.erase(element); }
  bool contains(const Element &element) const {
    return m_elements.find(element) != m_elements.end();
  }

  // 집합 연산자
  SetBasedDataFlowSet
  operator|(const SetBasedDataFlowSet &other) const; // 합집합
  SetBasedDataFlowSet
  operator&(const SetBasedDataFlowSet &other) const; // 교집합
  SetBasedDataFlowSet
  operator-(const SetBasedDataFlowSet &other) const; // 차집합

  bool operator==(const SetBasedDataFlowSet &other) const;
  bool operator!=(const SetBasedDataFlowSet &other) const;

  // 접근자
  const std::unordered_set<Element> &elements() const { return m_elements; }
  size_t size() const { return m_elements.size(); }
  bool empty() const { return m_elements.empty(); }

  // 반복자
  auto begin() const { return m_elements.begin(); }
  auto end() const { return m_elements.end(); }

private:
  std::unordered_set<Element> m_elements;
};

/**
 * @brief 변수 정의 정보
 */
struct VariableDefinition {
  std::string variable_name;
  BasicBlock::BlockId defining_block;
  size_t statement_index; // 블록 내에서의 위치

  bool operator==(const VariableDefinition &other) const {
    return variable_name == other.variable_name &&
           defining_block == other.defining_block &&
           statement_index == other.statement_index;
  }

  struct Hash {
    size_t operator()(const VariableDefinition &def) const {
      return std::hash<std::string>{}(def.variable_name) ^
             std::hash<BasicBlock::BlockId>{}(def.defining_block) ^
             std::hash<size_t>{}(def.statement_index);
    }
  };
};

/**
 * @brief 변수 사용 정보
 */
struct VariableUse {
  std::string variable_name;
  BasicBlock::BlockId using_block;
  size_t statement_index;

  bool operator==(const VariableUse &other) const {
    return variable_name == other.variable_name &&
           using_block == other.using_block &&
           statement_index == other.statement_index;
  }

  struct Hash {
    size_t operator()(const VariableUse &use) const {
      return std::hash<std::string>{}(use.variable_name) ^
             std::hash<BasicBlock::BlockId>{}(use.using_block) ^
             std::hash<size_t>{}(use.statement_index);
    }
  };
};

// 특수화된 데이터 흐름 집합 타입들
using DefinitionSet = SetBasedDataFlowSet<VariableDefinition>;
using UseSet = SetBasedDataFlowSet<VariableUse>;
using VariableSet = SetBasedDataFlowSet<std::string>;

/**
 * @brief 데이터 흐름 분석 결과를 담는 컨테이너
 */
class DataFlowAnalysisResult {
public:
  DataFlowAnalysisResult() = default;

  // 결과 저장
  void set_reaching_definitions(BasicBlock::BlockId block_id,
                                const DefinitionSet &definitions);
  void set_live_variables(BasicBlock::BlockId block_id,
                          const VariableSet &variables);

  // 결과 조회
  const DefinitionSet &
  get_reaching_definitions(BasicBlock::BlockId block_id) const;
  const VariableSet &get_live_variables(BasicBlock::BlockId block_id) const;

  // 분석 질의
  std::vector<VariableDefinition>
  get_definitions_for_variable(const std::string &variable,
                               BasicBlock::BlockId block_id) const;

  bool is_variable_live_at_exit(const std::string &variable,
                                BasicBlock::BlockId block_id) const;

  std::vector<std::string>
  get_uninitialized_variables(BasicBlock::BlockId block_id) const;

private:
  std::unordered_map<BasicBlock::BlockId, DefinitionSet> m_reaching_definitions;
  std::unordered_map<BasicBlock::BlockId, VariableSet> m_live_variables;
};

/**
 * @brief AST에서 정의(def)와 사용(use) 추출
 */
class DefUseExtractor : public ast::DefaultASTVisitor {
public:
  struct DefUseInfo {
    std::vector<VariableDefinition> definitions;
    std::vector<VariableUse> uses;
  };

  DefUseInfo extract_def_use(const BasicBlock &block);

  void visit(ast::VariableDeclaration &node) override;
  void visit(ast::AssignmentExpression &node) override;
  void visit(ast::Identifier &node) override;
  void visit(ast::PostfixExpression &node) override; // for ++, --

private:
  DefUseInfo m_current_info;
  BasicBlock::BlockId m_current_block_id = 0;
  size_t m_current_statement_index = 0;

  void add_definition(const std::string &variable);
  void add_use(const std::string &variable);
  bool is_assignment_target(const ast::Identifier &identifier) const;
};

} // namespace nugdev::compiler::analysis

// 해시 특수화
namespace std {
template <> struct hash<nugdev::compiler::analysis::VariableDefinition> {
  size_t
  operator()(const nugdev::compiler::analysis::VariableDefinition &def) const {
    return nugdev::compiler::analysis::VariableDefinition::Hash{}(def);
  }
};

template <> struct hash<nugdev::compiler::analysis::VariableUse> {
  size_t operator()(const nugdev::compiler::analysis::VariableUse &use) const {
    return nugdev::compiler::analysis::VariableUse::Hash{}(use);
  }
};
} // namespace std