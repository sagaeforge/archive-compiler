#include "04_generation/CodeGenerator.h"
#include "04_generation/instruction/Instruction.h"
#include "04_generation/register/Register.hpp"

#include <any>
#include <optional>
#include <unicode/unistr.h>

namespace nugdev::compiler::generation {

CodeGenerator::CodeGenerator() : m_registers({}), m_sections({{u"global"}}) {}

bool CodeGenerator::requires_context() const { return true; }

std::any CodeGenerator::visit_program(const Super::NodePtr<ast::module::ProgramNode> &node, const Super::Context &context) {
    std::vector<ast::StatementPtr> &statements = node->get_statements();
    for (auto &statement : statements) {
        auto result = statement->accept(self(), context);
        try {
            RegisterTag tag = std::any_cast<RegisterTag>(result);
            free_register(tag);
        } catch (const std::bad_any_cast &) {
        }
    }

    return {};
}

std::any CodeGenerator::visit_block_statement(const Super::NodePtr<ast::statement::BlockStatementNode> &node, const Super::Context &context) {
    auto &statements = node->get_statements();
    if (statements.empty()) {
        auto result = allocate_register();
        result.set_value(0);
        auto instruction = Instruction{
            InstructionCode::LOAD_CONST,
            {result},
        };
        auto currentSection = *m_sections.current();
        currentSection.add_instruction(instruction);
        m_sections.set(m_sections.current(), currentSection);
    }

    auto lastResult = std::optional<UniversalRegister>();
    for (auto &statement : statements) {
        if (lastResult.has_value()) {
            free_register(lastResult->get_tag());
        }

        auto result = statement->accept(self(), context);
        if (!result.has_value()) {
            lastResult = std::nullopt;
        } else {
            try {
                lastResult = std::any_cast<UniversalRegister>(result);
            } catch (const std::bad_any_cast &) {
                lastResult = std::nullopt;
            }
        }
    }

    if (!lastResult.has_value()) {
        lastResult = allocate_register();
        lastResult->set_value(0);
        auto instruction = Instruction{
            InstructionCode::LOAD_CONST,
            {lastResult->get_tag()},
        };
        auto currentSection = *m_sections.current();
        currentSection.add_instruction(instruction);
        m_sections.set(m_sections.current(), currentSection);
    }

    return lastResult;
}

std::any CodeGenerator::visit_break_statement(const Super::NodePtr<ast::statement::BreakStatementNode> &node, const Super::Context &context) {
    auto labelExpr = node->get_label();

    icu::UnicodeString targetLoopLabel;
    if (labelExpr) {
        auto identifier = labelExpr->as<ast::expression::IdentifierLiteralNode>();
        targetLoopLabel = identifier->get_value();
    }

    std::optional<generation::Context> targetLoopContext;
    if (targetLoopLabel.isEmpty()) {
        if (m_loopContextStack.empty()) {
            throw std::runtime_error("No loop context found");
        }
        targetLoopContext = m_loopContextStack.top();
    } else {
        targetLoopContext = m_loopContextStack.find_by_name(targetLoopLabel)->value();
    }

    auto endLabel = targetLoopContext->find_label("endLabel");
    if (!endLabel.has_value()) {
        throw std::runtime_error("No end label found");
    }

    auto instruction = Instruction {}
}

} // namespace nugdev::compiler::generation
