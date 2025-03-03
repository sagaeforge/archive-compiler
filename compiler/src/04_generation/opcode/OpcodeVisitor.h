#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeVisitor.h"

namespace nugdev::compiler::generation {

class OpcodeVisitor : public ast::ASTNodeVisitor {
  public:
    // Module
    std::any visit_program(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;

    // Expression

    // Statement
    std::any visit_expression_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
};

} // namespace nugdev::compiler::generation