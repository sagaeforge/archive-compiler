#include "BytecodeGenerator.h"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/expression/boolean/BooleanLiteralNode.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNode.h"
#include "02_parsing/ast/expression/infix/InfixExpressionNode.h"
#include "02_parsing/ast/expression/number/NumberLiteralNode.h"
#include "02_parsing/ast/expression/post/PostNode.h"
#include "02_parsing/ast/expression/prefix/PrefixExpressionNode.h"
#include "02_parsing/ast/expression/string/StringLiteralNode.h"
#include <format>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace nugdev::compiler::generation {

// 헬퍼 함수 선언
std::string opCodeToString(BytecodeOpCode opcode);
std::string operandToString(const BytecodeOperand &operand);

BytecodeGenerator::BytecodeGenerator() : m_currentSectionIndex(-1), m_nextRegister(0), m_nextLabelId(0) {
    // 기본 섹션 생성 (전역 코드)
    m_sections.emplace_back("global");
    m_currentSectionIndex = 0;
}

BytecodeGenerator::~BytecodeGenerator() {
    // 필요한 정리 작업 수행
}

std::any BytecodeGenerator::visit(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // ASTNodeVisitor의 기본 방문 메서드 구현
    return ast::ASTNodeVisitor::visit(node, context);
}

std::any BytecodeGenerator::visit_program(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 프로그램 노드 방문
    // 각 자식 노드를 순회하며 바이트코드 생성
    // 여기서는 간단한 뼈대만 제공

    // 먼저 전역 섹션 설정
    m_currentSectionIndex = 0;

    // 일반적으로 여기서 자식 노드들을 방문하여 바이트코드 생성

    return {};
}

std::any BytecodeGenerator::visit_block_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 블록 문장 노드 방문
    // 블록 내의 각 문장을 순차적으로 방문하여 바이트코드 생성

    return {};
}

std::any BytecodeGenerator::visit_break_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // break 문장 처리
    // 현재 루프의 끝으로 점프하는 명령어 생성

    return {};
}

std::any BytecodeGenerator::visit_continue_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // continue 문장 처리
    // 현재 루프의 조건 검사 부분으로 점프하는 명령어 생성

    return {};
}

std::any BytecodeGenerator::visit_expression_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 표현식 문장 처리
    // 표현식을 방문하여 바이트코드 생성

    return {};
}

std::any BytecodeGenerator::visit_for_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // for 문장 처리
    // 초기화, 조건, 증감, 본문 부분을 각각 방문하여 바이트코드 생성

    return {};
}

std::any BytecodeGenerator::visit_let_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 변수 선언 처리
    // 변수 이름을 심볼 테이블에 등록하고 초기화 표현식 방문

    return {};
}

std::any BytecodeGenerator::visit_return_statement(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // return 문장 처리
    // 반환값이 있다면 해당 표현식 방문 후 반환 명령어 생성

    return {};
}

