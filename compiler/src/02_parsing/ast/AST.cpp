#include "02_parsing/ast/AST.h"

#include "02_parsing/ast/ASTNodeVisitor.h"

namespace nugdev::compiler::ast {

std::shared_ptr<ASTNode> ASTNode::accept(std::shared_ptr<ASTNodeVisitor> &visitor) { return visitor->visit(self()); }

} // namespace nugdev::compiler::ast
