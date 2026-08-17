#include "AdvancedExpressions.hpp"
#include <04_parsing/ast/core/AST.hpp>
#include <04_parsing/ast/core/ASTVisitor.h>

namespace nugdev {
namespace ast {

// LambdaExpression implementations (if defined in AdvancedExpressions.hpp)
// Note: If this file doesn't exist or is empty, this can be left minimal

// CastExpression implementations
void CastExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void CastExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<CastExpression &>(*this));
}

// ArrayComprehension implementations
void ArrayComprehension::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ArrayComprehension::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ArrayComprehension &>(*this));
}

// TemplateExpression implementations
void TemplateExpression::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void TemplateExpression::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<TemplateExpression &>(*this));
}

} // namespace ast
} // namespace nugdev