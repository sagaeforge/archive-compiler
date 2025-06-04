#pragma once

#include <04_parsing/ast/core/ASTNode.hpp>
#include <04_parsing/ast/expressions/Expressions.hpp>
#include <memory>
#include <string>
#include <vector>

namespace nugdev {
namespace ast {

// Forward declarations
class ASTVisitor;
class ObjectProperty;

/**
 * @brief Base class for all literal values
 *
 * Literals are expressions that represent constant values
 */
class Literal : public Expression {
public:
  explicit Literal(NodeType type) : Expression(type) {}

  virtual ~Literal() = default;

  // All literals must provide their literal value as string
  virtual std::string get_literal_value() const = 0;

  std::string get_expression_type() const override { return "literal"; }
};

/**
 * @brief Number literals (integers and floating-point)
 *
 * EBNF: number_literal = decimal_integer | binary_integer | octal_integer |
 * hexadecimal_integer | floating_point ;
 */
class NumberLiteral : public Literal {
public:
  enum class NumberType {
    DECIMAL_INTEGER,
    BINARY_INTEGER,
    OCTAL_INTEGER,
    HEXADECIMAL_INTEGER,
    FLOATING_POINT
  };

  explicit NumberLiteral(const std::string &value, NumberType type)
      : Literal(NodeType::NUMBER_LITERAL), value(value), numberType(type) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "NumberLiteral(" + value + ")";
  }

  std::string get_literal_value() const override { return value; }

  NumberType get_number_type() const { return numberType; }

  // Utility methods for type checking
  bool is_integer() const { return numberType != NumberType::FLOATING_POINT; }

  bool is_floating_point() const {
    return numberType == NumberType::FLOATING_POINT;
  }

  // Convert to actual numeric values
  int64_t get_integer_value() const;
  double get_floating_point_value() const;

private:
  std::string value;
  NumberType numberType;
};

/**
 * @brief String literals (simple, raw, template)
 *
 * EBNF: string_literal = simple_string | raw_string | template_string ;
 */
class StringLiteral : public Literal {
public:
  enum class StringType {
    SIMPLE,  // "string"
    RAW,     // r"string"
    TEMPLATE // `string with ${expr}`
  };

  explicit StringLiteral(const std::string &value, StringType type)
      : Literal(NodeType::STRING_LITERAL), value(value), stringType(type) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "StringLiteral(\"" + value + "\")";
  }

  std::string get_literal_value() const override { return value; }

  StringType get_string_type() const { return stringType; }

  bool is_template() const { return stringType == StringType::TEMPLATE; }

  // For template strings, store template expressions
  void add_template_expression(std::unique_ptr<Expression> expr) {
    templateExpressions.push_back(std::move(expr));
  }

  const std::vector<std::unique_ptr<Expression>> &
  get_template_expressions() const {
    return templateExpressions;
  }

private:
  std::string value;
  StringType stringType;
  std::vector<std::unique_ptr<Expression>> templateExpressions;
};

/**
 * @brief Character literals
 *
 * EBNF: character_literal = "'" ( ESCAPE_SEQUENCE | ~( "'" | "\\" | NEWLINE ) )
 * "'" ;
 */
class CharacterLiteral : public Literal {
public:
  explicit CharacterLiteral(char value)
      : Literal(NodeType::CHARACTER_LITERAL), value(value) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "CharacterLiteral('" + std::string(1, value) + "')";
  }

  std::string get_literal_value() const override {
    return std::string(1, value);
  }

  char get_character_value() const { return value; }

private:
  char value;
};

/**
 * @brief Boolean literals (true, false)
 *
 * EBNF: boolean_literal = "true" | "false" ;
 */
class BooleanLiteral : public Literal {
public:
  explicit BooleanLiteral(bool value)
      : Literal(NodeType::BOOLEAN_LITERAL), value(value) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "BooleanLiteral(" + std::string(value ? "true" : "false") + ")";
  }

  std::string get_literal_value() const override {
    return value ? "true" : "false";
  }

  bool get_boolean_value() const { return value; }

private:
  bool value;
};

/**
 * @brief Null literal
 *
 * EBNF: null_literal = "null" ;
 */
class NullLiteral : public Literal {
public:
  explicit NullLiteral() : Literal(NodeType::NULL_LITERAL) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override { return "NullLiteral(null)"; }

