#include "04_generation/instruction/Section.h"
#include "04_generation/memory/Memory.hpp"

namespace nugdev::compiler::generation {

CodeSection::CodeSection(const icu::UnicodeString &name)
    : m_name(name), m_instructions({}), m_memoryMap({}), m_patchTable({}), m_loopContext({}), m_ifContext({}), m_variables({}) {}

void CodeSection::push_instruction(const std::shared_ptr<Instruction> &instruction) { m_instructions.push(instruction); }

std::shared_ptr<Instruction> CodeSection::current_instruction() { return m_instructions.current().value(); }

std::shared_ptr<Instruction> CodeSection::pop_instruction() { return m_instructions.pop(); }

const icu::UnicodeString &CodeSection::get_name() const { return m_name; }

const stream::MutableStream<CodeSection::InstructionPtr> &CodeSection::get_instructions() const { return m_instructions; }

Memory CodeSection::allocate_memory(const size_t &size, const size_t &alignment) {
    auto memory = Memory(MemoryTag::create<MemoryTag>(), size, alignment);
    m_memoryMap.push(memory);
    return memory;
}

Memory CodeSection::get_memory(const MemoryTag &tag) {
    auto memory = m_memoryMap.find([&tag](const Memory &memory) { return memory.get_tag() <=> tag; });
    if (!memory.valid()) {
        throw std::runtime_error("Memory not found");
    }
    return memory.value();
}

void CodeSection::free_memory(const MemoryTag &tag) {
    auto memory = m_memoryMap.find([&tag](const Memory &memory) { return memory.get_tag() <=> tag; });
    if (!memory.valid()) {
        throw std::runtime_error("Memory not found");
    }
    m_memoryMap.remove(memory);
}

CodeSection::InstructionPtr CodeSection::get_instruction(const size_t &index) { return *(m_instructions.current() + index); }

int CodeSection::current_instruction_index() { return m_instructions.size(); }

MemoryTag CodeSection::allocate_variable(const lib::String &name, const size_t &size) {
    auto memoryTag = MemoryTag::create<MemoryTag>();
    m_variables.push({name, memoryTag});
    return memoryTag;
}

std::optional<MemoryTag> CodeSection::find_variable(const lib::String &name) {
    auto variable = m_variables.find([&name](const std::tuple<lib::String, MemoryTag> &variable) { return std::get<0>(variable) == name; });
    if (!variable.valid()) {
        return std::nullopt;
    }
    return std::get<1>(variable.value());
}

void CodeSection::free_variable(const lib::String &name) {
    auto variable = m_variables.find([&name](const std::tuple<lib::String, MemoryTag> &variable) { return std::get<0>(variable) == name; });
    if (!variable.valid()) {
        throw std::runtime_error("Variable not found");
    }
    m_variables.remove(variable);
}

ContextTag CodeSection::push_loop_context(const std::optional<lib::String> &label) {
    auto contextTag = ContextTag::create<ContextTag>();
    m_loopContext.push(Context{.m_id = contextTag, .m_keyword = u"for", .m_label = label});
    return contextTag;
}

void CodeSection::pop_loop_context() { m_loopContext.pop(); }

ContextTag CodeSection::current_loop_context() { return m_loopContext.current()->m_id; }

ContextTag CodeSection::find_loop_context(const lib::String &label) {
    auto context = m_loopContext.find([&](const Context &context) { return context.m_label.has_value() && context.m_label.value() == label; });
    if (!context.valid()) {
        throw std::runtime_error("Loop context not found");
    }
    return context.value().m_id;
}

void CodeSection::add_patch_table(const size_t &current_position, const PatchLabel target) { m_patchTable.push(target); }

void CodeSection::solve_patch_table(const PatchLabel &target) {
    auto itr = m_patchTable.find([&target](const PatchLabel &patch) { return patch == target; });
    if (!itr.valid()) {
        throw std::runtime_error("Patch not found");
    }
    auto patch = itr.value();
}

ContextTag CodeSection::push_if_context() {
    auto contextTag = ContextTag::create<ContextTag>();
    m_ifContext.push(Context{.m_id = contextTag, .m_keyword = u"if"});
    return contextTag;
}

void CodeSection::pop_if_context() { m_ifContext.pop(); }

ContextTag CodeSection::current_if_context() { return m_ifContext.current()->m_id; }

} // namespace nugdev::compiler::generation
