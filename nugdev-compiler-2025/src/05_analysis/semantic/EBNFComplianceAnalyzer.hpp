#pragma once

#include "04_parsing/ast/core/ASTVisitor.h"
#include "05_analysis/errors/AnalysisError.hpp"
#include "05_analysis/semantic/SymbolTable.hpp"
#include <memory>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief EBNF 문법 준수 검증 분석기
 *
 * nugdev EBNF 문법의 모든 요소가 올바르게 구현되고 사용되는지 검증:
 * - 모든 문법 요소의 구현 확인
 * - EBNF 규칙 준수 검증
 * - 누락된 기능 탐지
 * - 문법 일관성 검사
 */
class EBNFComplianceAnalyzer : public ast::DefaultASTVisitor {
public:
  explicit EBNFComplianceAnalyzer(const SymbolTable &symbol_table);

  /**
   * @brief EBNF 호환성 분석 실행
   */
  std::vector<AnalysisError> analyze_ebnf_compliance(ast::Program &program);

  /**
   * @brief 누락된 EBNF 요소들 검사
   */
  std::vector<AnalysisError> check_missing_ebnf_features();

  // EBNF 모든 노드 타입 방문자 메서드들

  // Program Structure (EBNF: program = { module })
  void visit(ast::Program &node) override;
  void visit(ast::Module &node) override;

  // Import/Export (EBNF: import_statement, export_statement)
  void visit(ast::ImportStatement &node) override;
  void visit(ast::ExportStatement &node) override;

  // Statements (EBNF: statement alternatives)
  void visit(ast::VariableDeclaration &node) override;
  void visit(ast::FunctionDeclaration &node) override;
  void visit(ast::StructDeclaration &node) override;
  void visit(ast::InterfaceDeclaration &node) override;
  void visit(ast::ExpressionStatement &node) override;

  // Control Flow Statements (EBNF: control_statement, loop_statement,
  // if_statement)
  void visit(ast::IfStatement &node) override;
  void visit(ast::ForStatement &node) override;
  void visit(ast::BreakStatement &node) override;
  void visit(ast::ContinueStatement &node) override;
  void visit(ast::ReturnStatement &node) override;

  // Expressions (EBNF: expression hierarchy)
  void visit(ast::AssignmentExpression &node) override;
  void visit(ast::TernaryExpression &node) override;

  // EBNF 특별 표현식들 (BinaryExpression의 특수 경우로 구현)
  void analyze_null_coalescing_expression(
      const ast::BinaryExpression
          &node); // EBNF: null_coalescing_expression (??)
  void analyze_type_check_expression(
      const ast::BinaryExpression &node); // EBNF: type_check_expression (is)

  void visit(ast::BinaryExpression &node) override;
  void visit(ast::UnaryExpression &node) override;
  void visit(ast::PostfixExpression &node) override;

  // Primary Expressions (EBNF: primary_expression)
  void visit(ast::Identifier &node) override;
  void visit(ast::BlockExpression &node) override;
  void visit(ast::IfExpression &node) override;   // EBNF: if_expression
  void visit(ast::WhenExpression &node) override; // EBNF: when_expression
  void
  visit(ast::FunctionExpression &node) override;    // EBNF: function_expression
  void visit(ast::LambdaExpression &node) override; // EBNF: lambda_expression

  // Literals (EBNF: literal alternatives)
  void visit(ast::NumberLiteral &node) override;    // EBNF: number_literal
  void visit(ast::StringLiteral &node) override;    // EBNF: string_literal
  void visit(ast::CharacterLiteral &node) override; // EBNF: character_literal
  void visit(ast::BooleanLiteral &node) override;   // EBNF: boolean_literal
  void visit(ast::NullLiteral &node) override;      // EBNF: null_literal
  void visit(ast::NoneLiteral &node) override;      // EBNF: none_literal
  void visit(ast::RangeLiteral &node) override;     // EBNF: range_literal
  void visit(ast::ArrayLiteral &node) override;
  void visit(ast::ObjectLiteral &node) override;

  // Type Literals (EBNF: type_literal, complex_type)
  void visit(ast::TypeLiteral &node) override;
  void visit(ast::FunctionType &node) override; // EBNF: function_type
  void visit(ast::OptionalType &node) override; // EBNF: optional_type
  void visit(ast::TupleType &node) override;    // EBNF: tuple_type

  // Cast Operations (EBNF: cast_operator)
  void visit(ast::CastExpression &node)
      override; // EBNF: "as" type_literal, "as?" type_literal

  // When Conditions (EBNF: when_condition alternatives)
  void visit(ast::ValueCondition &node);    // EBNF: value_condition
  void visit(ast::RangeCondition &node);    // EBNF: range_condition
  void visit(ast::TypeCondition &node);     // EBNF: type_condition
  void visit(ast::GuardCondition &node);    // EBNF: guard_condition
  void visit(ast::MultipleCondition &node); // EBNF: multiple_condition

private:
  const SymbolTable &m_symbol_table;
  std::vector<AnalysisError> m_errors;