  std::string get_literal_value() const override { return "null"; }
};

/**
 * @brief None literal
 *
 * EBNF: none_literal = "None" ;
 */
class NoneLiteral : public Literal {
public:
  explicit NoneLiteral() : Literal(NodeType::NONE_LITERAL) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override { return "NoneLiteral(None)"; }

  std::string get_literal_value() const override { return "None"; }
};

/**
 * @brief Range literals
 *
 * EBNF: range_literal = expression ".." [ expression ] ;
 */
class RangeLiteral : public Literal {
public:
  explicit RangeLiteral(std::unique_ptr<Expression> start,
                        std::unique_ptr<Expression> end = nullptr)
      : Literal(NodeType::RANGE_LITERAL), start(std::move(start)),
        end(std::move(end)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "RangeLiteral(" + start->to_string() + ".." +
           (end ? end->to_string() : "") + ")";
  }

  std::string get_literal_value() const override { return "range"; }

  const Expression &get_start() const { return *start; }
  const Expression *get_end() const { return end.get(); }
  bool has_end() const { return end != nullptr; }

private:
  std::unique_ptr<Expression> start;
  std::unique_ptr<Expression> end; // Optional for infinite ranges
};

/**
 * @brief Array literals
 *
 * EBNF: array_literal = "[" [ expression { "," expression } [ "," ] ] "]" ;
 */
class ArrayLiteral : public Literal {
public:
  explicit ArrayLiteral(std::vector<std::unique_ptr<Expression>> elements)
      : Literal(NodeType::ARRAY_LITERAL), elements(std::move(elements)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "ArrayLiteral([" + std::to_string(elements.size()) + " elements])";
  }

  std::string get_literal_value() const override { return "array"; }

  const std::vector<std::unique_ptr<Expression>> &get_elements() const {
    return elements;
  }

  size_t get_element_count() const { return elements.size(); }

  bool is_empty() const { return elements.empty(); }

private:
  std::vector<std::unique_ptr<Expression>> elements;
};

/**
 * @brief Object literals
 *
 * EBNF: object_literal = "{" [ object_property { "," object_property } [ ","
 * ] ] "}" ;
 */
class ObjectLiteral : public Literal {
public:
  explicit ObjectLiteral(
      std::vector<std::unique_ptr<ObjectProperty>> properties)
      : Literal(NodeType::OBJECT_LITERAL), properties(std::move(properties)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "ObjectLiteral({" + std::to_string(properties.size()) +
           " properties})";
  }

  std::string get_literal_value() const override { return "object"; }

  const std::vector<std::unique_ptr<ObjectProperty>> &get_properties() const {
    return properties;
  }

  size_t get_property_count() const { return properties.size(); }

  bool is_empty() const { return properties.empty(); }

private:
  std::vector<std::unique_ptr<ObjectProperty>> properties;
};

/**
 * @brief Object property (key-value pairs in object literals)
 *
 * EBNF: object_property = ( identifier | computed_property_name | string |
 * spread ) ":" expression | shorthand_property | spread_property ;
 */
class ObjectProperty : public ASTNode {
public:
  enum class PropertyType {
    NORMAL,    // key: value
    COMPUTED,  // [key]: value
    SHORTHAND, // identifier (same as identifier: identifier)
    SPREAD     // ...expression
  };

  explicit ObjectProperty(PropertyType type,
                          std::unique_ptr<Expression> key = nullptr,
                          std::unique_ptr<Expression> value = nullptr)
      : ASTNode(NodeType::OBJECT_PROPERTY), propertyType(type),
        key(std::move(key)), value(std::move(value)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override { return "ObjectProperty"; }

  PropertyType get_property_type() const { return propertyType; }

  const Expression *get_key() const { return key.get(); }
  const Expression *get_value() const { return value.get(); }

  void set_key(std::unique_ptr<Expression> newKey) { key = std::move(newKey); }
  void set_value(std::unique_ptr<Expression> newValue) {
    value = std::move(newValue);
  }

private:
  PropertyType propertyType;
  std::unique_ptr<Expression> key;   // For normal and computed properties
  std::unique_ptr<Expression> value; // Property value
};

} // namespace ast
} // namespace nugdev