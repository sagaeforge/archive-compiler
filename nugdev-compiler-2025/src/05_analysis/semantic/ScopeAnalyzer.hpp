#pragma once

#include "04_parsing/ast/core/ASTVisitor.h"
#include "05_analysis/errors/AnalysisError.hpp"
#include "05_analysis/semantic/SymbolTable.hpp"
#include <memory>
#include <stack>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 스코프 분석을 담당하는 클래스
 *
 * 변수의 선언과 사용에 대한 스코프 규칙을 검증:
 * - 변수 재선언 검사
 * - 변수 사용 전 선언 검사
 * - 스코프 규칙 준수 검사
 * - 변수 섀도잉 경고
 */
class ScopeAnalyzer : public ast::DefaultASTVisitor {
public:
  /**
   * @brief 스코프 분석 옵션
   */
  struct ScopeOptions {
    bool warn_on_shadowing;
    bool strict_scoping;
    bool allow_forward_declarations;

    ScopeOptions()
        : warn_on_shadowing(true), strict_scoping(true),
          allow_forward_declarations(false) {}
  };

  explicit ScopeAnalyzer(SymbolTable &symbol_table,
                         const ScopeOptions &options = ScopeOptions{});

  /**
   * @brief 스코프 분석 수행
   */
  std::vector<AnalysisError> analyze_scopes(ast::ASTNode &root);

  // AST 방문자 메서드들
  void visit(ast::Module &node) override;
  void visit(ast::VariableDeclaration &node) override;
  void visit(ast::FunctionDeclaration &node) override;
  void visit(ast::StructDeclaration &node) override;
  void visit(ast::InterfaceDeclaration &node) override;
  void visit(ast::Identifier &node) override;
  void visit(ast::BlockExpression &node) override;
  void visit(ast::IfStatement &node) override;
  void visit(ast::ForStatement &node) override;
  void visit(ast::WhenExpression &node) override;

private:
  SymbolTable &m_symbol_table;
  ScopeOptions m_options;
  std::vector<AnalysisError> m_errors;

  // 스코프 컨텍스트 관리
  struct ScopeContext {
    Scope::ScopeType type;
    std::unordered_set<std::string> declared_in_scope;
    size_t depth;
  };
  std::stack<ScopeContext> m_scope_stack;

  // 스코프 관리 헬퍼
  void enter_scope(Scope::ScopeType type);
  void exit_scope();
  void declare_symbol_in_current_scope(const std::string &name);
  bool is_declared_in_current_scope(const std::string &name) const;

  // 검사 메서드들
  void check_redeclaration(const std::string &name, const ast::ASTNode &node);
  void check_undefined_reference(const std::string &name,
                                 const ast::ASTNode &node);
  void check_shadowing(const std::string &name, const ast::ASTNode &node);

  // 에러 보고
  void report_redeclaration_error(const std::string &name,
                                  const ast::ASTNode &node);
  void report_undefined_reference_error(const std::string &name,
                                        const ast::ASTNode &node);
  void report_shadowing_warning(const std::string &name,
                                const ast::ASTNode &node);

  // 유틸리티
  size_t get_current_scope_depth() const;
  bool is_in_global_scope() const;
};

} // namespace nugdev::compiler::analysis