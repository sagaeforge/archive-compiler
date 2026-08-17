#pragma once

#include "04_parsing/ast/expressions/Expressions.hpp"
#include "04_parsing/ast/literals/Literals.hpp"
#include "05_analysis/optimization/ConstantFolder.hpp"
#include "05_analysis/semantic/SymbolTable.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>

namespace nugdev::compiler::analysis {

/**
 * @brief 컴파일 타임 값 표현
 */
class CompileTimeValue {
public:
  using ValueType =
      std::variant<int64_t, double, bool, std::string,
                   std::vector<CompileTimeValue>,                    // 배열
                   std::unordered_map<std::string, CompileTimeValue> // 객체
                   >;

  CompileTimeValue() = default;
  explicit CompileTimeValue(ValueType value) : m_value(std::move(value)) {}

  // 타입 검사
  bool is_integer() const { return std::holds_alternative<int64_t>(m_value); }
  bool is_double() const { return std::holds_alternative<double>(m_value); }
  bool is_bool() const { return std::holds_alternative<bool>(m_value); }
  bool is_string() const {
    return std::holds_alternative<std::string>(m_value);
  }
  bool is_array() const {
    return std::holds_alternative<std::vector<CompileTimeValue>>(m_value);
  }
  bool is_object() const {
    return std::holds_alternative<
        std::unordered_map<std::string, CompileTimeValue>>(m_value);
  }

  // 값 접근
  template <typename T> const T &get() const { return std::get<T>(m_value); }

  template <typename T> T &get() { return std::get<T>(m_value); }

  // 산술 연산
  CompileTimeValue operator+(const CompileTimeValue &other) const;
  CompileTimeValue operator-(const CompileTimeValue &other) const;
  CompileTimeValue operator*(const CompileTimeValue &other) const;
  CompileTimeValue operator/(const CompileTimeValue &other) const;
  CompileTimeValue operator%(const CompileTimeValue &other) const;

  // 비교 연산
  bool operator==(const CompileTimeValue &other) const;
  bool operator!=(const CompileTimeValue &other) const;
  bool operator<(const CompileTimeValue &other) const;
  bool operator>(const CompileTimeValue &other) const;
  bool operator<=(const CompileTimeValue &other) const;
  bool operator>=(const CompileTimeValue &other) const;

  // 논리 연산
  CompileTimeValue operator&&(const CompileTimeValue &other) const;
  CompileTimeValue operator||(const CompileTimeValue &other) const;
  CompileTimeValue operator!() const;

  // 문자열 변환
  std::string to_string() const;

private:
  ValueType m_value;
};

/**
 * @brief 컴파일 타임 표현식 평가기
 *
 * constexpr 표현식과 상수 표현식을 컴파일 타임에 평가:
 * - 상수 접기 확장
 * - constexpr 함수 실행
 * - 컴파일 타임 조건부 컴파일
 * - 템플릿 매개변수 계산
 */
class CompileTimeEvaluator : public ast::DefaultASTVisitor {
public:
  explicit CompileTimeEvaluator(const SymbolTable &symbol_table);

  /**
   * @brief 표현식을 컴파일 타임에 평가
   */
  std::optional<CompileTimeValue> evaluate(const ast::Expression &expr);

  /**
   * @brief constexpr 함수 실행
   */
  std::optional<CompileTimeValue>
  evaluate_constexpr_function(const ast::FunctionDeclaration &function,
                              const std::vector<CompileTimeValue> &arguments);

  /**
   * @brief 컴파일 타임 조건 검사
   */
  bool evaluate_static_condition(const ast::Expression &condition);

  // AST 방문자 메서드들
  void visit(ast::NumberLiteral &node) override;
  void visit(ast::BooleanLiteral &node) override;
  void visit(ast::StringLiteral &node) override;
  void visit(ast::CharacterLiteral &node) override; // EBNF: character_literal
  void visit(ast::NullLiteral &node) override;      // EBNF: null_literal
  void visit(ast::NoneLiteral &node) override;      // EBNF: none_literal
  void visit(ast::RangeLiteral &node) override;     // EBNF: range_literal
  void visit(ast::ArrayLiteral &node) override;
  void visit(ast::ObjectLiteral &node) override;
  void visit(ast::BinaryExpression &node) override;
  void visit(ast::UnaryExpression &node) override;
  void visit(ast::TernaryExpression &node) override;
  void visit(ast::Identifier &node) override;
  void visit(ast::PostfixExpression &node) override;
  void visit(ast::IfExpression &node) override;     // EBNF: if_expression
  void visit(ast::WhenExpression &node) override;   // EBNF: when_expression
  void visit(ast::LambdaExpression &node) override; // EBNF: lambda_expression
  void visit(ast::CastExpression &node) override;   // EBNF: cast_operator

private:
  const SymbolTable &m_symbol_table;
  std::optional<CompileTimeValue> m_result;
  std::unordered_map<std::string, CompileTimeValue> m_constexpr_variables;

