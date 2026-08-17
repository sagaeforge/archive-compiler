#pragma once

#include <04_parsing/ast/core/ASTNode.hpp>
#include <04_parsing/ast/expressions/Expressions.hpp>
#include <04_parsing/ast/types/Types.hpp>
#include <memory>
#include <string>
#include <vector>

namespace nugdev {
namespace ast {

// Forward declarations
class ASTVisitor;
class Expression;
class TypeLiteral;
class Identifier;

/**
 * @brief Type casting expression
 *
 * EBNF: cast_operator = "as" type_literal      (* unsafe cast *)
 *                     | "as?" type_literal ;   (* safe cast *)
 */
class CastExpression : public Expression {
public:
  enum class CastType {
    UNSAFE, // as
    SAFE    // as?
  };

  explicit CastExpression(std::unique_ptr<Expression> expression,
                          std::unique_ptr<TypeLiteral> targetType,
                          CastType castType)
      : Expression(NodeType::CAST_EXPRESSION),
        expression(std::move(expression)), targetType(std::move(targetType)),
        castType(castType) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    std::string opStr = (castType == CastType::SAFE) ? " as? " : " as ";
    return "CastExpression(" + expression->to_string() + opStr +
           targetType->to_string() + ")";
  }

  std::string get_expression_type() const override { return "cast"; }

  const Expression &get_expression() const { return *expression; }
  const TypeLiteral &get_target_type() const { return *targetType; }
  CastType get_cast_type() const { return castType; }

  bool is_safe_cast() const { return castType == CastType::SAFE; }

private:
  std::unique_ptr<Expression> expression;
  std::unique_ptr<TypeLiteral> targetType;
  CastType castType;
};

/**
 * @brief Array comprehension expression
 *
 * EBNF: array_comprehension = "[" expression "for" identifier "in" expression [
 * "if" expression ] "]" ;
 */
class ArrayComprehension : public Expression {
public:
  explicit ArrayComprehension(
      std::unique_ptr<Expression> elementExpression,
      const std::string &iteratorVariable,
      std::unique_ptr<Expression> iterableExpression,
      std::unique_ptr<Expression> filterExpression = nullptr)
      : Expression(NodeType::ARRAY_COMPREHENSION),
        elementExpression(std::move(elementExpression)),
        iteratorVariable(iteratorVariable),
        iterableExpression(std::move(iterableExpression)),
        filterExpression(std::move(filterExpression)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    std::string result =
        "ArrayComprehension([" + elementExpression->to_string() + " for " +
        iteratorVariable + " in " + iterableExpression->to_string();
    if (filterExpression) {
      result += " if " + filterExpression->to_string();
    }
    result += "])";
    return result;
  }

  std::string get_expression_type() const override {
    return "array_comprehension";
  }

  const Expression &get_element_expression() const {
    return *elementExpression;
  }
  const std::string &get_iterator_variable() const { return iteratorVariable; }
  const Expression &get_iterable_expression() const {
    return *iterableExpression;
  }

  bool has_filter() const { return filterExpression != nullptr; }
  const Expression *get_filter_expression() const {
    return filterExpression.get();
  }

private:
  std::unique_ptr<Expression>
      elementExpression;        // Expression to generate elements
  std::string iteratorVariable; // Iterator variable name
  std::unique_ptr<Expression> iterableExpression; // Expression to iterate over
  std::unique_ptr<Expression> filterExpression;   // Optional filter condition
};

/**
 * @brief Template expression for string interpolation
 *
 * EBNF: template_expression = "${" expression "}" ;
 * Note: This is used within template strings
 */
class TemplateExpression : public Expression {
public:
  explicit TemplateExpression(std::unique_ptr<Expression> expression)
      : Expression(NodeType::TEMPLATE_EXPRESSION),
        expression(std::move(expression)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "TemplateExpression(${" + expression->to_string() + "})";
  }

  std::string get_expression_type() const override { return "template"; }

  const Expression &get_expression() const { return *expression; }

private:
  std::unique_ptr<Expression> expression; // Expression to interpolate
};

} // namespace ast
} // namespace nugdev