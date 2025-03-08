#include "04_generation/instruction/Section.h"

namespace nugdev::compiler::generation {

CodeSection::CodeSection(const icu::UnicodeString &name) : m_name(name) {}

void CodeSection::add_instruction(const std::shared_ptr<Instruction> &instruction) { m_instructions.push_back(instruction); }

const icu::UnicodeString &CodeSection::get_name() const { return m_name; }

const std::vector<std::shared_ptr<Instruction>> &CodeSection::get_instructions() const { return m_instructions; }

const std::vector<DataSectionField> &CodeSection::get_data_section() const { return m_data_section; }

void CodeSection::add_data_section_field(const DataSectionField &field) { m_data_section.push_back(field); }

} // namespace nugdev::compiler::generation
