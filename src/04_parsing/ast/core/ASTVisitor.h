#pragma once

#include <04_parsing/ast/core/ASTNode.hpp>

namespace nugdev {
namespace ast {

// Forward declarations for all AST node types
class Program;
class Module;

// Statements
class Statement;
class VariableDeclaration;
class FunctionDeclaration;
class StructDeclaration;
class InterfaceDeclaration;
class ExpressionStatement;

// Control Flow
class IfStatement;
class ForStatement;
class BreakStatement;
class ContinueStatement;
class ReturnStatement;

// Expressions
class Expression;
class BinaryExpression;
class UnaryExpression;
class PostfixExpression;
class AssignmentExpression;
class TernaryExpression;

// Primary Expressions
class Identifier;
class BlockExpression;
class IfExpression;
class WhenExpression;
class FunctionExpression;
class LambdaExpression;

// Literals
class Literal;
class NumberLiteral;
class StringLiteral;
class CharacterLiteral;
class BooleanLiteral;
class NullLiteral;
class NoneLiteral;
class RangeLiteral;
class ArrayLiteral;
class ObjectLiteral;

// Type Literals
class TypeLiteral;
class FunctionType;
class OptionalType;
class TupleType;

// Import/Export
class ImportStatement;
class ExportStatement;

// Parameters and Arguments
class Parameter;
class ArgumentList;

// Properties and Fields
class StructField;
class ObjectProperty;

// Casting
class CastExpression;

// Collection Operations
class ArrayComprehension;

// Template Expressions
class TemplateExpression;

// When Conditions
class WhenCondition;
class ValueCondition;
class RangeCondition;
class TypeCondition;
class GuardCondition;
class MultipleCondition;

/**
 * @brief Abstract base class for AST visitors
 *
 * Uses the visitor pattern to traverse and process AST nodes.
 * Derived classes should implement the visit methods for the
 * node types they need to handle.
 */
class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  // Program Structure
  virtual void visit(Program &node) = 0;
  virtual void visit(Module &node) = 0;

  // Statements
  virtual void visit(VariableDeclaration &node) = 0;
  virtual void visit(FunctionDeclaration &node) = 0;
  virtual void visit(StructDeclaration &node) = 0;
  virtual void visit(InterfaceDeclaration &node) = 0;
  virtual void visit(ExpressionStatement &node) = 0;

  // Control Flow Statements
  virtual void visit(IfStatement &node) = 0;
  virtual void visit(ForStatement &node) = 0;
  virtual void visit(BreakStatement &node) = 0;
  virtual void visit(ContinueStatement &node) = 0;
  virtual void visit(ReturnStatement &node) = 0;

  // Expressions
  virtual void visit(BinaryExpression &node) = 0;
  virtual void visit(UnaryExpression &node) = 0;
  virtual void visit(PostfixExpression &node) = 0;
  virtual void visit(AssignmentExpression &node) = 0;
  virtual void visit(TernaryExpression &node) = 0;

  // Primary Expressions
  virtual void visit(Identifier &node) = 0;
  virtual void visit(BlockExpression &node) = 0;
  virtual void visit(IfExpression &node) = 0;
  virtual void visit(WhenExpression &node) = 0;
  virtual void visit(FunctionExpression &node) = 0;
  virtual void visit(LambdaExpression &node) = 0;

  // Literals
  virtual void visit(NumberLiteral &node) = 0;
  virtual void visit(StringLiteral &node) = 0;
  virtual void visit(CharacterLiteral &node) = 0;
  virtual void visit(BooleanLiteral &node) = 0;
  virtual void visit(NullLiteral &node) = 0;
  virtual void visit(NoneLiteral &node) = 0;
  virtual void visit(RangeLiteral &node) = 0;
  virtual void visit(ArrayLiteral &node) = 0;
  virtual void visit(ObjectLiteral &node) = 0;

  // Type Literals
  virtual void visit(TypeLiteral &node) = 0;
  virtual void visit(FunctionType &node) = 0;
  virtual void visit(OptionalType &node) = 0;
  virtual void visit(TupleType &node) = 0;

  // Import/Export
  virtual void visit(ImportStatement &node) = 0;
  virtual void visit(ExportStatement &node) = 0;

  // Parameters and Arguments
  virtual void visit(Parameter &node) = 0;
  virtual void visit(ArgumentList &node) = 0;

  // Properties and Fields
  virtual void visit(StructField &node) = 0;
  virtual void visit(ObjectProperty &node) = 0;

  // Casting
  virtual void visit(CastExpression &node) = 0;

  // Collection Operations
  virtual void visit(ArrayComprehension &node) = 0;

  // Template Expressions
  virtual void visit(TemplateExpression &node) = 0;

  // When Conditions
  virtual void visit(WhenCondition &node) = 0;
  virtual void visit(ValueCondition &node) = 0;
  virtual void visit(RangeCondition &node) = 0;
  virtual void visit(TypeCondition &node) = 0;
  virtual void visit(GuardCondition &node) = 0;
  virtual void visit(MultipleCondition &node) = 0;
};

/**
 * @brief Abstract base class for const AST visitors
 *
 * Similar to ASTVisitor but for read-only operations on AST nodes.
 */
class ConstASTVisitor {
public:
  virtual ~ConstASTVisitor() = default;

  // Program Structure
  virtual void visit(const Program &node) = 0;
  virtual void visit(const Module &node) = 0;

  // Statements
  virtual void visit(const VariableDeclaration &node) = 0;
  virtual void visit(const FunctionDeclaration &node) = 0;
  virtual void visit(const StructDeclaration &node) = 0;
  virtual void visit(const InterfaceDeclaration &node) = 0;
  virtual void visit(const ExpressionStatement &node) = 0;