std::any BytecodeGenerator::visit_array_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 배열 리터럴 표현식 처리

    // 이 예제에서는 간단한 구현만 제공합니다.
    // 실제 구현에서는 각 배열 요소를 평가하고 배열 객체 생성 등이 필요합니다.

    // 결과를 저장할 레지스터 할당 (배열 참조를 저장)
    int resultReg = allocateRegister();

    // 배열 생성 명령어 (실제로는 특별한 명령어나 함수 호출이 필요할 수 있음)
    BytecodeInstruction createArrayInstr(BytecodeOpCode::LOAD_CONST);
    createArrayInstr.registers.push_back(resultReg);
    createArrayInstr.operands.push_back(std::string("[]")); // 배열 생성 표시 (예시)
    addInstruction(createArrayInstr);

    // 배열 요소 추가 (실제로는 각 요소를 평가하고 배열에 추가하는 코드 필요)
    // 이 예제에서는 상수 요소 3개를 추가하는 것으로 시뮬레이션

    // 요소 1
    int elem1Reg = allocateRegister();
    BytecodeInstruction elem1Instr(BytecodeOpCode::LOAD_CONST);
    elem1Instr.registers.push_back(elem1Reg);
    elem1Instr.operands.push_back(1);
    addInstruction(elem1Instr);

    // 요소 2
    int elem2Reg = allocateRegister();
    BytecodeInstruction elem2Instr(BytecodeOpCode::LOAD_CONST);
    elem2Instr.registers.push_back(elem2Reg);
    elem2Instr.operands.push_back(2);
    addInstruction(elem2Instr);

    // 요소 3
    int elem3Reg = allocateRegister();
    BytecodeInstruction elem3Instr(BytecodeOpCode::LOAD_CONST);
    elem3Instr.registers.push_back(elem3Reg);
    elem3Instr.operands.push_back(3);
    addInstruction(elem3Instr);

    // 요소 레지스터 해제
    freeRegister(elem1Reg);
    freeRegister(elem2Reg);
    freeRegister(elem3Reg);

    // 결과 레지스터 번호 반환 (배열 참조)
    return resultReg;
}

std::any BytecodeGenerator::visit_boolean_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // Boolean 리터럴 AST 노드 처리
    // 상수 값을 레지스터에 로드

    // 다운캐스팅을 통해 실제 Boolean 리터럴 노드 접근
    auto boolNode = std::dynamic_pointer_cast<ast::expression::BooleanLiteralNode>(node);
    if (!boolNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 불리언 값 가져오기
    bool value = boolNode->get_value();

    // 상수 로드 명령어 생성
    BytecodeInstruction instr(BytecodeOpCode::LOAD_CONST);
    instr.registers.push_back(resultReg);              // 결과를 저장할 레지스터
    instr.operands.push_back(static_cast<int>(value)); // 불리언 값을 정수로 변환 (0 또는 1)

    // 명령어 추가
    addInstruction(instr);

    // 결과 레지스터 번호 반환 - 다른 표현식에서 이 레지스터를 사용할 수 있음
    return resultReg;
}

std::any BytecodeGenerator::visit_call_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 함수 호출 처리
    // 인자 표현식들을 방문한 후 함수 호출 명령어 생성

    return {};
}

std::any BytecodeGenerator::visit_function_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 함수 리터럴 처리
    // 새로운 섹션 생성 후 함수 본문 방문

    return {};
}

