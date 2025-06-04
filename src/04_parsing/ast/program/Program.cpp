#include <04_parsing/ast/core/AST.hpp>

namespace nugdev {
namespace ast {

// Program implementations
void Program::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Program::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<Program &>(*this));
}

// Module implementations
void Module::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void Module::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<Module &>(*this));
}

} // namespace ast
} // namespace nugdev