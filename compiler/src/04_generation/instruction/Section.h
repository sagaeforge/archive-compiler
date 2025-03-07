#pragma once

#include "04_generation/instruction/Instruction.h"

#include <unicode/unistr.h>

namespace nugdev::compiler::generation {

class CodeSection {
  public:
    CodeSection(const icu::UnicodeString &name);

  public:
    void add_instruction(const Instruction &instruction);

    const icu::UnicodeString &get_name() const;
    const std::vector<Instruction> &get_instructions() const;

  private:
    icu::UnicodeString m_name;
    std::vector<Instruction> m_instructions;
};

} // namespace nugdev::compiler::generation
