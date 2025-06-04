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
 * @brief Base class for all statements
 *
 * Statements do not return values and are executed for their side effects.
 */
class Statement : public ASTNode {
public:
  explicit Statement(NodeType type) : ASTNode(type) {}

  virtual ~Statement() = default;

  // Labeled statements support
  void set_label(const std::string &label) { this->label = label; }
  const std::string &get_label() const { return label; }
  bool has_label() const { return !label.empty(); }

private:
  std::string label; // Optional label for break/continue
};

/**
 * @brief Variable declaration statement
 *
 * EBNF: variable_declaration = ( "let" | "mut" | "const" | "var" | "final"
 * ) identifier ":" type_literal "=" expression [ ";" ] ;
 */
class VariableDeclaration : public Statement {
public:
  enum class Mutability {
    LET, // Immutable
    MUT  // Mutable
  };

  explicit VariableDeclaration(
      Mutability mutability, const std::string &name,
      std::unique_ptr<TypeLiteral> type = nullptr,
      std::unique_ptr<Expression> initializer = nullptr)
      : Statement(NodeType::VARIABLE_DECLARATION), mutability(mutability),
        variableName(name), type(std::move(type)),
        initializer(std::move(initializer)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    std::string mutStr = (mutability == Mutability::MUT) ? "mut " : "let ";
    return "VariableDeclaration(" + mutStr + variableName + ")";
  }

  Mutability get_mutability() const { return mutability; }
  const std::string &get_variable_name() const { return variableName; }

  bool has_type() const { return type != nullptr; }
  const TypeLiteral *get_type() const { return type.get(); }

  bool has_initializer() const { return initializer != nullptr; }
  const Expression *get_initializer() const { return initializer.get(); }

  void set_type(std::unique_ptr<TypeLiteral> newType) {
    type = std::move(newType);
  }
  void set_initializer(std::unique_ptr<Expression> init) {
    initializer = std::move(init);
  }

private:
  Mutability mutability;
  std::string variableName;
  std::unique_ptr<TypeLiteral> type;       // Optional type annotation
  std::unique_ptr<Expression> initializer; // Optional initializer
};

// Forward declarations for function-related classes
class Parameter;
class BlockExpression;

/**
 * @brief Function declaration statement
 *
 * EBNF: function_declaration = "fun" identifier "(" [ parameter_list ] ")" ":"
 * type_literal function_body ; function_body = "{" { statement } "}" | "="
 * expression [ ";" ] ;
 */
class FunctionDeclaration : public Statement {
public:
  enum class BodyType {
    BLOCK,     // { statements }
    EXPRESSION // = expression
  };

  explicit FunctionDeclaration(
      const std::string &name,
      std::vector<std::unique_ptr<Parameter>> parameters,
      std::unique_ptr<TypeLiteral> returnType)
      : Statement(NodeType::FUNCTION_DECLARATION), functionName(name),
        parameters(std::move(parameters)), returnType(std::move(returnType)),
        bodyType(BodyType::BLOCK) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "FunctionDeclaration(" + functionName + ")";
  }

  const std::string &get_function_name() const { return functionName; }

  const std::vector<std::unique_ptr<Parameter>> &get_parameters() const {
    return parameters;
  }

  const TypeLiteral &get_return_type() const { return *returnType; }

  BodyType get_body_type() const { return bodyType; }

  // For block body
  void set_block_body(std::unique_ptr<BlockExpression> block) {
    blockBody = std::move(block);
    bodyType = BodyType::BLOCK;
  }

  const BlockExpression *get_block_body() const { return blockBody.get(); }

  // For expression body
  void set_expression_body(std::unique_ptr<Expression> expr) {
    expressionBody = std::move(expr);
    bodyType = BodyType::EXPRESSION;
  }

  const Expression *get_expression_body() const { return expressionBody.get(); }

private:
  std::string functionName;
  std::vector<std::unique_ptr<Parameter>> parameters;
  std::unique_ptr<TypeLiteral> returnType;

  BodyType bodyType;
  std::unique_ptr<BlockExpression> blockBody; // For block body
  std::unique_ptr<Expression> expressionBody; // For expression body
};

// Forward declaration for struct field
class StructField;

/**
 * @brief Struct declaration statement
 *
 * EBNF: struct_declaration = "struct" identifier "{" [ struct_field_list ] "}"
 * [ ";" ] ;
 */
class StructDeclaration : public Statement {
public:
  explicit StructDeclaration(const std::string &name,
                             std::vector<std::unique_ptr<StructField>> fields)
      : Statement(NodeType::STRUCT_DECLARATION), structName(name),
        fields(std::move(fields)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "StructDeclaration(" + structName + ", " +
           std::to_string(fields.size()) + " fields)";
  }

  const std::string &get_struct_name() const { return structName; }

  const std::vector<std::unique_ptr<StructField>> &get_fields() const {
    return fields;
  }

  size_t get_field_count() const { return fields.size(); }

  bool is_empty() const { return fields.empty(); }

private:
  std::string structName;
  std::vector<std::unique_ptr<StructField>> fields;
};

/**
 * @brief Interface declaration statement
 *
 * EBNF: interface_declaration = "interface" identifier "{" [
 * interface_member_list ] "}" [ ";" ] ;
 */
class InterfaceDeclaration : public Statement {
public:
  // Interface members can be properties or method signatures
  struct Member {
    enum class Type { PROPERTY, METHOD };

    Type memberType;
    std::string name;
    std::unique_ptr<TypeLiteral> type;
    std::vector<std::unique_ptr<Parameter>> parameters; // For methods only

    Member(Type type, const std::string &name,
           std::unique_ptr<TypeLiteral> memberType)
        : memberType(type), name(name), type(std::move(memberType)) {}
  };

  explicit InterfaceDeclaration(const std::string &name,
                                std::vector<std::unique_ptr<Member>> members)
      : Statement(NodeType::INTERFACE_DECLARATION), interfaceName(name),
        members(std::move(members)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "InterfaceDeclaration(" + interfaceName + ", " +
           std::to_string(members.size()) + " members)";
  }

  const std::string &get_interface_name() const { return interfaceName; }

  const std::vector<std::unique_ptr<Member>> &get_members() const {
    return members;
  }

  size_t get_member_count() const { return members.size(); }

  bool is_empty() const { return members.empty(); }

private:
  std::string interfaceName;
  std::vector<std::unique_ptr<Member>> members;
};

/**
 * @brief Expression statement (expression used as statement)
 *
 * EBNF: expression_statement = expression [ ";" ] ;
 */
class ExpressionStatement : public Statement {
public:
  explicit ExpressionStatement(std::unique_ptr<Expression> expression)
      : Statement(NodeType::EXPRESSION_STATEMENT),
        expression(std::move(expression)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "ExpressionStatement(" + expression->to_string() + ")";
  }

  const Expression &get_expression() const { return *expression; }

private:
  std::unique_ptr<Expression> expression;
};

} // namespace ast
} // namespace nugdev