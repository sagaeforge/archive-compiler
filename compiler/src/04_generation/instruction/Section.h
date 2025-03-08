#pragma once

#include "04_generation/instruction/Instruction.h"

#include <unicode/unistr.h>

namespace nugdev::compiler::generation {

class CodeSection {
  public:
    CodeSection(const icu::UnicodeString &name);

  public:
    void add_instruction(const std::shared_ptr<Instruction> &instruction);
    void add_data_section_field(const DataSectionField &field);

    const icu::UnicodeString &get_name() const;
    const std::vector<std::shared_ptr<Instruction>> &get_instructions() const;
    const std::vector<DataSectionField> &get_data_section() const;

  private:
    icu::UnicodeString m_name;

    // 코드 섹션
    std::vector<std::shared_ptr<Instruction>> m_instructions;

    // 데이터 섹션
    std::vector<DataSectionField> m_data_section;
};

} // namespace nugdev::compiler::generation
