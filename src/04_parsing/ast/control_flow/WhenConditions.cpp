#include <04_parsing/ast/core/AST.hpp>

namespace nugdev {
namespace ast {

// WhenCondition implementations - WhenCondition은 추상 클래스이므로 accept
// 메소드 제거

// ValueCondition implementations
void ValueCondition::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ValueCondition::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ValueCondition &>(*this));
}

// RangeCondition implementations
void RangeCondition::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void RangeCondition::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<RangeCondition &>(*this));
}

// TypeCondition implementations
void TypeCondition::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void TypeCondition::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<TypeCondition &>(*this));
}

// GuardCondition implementations
void GuardCondition::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void GuardCondition::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<GuardCondition &>(*this));
}

// MultipleCondition implementations
void MultipleCondition::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void MultipleCondition::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<MultipleCondition &>(*this));
}

} // namespace ast
} // namespace nugdev