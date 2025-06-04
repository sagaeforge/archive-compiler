#include <04_parsing/ast/core/AST.hpp>

namespace nugdev {
namespace ast {

// ImportStatement implementations
void ImportStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ImportStatement::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ImportStatement &>(*this));
}

// ExportStatement implementations
void ExportStatement::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ExportStatement::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ExportStatement &>(*this));
}

} // namespace ast
} // namespace nugdev