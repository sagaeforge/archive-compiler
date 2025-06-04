#pragma once

#include <04_parsing/ast/core/ASTNode.hpp>
#include <04_parsing/ast/types/Types.hpp>
#include <memory>
#include <string>
#include <vector>

namespace nugdev {
namespace ast {

// Forward declarations
class ASTVisitor;

/**
 * @brief Base class for all expressions
 *
 * Expressions have return values and can be evaluated.
 * This follows the EBNF grammar structure for expressions.
 */
class Expression : public ASTNode {
public:
  explicit Expression(NodeType type) : ASTNode(type) {}

  virtual ~Expression() = default;

  // Expressions can be evaluated and have types
  virtual std::string get_expression_type() const = 0;
};

/**
 * @brief Identifier expression
 *
 * EBNF: identifier = IDENTIFIER ;
 * Represents variable names, function names, type names, etc.
 */
class Identifier : public Expression {
public:
  explicit Identifier(const std::string &name)
      : Expression(NodeType::IDENTIFIER), name(name) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override { return "Identifier(" + name + ")"; }

  std::string get_expression_type() const override { return "identifier"; }

  const std::string &get_name() const { return name; }

private:
  std::string name;
};

/**
 * @brief Binary expression (left operator right)
 *
 * EBNF: Covers all binary operators from the precedence hierarchy
 */
class BinaryExpression : public Expression {
public:
  enum class Operator {
    // Assignment operators
    ASSIGN,             // =
    ADD_ASSIGN,         // +=
    SUB_ASSIGN,         // -=
    MUL_ASSIGN,         // *=
    DIV_ASSIGN,         // /=
    MOD_ASSIGN,         // %=
    BITWISE_AND_ASSIGN, // &=
    BITWISE_OR_ASSIGN,  // |=
    BITWISE_XOR_ASSIGN, // ^=
    BITWISE_NOT_ASSIGN, // ~=

    // Arithmetic operators
    ADD,      // +
    SUBTRACT, // -
    MULTIPLY, // *
    DIVIDE,   // /
    MODULO,   // %

    // Comparison operators
    EQUAL,         // ==
    NOT_EQUAL,     // !=
    LESS_THAN,     // <
    GREATER_THAN,  // >
    LESS_EQUAL,    // <=
    GREATER_EQUAL, // >=

    // Logical operators
    LOGICAL_AND, // and
    LOGICAL_OR,  // or

    // Bitwise operators
    BITWISE_AND, // &
    BITWISE_OR,  // |
    BITWISE_XOR, // ^
    LEFT_SHIFT,  // <<
    RIGHT_SHIFT, // >>

    // Special operators
    NULL_COALESCING, // ??
    TYPE_CHECK       // is
  };

  explicit BinaryExpression(Operator op, std::unique_ptr<Expression> left,
                            std::unique_ptr<Expression> right)
      : Expression(NodeType::BINARY_EXPRESSION), operator_(op),
        left(std::move(left)), right(std::move(right)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "BinaryExpression(" + get_operator_string() + ")";
  }

  std::string get_expression_type() const override { return "binary"; }

  Operator get_operator() const { return operator_; }
  const Expression &get_left() const { return *left; }
  const Expression &get_right() const { return *right; }

  std::string get_operator_string() const;

  // Helper methods
  bool is_assignment_operator() const;
  bool is_comparison_operator() const;
  bool is_arithmetic_operator() const;
  bool is_logical_operator() const;
  bool is_bitwise_operator() const;

private:
  Operator operator_;
  std::unique_ptr<Expression> left;
  std::unique_ptr<Expression> right;
};

/**
 * @brief Unary expression (operator operand)
 *
 * EBNF: unary_expression = ( "+" | "-" | "!" | "not" | "~" | "++" | "--" )
 * unary_expression
 */
class UnaryExpression : public Expression {
public:
  enum class Operator {
    PLUS,             // +
    MINUS,            // -
    LOGICAL_NOT,      // !
    LOGICAL_NOT_WORD, // not
    BITWISE_NOT,      // ~
    PRE_INCREMENT,    // ++
    PRE_DECREMENT     // --
  };

  explicit UnaryExpression(Operator op, std::unique_ptr<Expression> operand)
      : Expression(NodeType::UNARY_EXPRESSION), operator_(op),
        operand(std::move(operand)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "UnaryExpression(" + get_operator_string() + ")";
  }

  std::string get_expression_type() const override { return "unary"; }

  Operator get_operator() const { return operator_; }
  const Expression &get_operand() const { return *operand; }

