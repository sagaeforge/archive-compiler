#pragma once

#include "00_app/lib/PointerHelper.hpp"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/ASTNodeVisitor.h"
#include "04_generation/opcode/Opcode.h"
#include "register/RegisterTag.h"

#include <unicode/unistr.h>
#include <unordered_map>

namespace nugdev::compiler::generation {

class OpcodeVisitor : public ast::ASTNodeVisitor {
  public: // 외부에서 사용하는 객체
    class LiteralTag : public Tag {};
    class SectionEntryTag : public Tag {};
    struct Section {
        icu::UnicodeString m_name; // 파일명__센셕명 # 만약에 섹션이 high-level 섹션이라면, first-class object라는 의미임.
    };
    struct StaticSection { // code section이랑 페어가 될듯.
        // 리소스 - 실제 값
        std::unordered_map<std::shared_ptr<LiteralTag>, icu::UnicodeString> m_staticData;
    };
    // CodeSection 중 entry Section인지 모름.
    struct CodeSection {
        // 가상 레지스터 - 리소스 매핑
        std::unordered_map<std::shared_ptr<RegisterTag>, std::shared_ptr<LiteralTag>> m_resourceTable;
        // 실제 코드
        std::vector<std::shared_ptr<Opcode>> m_opcodes;
    };

  private:
    struct JumpTarget : lib::PointerHelper<JumpTarget> {};
    struct JumpForConditionCheckPosition : JumpTarget {};    // for 문의 조건 체크 위치(continue)
    struct JumpForEndPosition : JumpTarget {};               // for 문의 끝 위치(break)
    struct JumpWhenConditionCheckPosition : JumpTarget {};   // when 문의 조건 체크 위치
    struct JumpWhenEndPosition : JumpTarget {};              // when 문의 끝 위치(break)
    struct JumpElseIfConditionCheckPosition : JumpTarget {}; // elif 문의 조건 체크 위치
    struct JumpElsePosition : JumpTarget {};                 // else 문의 조건 체크 위치
    struct JumpIfOutPosition : JumpTarget {};                // if 문 블럭의 끝 위치(if-elif-else 문의 최종 위치)
    struct JumpFunctionOutPosition : JumpTarget {};          // 함수 블럭의 끝 위치(return)
    struct JumpLabelPosition : JumpTarget {}; // for(continue, break), function(return) 등등 레이블을 달아서 해당 레이블에 벗어나는 목적 주소

    // 내부적으로 사용하는 객체
    struct Codes {
        // 실제 생성된 코드인데, 이 안에 Opcode는 가상 레지스터를 가지고 있음.
        std::vector<std::shared_ptr<Opcode>> m_createOpcodes;
        // 가상 레지스터들 중, 실제 위치가 지정되지 않은 정보를 가지고 있는 객체. (넣어줘야하는 가상 레지스터 태그, 이 주소가 무엇인지 알 수 있는 정보)
        std::unordered_map<std::shared_ptr<RegisterTag>, std::shared_ptr<JumpTarget>> m_patchTable;
        // 실제 매핑해야하는 가상 레지스터 리스트
        std::vector<std::shared_ptr<RegisterTag>> m_registerTags;
        // 리소스-가상 레지스터 매핑
        std::unordered_map<std::shared_ptr<RegisterTag>, std::shared_ptr<LiteralTag>> m_resourceTable;
        // 리소스-실제 값
        std::unordered_map<std::shared_ptr<LiteralTag>, icu::UnicodeString> m_resourceValues;
    };

  public:
    // Module(return: std::vector<std::pair<StaticSection, CodeSection>>)
    std::any visit_program(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;

    // Statement
    std::any visit_block_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_break_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_continue_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_expression_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_for_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_let_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_return_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;

    // Expression
    std::any visit_array_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_boolean_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_call_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_function_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_identifier_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_if_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_index_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_infix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_number_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_postfix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_prefix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_string_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    std::any visit_when_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;

  public:
    bool requires_context() const override;
};

} // namespace nugdev::compiler::generation