#pragma once

#include "04_parsing/ast/expressions/Expressions.hpp"
#include "04_parsing/ast/statements/Statements.hpp"
#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include "05_analysis/optimization/OptimizationPass.hpp"
#include <memory>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::optimization {

/**
 * @brief 죽은 코드 제거 최적화 패스
 *
 * 실행되지 않는 코드를 감지하고 제거:
 * - 도달 불가능한 코드 제거
 * - 사용되지 않는 변수 제거
 * - 무조건 참/거짓인 조건문 최적화
 * - 빈 블록 제거
 */
class DeadCodeEliminator : public VisitorOptimizationPass {
public:
  DeadCodeEliminator() = default;

  std::string get_name() const override { return "DeadCodeEliminator"; }

  std::string get_description() const override {
    return "Removes unreachable and unused code";
  }

  // AST 방문자 메서드들
  void visit(ast::Module &node) override;
  void visit(ast::BlockExpression &node) override;
  void visit(ast::IfStatement &node) override;
  void visit(ast::ForStatement &node) override;
  void visit(ast::WhenExpression &node) override;
  void visit(ast::VariableDeclaration &node) override;

private:
  std::unordered_set<std::string> m_used_variables;
  std::unordered_set<ast::ASTNode *> m_reachable_nodes;

  /**
   * @brief 도달 가능성 분석 수행
   */
  void analyze_reachability(ast::ASTNode &root);

  /**
   * @brief 사용된 변수 추적
   */
  void track_variable_usage(ast::ASTNode &root);

  /**
   * @brief 조건문 분석 및 최적화
   */
  std::unique_ptr<ast::Statement>
  optimize_conditional(ast::IfStatement &if_stmt);

  /**
   * @brief 루프 분석 및 최적화
   */
  std::unique_ptr<ast::Statement> optimize_loop(ast::ForStatement &for_stmt);

  /**
   * @brief when 표현식 최적화
   */
  std::unique_ptr<ast::Expression>
  optimize_when_expression(ast::WhenExpression &when_expr);

  /**
   * @brief 블록에서 죽은 문장 제거
   */
  void remove_dead_statements(
      std::vector<std::unique_ptr<ast::Statement>> &statements);

  /**
   * @brief 표현식이 상수인지 확인
   */
  bool is_constant_expression(const ast::Expression &expr) const;

  /**
   * @brief 상수 표현식의 boolean 값 계산
   */
  std::optional<bool>
  evaluate_constant_boolean(const ast::Expression &expr) const;

  /**
   * @brief 변수가 사용되는지 확인
   */
  bool is_variable_used(const std::string &name) const;

  /**
   * @brief 노드가 도달 가능한지 확인
   */
  bool is_node_reachable(const ast::ASTNode &node) const;

  /**
   * @brief 빈 블록인지 확인
   */
  bool is_empty_block(const ast::BlockExpression &block) const;

  /**
   * @brief 무한 루프인지 확인
   */
  bool is_infinite_loop(const ast::ForStatement &for_stmt) const;

  /**
   * @brief 사용되지 않는 변수 수집
   */
  class UnusedVariableCollector : public ast::DefaultASTVisitor {
  public:
    explicit UnusedVariableCollector(
        std::unordered_set<std::string> &used_vars);

    void visit(ast::Identifier &node) override;
    void visit(ast::AssignmentExpression &node) override;

  private:
    std::unordered_set<std::string> &m_used_variables;
  };

  /**
   * @brief 도달 가능성 분석기
   */
  class ReachabilityAnalyzer : public ast::DefaultASTVisitor {
  public:
    explicit ReachabilityAnalyzer(
        std::unordered_set<ast::ASTNode *> &reachable);

    void visit(ast::IfStatement &node) override;
    void visit(ast::ForStatement &node) override;
    void visit(ast::ReturnStatement &node) override;
    void visit(ast::BreakStatement &node) override;
    void visit(ast::ContinueStatement &node) override;

  private:
    std::unordered_set<ast::ASTNode *> &m_reachable_nodes;
    bool m_in_unreachable_code = false;

    void mark_reachable(ast::ASTNode &node);
    void analyze_conditional_reachability(ast::IfStatement &if_stmt);
  };
};

/**
 * @brief 제어 흐름 기반 죽은 코드 제거
 *
 * ControlFlowGraph를 활용한 더 정교한 죽은 코드 분석
 */
class ControlFlowDeadCodeEliminator : public OptimizationPass {
public:
  ControlFlowDeadCodeEliminator() = default;

  std::string get_name() const override {
    return "ControlFlowDeadCodeEliminator";
  }

  std::string get_description() const override {
    return "Removes dead code using control flow analysis";
  }

  bool run(ast::ASTNode &node) override;

private:
  std::unique_ptr<nugdev::compiler::analysis::ControlFlowGraph> m_cfg;

  /**
   * @brief CFG 기반 도달 가능성 분석
   */
  void analyze_reachability_with_cfg(ast::ASTNode &root);

  /**
   * @brief 도달 불가능한 기본 블록들 제거
   */
  bool remove_unreachable_basic_blocks();

  /**
   * @brief CFG에서 AST 노드 매핑
   */
  void map_ast_nodes_to_blocks();
};

} // namespace nugdev::compiler::optimization