#pragma once

#include "04_generation/instruction/Instruction.h"

#include <unicode/unistr.h>

namespace nugdev::compiler::generation {

class CodeSection {
  public:
    CodeSection(const icu::UnicodeString &name);

  public:
    void add_instruction(const std::shared_ptr<Instruction> &instruction);
    std::shared_ptr<Instruction> pop_instruction();

    const icu::UnicodeString &get_name() const;
    const std::vector<std::shared_ptr<Instruction>> &get_instructions() const;

  private:
    icu::UnicodeString m_name;

    // 코드 섹션
    std::vector<std::shared_ptr<Instruction>> m_instructions;
};

} // namespace nugdev::compiler::generation
