#pragma once

#include <any>
#include <unicode/unistr.h>
#include <unordered_map>

namespace nugdev::compiler::ast {

class ASTNode;

class ASTNodeVisitor {
  public:
    virtual std::any visit(const std::shared_ptr<ASTNode> &node, const std::unordered_map<icu::UnicodeString, std::any> &context) = 0;
};

} // namespace nugdev::compiler::ast
