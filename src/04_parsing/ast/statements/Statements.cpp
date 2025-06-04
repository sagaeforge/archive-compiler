#include <04_parsing/ast/core/AST.hpp>

namespace nugdev {
namespace ast {

// VariableDeclaration implementations
void VariableDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void VariableDeclaration::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<VariableDeclaration &>(*this));
}

// FunctionDeclaration implementations
void FunctionDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void FunctionDeclaration::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<FunctionDeclaration &>(*this));
}

// StructDeclaration implementations
void StructDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void StructDeclaration::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<StructDeclaration &>(*this));
}

// InterfaceDeclaration implementations
void InterfaceDeclaration::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void InterfaceDeclaration::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<InterfaceDeclaration &>(*this));
}

// ExpressionStatement implementations
void ExpressionStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ExpressionStatement::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ExpressionStatement &>(*this));
}

} // namespace ast
} // namespace nugdev