```c
/**
 * 간단한 레지스터 기반 가상 머신 구현
 * 가변 크기 명령어 형식 지원 및 수동 메모리 관리
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// 최대 레지스터 수
#define NUM_REGISTERS 16
// 최대 힙 크기 (바이트)
#define MAX_HEAP_SIZE (1024 * 1024)
// 최대 코드 크기 (바이트)
#define MAX_CODE_SIZE (64 * 1024)

// 값의 타입 정의
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_PTR,
    TYPE_NULL
} ValueType;

// 값 구조체
typedef struct {
    ValueType type;
    union {
        int32_t i;
        float f;
        void* ptr;
    } as;
} Value;

// OpCode 열거형
typedef enum {
    // 0x00-0x0F: 기본 레지스터 연산 (1바이트 + 레지스터 지정자)
    OP_MOVE = 0x00,      // 0x00: Rx = Ry
    OP_LOAD_NULL,        // 0x01: Rx = null
    
    // 0x10-0x1F: 즉시값 로드 (1바이트 + 레지스터 + 즉시값)
    OP_LOAD_I = 0x10,    // 0x10: Rx = immediate integer
    OP_LOAD_F,           // 0x11: Rx = immediate float
    
    // 0x20-0x2F: 메모리 연산 (1바이트 + 레지스터들)
    OP_LOAD = 0x20,      // 0x20: Rx = Memory[Ry]
    OP_STORE,            // 0x21: Memory[Rx] = Ry
    OP_ALLOC,            // 0x22: Rx = alloc(Ry) - Ry는 크기
    OP_FREE,             // 0x23: free(Rx)
    
    // 0x30-0x3F: 산술 연산 (1바이트 + 레지스터들)
    OP_ADD = 0x30,       // 0x30: Rx = Ry + Rz
    OP_SUB,              // 0x31: Rx = Ry - Rz
    OP_MUL,              // 0x32: Rx = Ry * Rz
    OP_DIV,              // 0x33: Rx = Ry / Rz
    OP_MOD,              // 0x34: Rx = Ry % Rz
    
    // 0x40-0x4F: 논리 연산 (1바이트 + 레지스터들)
    OP_AND = 0x40,       // 0x40: Rx = Ry & Rz
    OP_OR,               // 0x41: Rx = Ry | Rz
    OP_XOR,              // 0x42: Rx = Ry ^ Rz
    OP_NOT,              // 0x43: Rx = ~Ry
    
    // 0x50-0x5F: 비교 연산 (1바이트 + 레지스터들)
    OP_EQ = 0x50,        // 0x50: Rx = (Ry == Rz)
    OP_LT,               // 0x51: Rx = (Ry < Rz)
    OP_LTE,              // 0x52: Rx = (Ry <= Rz)
    OP_GT,               // 0x53: Rx = (Ry > Rz)
    OP_GTE,              // 0x54: Rx = (Ry >= Rz)
    
    // 0x60-0x6F: 제어 흐름 (1바이트 + 추가 데이터)
    OP_JUMP = 0x60,      // 0x60: PC = address
    OP_JUMPZ,            // 0x61: if Rx == 0: PC = address
    OP_JUMPNZ,           // 0x62: if Rx != 0: PC = address
    OP_CALL,             // 0x63: Call function at address
    OP_RETURN,           // 0x64: Return from function
    
    // 0xF0-0xFF: 특수 명령어
    OP_PRINT = 0xF0,     // 0xF0: 디버그용 출력
    OP_HALT = 0xFF       // 0xFF: 실행 종료
} OpCode;

// 명령어 형식은 가변적
// 첫 바이트는 항상 OpCode
// 이후 바이트들은 명령어에 따라 다름

// VM 상태 구조체
typedef struct {
    // 레지스터
    Value registers[NUM_REGISTERS];
    
    // 프로그램 카운터
    uint32_t pc;
    
    // 메모리 관리
    uint8_t* heap;
    uint32_t heap_size;
    uint32_t heap_used;
    
    // 코드 영역
    uint8_t* code;
    uint32_t code_size;
    
    // 실행 상태
    bool running;
} VM;

// VM 초기화
VM* vm_init() {
    VM* vm = (VM*)malloc(sizeof(VM));
    if (!vm) return NULL;
    
    // 레지스터 초기화
    memset(vm->registers, 0, sizeof(vm->registers));
    for (int i = 0; i < NUM_REGISTERS; i++) {
        vm->registers[i].type = TYPE_NULL;
    }
    
    // 힙 메모리 할당
    vm->heap = (uint8_t*)malloc(MAX_HEAP_SIZE);
    if (!vm->heap) {
        free(vm);
        return NULL;
    }
    
    vm->heap_size = MAX_HEAP_SIZE;
    vm->heap_used = 0;
    
    // 코드 영역 할당
    vm->code = (uint8_t*)malloc(MAX_CODE_SIZE);
    if (!vm->code) {
        free(vm->heap);
        free(vm);
        return NULL;
    }
    
    memset(vm->code, 0, MAX_CODE_SIZE);
    vm->code_size = 0;
    vm->pc = 0;
    vm->running = false;
    
    return vm;
}

// VM 정리
void vm_cleanup(VM* vm) {
    if (!vm) return;
    
    // 힙 메모리 해제
    if (vm->heap) {
        free(vm->heap);
        vm->heap = NULL;
    }
    
    // 코드 영역 해제
    if (vm->code) {
        free(vm->code);
        vm->code = NULL;
    }
    
    // VM 해제
    free(vm);
}

// 메모리 할당 (단순 버전)
void* vm_alloc(VM* vm, uint32_t size) {
    // 메모리 정렬을 위해 크기 조정 (8바이트 경계)
    size = (size + 7) & ~7;
    
    if (vm->heap_used + size > vm->heap_size) {
        // 메모리 부족
        return NULL;
    }
    
    void* ptr = vm->heap + vm->heap_used;
    vm->heap_used += size;
    
    return ptr;
}

// 메모리 해제 (실제 구현에서는 더 복잡함)
void vm_free(VM* vm, void* ptr) {
    // 이 간단한 구현에서는 실제로 메모리를 해제하지 않음
    // 실제 구현에서는 메모리 관리자가 필요
}

// 명령어 디코딩 및 실행
uint32_t vm_execute_instruction(VM* vm) {
    if (vm->pc >= vm->code_size) {
        vm->running = false;
        return 0;
    }
    
    uint8_t* code = vm->code + vm->pc;
    OpCode op = (OpCode)code[0];
    uint32_t instr_size = 1;  // 기본 크기는 1바이트 (opcode)
    
    switch (op) {
        case OP_MOVE: {
            // 형식: [OP_MOVE][레지스터 정보]
            uint8_t reg_info = code[1];
            uint8_t rx = reg_info >> 4;   // 상위 4비트
            uint8_t ry = reg_info & 0x0F; // 하위 4비트
            
            vm->registers[rx] = vm->registers[ry];
            instr_size = 2;
            break;
        }
            
        case OP_LOAD_NULL: {
            // 형식: [OP_LOAD_NULL][레지스터]
            uint8_t rx = code[1];
            
            vm->registers[rx].type = TYPE_NULL;
            vm->registers[rx].as.ptr = NULL;
            instr_size = 2;
            break;
        }
            
        case OP_LOAD_I: {
            // 형식: [OP_LOAD_I][레지스터][값 4바이트]
            uint8_t rx = code[1];
            int32_t value;
            memcpy(&value, &code[2], sizeof(int32_t));
            
            vm->registers[rx].type = TYPE_INT;
            vm->registers[rx].as.i = value;
            instr_size = 6;  // 1(opcode) + 1(register) + 4(value)
            break;
        }
            
        case OP_LOAD_F: {
            // 형식: [OP_LOAD_F][레지스터][값 4바이트]
            uint8_t rx = code[1];
            float value;
            memcpy(&value, &code[2], sizeof(float));
            
            vm->registers[rx].type = TYPE_FLOAT;
            vm->registers[rx].as.f = value;
            instr_size = 6;  // 1(opcode) + 1(register) + 4(value)
            break;
        }
            
        case OP_ADD: {
            // 형식: [OP_ADD][rx][ry][rz]
            uint8_t rx = code[1];
            uint8_t ry = code[2];
            uint8_t rz = code[3];
            
            if (vm->registers[ry].type == TYPE_INT && vm->registers[rz].type == TYPE_INT) {
                vm->registers[rx].type = TYPE_INT;
                vm->registers[rx].as.i = vm->registers[ry].as.i + vm->registers[rz].as.i;
            } else if (vm->registers[ry].type == TYPE_FLOAT && vm->registers[rz].type == TYPE_FLOAT) {
                vm->registers[rx].type = TYPE_FLOAT;
                vm->registers[rx].as.f = vm->registers[ry].as.f + vm->registers[rz].as.f;
            }
            instr_size = 4;
            break;
        }
            
        case OP_SUB: {
            // 형식: [OP_SUB][rx][ry][rz]
            uint8_t rx = code[1];
            uint8_t ry = code[2];
            uint8_t rz = code[3];
            
            if (vm->registers[ry].type == TYPE_INT && vm->registers[rz].type == TYPE_INT) {
                vm->registers[rx].type = TYPE_INT;
                vm->registers[rx].as.i = vm->registers[ry].as.i - vm->registers[rz].as.i;
            } else if (vm->registers[ry].type == TYPE_FLOAT && vm->registers[rz].type == TYPE_FLOAT) {
                vm->registers[rx].type = TYPE_FLOAT;
                vm->registers[rx].as.f = vm->registers[ry].as.f - vm->registers[rz].as.f;
            }
            instr_size = 4;
            break;
        }
            
        case OP_MUL: {
            // 형식: [OP_MUL][rx][ry][rz]
            uint8_t rx = code[1];
            uint8_t ry = code[2];
            uint8_t rz = code[3];
            
            if (vm->registers[ry].type == TYPE_INT && vm->registers[rz].type == TYPE_INT) {
                vm->registers[rx].type = TYPE_INT;
                vm->registers[rx].as.i = vm->registers[ry].as.i * vm->registers[rz].as.i;
            } else if (vm->registers[ry].type == TYPE_FLOAT && vm->registers[rz].type == TYPE_FLOAT) {
                vm->registers[rx].type = TYPE_FLOAT;
                vm->registers[rx].as.f = vm->registers[ry].as.f * vm->registers[rz].as.f;
            }
            instr_size = 4;
            break;
        }
            
        case OP_DIV: {
            // 형식: [OP_DIV][rx][ry][rz]
            uint8_t rx = code[1];
            uint8_t ry = code[2];
            uint8_t rz = code[3];
            
            if (vm->registers[ry].type == TYPE_INT && vm->registers[rz].type == TYPE_INT && 
                vm->registers[rz].as.i != 0) {
                vm->registers[rx].type = TYPE_INT;
                vm->registers[rx].as.i = vm->registers[ry].as.i / vm->registers[rz].as.i;
            } else if (vm->registers[ry].type == TYPE_FLOAT && vm->registers[rz].type == TYPE_FLOAT && 
                       vm->registers[rz].as.f != 0.0f) {
                vm->registers[rx].type = TYPE_FLOAT;
                vm->registers[rx].as.f = vm->registers[ry].as.f / vm->registers[rz].as.f;
            }
            instr_size = 4;
            break;
        }
            
        case OP_ALLOC: {
            // 형식: [OP_ALLOC][rx][ry]
            uint8_t rx = code[1];
            uint8_t ry = code[2];
            
            if (vm->registers[ry].type == TYPE_INT) {
                vm->registers[rx].type = TYPE_PTR;
                vm->registers[rx].as.ptr = vm_alloc(vm, vm->registers[ry].as.i);
            }
            instr_size = 3;
            break;
        }
            
        case OP_FREE: {
            // 형식: [OP_FREE][rx]
            uint8_t rx = code[1];
            
            if (vm->registers[rx].type == TYPE_PTR) {
                vm_free(vm, vm->registers[rx].as.ptr);
                vm->registers[rx].type = TYPE_NULL;
                vm->registers[rx].as.ptr = NULL;
            }
            instr_size = 2;
            break;
        }
            
        case OP_JUMP: {
            // 형식: [OP_JUMP][주소 4바이트]
            uint32_t addr;
            memcpy(&addr, &code[1], sizeof(uint32_t));
            
            if (addr < vm->code_size) {
                vm->pc = addr;
                return 0;  // 이미 PC를 업데이트했으므로 반환
            }
            instr_size = 5;
            break;
        }
            
        case OP_JUMPZ: {
            // 형식: [OP_JUMPZ][레지스터][주소 4바이트]
            uint8_t rx = code[1];
            uint32_t addr;
            memcpy(&addr, &code[2], sizeof(uint32_t));
            
            if (vm->registers[rx].type == TYPE_INT && vm->registers[rx].as.i == 0 && 
                addr < vm->code_size) {
                vm->pc = addr;
                return 0;  // 이미 PC를 업데이트했으므로 반환
            }
            instr_size = 6;
            break;
        }
            
        case OP_JUMPNZ: {
            // 형식: [OP_JUMPNZ][레지스터][주소 4바이트]
            uint8_t rx = code[1];
            uint32_t addr;
            memcpy(&addr, &code[2], sizeof(uint32_t));
            
            if (vm->registers[rx].type == TYPE_INT && vm->registers[rx].as.i != 0 &&
                addr < vm->code_size) {
                vm->pc = addr;
                return 0;  // 이미 PC를 업데이트했으므로 반환
            }
            instr_size = 6;
            break;
        }
            
        case OP_PRINT: {
            // 형식: [OP_PRINT][레지스터]
            uint8_t rx = code[1];
            
            if (vm->registers[rx].type == TYPE_INT) {
                printf("INT: %d\n", vm->registers[rx].as.i);
            } else if (vm->registers[rx].type == TYPE_FLOAT) {
                printf("FLOAT: %f\n", vm->registers[rx].as.f);
            } else if (vm->registers[rx].type == TYPE_PTR) {
                printf("POINTER: %p\n", vm->registers[rx].as.ptr);
            } else {
                printf("NULL\n");
            }
            instr_size = 2;
            break;
        }
            
        case OP_HALT:
            vm->running = false;
            instr_size = 1;
            break;
            
        default:
            // 알 수 없는 명령어 (향후 오류 처리)
            printf("Unknown opcode: 0x%02X at position %u\n", op, vm->pc);
            vm->running = false;
            instr_size = 1;
            break;
    }
    
    return instr_size;
}

// VM 실행
void vm_run(VM* vm) {
    if (!vm || !vm->code || vm->code_size == 0) return;
    
    vm->running = true;
    vm->pc = 0;
    
    while (vm->running && vm->pc < vm->code_size) {
        uint32_t instr_size = vm_execute_instruction(vm);
        if (instr_size > 0) {
            vm->pc += instr_size;
        }
    }
}

// 코드 로드 함수
bool vm_load_code(VM* vm, uint8_t* code, uint32_t size) {
    if (!vm || !code || size > MAX_CODE_SIZE) return false;
    
    memcpy(vm->code, code, size);
    vm->code_size = size;
    return true;
}

// 바이트코드 생성을 위한 헬퍼 함수들
void bytecode_append_byte(uint8_t** code, uint32_t* size, uint8_t value) {
    (*code)[(*size)++] = value;
}

void bytecode_append_int32(uint8_t** code, uint32_t* size, int32_t value) {
    memcpy(*code + *size, &value, sizeof(int32_t));
    *size += sizeof(int32_t);
}

void bytecode_append_float(uint8_t** code, uint32_t* size, float value) {
    memcpy(*code + *size, &value, sizeof(float));
    *size += sizeof(float);
}

// 예제 실행을 위한 바이트코드 생성 함수
uint8_t* create_example_bytecode(uint32_t* size) {
    uint8_t* code = (uint8_t*)malloc(1024);  // 충분한 공간 할당
    if (!code) return NULL;
    
    *size = 0;
    
    // R0 = 10
    bytecode_append_byte(&code, size, OP_LOAD_I);      // opcode
    bytecode_append_byte(&code, size, 0);              // 레지스터 0
    bytecode_append_int32(&code, size, 10);            // 값 10
    
    // R1 = 20
    bytecode_append_byte(&code, size, OP_LOAD_I);      // opcode
    bytecode_append_byte(&code, size, 1);              // 레지스터 1
    bytecode_append_int32(&code, size, 20);            // 값 20
    
    // R2 = R0 + R1
    bytecode_append_byte(&code, size, OP_ADD);         // opcode
    bytecode_append_byte(&code, size, 2);              // 결과 레지스터 (R2)
    bytecode_append_byte(&code, size, 0);              // 첫 번째 피연산자 (R0)
    bytecode_append_byte(&code, size, 1);              // 두 번째 피연산자 (R1)
    
    // R2 값 출력
    bytecode_append_byte(&code, size, OP_PRINT);       // opcode
    bytecode_append_byte(&code, size, 2);              // 레지스터 2
    
    // 종료
    bytecode_append_byte(&code, size, OP_HALT);        // opcode
    
    return code;
}

// 메인 함수 - 예제 실행
int main() {
    // VM 초기화
    VM* vm = vm_init();
    if (!vm) {
        printf("VM 초기화 실패\n");
        return 1;
    }
    
    // 예제 바이트코드 생성
    uint32_t code_size;
    uint8_t* example_code = create_example_bytecode(&code_size);
    if (!example_code) {
        printf("바이트코드 생성 실패\n");
        vm_cleanup(vm);
        return 1;
    }
    
    // 코드 로드
    if (!vm_load_code(vm, example_code, code_size)) {
        printf("코드 로드 실패\n");
        free(example_code);
        vm_cleanup(vm);
        return 1;
    }
    
    // 예제 코드 해제 (이미 VM에 복사됨)
    free(example_code);
    
    // VM 실행
    printf("VM 실행 중...\n");
    vm_run(vm);
    printf("VM 실행 완료\n");
    
    // VM 정리
    vm_cleanup(vm);
    
    return 0;
}
```