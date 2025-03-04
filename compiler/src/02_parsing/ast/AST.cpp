#include "02_parsing/ast/AST.h"

#include "02_parsing/ast/ASTNodeVisitor.h"

namespace nugdev::compiler::ast {

std::any ASTNode::accept(const std::shared_ptr<ASTNodeVisitor> &visitor, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    return visitor->visit(self(), context);
}

} // namespace nugdev::compiler::ast
