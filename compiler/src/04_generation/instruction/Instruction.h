#pragma once

#include "04_generation/register/Register.hpp"

namespace nugdev::compiler::generation {

enum class InstructionCode {
    // 데이터 로드/저장 명령어
    LOAD,       // 메모리에서 레지스터로 값 로드
    STORE,      // 레지스터 값을 메모리에 저장
    LOAD_CONST, // 상수 값을 레지스터에 로드
    MOV,        // 레지스터 간 값 이동

    // 산술 연산 명령어
    ADD, // 덧셈
    SUB, // 뺄셈
    MUL, // 곱셈
    DIV, // 나눗셈
    MOD, // 나머지
    INC, // 증가
    DEC, // 감소

    // 비트 및 논리 연산
    AND, // 비트 AND
    OR,  // 비트 OR
    XOR, // 비트 XOR
    NOT, // 비트 NOT
    SHL, // 왼쪽 시프트
    SHR, // 오른쪽 시프트

    // 비교 연산
    CMP,  // 비교
    TEST, // 비트 테스트

    // 분기 명령어
    JMP, // 무조건 점프
    JE,  // 같으면 점프
    JNE, // 같지 않으면 점프
    JG,  // 크면 점프
    JGE, // 크거나 같으면 점프
    JL,  // 작으면 점프
    JLE, // 작거나 같으면 점프

    // 함수 호출 관련
    CALL,     // 함수 호출 (레지스터에 반환 주소 저장)
    RET,      // 함수에서 반환
    SAVE_REG, // 레지스터 상태 저장
    REST_REG, // 레지스터 상태 복원

    // 시스템 콜 및 VM 제어
    SYSCALL, // 시스템 콜 실행
    INT,     // 인터럽트
    HALT,    // VM 실행 중지
    NOP      // 아무 작업 없음
};

struct Instruction {
    InstructionCode code;
    std::vector<UniversalRegister> registers;
};

} // namespace nugdev::compiler::generation