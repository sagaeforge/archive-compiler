#pragma once

#include "00_app/tag/Tag.h"
#include "02_parsing/ast/ASTNodeVisitor.h"
#include <any>
#include <memory>
#include <string>
#include <unicode/unistr.h>
#include <unordered_map>
#include <variant>
#include <vector>

namespace nugdev::compiler::generation {

// 바이트코드 명령어 종류 (OpCode)
enum class BytecodeOpCode {
    // 스택 및 메모리 명령어
    LOAD_CONST, // 상수 로드
    LOAD_VAR,   // 변수 로드
    STORE_VAR,  // 변수 저장

    // 레지스터 명령어
    MOV_REG, // 레지스터 간 이동

    // 산술 연산
    ADD, // 덧셈
    SUB, // 뺄셈
    MUL, // 곱셈
    DIV, // 나눗셈
    MOD, // 나머지

    // 비교 연산
    CMP_EQ, // 같음
    CMP_NE, // 다름
    CMP_LT, // 작음
    CMP_LE, // 작거나 같음
    CMP_GT, // 큼
    CMP_GE, // 크거나 같음

    // 논리 연산
    AND, // 논리 AND
    OR,  // 논리 OR
    NOT, // 논리 NOT

    // 제어 흐름
    JMP,          // 무조건 점프
    JMP_IF_TRUE,  // 조건부 점프 (참인 경우)
    JMP_IF_FALSE, // 조건부 점프 (거짓인 경우)
    CALL,         // 함수 호출
    RET,          // 함수 리턴

    // 기타
    NOP, // 아무 작업 안함
};

// 바이트코드 명령어의 피연산자 타입
using BytecodeOperand = std::variant<int, float, std::string, Tag>;

// 바이트코드 명령어 구조체
struct BytecodeInstruction {
    BytecodeOpCode opcode;
    std::vector<int> registers;            // 사용되는 레지스터 번호
    std::vector<BytecodeOperand> operands; // 추가 피연산자

    BytecodeInstruction(BytecodeOpCode op) : opcode(op) {}
};

// 바이트코드 섹션 (함수, 메소드 등의 코드 블록)
class BytecodeSection {
  public:
    BytecodeSection(const std::string &name) : m_name(name) {}

    void addInstruction(const BytecodeInstruction &instruction) { m_instructions.push_back(instruction); }

    const std::vector<BytecodeInstruction> &getInstructions() const { return m_instructions; }

    const std::string &getName() const { return m_name; }

  private:
    std::string m_name;
    std::vector<BytecodeInstruction> m_instructions;
};

class BytecodeGenerator : public ast::ASTNodeVisitor {
  public:
    BytecodeGenerator();
    virtual ~BytecodeGenerator();

    // ASTNodeVisitor 메소드 구현
    virtual std::any visit(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context = {}) override;

  protected:
    // 모듈 방문자
    virtual std::any visit_program(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;

    // 문장 방문자
    virtual std::any visit_block_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_break_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_continue_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_expression_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_for_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_let_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_return_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;

    // 표현식 방문자
    virtual std::any visit_array_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_boolean_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_call_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_function_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_identifier_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_if_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_index_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_infix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_number_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_postfix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_prefix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_string_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;
    virtual std::any visit_when_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) override;

    // 컨텍스트 필요 여부
    virtual bool requires_context() const override { return true; }

  public:
    // 바이트코드 생성 및 관리 메소드
    void generate(const ast::ASTNodePtr &rootNode);
    BytecodeSection &getCurrentSection();
    void addInstruction(const BytecodeInstruction &instruction);

    // 레지스터 관리
    int allocateRegister();
    void freeRegister(int regId);
    void resetRegisters();

    // 결과 접근
    const std::vector<BytecodeSection> &getSections() const;
    std::string dumpBytecode() const; // 디버깅용 바이트코드 덤프

  private:
    // 섹션 관리
    std::vector<BytecodeSection> m_sections;
    int m_currentSectionIndex;

    // 레지스터 관리
    int m_nextRegister;
    std::vector<int> m_freeRegisters;

    // 심볼 관리 (변수/함수 이름을 레지스터/주소에 매핑)
    std::unordered_map<std::string, int> m_variables;

    // 레이블 관리 (점프 지점)
    std::unordered_map<std::string, int> m_labels;
    int m_nextLabelId;

    // 내부 유틸리티 함수
    std::string generateUniqueLabel();
};

} // namespace nugdev::compiler::generation
