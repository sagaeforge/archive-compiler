#pragma once

#include "04_parsing/ast/control_flow/ControlFlow.hpp"
#include "04_parsing/ast/core/ASTNode.hpp"
#include "04_parsing/ast/expressions/ComplexExpressions.hpp"
#include "04_parsing/ast/expressions/Expressions.hpp"
#include "04_parsing/ast/statements/Statements.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::analysis {

// Forward declarations
class BasicBlock;
class ControlFlowGraph;

/**
 * @brief 기본 블록 - 제어 흐름이 분기되지 않는 연속된 명령문들
 */
class BasicBlock {
public:
  using BlockId = size_t;

  explicit BasicBlock(BlockId id);

  // 기본 블록 식별자
  BlockId get_id() const { return m_id; }

  // AST 노드 관리
  void add_statement(ast::ASTNode *statement);
  void add_expression(ast::Expression *expression);
  const std::vector<ast::ASTNode *> &get_statements() const {
    return m_statements;
  }

  // 후속자(successor) 관리
  void add_successor(BasicBlock *successor);
  void remove_successor(BasicBlock *successor);
  const std::vector<BasicBlock *> &get_successors() const {
    return m_successors;
  }

  // 선행자(predecessor) 관리
  void add_predecessor(BasicBlock *predecessor);
  void remove_predecessor(BasicBlock *predecessor);
  const std::vector<BasicBlock *> &get_predecessors() const {
    return m_predecessors;
  }

  // 블록 속성
  bool is_entry_block() const { return m_predecessors.empty(); }
  bool is_exit_block() const { return m_successors.empty(); }
  bool is_unreachable() const { return m_is_unreachable; }
  void mark_unreachable() { m_is_unreachable = true; }

  // 지배(dominance) 정보
  void set_dominator(BasicBlock *dominator) { m_dominator = dominator; }
  BasicBlock *get_dominator() const { return m_dominator; }
  void add_dominated_block(BasicBlock *dominated) {
    m_dominated_blocks.push_back(dominated);
  }
  const std::vector<BasicBlock *> &get_dominated_blocks() const {
    return m_dominated_blocks;
  }

  // 루프 정보
  bool is_loop_header() const { return m_is_loop_header; }
  void mark_as_loop_header() { m_is_loop_header = true; }
  void set_loop_depth(size_t depth) { m_loop_depth = depth; }
  size_t get_loop_depth() const { return m_loop_depth; }

  // 디버깅 및 출력
  std::string to_string() const;

private:
  BlockId m_id;
  std::vector<ast::ASTNode *> m_statements;

  // 제어 흐름 그래프 연결
  std::vector<BasicBlock *> m_successors;
  std::vector<BasicBlock *> m_predecessors;

  // 분석 결과
  bool m_is_unreachable = false;
  BasicBlock *m_dominator = nullptr;
  std::vector<BasicBlock *> m_dominated_blocks;

  // 루프 정보
  bool m_is_loop_header = false;
  size_t m_loop_depth = 0;
};

/**
 * @brief 제어 흐름 그래프
 */
class ControlFlowGraph {
public:
  ControlFlowGraph();
  ~ControlFlowGraph();

  // 기본 블록 생성 및 관리
  BasicBlock *create_block();
  BasicBlock *get_entry_block() const { return m_entry_block; }
  BasicBlock *get_exit_block() const { return m_exit_block; }

  void set_entry_block(BasicBlock *block) { m_entry_block = block; }
  void set_exit_block(BasicBlock *block) { m_exit_block = block; }

  const std::vector<std::unique_ptr<BasicBlock>> &get_blocks() const {
    return m_blocks;
  }
  size_t get_block_count() const { return m_blocks.size(); }

  // 그래프 연결
  void add_edge(BasicBlock *from, BasicBlock *to);
  void remove_edge(BasicBlock *from, BasicBlock *to);

  // 분석 알고리즘들
  void compute_dominators();
  void compute_post_dominators();
  void detect_loops();
  void mark_unreachable_blocks();

  // 그래프 분석 결과
  std::vector<BasicBlock *> get_unreachable_blocks() const;
  std::vector<BasicBlock *> get_loop_headers() const;
  bool has_unreachable_code() const;

  // 그래프 변환
  void remove_unreachable_blocks();
  void merge_blocks(); // 단일 후속자를 가진 블록들 병합

  // 디버깅 및 시각화
  std::string to_dot_format() const; // Graphviz DOT 형식 출력
  void print_statistics() const;

private:
  std::vector<std::unique_ptr<BasicBlock>> m_blocks;
  BasicBlock *m_entry_block = nullptr;
  BasicBlock *m_exit_block = nullptr;
  BasicBlock::BlockId m_next_block_id = 0;

  // 분석 상태
  bool m_dominators_computed = false;
  bool m_loops_detected = false;

  // 지배자 트리 계산 (Lengauer-Tarjan 알고리즘)
  void compute_dominators_lengauer_tarjan();

  // 루프 감지 (Tarjan의 strongly connected components)
  void detect_natural_loops();
  void find_back_edges(
      std::vector<std::pair<BasicBlock *, BasicBlock *>> &back_edges);

  // DFS 유틸리티
  void depth_first_search(BasicBlock *start,
                          std::unordered_set<BasicBlock *> &visited,
                          std::vector<BasicBlock *> &post_order);
};

/**
 * @brief CFG 구축기 - AST로부터 제어 흐름 그래프를 생성
 */
class ControlFlowGraphBuilder {
public:
  explicit ControlFlowGraphBuilder();

  /**
   * @brief AST로부터 CFG를 구축
   */
  std::unique_ptr<ControlFlowGraph> build_cfg(ast::ASTNode &root);

private:
  std::unique_ptr<ControlFlowGraph> m_cfg;
  BasicBlock *m_current_block = nullptr;

  // 제어 흐름 컨텍스트
  struct ControlContext {
    BasicBlock *break_target = nullptr;
    BasicBlock *continue_target = nullptr;
  };
  std::vector<ControlContext> m_control_stack;

  // AST 노드별 CFG 구축
  void visit_statement(ast::Statement &stmt);
  void visit_if_statement(ast::IfStatement &if_stmt);
  void visit_for_statement(ast::ForStatement &for_stmt);
  void visit_break_statement(ast::BreakStatement &break_stmt);
  void visit_continue_statement(ast::ContinueStatement &continue_stmt);
  void visit_return_statement(ast::ReturnStatement &return_stmt);
  void visit_block_expression(ast::BlockExpression &block);

  // 블록 관리 헬퍼
  BasicBlock *create_new_block();
  void set_current_block(BasicBlock *block);
  void add_statement_to_current_block(ast::ASTNode *stmt);

  // 제어 흐름 헬퍼
  void push_control_context(BasicBlock *break_target,
                            BasicBlock *continue_target);
  void pop_control_context();
  ControlContext &get_current_context();
};

} // namespace nugdev::compiler::analysis