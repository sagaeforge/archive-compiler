#pragma once

#include "04_parsing/ast/control_flow/ControlFlow.hpp"
#include "04_parsing/ast/core/ASTVisitor.h"
#include "05_analysis/control_flow/ControlFlowGraph.hpp"
#include "05_analysis/errors/AnalysisError.hpp"
#include <memory>
#include <stack>
#include <unordered_map>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 제어 흐름 검증을 담당하는 클래스
 *
 * 제어 흐름 구조의 유효성을 검사:
 * - break/continue 문의 적절한 사용
 * - return 문의 타입 일치
 * - 레이블된 제어 흐름 검증
 * - 무한 루프 감지
 * - 도달 불가능한 코드 감지
 */
class ControlFlowAnalyzer : public ast::DefaultASTVisitor {
public:
  /**
   * @brief 제어 흐름 분석 옵션
   */
  struct AnalysisOptions {
    bool check_unreachable_code;
    bool warn_on_infinite_loops;
    bool strict_return_checking;
    bool validate_labels;

    AnalysisOptions()
        : check_unreachable_code(true), warn_on_infinite_loops(true),
          strict_return_checking(true), validate_labels(true) {}
  };

  explicit ControlFlowAnalyzer(
      const AnalysisOptions &options = AnalysisOptions{});

  /**
   * @brief 제어 흐름 분석 수행
   */
  std::vector<AnalysisError> analyze_control_flow(ast::ASTNode &root);

  // AST 방문자 메서드들
  void visit(ast::FunctionDeclaration &node) override;
  void visit(ast::IfStatement &node) override;
  void visit(ast::ForStatement &node) override;
  void visit(ast::BreakStatement &node) override;
  void visit(ast::ContinueStatement &node) override;
  void visit(ast::ReturnStatement &node) override;
  void visit(ast::BlockExpression &node) override;
  void visit(ast::WhenExpression &node) override;

private:
  AnalysisOptions m_options;
  std::vector<AnalysisError> m_errors;

  // 제어 흐름 컨텍스트
  struct ControlContext {
    enum Type { FUNCTION, LOOP, CONDITIONAL, BLOCK };

    Type type;
    std::string label;
    ast::TypeLiteral *return_type; // 함수 컨텍스트에서만 사용
    bool has_return_path;
    bool is_infinite_loop;

    ControlContext(Type t, const std::string &l = "")
        : type(t), label(l), return_type(nullptr), has_return_path(false),
          is_infinite_loop(false) {}
  };

  std::stack<ControlContext> m_context_stack;

  // 레이블 관리
  std::unordered_map<std::string, ControlContext *> m_label_map;

  // CFG 분석 결과
  std::unique_ptr<ControlFlowGraph> m_cfg;

  // 컨텍스트 관리
  void push_context(ControlContext::Type type, const std::string &label = "");
  void pop_context();
  ControlContext *get_current_context();
  ControlContext *find_context_by_type(ControlContext::Type type);
  ControlContext *find_context_by_label(const std::string &label);

  // 제어 흐름 검증
  void validate_break_statement(const ast::BreakStatement &stmt);
  void validate_continue_statement(const ast::ContinueStatement &stmt);
  void validate_return_statement(const ast::ReturnStatement &stmt);
  void validate_labeled_statement(const std::string &label,
                                  const ast::ASTNode &node);

  // 경로 분석
  void analyze_return_paths(ast::FunctionDeclaration &func);
  bool all_paths_return(const ast::BlockExpression &block);
  bool statement_always_returns(const ast::Statement &stmt);

  // 무한 루프 검사
  bool is_infinite_loop(const ast::ForStatement &for_stmt);
  bool has_break_in_loop(const ast::ASTNode &loop_body);

  // 도달 가능성 분석
  void check_unreachable_code(
      const std::vector<std::unique_ptr<ast::Statement>> &statements);
  bool statement_terminates_execution(const ast::Statement &stmt);

  // CFG 기반 분석
  void build_and_analyze_cfg(ast::ASTNode &root);
  void analyze_dominators();
  void detect_natural_loops();

  // 에러 보고
  void report_break_outside_loop(const ast::BreakStatement &stmt);
  void report_continue_outside_loop(const ast::ContinueStatement &stmt);
  void report_invalid_label(const std::string &label, const ast::ASTNode &node);
  void report_missing_return(const ast::FunctionDeclaration &func);
  void report_unreachable_code(const ast::Statement &stmt);
  void report_infinite_loop_warning(const ast::ForStatement &loop);
  void report_return_type_mismatch(const ast::ReturnStatement &stmt,
                                   const ast::TypeLiteral &expected,
                                   const ast::TypeLiteral &actual);
};

/**
 * @brief 제어 흐름 그래프 분석 유틸리티
 */
class CFGAnalysisHelper {
public:
  static std::vector<AnalysisError> validate_cfg(const ControlFlowGraph &cfg);

  /**
   * @brief 도달 불가능한 기본 블록들 찾기
   */
  static std::vector<BasicBlock *>
  find_unreachable_blocks(const ControlFlowGraph &cfg);

  /**
   * @brief 무한 루프 감지
   */
  static std::vector<std::vector<BasicBlock *>>
  find_infinite_loops(const ControlFlowGraph &cfg);

  /**
   * @brief 모든 경로가 return하는지 확인
   */
  static bool all_paths_have_return(const ControlFlowGraph &cfg);

  /**
   * @brief 제어 흐름 복잡도 계산 (McCabe's Cyclomatic Complexity)
   */
  static size_t calculate_cyclomatic_complexity(const ControlFlowGraph &cfg);

private:
  static void dfs_find_unreachable(BasicBlock *block,
                                   std::unordered_set<BasicBlock *> &visited,
                                   std::vector<BasicBlock *> &unreachable);

  static bool has_path_to_exit(BasicBlock *block, BasicBlock *exit_block,
                               std::unordered_set<BasicBlock *> &visited);
};

/**
 * @brief 레이블 검증을 담당하는 헬퍼 클래스
 */
class LabelValidator {
public:
  std::vector<AnalysisError> validate_labels(ast::ASTNode &root);

private:
  std::unordered_map<std::string, ast::ASTNode *> m_label_definitions;
  std::vector<std::pair<std::string, ast::ASTNode *>> m_label_references;
  std::vector<AnalysisError> m_errors;

  void collect_label_definitions(ast::ASTNode &node);
  void collect_label_references(ast::ASTNode &node);
  void validate_label_references();

  bool is_valid_label_target(const std::string &label,
                             const ast::ASTNode &reference);
};

} // namespace nugdev::compiler::analysis