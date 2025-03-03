#pragma once

#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeVisitor.h"
#include "04_generation/opcode/Opcode.h"

#include <functional>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace nugdev::compiler::generation {

class OpcodeVisitor : public ast::ASTNodeVisitor {
  public:
    using NodePredicate = std::function<bool(const ast::ASTNodePtr &)>;
    using NodeVisitor = std::function<std::any(const ast::ASTNodePtr &, const std::unordered_map<icu::UnicodeString, std::any> &)>;

  public:
    OpcodeVisitor();
    OpcodeVisitor(const std::vector<std::tuple<NodePredicate, NodeVisitor>> &strategies);

  public:
    std::any visit(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;

  public:
    // Module
    std::vector<Opcode> visitProgram(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context);

    // Expression

    // Statement
    std::vector<Opcode> visitExpressionStatement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context);

  private:
    std::vector<std::tuple<NodePredicate, NodeVisitor>> m_strategies;
};

} // namespace nugdev::compiler::generation