  // Control Flow Statements
  virtual void visit(const IfStatement &node) = 0;
  virtual void visit(const ForStatement &node) = 0;
  virtual void visit(const BreakStatement &node) = 0;
  virtual void visit(const ContinueStatement &node) = 0;
  virtual void visit(const ReturnStatement &node) = 0;

  // Expressions
  virtual void visit(const BinaryExpression &node) = 0;
  virtual void visit(const UnaryExpression &node) = 0;
  virtual void visit(const PostfixExpression &node) = 0;
  virtual void visit(const AssignmentExpression &node) = 0;
  virtual void visit(const TernaryExpression &node) = 0;

  // Primary Expressions
  virtual void visit(const Identifier &node) = 0;
  virtual void visit(const BlockExpression &node) = 0;
  virtual void visit(const IfExpression &node) = 0;
  virtual void visit(const WhenExpression &node) = 0;
  virtual void visit(const FunctionExpression &node) = 0;
  virtual void visit(const LambdaExpression &node) = 0;

  // Literals
  virtual void visit(const NumberLiteral &node) = 0;
  virtual void visit(const StringLiteral &node) = 0;
  virtual void visit(const CharacterLiteral &node) = 0;
  virtual void visit(const BooleanLiteral &node) = 0;
  virtual void visit(const NullLiteral &node) = 0;
  virtual void visit(const NoneLiteral &node) = 0;
  virtual void visit(const RangeLiteral &node) = 0;
  virtual void visit(const ArrayLiteral &node) = 0;
  virtual void visit(const ObjectLiteral &node) = 0;

  // Type Literals
  virtual void visit(const TypeLiteral &node) = 0;
  virtual void visit(const FunctionType &node) = 0;
  virtual void visit(const OptionalType &node) = 0;
  virtual void visit(const TupleType &node) = 0;

  // Import/Export
  virtual void visit(const ImportStatement &node) = 0;
  virtual void visit(const ExportStatement &node) = 0;

  // Parameters and Arguments
  virtual void visit(const Parameter &node) = 0;
  virtual void visit(const ArgumentList &node) = 0;

  // Properties and Fields
  virtual void visit(const StructField &node) = 0;
  virtual void visit(const ObjectProperty &node) = 0;

  // Casting
  virtual void visit(const CastExpression &node) = 0;

  // Collection Operations
  virtual void visit(const ArrayComprehension &node) = 0;

  // Template Expressions
  virtual void visit(const TemplateExpression &node) = 0;

  // When Conditions
  virtual void visit(const WhenCondition &node) = 0;
  virtual void visit(const ValueCondition &node) = 0;
  virtual void visit(const RangeCondition &node) = 0;
  virtual void visit(const TypeCondition &node) = 0;
  virtual void visit(const GuardCondition &node) = 0;
  virtual void visit(const MultipleCondition &node) = 0;
};

/**
 * @brief Base class for visitors that provide default empty implementations
 *
 * Useful for visitors that only need to handle specific node types.
 * Override only the methods you need.
 */
class DefaultASTVisitor : public ASTVisitor {
public:
  // Program Structure
  void visit(Program &) override {}
  void visit(Module &) override {}

  // Statements
  void visit(VariableDeclaration &) override {}
  void visit(FunctionDeclaration &) override {}
  void visit(StructDeclaration &) override {}
  void visit(InterfaceDeclaration &) override {}
  void visit(ExpressionStatement &) override {}

  // Control Flow Statements
  void visit(IfStatement &) override {}
  void visit(ForStatement &) override {}
  void visit(BreakStatement &) override {}
  void visit(ContinueStatement &) override {}
  void visit(ReturnStatement &) override {}

  // Expressions
  void visit(BinaryExpression &) override {}
  void visit(UnaryExpression &) override {}
  void visit(PostfixExpression &) override {}
  void visit(AssignmentExpression &) override {}
  void visit(TernaryExpression &) override {}

  // Primary Expressions
  void visit(Identifier &) override {}
  void visit(BlockExpression &) override {}
  void visit(IfExpression &) override {}
  void visit(WhenExpression &) override {}
  void visit(FunctionExpression &) override {}
  void visit(LambdaExpression &) override {}

  // Literals
  void visit(NumberLiteral &) override {}
  void visit(StringLiteral &) override {}
  void visit(CharacterLiteral &) override {}
  void visit(BooleanLiteral &) override {}
  void visit(NullLiteral &) override {}
  void visit(NoneLiteral &) override {}
  void visit(RangeLiteral &) override {}
  void visit(ArrayLiteral &) override {}
  void visit(ObjectLiteral &) override {}

  // Type Literals
  void visit(TypeLiteral &) override {}
  void visit(FunctionType &) override {}
  void visit(OptionalType &) override {}
  void visit(TupleType &) override {}

  // Import/Export
  void visit(ImportStatement &) override {}
  void visit(ExportStatement &) override {}

  // Parameters and Arguments
  void visit(Parameter &) override {}
  void visit(ArgumentList &) override {}

  // Properties and Fields
  void visit(StructField &) override {}
  void visit(ObjectProperty &) override {}

  // Casting
  void visit(CastExpression &) override {}

  // Collection Operations
  void visit(ArrayComprehension &) override {}

  // Template Expressions
  void visit(TemplateExpression &) override {}

  // When Conditions
  void visit(WhenCondition &) override {}
  void visit(ValueCondition &) override {}
  void visit(RangeCondition &) override {}
  void visit(TypeCondition &) override {}
  void visit(GuardCondition &) override {}
  void visit(MultipleCondition &) override {}
};

} // namespace ast
} // namespace nugdev