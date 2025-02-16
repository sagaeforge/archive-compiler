#pragma once

#include <memory>

namespace nugdev::compiler::ast {

class ASTNode;

class ASTNodeVisitor {
  public:
    virtual std::shared_ptr<ASTNode> visit(const std::shared_ptr<ASTNode> &node) = 0;
};

} // namespace nugdev::compiler::ast