  std::string get_operator_string() const;

private:
  Operator operator_;
  std::unique_ptr<Expression> operand;
};

/**
 * @brief Postfix expression (operand operator)
 *
 * EBNF: postfix_expression = primary_expression { postfix_operator }
 * postfix_operator = "++" | "--" | "." identifier | "?." identifier | "["
 * expression "]" | "(" [ argument_list ] ")" | cast_operator
 */
class PostfixExpression : public Expression {
public:
  enum class OperatorType {
    POST_INCREMENT,     // operand++
    POST_DECREMENT,     // operand--
    MEMBER_ACCESS,      // operand.member
    SAFE_MEMBER_ACCESS, // operand?.member
    ARRAY_ACCESS,       // operand[index]
    FUNCTION_CALL,      // operand(args...)
    CAST                // operand as Type
  };

  explicit PostfixExpression(OperatorType opType,
                             std::unique_ptr<Expression> operand)
      : Expression(NodeType::POSTFIX_EXPRESSION), operatorType(opType),
        operand(std::move(operand)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "PostfixExpression(" + get_operator_string() + ")";
  }

  std::string get_expression_type() const override { return "postfix"; }

  OperatorType get_operator_type() const { return operatorType; }
  const Expression &get_operand() const { return *operand; }

  std::string get_operator_string() const;

  // For member access
  void set_member_name(const std::string &name) { memberName = name; }
  const std::string &get_member_name() const { return memberName; }

  // For array access
  void set_index_expression(std::unique_ptr<Expression> index) {
    indexExpression = std::move(index);
  }
  const Expression *get_index_expression() const {
    return indexExpression.get();
  }

  // For function calls
  void set_arguments(std::vector<std::unique_ptr<Expression>> args) {
    arguments = std::move(args);
  }
  const std::vector<std::unique_ptr<Expression>> &get_arguments() const {
    return arguments;
  }

  // For casting
  void set_cast_type(std::unique_ptr<TypeLiteral> type) {
    castType = std::move(type);
  }
  const TypeLiteral *get_cast_type() const { return castType.get(); }
  void set_safe_cast(bool safe) { safeCast = safe; }
  bool is_safe_cast() const { return safeCast; }

private:
  OperatorType operatorType;
  std::unique_ptr<Expression> operand;

  // Additional data based on operator type
  std::string memberName;                             // For member access
  std::unique_ptr<Expression> indexExpression;        // For array access
  std::vector<std::unique_ptr<Expression>> arguments; // For function calls
  std::unique_ptr<TypeLiteral> castType;              // For casting
  bool safeCast = false;                              // For safe casting (as?)
};

/**
 * @brief Ternary conditional expression (condition ? true_expr : false_expr)
 *
 * EBNF: ternary_expression = null_coalescing_expression [ "?" expression ":"
 * expression ]
 */
class TernaryExpression : public Expression {
public:
  explicit TernaryExpression(std::unique_ptr<Expression> condition,
                             std::unique_ptr<Expression> trueExpr,
                             std::unique_ptr<Expression> falseExpr)
      : Expression(NodeType::TERNARY_EXPRESSION),
        condition(std::move(condition)), trueExpression(std::move(trueExpr)),
        falseExpression(std::move(falseExpr)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override { return "TernaryExpression(? :)"; }

  std::string get_expression_type() const override { return "ternary"; }

  const Expression &get_condition() const { return *condition; }
  const Expression &get_true_expression() const { return *trueExpression; }
  const Expression &get_false_expression() const { return *falseExpression; }

private:
  std::unique_ptr<Expression> condition;
  std::unique_ptr<Expression> trueExpression;
  std::unique_ptr<Expression> falseExpression;
};

/**
 * @brief Assignment expression (left = right, left += right, etc.)
 *
 * EBNF: assignment_expression = ternary_expression [ assignment_operator
 * assignment_expression ]
 */
class AssignmentExpression : public Expression {
public:
  using Operator =
      BinaryExpression::Operator; // Reuse binary operators for assignment

  explicit AssignmentExpression(Operator op, std::unique_ptr<Expression> left,
                                std::unique_ptr<Expression> right)
      : Expression(NodeType::ASSIGNMENT_EXPRESSION), operator_(op),
        left(std::move(left)), right(std::move(right)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override { return "AssignmentExpression"; }

  std::string get_expression_type() const override { return "assignment"; }

  Operator get_operator() const { return operator_; }
  const Expression &get_left() const { return *left; }
  const Expression &get_right() const { return *right; }

private:
  Operator operator_;
  std::unique_ptr<Expression> left;
  std::unique_ptr<Expression> right;
};

} // namespace ast
} // namespace nugdev