#include "04_generation/opcode/OpcodeVisitor.h"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/module/program/ProgramNode.h"
#include "02_parsing/ast/statement/expression/ExpressionStatementNode.h"

#include <tuple>

namespace nugdev::compiler::generation {

OpcodeVisitor::OpcodeVisitor() {
    m_strategies = {{[this](const ast::ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::module::ProgramNode>(); },
                     [this](const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
                         return this->visitProgram(node, context);
                     }},
                    {[this](const ast::ASTNodePtr &node) -> bool { return node != nullptr && node->is<ast::statement::ExpressionStatementNode>(); },
                     [this](const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) -> std::any {
                         return this->visitExpressionStatement(node, context);
                     }}};
}
} // namespace nugdev::compiler::generation
