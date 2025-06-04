#pragma once

/**
 * @file AST.hpp
 * @brief Comprehensive AST system for the nugdev programming language
 *
 * This header provides a complete Abstract Syntax Tree implementation
 * based on the nugdev language EBNF grammar. It includes:
 *
 * - Base AST node infrastructure with visitor pattern
 * - Type system (TypeLiteral, FunctionType, OptionalType, TupleType)
 * - Literals (NumberLiteral, StringLiteral, BooleanLiteral, etc.)
 * - Expressions (BinaryExpression, UnaryExpression, PostfixExpression, etc.)
 * - Memory-safe smart pointer usage
 *
 * Usage example:
 * @code
 * using namespace nugdev::ast;
 *
 * // Create a number literal
 * auto numberLit = ASTFactory::create_number_literal("42",
 * NumberLiteral::NumberType::DECIMAL_INTEGER);
 *
 * // Create an identifier
 * auto identifier = ASTFactory::create_identifier("x");
 *
 * // Create a binary expression: x + 42
 * auto binaryExpr = ASTFactory::create_binary_expression(
 *     BinaryExpression::Operator::ADD,
 *     std::move(identifier),
 *     std::move(numberLit)
 * );
 * @endcode
 */

#include <algorithm>
#include <sstream>

// Core AST components
#include <04_parsing/ast/core/ASTNode.hpp>
#include <04_parsing/ast/core/ASTVisitor.hpp>

// Type system
#include <04_parsing/ast/types/Parameters.hpp>
#include <04_parsing/ast/types/Types.hpp>

// Expressions
#include <04_parsing/ast/expressions/AdvancedExpressions.hpp>
#include <04_parsing/ast/expressions/ComplexExpressions.hpp>
#include <04_parsing/ast/expressions/Expressions.hpp>

// Literals
#include <04_parsing/ast/literals/Literals.hpp>

// Control flow and statements
#include <04_parsing/ast/control_flow/ControlFlow.hpp>
#include <04_parsing/ast/control_flow/WhenConditions.hpp>
#include <04_parsing/ast/statements/Statements.hpp>

// Program structure
#include <04_parsing/ast/program/ImportExport.hpp>
#include <04_parsing/ast/program/Program.hpp>

namespace nugdev {
namespace ast {

/**
 * @brief Main AST namespace containing all AST node types and utilities
 *
 * This namespace provides:
 * - All AST node classes following the nugdev EBNF grammar
 * - Visitor pattern implementation for tree traversal
 * - Type-safe node creation utilities
 */

/**
 * @brief Factory class for creating AST nodes
 *
 * Provides convenient static methods to create various AST node types
 * with proper initialization and type safety.
 */
class ASTFactory {
public:
  // Program structure
  static std::unique_ptr<Program> create_program() {
    return std::make_unique<Program>();
  }

  static std::unique_ptr<Module> create_module(const std::string &name = "") {
    return std::make_unique<Module>(name);
  }

  // Literals - with correct constructors
  static std::unique_ptr<NumberLiteral>
  create_number_literal(const std::string &value,
                        NumberLiteral::NumberType type) {
    return std::make_unique<NumberLiteral>(value, type);
  }

  static std::unique_ptr<StringLiteral> create_string_literal(
      const std::string &value,
      StringLiteral::StringType type = StringLiteral::StringType::SIMPLE) {
    return std::make_unique<StringLiteral>(value, type);
  }

  static std::unique_ptr<BooleanLiteral> create_boolean_literal(bool value) {
    return std::make_unique<BooleanLiteral>(value);
  }

  static std::unique_ptr<NullLiteral> create_null_literal() {
    return std::make_unique<NullLiteral>();
  }

  static std::unique_ptr<ArrayLiteral>
  create_array_literal(std::vector<std::unique_ptr<Expression>> elements) {
    return std::make_unique<ArrayLiteral>(std::move(elements));
  }