std::any BytecodeGenerator::visit_identifier_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 식별자 표현식 처리
    // 변수의 값을 레지스터로 로드

    // 다운캐스팅을 통해 실제 식별자 노드 접근
    auto idNode = std::dynamic_pointer_cast<ast::expression::IdentifierLiteralNode>(node);
    if (!idNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 변수 이름 가져오기
    icu::UnicodeString uniStr = idNode->to_str();
    std::string varName;
    uniStr.toUTF8String(varName);

    // 변수 값 로드 명령어 생성
    BytecodeInstruction instr(BytecodeOpCode::LOAD_VAR);
    instr.registers.push_back(resultReg); // 결과를 저장할 레지스터
    instr.operands.push_back(varName);    // 변수 이름

    // 명령어 추가
    addInstruction(instr);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_if_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // if 표현식 처리
    // 조건 평가 후 결과에 따라 분기

    // AST 노드의 문자열 표현으로부터 필요한 정보 추출
    // 이 예제에서는 간단한 구현만 제공합니다.

    // 결과를 저장할 레지스터 할당
    int resultReg = allocateRegister();

    // 조건 평가 (예시 값)
    int condReg = allocateRegister();
    BytecodeInstruction condInstr(BytecodeOpCode::LOAD_CONST);
    condInstr.registers.push_back(condReg);
    condInstr.operands.push_back(1); // 예시 값 (true)
    addInstruction(condInstr);

    // 조건부 점프를 위한 라벨 생성
    std::string elseLabel = generateUniqueLabel();
    std::string endLabel = generateUniqueLabel();

    // 조건이 거짓이면 else 블록으로 점프
    BytecodeInstruction jumpIfFalse(BytecodeOpCode::JMP_IF_FALSE);
    jumpIfFalse.registers.push_back(condReg);
    jumpIfFalse.operands.push_back(elseLabel);
    addInstruction(jumpIfFalse);

    // then 블록 (예시)
    BytecodeInstruction thenInstr(BytecodeOpCode::LOAD_CONST);
    thenInstr.registers.push_back(resultReg);
    thenInstr.operands.push_back(10); // then 결과 값
    addInstruction(thenInstr);

    // then 블록 처리 후 end로 점프
    BytecodeInstruction jumpToEnd(BytecodeOpCode::JMP);
    jumpToEnd.operands.push_back(endLabel);
    addInstruction(jumpToEnd);

    // else 라벨
    // (실제 코드에서는 여기에 라벨 정의 관련 코드 추가)

    // else 블록 (예시)
    BytecodeInstruction elseInstr(BytecodeOpCode::LOAD_CONST);
    elseInstr.registers.push_back(resultReg);
    elseInstr.operands.push_back(20); // else 결과 값
    addInstruction(elseInstr);

    // end 라벨
    // (실제 코드에서는 여기에 라벨 정의 관련 코드 추가)

    // 조건 레지스터 해제
    freeRegister(condReg);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_index_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 배열 인덱싱 표현식 처리

    // 이 예제에서는 간단한 구현만 제공합니다.

    // 결과를 저장할 레지스터 할당
    int resultReg = allocateRegister();

    // 배열 참조를 담을 레지스터
    int arrayReg = allocateRegister();
    BytecodeInstruction arrayInstr(BytecodeOpCode::LOAD_VAR);
    arrayInstr.registers.push_back(arrayReg);
    arrayInstr.operands.push_back(std::string("array")); // 배열 변수 이름 (예시)
    addInstruction(arrayInstr);

    // 인덱스를 담을 레지스터
    int indexReg = allocateRegister();
    BytecodeInstruction indexInstr(BytecodeOpCode::LOAD_CONST);
    indexInstr.registers.push_back(indexReg);
    indexInstr.operands.push_back(1); // 인덱스 값 (예시)
    addInstruction(indexInstr);

    // 인덱싱 연산 (실제로는 특별한 명령어나 함수 호출이 필요할 수 있음)
    // 여기서는 가상의 "INDEX" 명령어를 가정하여 시뮬레이션
    BytecodeInstruction indexingInstr(BytecodeOpCode::LOAD_CONST); // 실제로는 INDEX 등의 전용 명령어 필요
    indexingInstr.registers.push_back(resultReg);
    indexingInstr.registers.push_back(arrayReg);
    indexingInstr.registers.push_back(indexReg);
    addInstruction(indexingInstr);

    // 임시 레지스터 해제
    freeRegister(arrayReg);
    freeRegister(indexReg);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_infix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 이항 연산자 표현식 처리

    // 다운캐스팅을 통해 실제 이항 연산자 노드 접근
    auto infixNode = std::dynamic_pointer_cast<ast::expression::InfixExpressionNode>(node);
    if (!infixNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 클래스 구조를 확인한 결과, private 멤버 변수에 직접 접근할 수 없음
    // 따라서 간접적인 방법으로 처리해야 함

    // AST 노드의 문자열 표현으로부터 필요한 정보 추출
    icu::UnicodeString nodeStr = infixNode->to_str();

    // 좌측 피연산자 평가
    // 실제 구현에서는 AST 구조에 따라 자식 노드를 가져오는 코드가 필요합니다.
    // 이 예제에서는 간단하게 상수 값을 사용하여 시뮬레이션합니다.
    int leftReg = allocateRegister();
    BytecodeInstruction leftInstr(BytecodeOpCode::LOAD_CONST);
    leftInstr.registers.push_back(leftReg);
    leftInstr.operands.push_back(1); // 예시 값
    addInstruction(leftInstr);

    // 우측 피연산자 평가
    int rightReg = allocateRegister();
    BytecodeInstruction rightInstr(BytecodeOpCode::LOAD_CONST);
    rightInstr.registers.push_back(rightReg);
    rightInstr.operands.push_back(2); // 예시 값
    addInstruction(rightInstr);

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 연산자 가져오기 (to_str 메소드를 사용하여 연산자 문자열 추출)
    icu::UnicodeString opStr = nodeStr;
    std::string op;

    // InfixExpressionNode의 to_str()은 대략 "left operator right" 형태의 문자열 반환
    // 연산자 부분만 추출 (간단한 구현을 위해 + - * / 등 기본 연산자만 지원)
    if (opStr.indexOf('+') >= 0) {
        op = "+";
    } else if (opStr.indexOf('-') >= 0) {
        op = "-";
    } else if (opStr.indexOf('*') >= 0) {
        op = "*";
    } else if (opStr.indexOf('/') >= 0) {
        op = "/";
    } else if (opStr.indexOf('%') >= 0) {
        op = "%";
    } else if (opStr.indexOf("==") >= 0) {
        op = "==";
    } else if (opStr.indexOf("!=") >= 0) {
        op = "!=";
    } else if (opStr.indexOf("<=") >= 0) {
        op = "<=";
    } else if (opStr.indexOf(">=") >= 0) {
        op = ">=";
    } else if (opStr.indexOf('<') >= 0) {
        op = "<";
    } else if (opStr.indexOf('>') >= 0) {
        op = ">";
    } else if (opStr.indexOf("&&") >= 0) {
        op = "&&";
    } else if (opStr.indexOf("||") >= 0) {
        op = "||";
    } else {
        // 지원하지 않는 연산자
        freeRegister(leftReg);
        freeRegister(rightReg);
        freeRegister(resultReg);
        return {};
    }

    // 연산자에 따라 적절한 명령어 생성
    BytecodeOpCode opcode;

    if (op == "+") {
        opcode = BytecodeOpCode::ADD;
    } else if (op == "-") {
        opcode = BytecodeOpCode::SUB;
    } else if (op == "*") {
        opcode = BytecodeOpCode::MUL;
    } else if (op == "/") {
        opcode = BytecodeOpCode::DIV;
    } else if (op == "%") {
        opcode = BytecodeOpCode::MOD;
    } else if (op == "==") {
        opcode = BytecodeOpCode::CMP_EQ;
    } else if (op == "!=") {
        opcode = BytecodeOpCode::CMP_NE;
    } else if (op == "<") {
        opcode = BytecodeOpCode::CMP_LT;
    } else if (op == "<=") {
        opcode = BytecodeOpCode::CMP_LE;
    } else if (op == ">") {
        opcode = BytecodeOpCode::CMP_GT;
    } else if (op == ">=") {
        opcode = BytecodeOpCode::CMP_GE;
    } else if (op == "&&") {
        opcode = BytecodeOpCode::AND;
    } else if (op == "||") {
        opcode = BytecodeOpCode::OR;
    } else {
        // 지원하지 않는 연산자
        freeRegister(leftReg);
        freeRegister(rightReg);
        freeRegister(resultReg);
        return {};
    }

    // 연산 명령어 생성
    BytecodeInstruction instr(opcode);
    instr.registers.push_back(resultReg); // 결과 레지스터
    instr.registers.push_back(leftReg);   // 좌측 피연산자 레지스터
    instr.registers.push_back(rightReg);  // 우측 피연산자 레지스터

    // 명령어 추가
    addInstruction(instr);

    // 피연산자 레지스터 해제 (더 이상 필요 없음)
    freeRegister(leftReg);
    freeRegister(rightReg);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_number_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // Number 리터럴 AST 노드 처리
    // 상수 값을 레지스터에 로드

    // 다운캐스팅을 통해 실제 Number 리터럴 노드 접근
    auto numNode = std::dynamic_pointer_cast<ast::expression::NumberLiteralNode>(node);
    if (!numNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 숫자 값 가져오기
    icu::UnicodeString valueStr = numNode->to_str();
    std::string value;
    valueStr.toUTF8String(value);

    // 정수인지 실수인지 확인
    bool isInt = valueStr.indexOf('.') < 0;
    BytecodeOperand operand;

    if (isInt) {
        // 정수로 처리
        try {
            operand = std::stoi(value);
        } catch (...) {
            // 변환 실패 시 0으로 기본값 설정
            operand = 0;
        }
    } else {
        // 실수로 처리
        try {
            operand = std::stof(value);
        } catch (...) {
            // 변환 실패 시 0.0으로 기본값 설정
            operand = 0.0f;
        }
    }

    // 상수 로드 명령어 생성
    BytecodeInstruction instr(BytecodeOpCode::LOAD_CONST);
    instr.registers.push_back(resultReg); // 결과를 저장할 레지스터
    instr.operands.push_back(operand);    // 숫자 값

    // 명령어 추가
    addInstruction(instr);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_postfix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 후위 연산자 표현식 처리

    // 다운캐스팅을 통해 실제 후위 연산자 노드 접근
    auto postNode = std::dynamic_pointer_cast<ast::expression::PostExpressionNode>(node);
    if (!postNode) {
        // 잘못된 노드 타입
        return {};
    }

    // AST 노드의 문자열 표현으로부터 필요한 정보 추출
    icu::UnicodeString nodeStr = postNode->to_str();

    // 피연산자 평가
    // 실제 구현에서는 AST 구조에 따라 자식 노드를 가져오는 코드가 필요합니다.
    // 이 예제에서는 간단하게 상수 값을 사용하여 시뮬레이션합니다.
    int operandReg = allocateRegister();
    BytecodeInstruction operandInstr(BytecodeOpCode::LOAD_CONST);
    operandInstr.registers.push_back(operandReg);
    operandInstr.operands.push_back(10); // 예시 값
    addInstruction(operandInstr);

    // 변수 이름 (실제로는 AST 노드에서 가져와야 함)
    std::string varName = "x"; // 예시 값

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();
    int updatedReg = allocateRegister();

    // 연산자 가져오기 (to_str 메소드로 추출)
    icu::UnicodeString opStr = nodeStr;
    std::string op;

    // 연산자 추출 (간단한 구현을 위해 ++ 와 -- 만 지원)
    if (opStr.indexOf("++") >= 0) {
        op = "++";
    } else if (opStr.indexOf("--") >= 0) {
        op = "--";
    } else {
        // 지원하지 않는 연산자
        freeRegister(operandReg);
        freeRegister(resultReg);
        freeRegister(updatedReg);
        return {};
    }

    // 연산자에 따라 적절한 명령어 생성
    if (op == "++") {
        // 원래 값을 결과 레지스터에 복사
        BytecodeInstruction movInstr(BytecodeOpCode::MOV_REG);
        movInstr.registers.push_back(resultReg);  // 대상 레지스터
        movInstr.registers.push_back(operandReg); // 소스 레지스터
        addInstruction(movInstr);

        // 1을 담을 임시 레지스터
        int oneReg = allocateRegister();

        // 1 로드
        BytecodeInstruction loadOne(BytecodeOpCode::LOAD_CONST);
        loadOne.registers.push_back(oneReg);
        loadOne.operands.push_back(1);
        addInstruction(loadOne);

        // 덧셈 수행 (증가)
        BytecodeInstruction addInstr(BytecodeOpCode::ADD);
        addInstr.registers.push_back(updatedReg); // 업데이트된 값 레지스터
        addInstr.registers.push_back(operandReg); // 원래 값 레지스터
        addInstr.registers.push_back(oneReg);     // 1을 담은 레지스터
        addInstruction(addInstr);

        // 임시 레지스터 해제
        freeRegister(oneReg);

    } else if (op == "--") {
        // 원래 값을 결과 레지스터에 복사
        BytecodeInstruction movInstr(BytecodeOpCode::MOV_REG);
        movInstr.registers.push_back(resultReg);  // 대상 레지스터
        movInstr.registers.push_back(operandReg); // 소스 레지스터
        addInstruction(movInstr);

        // 1을 담을 임시 레지스터
        int oneReg = allocateRegister();

        // 1 로드
        BytecodeInstruction loadOne(BytecodeOpCode::LOAD_CONST);
        loadOne.registers.push_back(oneReg);
        loadOne.operands.push_back(1);
        addInstruction(loadOne);

        // 뺄셈 수행 (감소)
        BytecodeInstruction subInstr(BytecodeOpCode::SUB);
        subInstr.registers.push_back(updatedReg); // 업데이트된 값 레지스터
        subInstr.registers.push_back(operandReg); // 원래 값 레지스터
        subInstr.registers.push_back(oneReg);     // 1을 담은 레지스터
        addInstruction(subInstr);

        // 임시 레지스터 해제
        freeRegister(oneReg);

    } else {
        // 지원하지 않는 연산자
        freeRegister(operandReg);
        freeRegister(resultReg);
        freeRegister(updatedReg);
        return {};
    }

    // 변수에 업데이트된 값 저장
    BytecodeInstruction storeInstr(BytecodeOpCode::STORE_VAR);
    storeInstr.registers.push_back(updatedReg); // 저장할 값이 있는 레지스터
    storeInstr.operands.push_back(varName);     // 변수 이름
    addInstruction(storeInstr);

    // 피연산자 및 임시 레지스터 해제
    freeRegister(operandReg);
    freeRegister(updatedReg);

    // 결과 레지스터 번호 반환 (원래 값)
    return resultReg;
}

std::any BytecodeGenerator::visit_prefix_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // 전위 연산자 표현식 처리

    // 다운캐스팅을 통해 실제 전위 연산자 노드 접근
    auto prefixNode = std::dynamic_pointer_cast<ast::expression::PrefixExpressionNode>(node);
    if (!prefixNode) {
        // 잘못된 노드 타입
        return {};
    }

    // AST 노드의 문자열 표현으로부터 필요한 정보 추출
    icu::UnicodeString nodeStr = prefixNode->to_str();

    // 피연산자 평가
    // 실제 구현에서는 AST 구조에 따라 자식 노드를 가져오는 코드가 필요합니다.
    // 이 예제에서는 간단하게 상수 값을 사용하여 시뮬레이션합니다.
    int operandReg = allocateRegister();
    BytecodeInstruction operandInstr(BytecodeOpCode::LOAD_CONST);
    operandInstr.registers.push_back(operandReg);
    operandInstr.operands.push_back(5); // 예시 값
    addInstruction(operandInstr);

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 연산자 가져오기 (to_str 메소드로 추출)
    icu::UnicodeString opStr = nodeStr;
    std::string op;

    // 연산자 추출 (간단한 구현을 위해 - 와 ! 만 지원)
    if (opStr.indexOf('-') >= 0) {
        op = "-";
    } else if (opStr.indexOf('!') >= 0) {
        op = "!";
    } else {
        // 지원하지 않는 연산자
        freeRegister(operandReg);
        freeRegister(resultReg);
        return {};
    }

    // 연산자에 따라 적절한 명령어 생성
    if (op == "-") {
        // 단항 마이너스 (부호 반전)
        // SUB 명령어를 사용하여 0에서 값을 뺌

        // 0을 담을 임시 레지스터
        int zeroReg = allocateRegister();

        // 0 로드
        BytecodeInstruction loadZero(BytecodeOpCode::LOAD_CONST);
        loadZero.registers.push_back(zeroReg);
        loadZero.operands.push_back(0);
        addInstruction(loadZero);

        // 뺄셈 수행
        BytecodeInstruction subInstr(BytecodeOpCode::SUB);
        subInstr.registers.push_back(resultReg);  // 결과 레지스터
        subInstr.registers.push_back(zeroReg);    // 0을 담은 레지스터
        subInstr.registers.push_back(operandReg); // 피연산자 레지스터
        addInstruction(subInstr);

        // 임시 레지스터 해제
        freeRegister(zeroReg);

    } else if (op == "!") {
        // 논리 NOT
        BytecodeInstruction notInstr(BytecodeOpCode::NOT);
        notInstr.registers.push_back(resultReg);  // 결과 레지스터
        notInstr.registers.push_back(operandReg); // 피연산자 레지스터
        addInstruction(notInstr);

    } else {
        // 지원하지 않는 연산자
        freeRegister(operandReg);
        freeRegister(resultReg);
        return {};
    }

    // 피연산자 레지스터 해제
    freeRegister(operandReg);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_string_literal_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // String 리터럴 AST 노드 처리
    // 문자열 상수를 레지스터에 로드

    // 다운캐스팅을 통해 실제 String 리터럴 노드 접근
    auto strNode = std::dynamic_pointer_cast<ast::expression::StringLiteralNode>(node);
    if (!strNode) {
        // 잘못된 노드 타입
        return {};
    }

    // 결과를 저장할 새 레지스터 할당
    int resultReg = allocateRegister();

    // 문자열 값 가져오기
    icu::UnicodeString uniStr = strNode->to_str();
    std::string value;
    uniStr.toUTF8String(value);

    // 따옴표 제거 (첫 번째와 마지막 문자)
    if (value.size() >= 2 && (value[0] == '"' || value[0] == '\'') && value[0] == value[value.size() - 1]) {
        value = value.substr(1, value.size() - 2);
    }

    // 상수 로드 명령어 생성
    BytecodeInstruction instr(BytecodeOpCode::LOAD_CONST);
    instr.registers.push_back(resultReg); // 결과를 저장할 레지스터
    instr.operands.push_back(value);      // 문자열 값

    // 명령어 추가
    addInstruction(instr);

    // 결과 레지스터 번호 반환
    return resultReg;
}

std::any BytecodeGenerator::visit_when_expression(const ast::ASTNodePtr &node, const std::unordered_map<icu::UnicodeString, std::any> &context) {
    // when 표현식 처리 (switch-case와 유사)
    // 조건과 각 케이스를 방문하여 바이트코드 생성

    return {};
}

void BytecodeGenerator::generate(const ast::ASTNodePtr &rootNode) {
    // AST 루트 노드부터 시작하여 바이트코드 생성
    visit(rootNode);
}

BytecodeSection &BytecodeGenerator::getCurrentSection() {
    if (m_currentSectionIndex < 0 || m_currentSectionIndex >= static_cast<int>(m_sections.size())) {
        throw std::runtime_error("Invalid bytecode section index");
    }

    return m_sections[m_currentSectionIndex];
}

void BytecodeGenerator::addInstruction(const BytecodeInstruction &instruction) { getCurrentSection().addInstruction(instruction); }

int BytecodeGenerator::allocateRegister() {
    // 가용한 레지스터가 있다면 재사용
    if (!m_freeRegisters.empty()) {
        int reg = m_freeRegisters.back();
        m_freeRegisters.pop_back();
        return reg;
    }

    // 새 레지스터 할당
    return m_nextRegister++;
}

void BytecodeGenerator::freeRegister(int regId) {
    // 레지스터 반환 (재사용 가능하게)
    m_freeRegisters.push_back(regId);
}

void BytecodeGenerator::resetRegisters() {
    // 모든 레지스터 초기화
    m_nextRegister = 0;
    m_freeRegisters.clear();
}

const std::vector<BytecodeSection> &BytecodeGenerator::getSections() const { return m_sections; }

std::string BytecodeGenerator::dumpBytecode() const {
    std::stringstream ss;

    // 각 섹션 출력
    for (size_t i = 0; i < m_sections.size(); ++i) {
        const auto &section = m_sections[i];
        ss << "Section: " << section.getName() << "\n";

        // 섹션 내 명령어 출력
        const auto &instructions = section.getInstructions();
        for (size_t j = 0; j < instructions.size(); ++j) {
            const auto &instr = instructions[j];

            // 명령어 주소(오프셋)
            ss << std::setw(4) << std::setfill('0') << j << ": ";

            // 명령어 유형
            ss << opCodeToString(instr.opcode) << " ";

            // 레지스터 정보
            for (size_t k = 0; k < instr.registers.size(); ++k) {
                ss << "r" << instr.registers[k];
                if (k < instr.registers.size() - 1) {
                    ss << ", ";
                }
            }

            // 추가 피연산자 정보 (선택적)
            if (!instr.operands.empty()) {
                if (!instr.registers.empty()) {
                    ss << ", ";
                }

                for (size_t k = 0; k < instr.operands.size(); ++k) {
                    ss << operandToString(instr.operands[k]);
                    if (k < instr.operands.size() - 1) {
                        ss << ", ";
                    }
                }
            }

            ss << "\n";
        }

        ss << "\n";
    }

    return ss.str();
}

std::string BytecodeGenerator::generateUniqueLabel() { return "L_" + std::to_string(m_nextLabelId++); }

// 헬퍼 함수 구현
std::string opCodeToString(BytecodeOpCode opcode) {
    switch (opcode) {
    case BytecodeOpCode::LOAD_CONST:
        return "LOAD_CONST";
    case BytecodeOpCode::LOAD_VAR:
        return "LOAD_VAR";
    case BytecodeOpCode::STORE_VAR:
        return "STORE_VAR";
    case BytecodeOpCode::MOV_REG:
        return "MOV_REG";
    case BytecodeOpCode::ADD:
        return "ADD";
    case BytecodeOpCode::SUB:
        return "SUB";
    case BytecodeOpCode::MUL:
        return "MUL";
    case BytecodeOpCode::DIV:
        return "DIV";
    case BytecodeOpCode::MOD:
        return "MOD";
    case BytecodeOpCode::CMP_EQ:
        return "CMP_EQ";
    case BytecodeOpCode::CMP_NE:
        return "CMP_NE";
    case BytecodeOpCode::CMP_LT:
        return "CMP_LT";
    case BytecodeOpCode::CMP_LE:
        return "CMP_LE";
    case BytecodeOpCode::CMP_GT:
        return "CMP_GT";
    case BytecodeOpCode::CMP_GE:
        return "CMP_GE";
    case BytecodeOpCode::AND:
        return "AND";
    case BytecodeOpCode::OR:
        return "OR";
    case BytecodeOpCode::NOT:
        return "NOT";
    case BytecodeOpCode::JMP:
        return "JMP";
    case BytecodeOpCode::JMP_IF_TRUE:
        return "JMP_IF_TRUE";
    case BytecodeOpCode::JMP_IF_FALSE:
        return "JMP_IF_FALSE";
    case BytecodeOpCode::CALL:
        return "CALL";
    case BytecodeOpCode::RET:
        return "RET";
    case BytecodeOpCode::NOP:
        return "NOP";
    default:
        return "UNKNOWN";
    }
}

std::string operandToString(const BytecodeOperand &operand) {
    return std::visit(
        [](const auto &value) -> std::string {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, int>) {
                return std::to_string(value);
            } else if constexpr (std::is_same_v<T, float>) {
                return std::to_string(value);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return "\"" + value + "\"";
            } else if constexpr (std::is_same_v<T, Tag>) {
                return "tag:" + std::to_string(value.hash());
            } else {
                return "unknown";
            }
        },
        operand);
}

} // namespace nugdev::compiler::generation