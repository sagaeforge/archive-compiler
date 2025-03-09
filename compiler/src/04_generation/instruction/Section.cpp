#include "04_generation/instruction/Section.h"

namespace nugdev::compiler::generation {

CodeSection::CodeSection(const icu::UnicodeString &name) : m_name(name) {}

void CodeSection::add_instruction(const std::shared_ptr<Instruction> &instruction) { m_instructions.push_back(instruction); }

std::shared_ptr<Instruction> CodeSection::pop_instruction() {
    auto instruction = m_instructions.back();
    m_instructions.pop_back();
    return instruction;
}

const icu::UnicodeString &CodeSection::get_name() const { return m_name; }

const std::vector<std::shared_ptr<Instruction>> &CodeSection::get_instructions() const { return m_instructions; }

} // namespace nugdev::compiler::generation
