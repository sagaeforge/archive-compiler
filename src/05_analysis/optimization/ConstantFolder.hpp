#pragma once

#include "04_parsing/ast/expressions/Expressions.hpp"
#include "04_parsing/ast/literals/Literals.hpp"
#include "05_analysis/optimization/OptimizationPass.hpp"
#include <memory>
#include <optional>
#include <variant>

namespace nugdev::compiler::optimization {

/**
 * @brief 상수 값을 저장하는 유니온 타입
 */
using ConstantValue = std::variant<int64_t,    // 정수
                                   double,     // 실수
                                   bool,       // 불린
                                   std::string // 문자열
                                   >;

/**
 * @brief 강화된 상수 접기 최적화 패스
 *
 * 컴파일 타임에 계산 가능한 표현식들을 미리 계산하여
 * 런타임 계산 비용을 줄입니다. constexpr 지원 포함.
 *
 * 예시:
 * - `2 + 3` → `5`
 * - `true and false` → `false`
 * - `"hello" + " world"` → `"hello world"`
 * - `10 * 0` → `0`
 * - constexpr 함수 호출 평가
 * - 템플릿 매개변수 계산
 */
class ConstantFolder : public VisitorOptimizationPass {
public:
  ConstantFolder() = default;

  std::string get_name() const override { return "EnhancedConstantFolder"; }
  std::string get_description() const override {
    return "Folds constant expressions at compile time with constexpr support";
  }

  /**
   * @brief constexpr 함수 평가 가능 여부 확인
   */
  bool
  can_evaluate_constexpr_function(const ast::PostfixExpression &call) const;

  /**
   * @brief 컴파일 타임 안전 계산 (오버플로우 검사 포함)
   */
  std::optional<ConstantValue>
  safe_compute_operation(ast::BinaryExpression::Operator op,
                         const ConstantValue &left,
                         const ConstantValue &right) const;

  // AST 방문자 메서드들
  void visit(ast::BinaryExpression &node) override;
  void visit(ast::UnaryExpression &node) override;
  void visit(ast::TernaryExpression &node) override;

  // EBNF 리터럴 지원
  void visit(ast::NumberLiteral &node) override;
  void visit(ast::StringLiteral &node) override;
  void visit(ast::BooleanLiteral &node) override;
  void visit(ast::CharacterLiteral &node) override; // EBNF: character_literal
  void visit(ast::NullLiteral &node) override;      // EBNF: null_literal
  void visit(ast::NoneLiteral &node) override;      // EBNF: none_literal
  void visit(ast::RangeLiteral &node) override;     // EBNF: range_literal

private:
  /**
   * @brief 표현식이 상수인지 확인하고 값을 반환
   */
  std::optional<ConstantValue> evaluate_constant(const ast::Expression &expr);

  /**
   * @brief 리터럴에서 상수 값 추출
   */
  std::optional<ConstantValue>
  extract_literal_value(const ast::Literal &literal);

  /**
   * @brief 이항 연산 계산
   */
  std::optional<ConstantValue>
  compute_binary_operation(ast::BinaryExpression::Operator op,
                           const ConstantValue &left,
                           const ConstantValue &right);

  /**
   * @brief 단항 연산 계산
   */
  std::optional<ConstantValue>
  compute_unary_operation(ast::UnaryExpression::Operator op,
                          const ConstantValue &operand);

  /**
   * @brief 상수 값을 적절한 리터럴 노드로 변환
   */
  std::unique_ptr<ast::Literal>
  create_literal_from_constant(const ConstantValue &value);

  // 산술 연산
  std::optional<ConstantValue> add_values(const ConstantValue &left,
                                          const ConstantValue &right);
  std::optional<ConstantValue> subtract_values(const ConstantValue &left,
                                               const ConstantValue &right);
  std::optional<ConstantValue> multiply_values(const ConstantValue &left,
                                               const ConstantValue &right);
  std::optional<ConstantValue> divide_values(const ConstantValue &left,
                                             const ConstantValue &right);
  std::optional<ConstantValue> modulo_values(const ConstantValue &left,
                                             const ConstantValue &right);

  // 비교 연산
  std::optional<ConstantValue> compare_equal(const ConstantValue &left,
                                             const ConstantValue &right);
  std::optional<ConstantValue> compare_not_equal(const ConstantValue &left,
                                                 const ConstantValue &right);
  std::optional<ConstantValue> compare_less(const ConstantValue &left,
                                            const ConstantValue &right);
  std::optional<ConstantValue> compare_greater(const ConstantValue &left,
                                               const ConstantValue &right);
  std::optional<ConstantValue> compare_less_equal(const ConstantValue &left,
                                                  const ConstantValue &right);
  std::optional<ConstantValue>
  compare_greater_equal(const ConstantValue &left, const ConstantValue &right);

  // 논리 연산
  std::optional<ConstantValue> logical_and(const ConstantValue &left,
                                           const ConstantValue &right);
  std::optional<ConstantValue> logical_or(const ConstantValue &left,
                                          const ConstantValue &right);
  std::optional<ConstantValue> logical_not(const ConstantValue &operand);

  // 비트 연산
  std::optional<ConstantValue> bitwise_and(const ConstantValue &left,
                                           const ConstantValue &right);
  std::optional<ConstantValue> bitwise_or(const ConstantValue &left,
                                          const ConstantValue &right);
  std::optional<ConstantValue> bitwise_xor(const ConstantValue &left,
                                           const ConstantValue &right);
  std::optional<ConstantValue> bitwise_not(const ConstantValue &operand);
  std::optional<ConstantValue> left_shift(const ConstantValue &left,
                                          const ConstantValue &right);
  std::optional<ConstantValue> right_shift(const ConstantValue &left,
                                           const ConstantValue &right);

  // 타입 검사 헬퍼
  bool is_numeric(const ConstantValue &value) const;
  bool is_integer(const ConstantValue &value) const;
  bool is_boolean(const ConstantValue &value) const;
  bool is_string(const ConstantValue &value) const;

  // 타입 변환 헬퍼
  std::optional<int64_t> to_integer(const ConstantValue &value) const;
  std::optional<double> to_double(const ConstantValue &value) const;
  std::optional<bool> to_boolean(const ConstantValue &value) const;
  std::optional<std::string> to_string(const ConstantValue &value) const;
};

} // namespace nugdev::compiler::optimization