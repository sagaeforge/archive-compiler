#pragma once

#include "04_parsing/ast/expressions/Expressions.hpp"
#include "04_parsing/ast/literals/Literals.hpp"
#include "05_analysis/optimization/OptimizationPass.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace nugdev::compiler::optimization {

// ConstantFolder.hpp에서 가져온 타입
using ConstantValue = std::variant<int64_t, double, bool, std::string>;

/**
 * @brief 대수적 최적화 규칙 적용기
 *
 * 수학적 항등식을 이용한 최적화:
 * - x + 0 = x, x * 1 = x, x * 0 = 0
 * - x - x = 0, x / 1 = x
 * - x && true = x, x || false = x
 * - !(!(x)) = x
 */
class AlgebraicSimplifier : public VisitorOptimizationPass {
public:
  AlgebraicSimplifier() = default;

  std::string get_name() const override { return "AlgebraicSimplifier"; }

  std::string get_description() const override {
    return "Applies algebraic simplification rules";
  }

  void visit(ast::BinaryExpression &node) override;
  void visit(ast::UnaryExpression &node) override;

private:
  /**
   * @brief 산술 연산 단순화
   */
  std::unique_ptr<ast::Expression>
  simplify_arithmetic(ast::BinaryExpression::Operator op,
                      std::unique_ptr<ast::Expression> left,
                      std::unique_ptr<ast::Expression> right);

  /**
   * @brief 논리 연산 단순화
   */
  std::unique_ptr<ast::Expression>
  simplify_logical(ast::BinaryExpression::Operator op,
                   std::unique_ptr<ast::Expression> left,
                   std::unique_ptr<ast::Expression> right);

  /**
   * @brief 비교 연산 단순화
   */
  std::unique_ptr<ast::Expression>
  simplify_comparison(ast::BinaryExpression::Operator op,
                      std::unique_ptr<ast::Expression> left,
                      std::unique_ptr<ast::Expression> right);

  /**
   * @brief 비트 연산 단순화
   */
  std::unique_ptr<ast::Expression>
  simplify_bitwise(ast::BinaryExpression::Operator op,
                   std::unique_ptr<ast::Expression> left,
                   std::unique_ptr<ast::Expression> right);

  /**
   * @brief 단항 연산 단순화
   */
  std::unique_ptr<ast::Expression>
  simplify_unary(ast::UnaryExpression::Operator op,
                 std::unique_ptr<ast::Expression> operand);

  /**
   * @brief 표현식이 특정 값인지 확인
   */
  bool is_constant_value(const ast::Expression &expr,
                         const ConstantValue &value);
  bool is_zero(const ast::Expression &expr);
  bool is_one(const ast::Expression &expr);
  bool is_true(const ast::Expression &expr);
  bool is_false(const ast::Expression &expr);
  bool is_negative_one(const ast::Expression &expr);

  /**
   * @brief 같은 표현식인지 확인 (구조적 동등성)
   */
  bool are_expressions_equal(const ast::Expression &left,
                             const ast::Expression &right);

  /**
   * @brief 상수 리터럴 생성
   */
  std::unique_ptr<ast::Literal> create_integer_literal(int64_t value);
  std::unique_ptr<ast::Literal> create_boolean_literal(bool value);
  std::unique_ptr<ast::Literal> create_double_literal(double value);

  /**
   * @brief 표현식 복사 (deep copy)
   */
  std::unique_ptr<ast::Expression>
  clone_expression(const ast::Expression &expr);
};

/**
 * @brief 강도 감소 최적화 (Strength Reduction)
 *
 * 비싼 연산을 더 저렴한 연산으로 대체:
 * - x * 2 → x << 1
 * - x / 4 → x >> 2
 * - x * x → pow(x, 2) (특정 경우)
 */
class StrengthReducer : public VisitorOptimizationPass {
public:
  StrengthReducer() = default;

  std::string get_name() const override { return "StrengthReducer"; }

  std::string get_description() const override {
    return "Reduces expensive operations to cheaper equivalents";
  }

  void visit(ast::BinaryExpression &node) override;

private:
  /**
   * @brief 곱셈을 쉬프트로 변환
   */
  std::unique_ptr<ast::Expression>
  convert_multiply_to_shift(std::unique_ptr<ast::Expression> operand,
                            int64_t power_of_two);

