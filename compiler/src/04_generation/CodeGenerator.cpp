#include "04_generation/CodeGenerator.h"
#include "00_app/lib/UnicodeString.hpp"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNode.h"
#include "02_parsing/ast/expression/string/StringLiteralNode.h"
#include "04_generation/context/Context.h"
#include "04_generation/instruction/Instruction.h"
#include "04_generation/instruction/Section.h"
#include "04_generation/memory/Register.hpp"

#include <memory>
#include <stdexcept>
#include <unicode/unistr.h>

namespace nugdev::compiler::generation {

CodeGenerator::CodeGenerator() : m_registers({}), m_sections({{u"global"}}), m_resultRegisterTags({}) {}

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

void CodeGenerator::push_result_register_tag(const RegisterTag &tag) { m_resultRegisterTags.push(tag); }

RegisterTag CodeGenerator::pop_result_register_tag() { return m_resultRegisterTags.pop(); }

MemoryTag CodeGenerator::find_variable(const lib::String &name) {
    for (auto section : m_sections) {
        auto variable = section.find_variable(name);
        if (variable.has_value()) {
            return variable.value();
        }
    }
    throw std::runtime_error("Variable not found");
}

// visit

void CodeGenerator::visit_boolean_literal_expression(const Super::NodePtr<ast::expression::BooleanLiteralNode> &node) {
    transaction([&](CodeSection &section) {
        auto value = node->get_value();
        auto registerTag = allocate_register(); // return 레지스터 할당

        section.push_instruction(std::make_shared<LoadValue>(registerTag, value));
        push_result_register_tag(registerTag);
    });
}

void CodeGenerator::visit_number_literal_expression(const Super::NodePtr<ast::expression::NumberLiteralNode> &node) {
    transaction([&](CodeSection &section) {
        auto value = lib::String(node->get_value()).to_double();
        auto registerTag = allocate_register(); // return 레지스터 할당

        section.push_instruction(std::make_shared<LoadValue>(registerTag, value));
        push_result_register_tag(registerTag);
    });
}

void CodeGenerator::visit_string_literal_expression(const Super::NodePtr<ast::expression::StringLiteralNode> &node) {
    transaction([&](CodeSection &section) {
        auto value = lib::String(node->get_value());

        // 메모리 할당 후, 문자열 복사. 첫번째 위치를 가지는 RegisterTag를 반환.
        auto memory = section.allocate_memory(value.get_buffer_size());
        memory.align(value);

        auto registerTag = allocate_register(); // return 레지스터 할당
        section.push_instruction(std::make_shared<Load>(registerTag, memory.get_tag(), 0));
        push_result_register_tag(registerTag);
    });
}

void CodeGenerator::visit_array_literal_expression(const Super::NodePtr<ast::expression::ArrayLiteralNode> &node) {
    transaction([&](CodeSection &section) {
        auto memory = section.allocate_memory(0);

        auto elements = node->get_elements();
        for (auto itr = elements.begin(); itr != elements.end(); itr++) {
            auto element = *itr;
            element->accept(self());

            if (element->is<ast::expression::BooleanLiteralNode>() || element->is<ast::expression::NumberLiteralNode>()) {
                if (itr == elements.begin()) {
                    auto alignment = sizeof(std::uint64_t);
                    memory.reserve(alignment * elements.size());
                }

                auto instruction = section.pop_instruction()->as<LoadValue>();
                free_register(pop_result_register_tag());
                memory.append(instruction->m_integer);
            } else if (element->is<ast::expression::StringLiteralNode>() || element->is<ast::expression::ArrayLiteralNode>()) {
                auto instruction = section.pop_instruction()->as<Load>();
                auto elementMemory = section.get_memory(instruction->memoryTag);
                if (itr == elements.begin()) {
                    auto alignment = elementMemory.get_alignment();
                    memory.reserve(alignment * elements.size());
                }

                // 메모리 태그를 통해서 메모리 가져오고, 메모리 반환 및 메모리 크기 증가.
                memory.append(elementMemory);
                section.free_memory(instruction->memoryTag);
                free_register(pop_result_register_tag());
            } else {
                throw std::runtime_error("Unsupported element type");
            }
        }

        auto registerTag = allocate_register(); // return 레지스터 할당
        section.push_instruction(std::make_shared<Load>(registerTag, memory.get_tag(), 0));
        push_result_register_tag(registerTag);
    });
}

void CodeGenerator::visit_block_statement(const Super::NodePtr<ast::statement::BlockStatementNode> &node) {
    for (auto &statement : node->get_statements()) {
        statement->accept(self());
    }
}

void CodeGenerator::visit_break_statement(const Super::NodePtr<ast::statement::BreakStatementNode> &node) {
    transaction([&](CodeSection &section) {
        // 명령어를 만들어 두는 데, 나중에 Patch가 필요함.
        std::optional<ContextTag> tag = std::nullopt;
        if (node->get_label()) {
            auto labelString = node->get_label()->as<ast::expression::IdentifierLiteralNode>()->get_value();
            tag = section.find_loop_context(labelString);
        } else {
            tag = section.current_loop_context();
        }

        PatchLabel patchLabel{u"for", u"out", tag.value()};
        auto registerTag = allocate_register();
        section.push_instruction(std::make_shared<LoadValue>(registerTag, -1));
        section.add_patch_table(section.current_instruction_index(), patchLabel);
        section.push_instruction(std::make_shared<Jump>(registerTag));
    });
}

void CodeGenerator::visit_continue_statement(const Super::NodePtr<ast::statement::ContinueStatementNode> &node) {
    transaction([&](CodeSection &section) {
        std::optional<ContextTag> tag = std::nullopt;
        if (node->get_label()) {
            auto labelString = node->get_label()->as<ast::expression::IdentifierLiteralNode>()->get_value();
            tag = section.find_loop_context(labelString);
        } else {
            tag = section.current_loop_context();
        }

        auto patchLabel = PatchLabel(u"for", u"condition", tag.value());
        auto registerTag = allocate_register();
        section.push_instruction(std::make_shared<LoadValue>(registerTag, -1));
        section.add_patch_table(section.current_instruction_index(), patchLabel);
        section.push_instruction(std::make_shared<Jump>(registerTag));
    });
}

void CodeGenerator::visit_expression_statement(const Super::NodePtr<ast::statement::ExpressionStatementNode> &node) { node->get_expression()->accept(self()); }

/*
    문법.
    for { blockstatement }
    for (i < 10) { blockstatement }
    for (i < 10; i++) { blockstatement }
    for (let i = 0; i < 10; i++) { blockstatement }
*/
void CodeGenerator::visit_for_statement(const Super::NodePtr<ast::statement::ForStatementNode> &node) {
    transaction([&](CodeSection &section) {
        std::optional<lib::String> label = std::nullopt;
        if (node->get_label()) {
            label = node->get_label()->as<ast::expression::IdentifierLiteralNode>()->get_value();
        }
        auto contextTag = section.push_loop_context(label);

        // let이 있으면 가장 먼저 실행
        if (node->get_init()) {
            node->get_init()->accept(self());
        }

        // condition이 있으면 조건 비교 명령어 생성
        auto condtionPosition = section.current_instruction_index(); // condition 포지션
        if (node->get_condition()) {
            node->get_condition()->accept(self());

            // 결과 레지스터를 가지고 있어야 함.
            auto resultRegisterTag = pop_result_register_tag();

            // cmp를 할지, test를 할지 모르지만,
            auto cmp = std::make_shared<Cmp>(resultRegisterTag, 0);
            section.push_instruction(cmp);

            // jump_eq로 0이라면, out으로 가야함.
            auto nextRegisterTag = allocate_register();
            section.push_instruction(std::make_shared<LoadValue>(nextRegisterTag, -1));
            PatchLabel patchLabel{u"for", u"out", contextTag};
            section.add_patch_table(section.current_instruction_index(), patchLabel);
            auto jumpEq = std::make_shared<JumpEq>(nextRegisterTag);
            section.push_instruction(jumpEq);
        }

        // post가 있으면 증가 명령어 생성
        auto postPosition = section.current_instruction();
        if (node->get_post()) {
            node->get_post()->accept(self());
        }

        // consequence 명령어 처리.
        node->get_consequence()->accept(self());

        // 패치 테이블에서 현재 처리할 수 있는 패치 리스트를 처리.
        auto consequencePosition = section.current_instruction_index(); // out 포지션
        auto patchLabels = section.find_all_from_patch_table([&](const PatchLabel &patchLabel) { return patchLabel.m_keyword == u"for"; });

        for (auto patchLabel : patchLabels) {
            auto [keyword, timing, tag] = *patchLabel;
            if (tag != contextTag) {
                continue;
            }

            auto instruction = section.get_instruction(condtionPosition);
            if (!instruction->is<LoadValue>()) {
                // 이 경우는 코드 생성이 잘못됨
                throw std::runtime_error("unsupported patch label");
            }
            auto loadValueInstruction = instruction->as<LoadValue>();

            if (timing == u"out") {
                loadValueInstruction->m_unsigned = consequencePosition;
            } else if (timing == u"condition") {
                loadValueInstruction->m_unsigned = condtionPosition;
            } else {
                throw std::runtime_error("unsupported patch label");
            }

            // 처리가 되었다면, patch_table에서 제거
            section.solve_patch_table(*patchLabel);
        }
    });
}

void CodeGenerator::visit_let_statement(const Super::NodePtr<ast::statement::LetStatementNode> &node) {
    transaction([&](CodeSection &section) {
        auto name = node->get_name()->as<ast::expression::IdentifierLiteralNode>()->get_value();

        // 현재 변수에 관련된 메모리 영역 산정.
        auto variableMemory = section.allocate_variable(name, 8);

        // right
        node->get_value()->accept(self());

        // 메모리에 저장후, 레지스터 패기.
        auto resultRegisterTag = pop_result_register_tag();
        section.push_instruction(std::make_shared<Store>(resultRegisterTag, variableMemory));
        free_register(resultRegisterTag);
    });
}

void CodeGenerator::visit_identifier_expression(const Super::NodePtr<ast::expression::IdentifierLiteralNode> &node) {
    transaction([&](CodeSection &section) {
        auto variableMemory = find_variable(node->get_value());
        auto registerTag = allocate_register();
        section.push_instruction(std::make_shared<Load>(registerTag, variableMemory, 0));
        push_result_register_tag(registerTag);
    });
}

void CodeGenerator::visit_if_expression(const Super::NodePtr<ast::expression::IfExpressionNode> &node) {
    transaction([&](CodeSection &section) {
        auto contextTag = section.push_if_context();

        node->get_condition()->accept(self());
        auto resultRegisterTag = pop_result_register_tag();

        // 명령어 봤을 때, 0이 아니라면, else-if 문장 확인 후, else 문장
        auto cmp = std::make_shared<Cmp>(resultRegisterTag, 0);
        section.push_instruction(cmp);

        // jump_eq로 0이라면, out으로 가야함.
        auto nextRegisterTag = allocate_register();
        section.push_instruction(std::make_shared<LoadValue>(nextRegisterTag, -1));
        PatchLabel patchLabel{u"if", u"other-branch", contextTag};
        section.add_patch_table(section.current_instruction_index(), patchLabel);
        section.push_instruction(std::make_shared<JumpNe>(nextRegisterTag));

        // 본문 처리
        node->get_consequence()->accept(self());

        // patch-table 처리
        auto consequencePosition = section.current_instruction_index();
        auto patchLabels = section.find_all_from_patch_table([&](const PatchLabel &patchLabel) { return patchLabel.m_keyword == u"if"; });

        for (auto patchLabel : patchLabels) {
            auto [keyword, timing, tag] = *patchLabel;
            if (tag != contextTag) {
                continue;
            }

            auto instruction = section.get_instruction(consequencePosition);
            if (!instruction->is<LoadValue>()) {
                throw std::runtime_error("unsupported patch label");
            }
            auto loadValueInstruction = instruction->as<LoadValue>();

            if (timing == u"other-branch") {
                loadValueInstruction->m_unsigned = consequencePosition;
            }
        }
        section.pop_if_context();

        // else
        if (node->get_alternative()) {
            node->get_alternative()->accept(self());
        }
    });
}

void CodeGenerator::visit_index_expression(const Super::NodePtr<ast::expression::IndexExpressionNode> &node) {
    transaction([&](CodeSection &section) {
        // 외쪽 표현식 처리인데,
        node->get_left()->accept(self());
        auto variableRegisterTag = pop_result_register_tag();
        auto variableInstruction = section.current_instruction();
        if (!variableInstruction->is<Load>()) {
            throw std::runtime_error("unsupported index expression");
        }
        auto variableLoadInstruction = variableInstruction->as<Load>();

        // index 표현식 처리.
        node->get_index()->accept(self());
        auto indexRegisterTag = pop_result_register_tag();

        // load 후 어차피 둘다 해제해야함. variableRegisterTag는 어떠한 값인지는 모르지만, 이전 명령어는 load여야하는 상황임.
        auto indexInstruction = section.current_instruction();
        if (!indexInstruction->is<Load>() && !indexInstruction->is<LoadValue>()) {
            throw std::runtime_error("unsupported index expression");
        }

        if (indexInstruction->is<LoadValue>()) {
            auto indexLoadValueInstruction = indexInstruction->as<LoadValue>(); // 0, 1 blur blur
            auto indexValue = indexLoadValueInstruction->m_integer;

            // 메모리 찾기
            auto memoryTag = variableLoadInstruction->memoryTag;
            auto memory = section.get_memory(memoryTag); // 0번째 element를 가르키고 있고.

            // 메모리 태그에 대한 메모리 크기 확인.
        } else {
            // register는 메모리를 가르키고 있는데, 그 값을 indexLoadInstruction에 넣어줘야함.
            auto indexLoadInstruction = indexInstruction->as<Load>();
            auto indexMemory = section.get_memory(indexLoadInstruction->memoryTag);

            // Load일땐, 메모리에 접근 후, 값을 가져오는 데, Load로 해야겠네.
        }
    });
}

} // namespace nugdev::compiler::generation
