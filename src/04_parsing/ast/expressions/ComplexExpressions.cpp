#include <04_parsing/ast/core/AST.hpp>

namespace nugdev {
namespace ast {

// BlockExpression implementations
void BlockExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void BlockExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<BlockExpression &>(*this));
}

// IfExpression implementations
void IfExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void IfExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<IfExpression &>(*this));
}

// WhenExpression implementations
void WhenExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void WhenExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<WhenExpression &>(*this));
}

// FunctionExpression implementations
void FunctionExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void FunctionExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<FunctionExpression &>(*this));
}

// LambdaExpression implementations
void LambdaExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void LambdaExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<LambdaExpression &>(*this));
}

} // namespace ast
} // namespace nugdev