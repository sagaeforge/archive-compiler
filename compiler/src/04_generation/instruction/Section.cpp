#include "04_generation/instruction/Section.h"

namespace nugdev::compiler::generation {

CodeSection::CodeSection(const icu::UnicodeString &name) : m_name(name) {}

void CodeSection::add_instruction(const Instruction &instruction) { m_instructions.push_back(instruction); }

const icu::UnicodeString &CodeSection::get_name() const { return m_name; }

const std::vector<Instruction> &CodeSection::get_instructions() const { return m_instructions; }

} // namespace nugdev::compiler::generation
