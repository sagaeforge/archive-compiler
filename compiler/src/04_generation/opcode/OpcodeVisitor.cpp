#include "04_generation/opcode/OpcodeVisitor.h"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/module/program/ProgramNode.h"
#include "02_parsing/ast/statement/block/BlockStatementNode.h"
#include "02_parsing/ast/statement/break/BreakNode.h"
#include "02_parsing/ast/statement/continue/ContinueNode.h"
#include "02_parsing/ast/statement/expression/ExpressionStatementNode.h"
#include "04_generation/opcode/Opcode.h"

#include <any>
#include <deque>
#include <tuple>
#include <unicode/unistr.h>

namespace nugdev::compiler::generation {

bool OpcodeVisitor::requires_context() const { return false; }

std::any OpcodeVisitor::visit_program(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    if (!node->is<ast::module::ProgramNode>()) {
        throw std::runtime_error("ProgramNode expected");
    }

    std::vector<Opcode> opcodes;
    auto programNode = node->as<ast::module::ProgramNode>();
    for (const auto &statement : programNode->get_statements()) {
        auto statementOpcodes = std::any_cast<std::vector<Opcode>>(statement->accept(self(), context));
        opcodes.insert(opcodes.end(), statementOpcodes.begin(), statementOpcodes.end());
    }

    opcodes.push_back(Code::Halt());

    return opcodes;
}

std::any OpcodeVisitor::visit_block_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    if (!node->is<ast::statement::BlockStatementNode>()) {
        throw std::runtime_error("BlockStatementNode expected");
    }

    std::vector<Opcode> opcodes;
    auto blockStatementNode = node->as<ast::statement::BlockStatementNode>();
    for (const auto &statement : blockStatementNode->get_statements()) {
        auto statementOpcodes = std::any_cast<std::vector<Opcode>>(statement->accept(self(), context));
        opcodes.insert(opcodes.end(), statementOpcodes.begin(), statementOpcodes.end());
    }

    return opcodes;
}

// std::any OpcodeVisitor::visit_break_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
//     if (!node->is<ast::statement::BreakNode>()) {
//         throw std::runtime_error("BreakNode expected");
//     }

//     // for-list: std::map<std::string, std::pair<int, int>> # 레이블 이름과 for문 조건문 위치, for문 나가는 위치라고 하려고 했지만,
//     // 그러기엔 너무 복잡하고 귀찮다.

//     if (context.find("loop_outer_position") == context.end()) {
//         throw std::runtime_error("Loop target not found");
//     }
//     auto loopOuterPosition = std::any_cast<std::deque<std::pair<icu::UnicodeString, std::pair<int, int>>>>(context.at("loop_outer_position"));

//     auto breakNode = node->as<ast::statement::BreakNode>();
//     auto label = breakNode->get_label();

//     for (const auto &[label, positions] : loopOuterPosition) {
//         if (label == label) {
//             auto [start, end] = positions;
//             return {Code::Jump(end)};
//         }
//     }

//     auto outer = loopOuterPosition.back();
//     return {Code::Jump(outer.second.second)};
// }

std::any OpcodeVisitor::visit_continue_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    return std::vector<Opcode>();
}

} // namespace nugdev::compiler::generation