  static std::unique_ptr<ObjectLiteral> create_object_literal(
      std::vector<std::unique_ptr<ObjectProperty>> properties) {
    return std::make_unique<ObjectLiteral>(std::move(properties));
  }

  // Expressions
  static std::unique_ptr<Identifier>
  create_identifier(const std::string &name) {
    return std::make_unique<Identifier>(name);
  }

  static std::unique_ptr<BinaryExpression>
  create_binary_expression(BinaryExpression::Operator op,
                           std::unique_ptr<Expression> left,
                           std::unique_ptr<Expression> right) {
    return std::make_unique<BinaryExpression>(op, std::move(left),
                                              std::move(right));
  }

  static std::unique_ptr<UnaryExpression>
  create_unary_expression(UnaryExpression::Operator op,
                          std::unique_ptr<Expression> operand) {
    return std::make_unique<UnaryExpression>(op, std::move(operand));
  }

  static std::unique_ptr<PostfixExpression>
  create_postfix_expression(PostfixExpression::OperatorType type,
                            std::unique_ptr<Expression> operand) {
    return std::make_unique<PostfixExpression>(type, std::move(operand));
  }

  static std::unique_ptr<BlockExpression> create_block_expression() {
    return std::make_unique<BlockExpression>();
  }

  // Types
  static std::unique_ptr<SimpleType>
  create_simple_type(const std::string &name) {
    return std::make_unique<SimpleType>(name);
  }

  static std::unique_ptr<FunctionType>
  create_function_type(std::vector<std::unique_ptr<TypeLiteral>> paramTypes,
                       std::unique_ptr<TypeLiteral> returnType) {
    return std::make_unique<FunctionType>(std::move(paramTypes),
                                          std::move(returnType));
  }

  static std::unique_ptr<OptionalType>
  create_optional_type(std::unique_ptr<TypeLiteral> innerType) {
    return std::make_unique<OptionalType>(std::move(innerType));
  }

  // Parameters
  static std::unique_ptr<Parameter>
  create_parameter(Parameter::Mutability mutability, const std::string &name,
                   std::unique_ptr<TypeLiteral> type,
                   std::unique_ptr<Expression> defaultValue = nullptr) {
    return std::make_unique<Parameter>(mutability, name, std::move(type),
                                       std::move(defaultValue));
  }

  // Statements
  static std::unique_ptr<ExpressionStatement>
  create_expression_statement(std::unique_ptr<Expression> expression) {
    return std::make_unique<ExpressionStatement>(std::move(expression));
  }

  static std::unique_ptr<VariableDeclaration> create_variable_declaration(
      VariableDeclaration::Mutability mutability, const std::string &name,
      std::unique_ptr<TypeLiteral> type = nullptr,
      std::unique_ptr<Expression> initializer = nullptr) {
    return std::make_unique<VariableDeclaration>(
        mutability, name, std::move(type), std::move(initializer));
  }

  static std::unique_ptr<FunctionDeclaration> create_function_declaration(
      const std::string &name,
      std::vector<std::unique_ptr<Parameter>> parameters,
      std::unique_ptr<TypeLiteral> returnType = nullptr) {
    return std::make_unique<FunctionDeclaration>(name, std::move(parameters),
                                                 std::move(returnType));
  }

  // Control flow
  static std::unique_ptr<BreakStatement>
  create_break_statement(const std::string &label = "") {
    return std::make_unique<BreakStatement>(label);
  }

  static std::unique_ptr<ContinueStatement>
  create_continue_statement(const std::string &label = "") {
    return std::make_unique<ContinueStatement>(label);
  }

  static std::unique_ptr<ReturnStatement>
  create_return_statement(std::unique_ptr<Expression> expression = nullptr,
                          const std::string &label = "") {
    return std::make_unique<ReturnStatement>(std::move(expression), label);
  }

  static std::unique_ptr<IfStatement> create_if_statement() {
    return std::make_unique<IfStatement>();
  }

