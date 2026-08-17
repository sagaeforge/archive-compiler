#pragma once

#include "04_parsing/ast/core/ASTVisitor.h"
#include "04_parsing/ast/expressions/Expressions.hpp"
#include "05_analysis/errors/AnalysisError.hpp"
#include "05_analysis/semantic/SymbolTable.hpp"
#include <memory>
#include <unordered_set>
#include <vector>

namespace nugdev::compiler::analysis {

/**
 * @brief 강화된 타입 검사를 수행하는 클래스
 *
 * nugdev 언어의 타입 시스템 규칙을 검증:
 * - 타입 호환성 검사
 * - 함수 호출 인자 타입 검사
 * - 연산자 타입 검사
 * - 할당 타입 검사
 * - 제네릭 타입 추론
 * - 컴파일 타임 타입 안전성
 * - 널 안전성 검사
 * - 메모리 안전성 검증
 */
class TypeChecker : public ast::DefaultASTVisitor {
public:
  explicit TypeChecker(SymbolTable &symbol_table);

  // 메인 검사 메서드
  std::vector<AnalysisError> check_types(ast::Program &program);

  // 강화된 타입 검사
  std::vector<AnalysisError> check_compile_time_safety(ast::Program &program);
  std::vector<AnalysisError> check_null_safety(ast::Program &program);
  std::vector<AnalysisError> check_memory_safety(ast::Program &program);
  std::vector<AnalysisError> check_overflow_safety(ast::Program &program);

  // AST 방문자 메서드들 - 주요 노드들만 override
  void visit(ast::VariableDeclaration &node) override;
  void visit(ast::FunctionDeclaration &node) override;
  void visit(ast::BinaryExpression &node) override;
  void visit(ast::UnaryExpression &node) override;
  void visit(ast::PostfixExpression &node) override;
  void visit(ast::AssignmentExpression &node) override;
  void visit(ast::TernaryExpression &node) override;
  void visit(ast::IfExpression &node) override;
  void visit(ast::WhenExpression &node) override;
  void visit(ast::FunctionExpression &node) override;
  void visit(ast::LambdaExpression &node) override; // EBNF: lambda_expression
  void visit(ast::ReturnStatement &node) override;
  void
  visit(ast::CastExpression &node) override; // EBNF: cast_operator (as, as?)

  // EBNF에서 추가된 리터럴들
  void visit(ast::CharacterLiteral &node) override;
  void visit(ast::RangeLiteral &node) override;
  void visit(ast::NoneLiteral &node) override;

  // 타입 추론 메서드들
  std::unique_ptr<ast::TypeLiteral>
  infer_expression_type(const ast::Expression &expr);
  std::unique_ptr<ast::TypeLiteral>
  infer_literal_type(const ast::Literal &literal);
  std::unique_ptr<ast::TypeLiteral>
  infer_binary_result_type(ast::BinaryExpression::Operator op,
                           const ast::TypeLiteral &left_type,
                           const ast::TypeLiteral &right_type);

private:
  SymbolTable &m_symbol_table;
  std::vector<AnalysisError> m_errors;
  std::unique_ptr<ast::TypeLiteral> m_current_function_return_type;

  // 타입 호환성 검사
  bool are_types_compatible(const ast::TypeLiteral &target,
                            const ast::TypeLiteral &source) const;
  bool is_numeric_type(const ast::TypeLiteral &type) const;
  bool is_comparable_type(const ast::TypeLiteral &type) const;
  bool is_boolean_type(const ast::TypeLiteral &type) const;
  bool is_string_type(const ast::TypeLiteral &type) const;

  // 타입 변환 검사
  bool can_convert_implicitly(const ast::TypeLiteral &from,
                              const ast::TypeLiteral &to) const;
  bool can_convert_explicitly(const ast::TypeLiteral &from,
                              const ast::TypeLiteral &to) const;

  // 강화된 타입 안전성 검사
  bool is_null_safe_type(const ast::TypeLiteral &type) const;
  bool is_bounds_safe_access(const ast::PostfixExpression &access) const;
  bool check_integer_overflow_risk(const ast::BinaryExpression &expr) const;
  bool verify_memory_safety(const ast::Expression &expr) const;
  bool is_constexpr_evaluable(const ast::Expression &expr) const;

  // 연산자별 타입 검사
  bool check_arithmetic_operator(ast::BinaryExpression::Operator op,
                                 const ast::TypeLiteral &left,
                                 const ast::TypeLiteral &right) const;
  bool check_comparison_operator(ast::BinaryExpression::Operator op,
                                 const ast::TypeLiteral &left,
                                 const ast::TypeLiteral &right) const;
  bool check_logical_operator(ast::BinaryExpression::Operator op,
                              const ast::TypeLiteral &left,
                              const ast::TypeLiteral &right) const;
  bool check_bitwise_operator(ast::BinaryExpression::Operator op,
                              const ast::TypeLiteral &left,
                              const ast::TypeLiteral &right) const;

  // 함수 호출 검사
  bool check_function_call(const ast::PostfixExpression &call_expr);
  bool check_parameter_compatibility(
      const std::vector<std::unique_ptr<ast::Parameter>> &parameters,
      const std::vector<std::unique_ptr<ast::Expression>> &arguments);

  // 제어 흐름 타입 검사
  bool check_condition_type(const ast::Expression &condition);
  bool check_return_type_compatibility(const ast::Expression *return_expr);

  // 에러 보고
  void report_type_error(const std::string &message, const ast::ASTNode &node);
  void report_incompatible_types(const ast::TypeLiteral &expected,
                                 const ast::TypeLiteral &actual,
                                 const ast::ASTNode &node);

  // 내장 타입 생성 헬퍼
  std::unique_ptr<ast::SimpleType>
  create_builtin_type(const std::string &name) const;
  std::unique_ptr<ast::SimpleType> create_number_type() const;
  std::unique_ptr<ast::SimpleType> create_string_type() const;
  std::unique_ptr<ast::SimpleType> create_boolean_type() const;
  std::unique_ptr<ast::SimpleType> create_void_type() const;
};

/**
 * @brief 타입 추론 유틸리티 클래스
 *
 * 복잡한 타입 추론 로직을 분리하여 관리
 */
class TypeInference {
public:
  static std::unique_ptr<ast::TypeLiteral> infer_array_element_type(
      const std::vector<std::unique_ptr<ast::Expression>> &elements);

  static std::unique_ptr<ast::TypeLiteral>
  unify_types(const std::vector<std::unique_ptr<ast::TypeLiteral>> &types);

  static std::unique_ptr<ast::TypeLiteral>
  get_most_general_type(const ast::TypeLiteral &type1,
                        const ast::TypeLiteral &type2);

private:
  static bool is_more_general(const ast::TypeLiteral &general,
                              const ast::TypeLiteral &specific);
};

} // namespace nugdev::compiler::analysis