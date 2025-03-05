#include "04_generation/opcode/OpcodeVisitor.h"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/boolean/BooleanLiteralNode.h"
#include "02_parsing/ast/expression/string/StringLiteralNode.h"
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

std::any OpcodeVisitor::visit_string_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    if (!node->is<ast::expression::StringLiteralNode>()) {
        throw std::runtime_error("StringLiteralExpressionNode expected");
    }

    auto stringLiteralNode = node->as<ast::expression::StringLiteralNode>();
    auto value = stringLiteralNode->get_value();

    auto literalTag = LiteralTag::create()->as<LiteralTag>();
    auto resultTag = std::any_cast<std::shared_ptr<RegisterTag>>(context.at("resultTag"));
    auto resourceTag = RegisterTag::create()->as<RegisterTag>();

    return Codes{// 생성해야하는 코드
                 {std::make_shared<Code::Load>(resultTag, resourceTag)},
                 {},
                 {resourceTag},
                 {{resourceTag, literalTag}},
                 {{literalTag, value}}};
}

std::any OpcodeVisitor::visit_boolean_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    if (!node->is<ast::expression::BooleanLiteralNode>()) {
        throw std::runtime_error("BooleanLiteralExpressionNode expected");
    }

    auto booleanLiteralNode = node->as<ast::expression::BooleanLiteralNode>();
    auto value = booleanLiteralNode->get_value();

    auto resultTag = std::any_cast<std::shared_ptr<RegisterTag>>(context.at("resultTag"));

    return Codes{// 생성해야하는 코드
                 {std::make_shared<Code::LoadInt>(resultTag, value ? 1 : 0)},
                 {},
                 {},
                 {},
                 {}};
}

} // namespace nugdev::compiler::generation