  static std::unique_ptr<ForStatement>
  create_for_statement(ForStatement::ForType type) {
    return std::make_unique<ForStatement>(type);
  }

  // When conditions
  static std::unique_ptr<ValueCondition>
  create_value_condition(std::unique_ptr<Expression> value) {
    return std::make_unique<ValueCondition>(std::move(value));
  }

  static std::unique_ptr<RangeCondition>
  create_range_condition(std::unique_ptr<Expression> value,
                         std::unique_ptr<Expression> range) {
    return std::make_unique<RangeCondition>(std::move(value), std::move(range));
  }

  static std::unique_ptr<TypeCondition>
  create_type_condition(std::unique_ptr<Expression> value,
                        std::unique_ptr<TypeLiteral> type) {
    return std::make_unique<TypeCondition>(std::move(value), std::move(type));
  }

  static std::unique_ptr<GuardCondition>
  create_guard_condition(std::unique_ptr<WhenCondition> baseCondition,
                         std::unique_ptr<Expression> guardExpression) {
    return std::make_unique<GuardCondition>(std::move(baseCondition),
                                            std::move(guardExpression));
  }

  static std::unique_ptr<MultipleCondition> create_multiple_condition() {
    return std::make_unique<MultipleCondition>();
  }
};

/**
 * @brief Utility class for AST operations
 */
class ASTUtils {
public:
  // Tree traversal utilities
  template <typename Visitor>
  static void traverse_depth_first(ASTNode &root, Visitor &visitor) {
    root.accept(visitor);
  }

  // Template-based type checking utilities
  template <typename T> static bool is_node_type(const ASTNode &node) {
    return node.is<T>();
  }

  template <typename T> static const T *as_node_type(const ASTNode &node) {
    return node.as<T>();
  }

  template <typename T> static T *as_node_type(ASTNode &node) {
    return node.as<T>();
  }

  // Type checking utilities
  static bool is_statement(const ASTNode &node) {
    NodeType type = node.get_node_type();
    return type == NodeType::VARIABLE_DECLARATION ||
           type == NodeType::FUNCTION_DECLARATION ||
           type == NodeType::STRUCT_DECLARATION ||
           type == NodeType::INTERFACE_DECLARATION ||
           type == NodeType::EXPRESSION_STATEMENT ||
           type == NodeType::IF_STATEMENT || type == NodeType::FOR_STATEMENT ||
           type == NodeType::BREAK_STATEMENT ||
           type == NodeType::CONTINUE_STATEMENT ||
           type == NodeType::RETURN_STATEMENT ||
           type == NodeType::IMPORT_STATEMENT ||
           type == NodeType::EXPORT_STATEMENT;
  }

  static bool is_expression(const ASTNode &node) {
    NodeType type = node.get_node_type();
    return type == NodeType::BINARY_EXPRESSION ||
           type == NodeType::UNARY_EXPRESSION ||
           type == NodeType::POSTFIX_EXPRESSION ||
           type == NodeType::ASSIGNMENT_EXPRESSION ||
           type == NodeType::TERNARY_EXPRESSION ||
           type == NodeType::IDENTIFIER || type == NodeType::BLOCK_EXPRESSION ||
           type == NodeType::IF_EXPRESSION ||
           type == NodeType::WHEN_EXPRESSION ||
           type == NodeType::FUNCTION_EXPRESSION ||
           type == NodeType::LAMBDA_EXPRESSION ||
           type == NodeType::CAST_EXPRESSION ||
           type == NodeType::ARRAY_COMPREHENSION ||
           type == NodeType::TEMPLATE_EXPRESSION || is_literal(node);
  }

