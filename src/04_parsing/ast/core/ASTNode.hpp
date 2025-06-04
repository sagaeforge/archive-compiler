#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace nugdev {
namespace ast {

// Forward declarations
class ASTVisitor;

/**
 * @brief Enumeration of all AST node types
 */
enum class NodeType {
  // Program Structure
  PROGRAM,
  MODULE,

  // Statements
  VARIABLE_DECLARATION,
  FUNCTION_DECLARATION,
  STRUCT_DECLARATION,
  INTERFACE_DECLARATION,
  EXPRESSION_STATEMENT,

  // Control Flow Statements
  IF_STATEMENT,
  FOR_STATEMENT,
  BREAK_STATEMENT,
  CONTINUE_STATEMENT,
  RETURN_STATEMENT,

  // Expressions
  BINARY_EXPRESSION,
  UNARY_EXPRESSION,
  POSTFIX_EXPRESSION,
  ASSIGNMENT_EXPRESSION,
  TERNARY_EXPRESSION,

  // Primary Expressions
  IDENTIFIER,
  BLOCK_EXPRESSION,
  IF_EXPRESSION,
  WHEN_EXPRESSION,
  FUNCTION_EXPRESSION,
  LAMBDA_EXPRESSION,

  // Literals
  NUMBER_LITERAL,
  STRING_LITERAL,
  CHARACTER_LITERAL,
  BOOLEAN_LITERAL,
  NULL_LITERAL,
  NONE_LITERAL,
  RANGE_LITERAL,
  ARRAY_LITERAL,
  OBJECT_LITERAL,

  // Type Literals
  TYPE_LITERAL,
  FUNCTION_TYPE,
  OPTIONAL_TYPE,
  TUPLE_TYPE,

  // Import/Export
  IMPORT_STATEMENT,
  EXPORT_STATEMENT,

  // Parameters and Arguments
  PARAMETER,
  ARGUMENT_LIST,

  // Properties and Fields
  STRUCT_FIELD,
  OBJECT_PROPERTY,

  // Casting
  CAST_EXPRESSION,

  // Collection Operations
  ARRAY_COMPREHENSION,

  // Template Expressions
  TEMPLATE_EXPRESSION,

  // When Conditions
  VALUE_CONDITION,
  RANGE_CONDITION,
  TYPE_CONDITION,
  GUARD_CONDITION,
  MULTIPLE_CONDITION
};

/**
 * @brief Base class for all AST nodes
 */
class ASTNode {
public:
  explicit ASTNode(NodeType type) : nodeType(type) {}

  virtual ~ASTNode() = default;

  // Non-copyable but movable
  ASTNode(const ASTNode &) = delete;
  ASTNode &operator=(const ASTNode &) = delete;
  ASTNode(ASTNode &&) = default;
  ASTNode &operator=(ASTNode &&) = default;

  // Visitor pattern support
  virtual void accept(ASTVisitor &visitor) = 0;
  virtual void accept(ASTVisitor &visitor) const = 0;

  // Accessors
  NodeType get_node_type() const { return nodeType; }

  // Utility methods
  virtual std::string to_string() const = 0;

  // Type checking helpers
  template <typename T> bool is() const {
    return dynamic_cast<const T *>(this) != nullptr;
  }

  template <typename T> T *as() { return dynamic_cast<T *>(this); }

  template <typename T> const T *as() const {
    return dynamic_cast<const T *>(this);
  }

protected:
  NodeType nodeType;
};

/**
 * @brief Smart pointer type for AST nodes
 */
using ASTNodePtr = std::unique_ptr<ASTNode>;

/**
 * @brief Vector of AST nodes
 */
using ASTNodeList = std::vector<ASTNodePtr>;

/**
 * @brief Utility function to create AST nodes
 */
template <typename T, typename... Args>
std::unique_ptr<T> make_ast_node(Args &&...args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

} // namespace ast
} // namespace nugdev