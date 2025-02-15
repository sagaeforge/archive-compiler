#include "02_parsing/ast/AST.h"

#include "02_parsing/ast/ASTNodeVisitor.h"

namespace nugdev::compiler::ast {

void ASTNode::accept(std::shared_ptr<ASTNodeVisitor> &visitor) { visitor->visit(self()); }

} // namespace nugdev::compiler::ast