  /**
   * @brief 나눗셈을 쉬프트로 변환
   */
  std::unique_ptr<ast::Expression>
  convert_divide_to_shift(std::unique_ptr<ast::Expression> operand,
                          int64_t power_of_two);

  /**
   * @brief 2의 거듭제곱인지 확인
   */
  std::optional<int> is_power_of_two(int64_t value);

  /**
   * @brief 연속된 덧셈/곱셈 최적화
   */
  std::unique_ptr<ast::Expression>
  optimize_associative_operation(ast::BinaryExpression &node);
};

/**
 * @brief 공통 부분식 제거 (Common Subexpression Elimination)
 *
 * 동일한 계산을 중복으로 수행하는 것을 방지:
 * - a * b + a * b → temp = a * b; temp + temp
 * - 지역적 CSE (블록 내에서만)
 */
class CommonSubexpressionEliminator : public VisitorOptimizationPass {
public:
  CommonSubexpressionEliminator() = default;

  std::string get_name() const override {
    return "CommonSubexpressionEliminator";
  }

  std::string get_description() const override {
    return "Eliminates redundant computations within basic blocks";
  }

  void visit(ast::BlockExpression &node) override;

private:
  /**
   * @brief 표현식 해시 계산 (구조적 해시)
   */
  size_t compute_expression_hash(const ast::Expression &expr);

  /**
   * @brief 블록 내 공통 부분식 찾기
   */
  std::unordered_map<size_t, std::vector<ast::Expression *>>
  find_common_subexpressions(const ast::BlockExpression &block);

  /**
   * @brief 부분식을 임시 변수로 대체
   */
  void replace_with_temporary(ast::Expression &expr,
                              const std::string &temp_name);

  /**
   * @brief 임시 변수 이름 생성
   */
  std::string generate_temp_variable_name();

  size_t m_temp_counter = 0;
  std::unordered_map<size_t, std::string> m_expression_to_temp;
};

/**
 * @brief 루프 불변 코드 이동 (Loop Invariant Code Motion)
 *
 * 루프 내에서 불변인 계산을 루프 밖으로 이동
 */
class LoopInvariantMover : public VisitorOptimizationPass {
public:
  LoopInvariantMover() = default;

  std::string get_name() const override { return "LoopInvariantMover"; }

  std::string get_description() const override {
    return "Moves loop-invariant computations outside loops";
  }

  void visit(ast::ForStatement &node) override;

private:
  /**
   * @brief 표현식이 루프 불변인지 확인
   */
  bool is_loop_invariant(const ast::Expression &expr,
                         const std::unordered_set<std::string> &loop_variables);

  /**
   * @brief 루프에서 수정되는 변수들 수집
   */
  std::unordered_set<std::string>
  collect_modified_variables(const ast::ForStatement &loop);

  /**
   * @brief 불변 표현식들을 루프 앞으로 이동
   */
  void
  hoist_invariant_expressions(ast::ForStatement &loop,
                              const std::unordered_set<std::string> &loop_vars);
};

/**
 * @brief 대수적 최적화 통합 관리자
 */
class AlgebraicOptimizationManager {
public:
  AlgebraicOptimizationManager();

  /**
   * @brief 모든 대수적 최적화 실행
   */
  bool run_all_optimizations(ast::ASTNode &root);

  /**
   * @brief 특정 최적화만 실행
   */
  bool run_simplification_only(ast::ASTNode &root);
  bool run_strength_reduction_only(ast::ASTNode &root);
  bool run_cse_only(ast::ASTNode &root);

  /**
   * @brief 반복 실행 (고정점까지)
   */
  bool run_until_fixpoint(ast::ASTNode &root, size_t max_iterations = 10);

  /**
   * @brief 최적화 통계
   */
  struct OptimizationStats {
    size_t simplifications_applied = 0;
    size_t strength_reductions = 0;
    size_t cse_eliminations = 0;
    size_t loop_invariant_moves = 0;
    size_t total_iterations = 0;
  };

  const OptimizationStats &get_statistics() const { return m_stats; }
  void reset_statistics() { m_stats = OptimizationStats{}; }

private:
  std::unique_ptr<AlgebraicSimplifier> m_simplifier;
  std::unique_ptr<StrengthReducer> m_strength_reducer;
  std::unique_ptr<CommonSubexpressionEliminator> m_cse;
  std::unique_ptr<LoopInvariantMover> m_licm;

  OptimizationStats m_stats;
};

} // namespace nugdev::compiler::optimization