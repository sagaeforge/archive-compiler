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

/**
 * @brief Function parameter
 *
 * EBNF: parameter = ( "let" | "mut" ) identifier ":" type_literal [ "="
 * expression ] ;
 */
class Parameter : public ASTNode {
public:
  enum class Mutability {
    LET, // Immutable parameter
    MUT  // Mutable parameter
  };

  explicit Parameter(Mutability mutability, const std::string &name,
                     std::unique_ptr<TypeLiteral> type,
                     std::unique_ptr<Expression> defaultValue = nullptr)
      : ASTNode(NodeType::PARAMETER), mutability(mutability),
        parameterName(name), type(std::move(type)),
        defaultValue(std::move(defaultValue)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    std::string mutStr = (mutability == Mutability::MUT) ? "mut " : "";
    return "Parameter(" + mutStr + parameterName + ":" + type->get_type_name() +
           ")";
  }

  Mutability get_mutability() const { return mutability; }
  const std::string &get_parameter_name() const { return parameterName; }
  const TypeLiteral &get_type() const { return *type; }

  bool has_default_value() const { return defaultValue != nullptr; }
  const Expression *get_default_value() const { return defaultValue.get(); }

  void set_default_value(std::unique_ptr<Expression> value) {
    defaultValue = std::move(value);
  }

private:
  Mutability mutability;
  std::string parameterName;
  std::unique_ptr<TypeLiteral> type;
  std::unique_ptr<Expression> defaultValue; // Optional default value
};

/**
 * @brief Argument list for function calls
 *
 * EBNF: argument_list = expression { "," expression } ;
 */
class ArgumentList : public ASTNode {
public:
  explicit ArgumentList() : ASTNode(NodeType::ARGUMENT_LIST) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "ArgumentList(" + std::to_string(arguments.size()) + " arguments)";
  }

  // Argument management
  void add_argument(std::unique_ptr<Expression> argument) {
    arguments.push_back(std::move(argument));
  }

  const std::vector<std::unique_ptr<Expression>> &get_arguments() const {
    return arguments;
  }

  size_t get_argument_count() const { return arguments.size(); }
  bool is_empty() const { return arguments.empty(); }

private:
  std::vector<std::unique_ptr<Expression>> arguments;
};

/**
 * @brief Struct field definition
 *
 * EBNF: struct_field = identifier ":" type_literal [ "=" expression ] ;
 */
class StructField : public ASTNode {
public:
  explicit StructField(const std::string &name,
                       std::unique_ptr<TypeLiteral> type,
                       std::unique_ptr<Expression> defaultValue = nullptr)
      : ASTNode(NodeType::STRUCT_FIELD), fieldName(name), type(std::move(type)),
        defaultValue(std::move(defaultValue)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "StructField(" + fieldName + ":" + type->get_type_name() + ")";
  }

  const std::string &get_field_name() const { return fieldName; }
  const TypeLiteral &get_type() const { return *type; }

  bool has_default_value() const { return defaultValue != nullptr; }
  const Expression *get_default_value() const { return defaultValue.get(); }

  void set_default_value(std::unique_ptr<Expression> value) {
    defaultValue = std::move(value);
  }

private:
  std::string fieldName;
  std::unique_ptr<TypeLiteral> type;
  std::unique_ptr<Expression> defaultValue; // Optional default value
};

} // namespace ast
} // namespace nugdev