  static bool is_literal(const ASTNode &node) {
    NodeType type = node.get_node_type();
    return type == NodeType::NUMBER_LITERAL ||
           type == NodeType::STRING_LITERAL ||
           type == NodeType::CHARACTER_LITERAL ||
           type == NodeType::BOOLEAN_LITERAL ||
           type == NodeType::NULL_LITERAL || type == NodeType::NONE_LITERAL ||
           type == NodeType::RANGE_LITERAL || type == NodeType::ARRAY_LITERAL ||
           type == NodeType::OBJECT_LITERAL;
  }

  static bool is_type_literal(const ASTNode &node) {
    NodeType type = node.get_node_type();
    return type == NodeType::TYPE_LITERAL || type == NodeType::FUNCTION_TYPE ||
           type == NodeType::OPTIONAL_TYPE || type == NodeType::TUPLE_TYPE;
  }

  // String conversion utilities
  static std::string node_type_to_string(NodeType type);
  static std::string to_json(const ASTNode &node);
};

/**
 * @brief Utility class for pretty-printing AST structures
 */
class ASTPrinter {
private:
  std::string result;
  int indentLevel = 0;

public:
  void print(const ASTNode &node) {
    result.clear();
    indentLevel = 0;
    visit_node(node);
  }

  std::string get_result() const { return result; }

private:
  void visit_node(const ASTNode &node) { append_line(node.to_string()); }

  void indent() { indentLevel++; }
  void dedent() { indentLevel--; }

  std::string get_indent() const { return std::string(indentLevel * 2, ' '); }
  void append_line(const std::string &line) {
    result += get_indent() + line + "\n";
  }
};

} // namespace ast
} // namespace nugdev

/**
 * @brief Documentation for nugdev AST system
 *
 * The nugdev AST system is designed to closely follow the EBNF grammar
 * specification. Key design principles:
 *
 * 1. **Type Safety**: All nodes use strong typing and smart pointers
 * 2. **Memory Safety**: RAII and unique_ptr prevent memory leaks
 * 3. **Visitor Pattern**: Enables extensible tree traversal and analysis
 * 4. **Immutability**: AST nodes are generally immutable after construction
 *
 * ## Node Hierarchy
 *
 * ```
 * ASTNode (abstract base)
 * ├── TypeLiteral (abstract)
 * │   ├── SimpleType
 * │   ├── FunctionType
 * │   ├── OptionalType
 * │   └── TupleType
 * ├── Expression (abstract)
 * │   ├── Identifier
 * │   ├── BinaryExpression
 * │   ├── UnaryExpression
 * │   ├── PostfixExpression
 * │   ├── TernaryExpression
 * │   ├── AssignmentExpression
 * │   └── Literal (abstract)
 * │       ├── NumberLiteral
 * │       ├── StringLiteral
 * │       ├── CharacterLiteral
 * │       ├── BooleanLiteral
 * │       ├── NullLiteral
 * │       ├── NoneLiteral
 * │       ├── RangeLiteral
 * │       ├── ArrayLiteral
 * │       └── ObjectLiteral
 * └── [Future: Statements, Declarations, etc.]
 * ```
 *
 * ## Usage Patterns
 *
 * ### Creating AST Nodes
 * ```cpp
 * // Method 1: Direct construction
 * auto node = std::make_unique<NumberLiteral>("42",
 * NumberLiteral::NumberType::DECIMAL_INTEGER);
 *
 * // Method 2: Using factory
 * auto node = ASTFactory::create_number_literal("42",
 * NumberLiteral::NumberType::DECIMAL_INTEGER);
 *
 * // Method 3: Using utility function
 * auto node = make_ast_node<NumberLiteral>("42",
 * NumberLiteral::NumberType::DECIMAL_INTEGER);
 * ```
 *
 * ### Traversing AST
 * ```cpp
 * class MyVisitor : public DefaultASTVisitor {
 * public:
 *     void visit(NumberLiteral& node) override {
 *         std::cout << "Found number: " << node.get_literal_value() <<
 * std::endl;
 *     }
 * };
 *
 * MyVisitor visitor;
 * astRoot->accept(visitor);
 * ```
 */