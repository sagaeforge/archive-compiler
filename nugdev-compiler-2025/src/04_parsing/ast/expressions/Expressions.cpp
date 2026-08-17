#include <04_parsing/ast/core/ASTVisitor.h>
#include <04_parsing/ast/expressions/Expressions.hpp>

namespace nugdev {
namespace ast {

// Identifier implementations
void Identifier::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Identifier::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<Identifier &>(*this));
}

// BinaryExpression implementations
void BinaryExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void BinaryExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<BinaryExpression &>(*this));
}

std::string BinaryExpression::get_operator_string() const {
  switch (operator_) {
  case Operator::ASSIGN:
    return "=";
  case Operator::ADD_ASSIGN:
    return "+=";
  case Operator::SUB_ASSIGN:
    return "-=";
  case Operator::MUL_ASSIGN:
    return "*=";
  case Operator::DIV_ASSIGN:
    return "/=";
  case Operator::MOD_ASSIGN:
    return "%=";
  case Operator::BITWISE_AND_ASSIGN:
    return "&=";
  case Operator::BITWISE_OR_ASSIGN:
    return "|=";
  case Operator::BITWISE_XOR_ASSIGN:
    return "^=";
  case Operator::BITWISE_NOT_ASSIGN:
    return "~=";
  case Operator::ADD:
    return "+";
  case Operator::SUBTRACT:
    return "-";
  case Operator::MULTIPLY:
    return "*";
  case Operator::DIVIDE:
    return "/";
  case Operator::MODULO:
    return "%";
  case Operator::EQUAL:
    return "==";
  case Operator::NOT_EQUAL:
    return "!=";
  case Operator::LESS_THAN:
    return "<";
  case Operator::GREATER_THAN:
    return ">";
  case Operator::LESS_EQUAL:
    return "<=";
  case Operator::GREATER_EQUAL:
    return ">=";
  case Operator::LOGICAL_AND:
    return "and";
  case Operator::LOGICAL_OR:
    return "or";
  case Operator::BITWISE_AND:
    return "&";
  case Operator::BITWISE_OR:
    return "|";
  case Operator::BITWISE_XOR:
    return "^";
  case Operator::LEFT_SHIFT:
    return "<<";
  case Operator::RIGHT_SHIFT:
    return ">>";
  case Operator::NULL_COALESCING:
    return "??";
  case Operator::TYPE_CHECK:
    return "is";
  case Operator::RANGE:
    return "..";
  case Operator::IN:
    return "in";
  default:
    return "unknown";
  }
}

bool BinaryExpression::is_assignment_operator() const {
  return operator_ >= Operator::ASSIGN &&
         operator_ <= Operator::BITWISE_NOT_ASSIGN;
}

bool BinaryExpression::is_comparison_operator() const {
  return operator_ >= Operator::EQUAL && operator_ <= Operator::GREATER_EQUAL;
}

bool BinaryExpression::is_arithmetic_operator() const {
  return operator_ >= Operator::ADD && operator_ <= Operator::MODULO;
}

bool BinaryExpression::is_logical_operator() const {
  return operator_ == Operator::LOGICAL_AND ||
         operator_ == Operator::LOGICAL_OR;
}

bool BinaryExpression::is_bitwise_operator() const {
  return (operator_ >= Operator::BITWISE_AND &&
          operator_ <= Operator::RIGHT_SHIFT) ||
         (operator_ >= Operator::BITWISE_AND_ASSIGN &&
          operator_ <= Operator::BITWISE_NOT_ASSIGN);
}

// UnaryExpression implementations
void UnaryExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void UnaryExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<UnaryExpression &>(*this));
}

std::string UnaryExpression::get_operator_string() const {
  switch (operator_) {
  case Operator::PLUS:
    return "+";
  case Operator::MINUS:
    return "-";
  case Operator::LOGICAL_NOT:
    return "!";
  case Operator::LOGICAL_NOT_WORD:
    return "not";
  case Operator::BITWISE_NOT:
    return "~";
  case Operator::PRE_INCREMENT:
    return "++";
  case Operator::PRE_DECREMENT:
    return "--";
  default:
    return "unknown";
  }
}

// PostfixExpression implementations
void PostfixExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void PostfixExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<PostfixExpression &>(*this));
}

std::string PostfixExpression::get_operator_string() const {
  switch (operatorType) {
  case OperatorType::POST_INCREMENT:
    return "++";
  case OperatorType::POST_DECREMENT:
    return "--";
  case OperatorType::MEMBER_ACCESS:
    return ".";
  case OperatorType::SAFE_MEMBER_ACCESS:
    return "?.";
  case OperatorType::ARRAY_ACCESS:
    return "[]";
  case OperatorType::FUNCTION_CALL:
    return "()";
  case OperatorType::CAST:
    return safeCast ? "as?" : "as";
  default:
    return "unknown";
  }
}

// TernaryExpression implementations
void TernaryExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void TernaryExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<TernaryExpression &>(*this));
}

// AssignmentExpression implementations
void AssignmentExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void AssignmentExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<AssignmentExpression &>(*this));
}

} // namespace ast
} // namespace nugdev