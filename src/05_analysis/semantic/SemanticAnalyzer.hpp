#pragma once

#include "04_parsing/ast/core/ASTNode.hpp"
#include "05_analysis/errors/AnalysisError.hpp"
#include "05_analysis/semantic/SymbolTable.hpp"
#include "05_analysis/semantic/TypeChecker.hpp"
#include <memory>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 의미 분석 결과를 담는 구조체
 */
struct SemanticAnalysisResult {
  std::vector<AnalysisError> errors;
  std::vector<AnalysisError> warnings;
  SymbolTable symbol_table;

  bool has_errors() const { return !errors.empty(); }
  bool has_warnings() const { return !warnings.empty(); }
  size_t get_error_count() const { return errors.size(); }
  size_t get_warning_count() const { return warnings.size(); }

  void add_error(AnalysisError error) { errors.push_back(std::move(error)); }
  void add_warning(AnalysisError warning) {
    warnings.push_back(std::move(warning));
  }

  void merge_results(const SemanticAnalysisResult &other) {
    errors.insert(errors.end(), other.errors.begin(), other.errors.end());
    warnings.insert(warnings.end(), other.warnings.begin(),
                    other.warnings.end());
  }
};

/**
 * @brief 메인 의미 분석기 클래스
 *
 * 모든 의미 분석 단계를 조정하고 통합합니다:
 * 1. 심볼 테이블 구축
 * 2. 스코프 분석
 * 3. 타입 검사
 * 4. 제어 흐름 검증
 * 5. 사용되지 않는 변수 감지
 */
class SemanticAnalyzer {
public:
  /**
   * @brief 분석 옵션 설정
   */
  struct AnalysisOptions {
    bool check_unused_variables;
    bool check_uninitialized_variables;
    bool check_unreachable_code;
    bool strict_type_checking;
    bool allow_implicit_conversions;
    bool warn_on_shadowing;

    // 경고 레벨 설정
    enum class WarningLevel {
      NONE,    // 경고 없음
      BASIC,   // 기본 경고
      PEDANTIC // 엄격한 경고
    };
    WarningLevel warning_level;

    AnalysisOptions()
        : check_unused_variables(true), check_uninitialized_variables(true),
          check_unreachable_code(true), strict_type_checking(true),
          allow_implicit_conversions(false), warn_on_shadowing(true),
          warning_level(WarningLevel::BASIC) {}
  };

  explicit SemanticAnalyzer(const AnalysisOptions &options = AnalysisOptions{});

  /**
   * @brief 프로그램에 대한 완전한 의미 분석 수행
   */
  SemanticAnalysisResult analyze(ast::Program &program);

  /**
   * @brief 모듈에 대한 의미 분석 수행
   */
  SemanticAnalysisResult analyze_module(ast::Module &module);

  // 개별 분석 단계들
  void build_symbol_table(ast::ASTNode &root);
  std::vector<AnalysisError> check_types(ast::ASTNode &root);
  std::vector<AnalysisError> check_control_flow(ast::ASTNode &root);
  std::vector<AnalysisError> check_unused_symbols();
  std::vector<AnalysisError> check_uninitialized_variables();

  // 분석 옵션 설정
  void set_options(const AnalysisOptions &options) { m_options = options; }
  const AnalysisOptions &get_options() const { return m_options; }

  // 심볼 테이블 접근
  SymbolTable &get_symbol_table() { return m_symbol_table; }
  const SymbolTable &get_symbol_table() const { return m_symbol_table; }

private:
  AnalysisOptions m_options;
  SymbolTable m_symbol_table;
  std::unique_ptr<TypeChecker> m_type_checker;

  // 분석 단계 구현
  class SymbolTableBuilder;
  class ControlFlowChecker;
  class UnusedSymbolChecker;
  class InitializationChecker;

  std::unique_ptr<SymbolTableBuilder> m_symbol_builder;
  std::unique_ptr<ControlFlowChecker> m_control_flow_checker;
  std::unique_ptr<UnusedSymbolChecker> m_unused_checker;
  std::unique_ptr<InitializationChecker> m_init_checker;

  void initialize_analyzers();
  void filter_warnings_by_level(std::vector<AnalysisError> &errors) const;
};

/**
 * @brief 심볼 테이블 구축을 담당하는 내부 클래스
 */
class SemanticAnalyzer::SymbolTableBuilder : public ast::DefaultASTVisitor {
public:
  explicit SymbolTableBuilder(SymbolTable &symbol_table);

  void visit(ast::Module &node) override;
  void visit(ast::VariableDeclaration &node) override;
  void visit(ast::FunctionDeclaration &node) override;
  void visit(ast::StructDeclaration &node) override;
  void visit(ast::InterfaceDeclaration &node) override;
  void visit(ast::IfStatement &node) override;
  void visit(ast::ForStatement &node) override;
  void visit(ast::BlockExpression &node) override;

private:
  SymbolTable &m_symbol_table;
  std::vector<AnalysisError> m_errors;

  void enter_scope(Scope::ScopeType type);
  void exit_scope();
  bool define_symbol(std::unique_ptr<Symbol> symbol);
};

/**
 * @brief 제어 흐름 검증을 담당하는 내부 클래스
 */
class SemanticAnalyzer::ControlFlowChecker : public ast::DefaultASTVisitor {
public:
  explicit ControlFlowChecker(SymbolTable &symbol_table);

  std::vector<AnalysisError> check(ast::ASTNode &root);

  void visit(ast::BreakStatement &node) override;
  void visit(ast::ContinueStatement &node) override;
  void visit(ast::ReturnStatement &node) override;
  void visit(ast::ForStatement &node) override;

private:
  SymbolTable &m_symbol_table;
  std::vector<AnalysisError> m_errors;

  // 제어 흐름 컨텍스트
  struct FlowContext {
    bool in_loop = false;
    bool in_function = false;
    std::string loop_label;
    std::string function_name;
  };
  std::vector<FlowContext> m_context_stack;

  void push_context(const FlowContext &context);
  void pop_context();
  FlowContext &current_context();
};

/**
 * @brief 사용되지 않는 심볼 검사를 담당하는 내부 클래스
 */
class SemanticAnalyzer::UnusedSymbolChecker {
public:
  explicit UnusedSymbolChecker(SymbolTable &symbol_table);

  std::vector<AnalysisError> check_unused_symbols();

private:
  SymbolTable &m_symbol_table;

  bool should_warn_about_unused(const Symbol &symbol) const;
  AnalysisError create_unused_warning(const Symbol &symbol) const;
};

/**
 * @brief 초기화 검사를 담당하는 내부 클래스
 */
class SemanticAnalyzer::InitializationChecker : public ast::DefaultASTVisitor {
public:
  explicit InitializationChecker(SymbolTable &symbol_table);

  std::vector<AnalysisError> check_initialization(ast::ASTNode &root);

  void visit(ast::VariableDeclaration &node) override;
  void visit(ast::Identifier &node) override;
  void visit(ast::AssignmentExpression &node) override;

private:
  SymbolTable &m_symbol_table;
  std::vector<AnalysisError> m_errors;

  void check_variable_initialization(const ast::Identifier &id);
  void mark_variable_initialized(const std::string &name);
};

} // namespace nugdev::compiler::analysis