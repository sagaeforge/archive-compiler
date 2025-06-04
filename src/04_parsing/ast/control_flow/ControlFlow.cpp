#include <04_parsing/ast/core/AST.hpp>

namespace nugdev {
namespace ast {

// IfStatement implementations
void IfStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void IfStatement::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<IfStatement &>(*this));
}

// ForStatement implementations
void ForStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ForStatement::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ForStatement &>(*this));
}

// BreakStatement implementations
void BreakStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void BreakStatement::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<BreakStatement &>(*this));
}

// ContinueStatement implementations
void ContinueStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ContinueStatement::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ContinueStatement &>(*this));
}

// ReturnStatement implementations
void ReturnStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ReturnStatement::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ReturnStatement &>(*this));
}

} // namespace ast
} // namespace nugdev