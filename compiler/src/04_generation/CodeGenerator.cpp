#include "04_generation/CodeGenerator.h"
#include "04_generation/instruction/Instruction.h"
#include "04_generation/register/Register.hpp"

#include <any>
#include <memory>
#include <optional>
#include <unicode/unistr.h>

namespace nugdev::compiler::generation {

CodeGenerator::CodeGenerator() : m_registers({}), m_sections({{u"global"}}) {}

bool CodeGenerator::requires_context() const { return true; }

RegisterTag CodeGenerator::allocate_register() {
    if (m_registers.empty()) {
        static auto registerCnt = 0;
        registerCnt++;
        if (registerCnt > 30) {
            // 30 개 이상인 경우, 로직이 잘못되었을 가능성이 매우 높음.
            throw std::runtime_error("Too many registers");
        }
        m_registers.push(RegisterTag::create<RegisterTag>());
    }
    return m_registers.pop();
};

void CodeGenerator::free_register(const RegisterTag &tag) { m_registers.push(tag); }

void CodeGenerator::push_instruction(const std::shared_ptr<Instruction> &instruction) {
    auto currentSectionItr = m_sections.current();
    auto currentSection = currentSectionItr.value();
    currentSection.add_instruction(instruction);
    m_sections.set(currentSectionItr, currentSection);
}

void CodeGenerator::allocate_data_section_field(const DataSectionField &field) {
    auto currentSectionItr = m_sections.current();
    auto currentSection = currentSectionItr.value();
    currentSection.add_data_section_field(field);
    m_sections.set(currentSectionItr, currentSection);
}

std::any CodeGenerator::visit_array_literal_expression(const Super::NodePtr<ast::expression::ArrayLiteralNode> &node, const Super::Context &context) {
    auto elems = node->get_elements();
    if (elems.empty()) {
        return {};
    }

    DataSectionField dataSectionField{.m_tag = DataSectionField::DataScetionFieldTag::create<DataSectionField::DataScetionFieldTag>(),
                                      .m_value =
                                          std::make_shared<DataSectionField::DataSectionFieldValue>(DataSectionField::DataSectionFieldValue::Type::Array, {})};
    for (auto &elem : elems) {
        auto registerTag = std::any_cast<RegisterTag>(elem->accept(self(), context));
        dataSectionField.m_value->m_array.push_back(
            std::make_shared<DataSectionField::DataSectionFieldValue>(DataSectionField::DataSectionFieldValue::Type::Literal, registerTag.get_value()));
    }
    allocate_data_section_field(dataSectionField);
}

} // namespace nugdev::compiler::generation
