#pragma once

#include "00_app/stream/Stream.hpp"
#include "04_generation/context/Context.h"
#include "04_generation/instruction/Instruction.h"
#include "04_generation/memory/Memory.hpp"

#include <unicode/unistr.h>

namespace nugdev::compiler::generation {

struct PatchLabel {
    lib::String m_keyword; // for, while, if, function, etc.
    lib::String m_timing;  // ex) for-condition, for-out, etc...
    ContextTag m_label;    // context_id

    PatchLabel(const lib::String &keyword, const lib::String &timing, const ContextTag &label) : m_keyword(keyword), m_timing(timing), m_label(label) {}
    PatchLabel(const lib::String str) : m_label(ContextTag::empty<ContextTag>()) {
        auto parts = str.split(u"_");
        m_keyword = parts[0];
        m_timing = parts[1];
        m_label = ContextTag(parts[2]);
    }

    lib::String to_str() const { return m_keyword + u"_" + m_timing + u"_" + m_label.to_str(); }

    bool operator==(const PatchLabel &other) const { return m_keyword == other.m_keyword && m_timing == other.m_timing && m_label == other.m_label; }
};

class CodeSection {
  public:
    using InstructionPtr = std::shared_ptr<Instruction>;

  public:
    CodeSection(const icu::UnicodeString &name);

  public: // getter
    const icu::UnicodeString &get_name() const;

  public: // instruction
    void push_instruction(const InstructionPtr &instruction);
    InstructionPtr current_instruction();
    InstructionPtr pop_instruction();
    const stream::MutableStream<InstructionPtr> &get_instructions() const;
    InstructionPtr get_instruction(const size_t &index);
    int current_instruction_index();

  public: // memory
    Memory allocate_memory(const size_t &size, const size_t &alignment = 1);
    Memory get_memory(const MemoryTag &tag);
    void free_memory(const MemoryTag &tag);

  public: // variable
    MemoryTag allocate_variable(const lib::String &name, const size_t &size);
    std::optional<MemoryTag> find_variable(const lib::String &name);
    void free_variable(const lib::String &name);

  public: // loop
    ContextTag push_loop_context(const std::optional<lib::String> &label);
    ContextTag current_loop_context();
    ContextTag find_loop_context(const lib::String &label);
    void pop_loop_context();

  public:
    ContextTag push_if_context();
    ContextTag current_if_context();
    void pop_if_context();

  public: // patch
    void add_patch_table(const size_t &current_position, const PatchLabel targetTag);
    template <typename Function> auto find_all_from_patch_table(Function &&func) { return m_patchTable.find_all(std::forward<Function>(func)); }
    void solve_patch_table(const PatchLabel &targetTag);

  private:
    icu::UnicodeString m_name;

    // 코드 섹션
    stream::MutableStream<InstructionPtr> m_instructions;

    // 해당 코드 섹션에서 활용하는 메모리 영역들
    stream::MutableStream<std::tuple<lib::String, MemoryTag>> m_variables;
    stream::MutableStream<Memory> m_memoryMap;

    // patch table
    stream::MutableStream<PatchLabel> m_patchTable;

    // context
    stream::MutableStream<Context> m_loopContext;
    stream::MutableStream<Context> m_ifContext;
};

} // namespace nugdev::compiler::generation
