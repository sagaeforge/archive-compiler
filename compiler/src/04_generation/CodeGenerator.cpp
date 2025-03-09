#include "04_generation/CodeGenerator.h"
#include "00_app/lib/UnicodeString.hpp"
#include "04_generation/instruction/Instruction.h"
#include "04_generation/memory/Memory.hpp"
#include "04_generation/memory/Register.hpp"

#include <memory>
#include <unicode/unistr.h>

namespace nugdev::compiler::generation {

CodeGenerator::CodeGenerator() : m_registers({}), m_sections({{u"global"}}), m_memoryMap({}) {}

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

std::shared_ptr<Instruction> CodeGenerator::pop_instruction() {
    auto currentSectionItr = m_sections.current();
    auto currentSection = currentSectionItr.value();
    auto instruction = currentSection.pop_instruction();
    m_sections.set(currentSectionItr, currentSection);
    return instruction;
}

Memory CodeGenerator::allocate_memory(const size_t &size) {
    auto memory = Memory(MemoryTag::create<MemoryTag>());
    m_memoryMap.push(memory);
    return memory;
}

Memory CodeGenerator::get_memory(const MemoryTag &tag) {
    auto memory = m_memoryMap.find([&tag](const Memory &memory) { return memory.get_tag() <=> tag; });
    if (!memory.valid()) {
        throw std::runtime_error("Memory not found");
    }
    return memory.value();
}

void CodeGenerator::free_memory(const MemoryTag &tag) {
    auto memory = m_memoryMap.find([&tag](const Memory &memory) { return memory.get_tag() <=> tag; });
    if (!memory.valid()) {
        throw std::runtime_error("Memory not found");
    }
    m_memoryMap.remove(memory);
}

void CodeGenerator::visit_boolean_literal_expression(const Super::NodePtr<ast::expression::BooleanLiteralNode> &node) {
    auto value = node->get_value();
    auto registerTag = allocate_register(); // return 레지스터 할당
    push_instruction(std::make_shared<LoadValue>(registerTag, value));
    push_result_register_tag(registerTag);
}

void CodeGenerator::visit_number_literal_expression(const Super::NodePtr<ast::expression::NumberLiteralNode> &node) {
    auto value = lib::String(node->get_value());
    auto valueDouble = value.to_double();

    auto registerTag = allocate_register(); // return 레지스터 할당

    push_instruction(std::make_shared<LoadValue>(registerTag, value.to_double()));
    push_result_register_tag(registerTag);
}

void CodeGenerator::visit_string_literal_expression(const Super::NodePtr<ast::expression::StringLiteralNode> &node) {
    auto value = lib::String(node->get_value());

    // 메모리 할당 후, 문자열 복사. 첫번째 위치를 가지는 RegisterTag를 반환.
    auto memory = allocate_memory(value.get_buffer_size());
    memory.allocate(value);

    auto registerTag = allocate_register(); // return 레지스터 할당
    push_instruction(std::make_shared<Load>(registerTag, memory.get_data()));
    push_result_register_tag(registerTag);
}

void CodeGenerator::visit_array_literal_expression(const Super::NodePtr<ast::expression::ArrayLiteralNode> &node) {
    // 복잡한데, element 하나하나 방문해서 메모리 할당하는 케이스가 필요할 수 있는데.
    auto currentMemoryPosition = 0;
    auto memory = allocate_memory(currentMemoryPosition);

    for (auto &element : node->get_elements()) {
        element->accept(self());

        if (element->is<ast::expression::BooleanLiteralNode>() || element->is<ast::expression::NumberLiteralNode>()) {
            // 64비트 미만이라면, 아까전 코드를 수정해야 함.
            auto instruction = pop_instruction()->as<LoadValue>();

            // 먼저 레지스터 해제
            free_register(instruction->destination);

            // 부족한 메모리 크기 확인.
            memory.append(8);
            memory.set(currentMemoryPosition, instruction->m_integer);
            currentMemoryPosition += 8; // 64비트
        } else if (element->is<ast::expression::StringLiteralNode>() || element->is<ast::expression::ArrayLiteralNode>()) {
            // 메모리 태그를 통해서 메모리 가져오고, 메모리 반환 및 메모리 크기 증가.
            auto instruction = pop_instruction()->as<Load>();
            auto elementMemory = get_memory(instruction->memoryTag);
            memory.append(elementMemory);
            free_memory(instruction->memoryTag);
            free_register(instruction->destination);
        } else {
            throw std::runtime_error("Unsupported element type");
        }
    }

    auto registerTag = allocate_register(); // return 레지스터 할당
    push_instruction(std::make_shared<Load>(registerTag, memory.get_data()));
    push_result_register_tag(registerTag);
}

void CodeGenerator::visit_block_statement(const Super::NodePtr<ast::statement::BlockStatementNode> &node) {
    for (auto &statement : node->get_statements()) {
        statement->accept(self());
    }
}

} // namespace nugdev::compiler::generation