  // EBNF 준수 검증 상태
  struct ComplianceState {
    bool has_null_coalescing = false;
    bool has_type_checking = false;
    bool has_safe_casting = false;
    bool has_range_literals = false;
    bool has_lambda_expressions = false;
    bool has_when_expressions = false;
    bool has_guard_conditions = false;
    bool has_multiple_conditions = false;
  } m_compliance_state;

  /**
   * @brief EBNF 연산자 우선순위 검증
   */
  bool verify_operator_precedence(const ast::BinaryExpression &expr);

  /**
   * @brief EBNF 키워드 사용 검증
   */
  bool verify_reserved_keywords(const ast::Identifier &identifier);

  /**
   * @brief EBNF 문법 구조 검증
   */
  bool verify_grammar_structure(const ast::ASTNode &node);

  /**
   * @brief 누락된 EBNF 기능 보고
   */
  void report_missing_ebnf_feature(const std::string &feature_name,
                                   const std::string &description);

  /**
   * @brief EBNF 규칙 위반 보고
   */
  void report_ebnf_violation(const std::string &rule, const ast::ASTNode &node,
                             const std::string &description);
};

/**
 * @brief Null Coalescing 표현식 (?? 연산자)
 * EBNF: null_coalescing_expression = logical_or_expression { "??"
 * logical_or_expression }
 */
class NullCoalescingExpressionAnalyzer {
public:
  static bool is_null_coalescing_safe(const ast::Expression &left_expr,
                                      const ast::Expression &right_expr);

  static std::unique_ptr<ast::TypeLiteral>
  infer_null_coalescing_type(const ast::Expression &left_expr,
                             const ast::Expression &right_expr);

  static std::vector<AnalysisError>
  validate_null_coalescing(const ast::Expression &left_expr,
                           const ast::Expression &right_expr);
};

/**
 * @brief Type Check 표현식 (is 연산자)
 * EBNF: type_check_expression = shift_expression { "is" type_literal }
 */
class TypeCheckExpressionAnalyzer {
public:
  static bool is_type_check_valid(const ast::Expression &expr,
                                  const ast::TypeLiteral &type);

  static std::vector<AnalysisError>
  validate_type_check(const ast::Expression &expr,
                      const ast::TypeLiteral &type);

  /**
   * @brief 타입 가드로 사용될 수 있는지 확인
   */
  static bool can_be_type_guard(const ast::Expression &expr);
};

/**
 * @brief Lambda 표현식 분석기
 * EBNF: lambda_expression = [ label ] "(" [ lambda_parameter_list ] ")" "=>"
 * expression
 */
class LambdaExpressionAnalyzer {
public:
  static std::vector<AnalysisError>
  validate_lambda_expression(const ast::LambdaExpression &lambda);

  static std::unique_ptr<ast::TypeLiteral>
  infer_lambda_type(const ast::LambdaExpression &lambda);

  /**
   * @brief 람다 파라미터 타입 주석 필수 검증
   * EBNF 주석: Lambda parameters MUST have type annotations
   */
  static bool verify_type_annotations(const ast::LambdaExpression &lambda);
};

/**
 * @brief Range 리터럴 분석기
 * EBNF: range_literal = expression ".." [ expression ]
 */
class RangeLiteralAnalyzer {
public:
  static std::vector<AnalysisError>
  validate_range_literal(const ast::RangeLiteral &range);

  static bool is_valid_range_type(const ast::Expression &start,
                                  const ast::Expression *end = nullptr);

  /**
   * @brief 무한 범위 여부 확인
   */
  static bool is_infinite_range(const ast::RangeLiteral &range);
};

/**
 * @brief When 표현식 및 조건 분석기
 * EBNF: when_expression, when_condition alternatives
 */
class WhenExpressionAnalyzer {
public:
  static std::vector<AnalysisError>
  validate_when_expression(const ast::WhenExpression &when_expr);

  static bool verify_exhaustive_matching(const ast::WhenExpression &when_expr);

  static std::vector<AnalysisError>
  validate_when_conditions(const std::vector<ast::ASTNode *> &conditions);

  /**
   * @brief Guard 조건 검증
   * EBNF: guard_condition = value_condition "if" expression
   */
  static bool is_valid_guard_condition(const ast::Expression &condition);
};

/**
 * @brief EBNF 호환성 검증 유틸리티
 */
class EBNFComplianceUtils {
public:
  /**
   * @brief EBNF에서 정의된 예약 키워드 목록
   */
  static const std::vector<std::string> &get_reserved_keywords();

  /**
   * @brief EBNF 연산자 우선순위 테이블
   */
  static int get_operator_precedence(const std::string &operator_symbol);

  /**
   * @brief EBNF 문법 규칙 검증
   */
  static bool is_valid_identifier(const std::string &name);

  /**
   * @brief 타입 이름이 예약되지 않았는지 확인
   * EBNF 주석: 'number', 'string', 'boolean' 등은 예약 키워드가 아님
   */
  static bool is_type_name_allowed(const std::string &type_name);
};

} // namespace nugdev::compiler::analysis