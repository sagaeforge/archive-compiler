#include <04_parsing/ast/core/AST.hpp>

namespace nugdev {
namespace ast {

// Parameter implementations
void Parameter::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Parameter::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<Parameter &>(*this));
}

// ArgumentList implementations
void ArgumentList::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ArgumentList::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ArgumentList &>(*this));
}

// StructField implementations
void StructField::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void StructField::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<StructField &>(*this));
}

} // namespace ast
} // namespace nugdev