  /**
   * @brief constexpr 변수 체크
   */
  bool is_constexpr_variable(const std::string &name) const;

  /**
   * @brief 이항 연산 평가
   */
  std::optional<CompileTimeValue>
  evaluate_binary_operation(ast::BinaryExpression::Operator op,
                            const CompileTimeValue &left,
                            const CompileTimeValue &right);

  /**
   * @brief 단항 연산 평가
   */
  std::optional<CompileTimeValue>
  evaluate_unary_operation(ast::UnaryExpression::Operator op,
                           const CompileTimeValue &operand);

  /**
   * @brief 배열/객체 접근 평가
   */
  std::optional<CompileTimeValue>
  evaluate_array_access(const CompileTimeValue &array,
                        const CompileTimeValue &index);

  std::optional<CompileTimeValue>
  evaluate_member_access(const CompileTimeValue &object,
                         const std::string &member);

  /**
   * @brief 타입 안전 산술 연산
   */
  template <typename Op>
  std::optional<CompileTimeValue>
  safe_arithmetic_operation(const CompileTimeValue &left,
                            const CompileTimeValue &right, Op operation);

  /**
   * @brief 오버플로우 검사
   */
  bool check_integer_overflow(int64_t a, int64_t b, char op) const;
  bool check_division_by_zero(const CompileTimeValue &divisor) const;
};

/**
 * @brief constexpr 함수 실행기
 */
class ConstexprFunctionExecutor {
public:
  explicit ConstexprFunctionExecutor(const CompileTimeEvaluator &evaluator);

  /**
   * @brief constexpr 함수 실행
   */
  std::optional<CompileTimeValue>
  execute(const ast::FunctionDeclaration &function,
          const std::vector<CompileTimeValue> &arguments);

private:
  const CompileTimeEvaluator &m_evaluator;
  std::unordered_map<std::string, CompileTimeValue> m_local_variables;

  /**
   * @brief 함수 바디 실행
   */
  std::optional<CompileTimeValue> execute_statements(
      const std::vector<std::unique_ptr<ast::Statement>> &statements);

  /**
   * @brief 지역 변수 관리
   */
  void set_local_variable(const std::string &name,
                          const CompileTimeValue &value);
  std::optional<CompileTimeValue>
  get_local_variable(const std::string &name) const;
};

/**
 * @brief 컴파일 타임 타입 계산기
 */
class CompileTimeTypeCalculator {
public:
  /**
   * @brief 컴파일 타임에 타입 계산
   */
  static std::unique_ptr<ast::TypeLiteral>
  calculate_type(const ast::Expression &expr, const SymbolTable &symbol_table);

  /**
   * @brief 템플릿 타입 인스턴스화
   */
  static std::unique_ptr<ast::TypeLiteral> instantiate_template_type(
      const ast::TypeLiteral &template_type,
      const std::unordered_map<std::string, ast::TypeLiteral *>
          &type_parameters);

  /**
   * @brief 의존 타입 해결
   */
  static std::unique_ptr<ast::TypeLiteral>
  resolve_dependent_type(const ast::TypeLiteral &dependent_type,
                         const std::unordered_map<std::string, CompileTimeValue>
                             &compile_time_values);

private:
  /**
   * @brief 타입 표현식 평가
   */
  static std::optional<CompileTimeValue>
  evaluate_type_expression(const ast::Expression &expr);
};

/**
 * @brief 컴파일 타임 최적화 기회 탐지
 */
class CompileTimeOptimizationDetector {
public:
  struct OptimizationOpportunity {
    enum class Type {
      CONSTANT_FOLDING,
      DEAD_CODE_ELIMINATION,
      FUNCTION_INLINING,
      TEMPLATE_SPECIALIZATION,
      LOOP_UNROLLING
    };

    Type type;
    const ast::ASTNode *node;
    std::string description;
    CompileTimeValue optimized_value;
  };

  explicit CompileTimeOptimizationDetector(
      const CompileTimeEvaluator &evaluator);

  /**
   * @brief 최적화 기회 탐지
   */
  std::vector<OptimizationOpportunity> detect_opportunities(ast::ASTNode &root);

private:
  const CompileTimeEvaluator &m_evaluator;

  /**
   * @brief 상수 접기 기회 탐지
   */
  std::vector<OptimizationOpportunity>
  detect_constant_folding(ast::ASTNode &root);

  /**
   * @brief 죽은 코드 탐지
   */
  std::vector<OptimizationOpportunity> detect_dead_code(ast::ASTNode &root);

  /**
   * @brief 인라이닝 기회 탐지
   */
  std::vector<OptimizationOpportunity>
  detect_inlining_opportunities(ast::ASTNode &root);
};

} // namespace nugdev::compiler::analysis