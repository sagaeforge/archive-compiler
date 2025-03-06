# Monkey 언어: 스택 기반에서 레지스터 기반으로의 전환

## 목차
1. [스택 기반 vs 레지스터 기반 VM](#1-스택-기반-vs-레지스터-기반-vm)
2. [현재 Monkey VM 아키텍처 개요](#2-현재-monkey-vm-아키텍처-개요)
3. [레지스터 기반 설계로의 전환](#3-레지스터-기반-설계로의-전환)
4. [명령어 세트 재설계](#4-명령어-세트-재설계)
5. [컴파일러 수정 방안](#5-컴파일러-수정-방안)
6. [가상 머신 구현 변경](#6-가상-머신-구현-변경)
7. [최적화 기회](#7-최적화-기회)
8. [구현 예시](#8-구현-예시)
9. [성능 비교 고려사항](#9-성능-비교-고려사항)

---

## 1. 스택 기반 vs 레지스터 기반 VM

### 스택 기반 VM의 특징
- 피연산자를 명시적으로 지정하지 않고, 암묵적으로 스택의 최상위 값을 사용
- 간단한 명령어 형식과 컴파일러 구현
- 더 적은 바이트코드 크기 (피연산자 위치를 명시할 필요 없음)
- 하드웨어 스택을 모방하므로 이해하기 쉬움
- 예: JVM, CLR, Python VM

```
// 스택 기반 예시 (a + b)
LOAD_A    // a를 스택에 푸시
LOAD_B    // b를 스택에 푸시
ADD       // 스택의 상위 두 값을 더하고 결과를 스택에 푸시
```

### 레지스터 기반 VM의 특징
- 명령어에 피연산자 위치(레지스터)를 명시적으로 지정
- 명령어 길이가 더 길지만, 전체 명령어 수는 감소
- 임시 값을 위한 스택 조작 명령어 불필요
- 일반적으로 더 나은 성능 (덜 빈번한 메모리 접근)
- 예: Lua VM, Dalvik VM, WebAssembly

```
// 레지스터 기반 예시 (a + b)
ADD R0, R1, R2  // R1(a)과 R2(b)를 더해서 R0에 저장
```

---

## 2. 현재 Monkey VM 아키텍처 개요

Monkey 언어는 현재 스택 기반 VM으로 구현되어 있으며, 다음과 같은 주요 특징을 갖고 있습니다:

- 단일 스택으로 모든 값을 관리 (스택 포인터 `sp`로 제어)
- 명령어는 Opcode(1바이트)와 가변 길이 피연산자로 구성
- 전역 변수는 별도 배열로 관리
- 컴파일된 함수와 클로저를 사용한 함수 호출 메커니즘
- 각 함수 호출은 별도의 호출 프레임(Frame)으로 관리

현재 VM은 다음과 같은 핵심 구조를 가지고 있습니다:

```go
type VM struct {
    constants []object.Object    // 상수 풀
    stack []object.Object        // 값 스택
    sp int                       // 스택 포인터
    globals []object.Object      // 전역 변수 저장소
    frames []*Frame              // 호출 프레임 스택
    framesIndex int              // 현재 프레임 인덱스
}
```

---

## 3. 레지스터 기반 설계로의 전환

레지스터 기반 VM으로 전환하기 위한 핵심 설계 변경 사항은 다음과 같습니다:

### 3.1 레지스터 개념 도입

```cpp
class VM {
private:
    std::vector<Object*> registers;   // 레지스터 배열
    std::vector<Object*> constants;   // 상수 풀
    std::vector<Object*> globals;     // 전역 변수
    std::vector<Frame*> frames;       // 호출 프레임
    int currentFrame;                 // 현재 프레임 인덱스
};
```

### 3.2 프레임 재설계

각 함수 호출 프레임이 자체 레지스터 집합을 가지도록 변경합니다:

```cpp
class Frame {
private:
    Closure* closure;               // 실행 중인 클로저
    int ip;                         // 명령어 포인터
    int registerOffset;             // 레지스터 시작 오프셋
    int registerCount;              // 레지스터 개수
};
```

### 3.3 레지스터 할당 전략

- **함수 매개변수**: 처음 N개 레지스터에 자동 할당
- **지역 변수**: 컴파일 시 레지스터 지정
- **임시 값**: 컴파일러가 사용 가능한 레지스터 할당
- **반환 값**: 관례적으로 R0에 저장

---

## 4. 명령어 세트 재설계

기존 스택 기반 명령어와 새로운 레지스터 기반 명령어 비교:

### 4.1 기본 명령어 형식

```
| Opcode(1바이트) | A(1바이트) | B(1바이트) | C(1바이트) |
```

- **A**: 일반적으로 대상 레지스터
- **B, C**: 소스 레지스터 또는 상수 인덱스

### 4.2 주요 명령어 변환 예시

| 스택 기반 | 레지스터 기반 | 설명 |
|----------|--------------|------|
| `OpConstant <idx>` | `LOADK R(A), K(Bx)` | 상수를 레지스터에 로드 |
| `OpAdd` | `ADD R(A), R(B), R(C)` | 두 레지스터 값을 더해 세 번째에 저장 |
| `OpPop` | (제거됨) | 레지스터 기반에서는 필요 없음 |
| `OpSetGlobal <idx>` | `SETGLOBAL K(Bx), R(A)` | 전역 변수에 레지스터 값 저장 |
| `OpGetGlobal <idx>` | `GETGLOBAL R(A), K(Bx)` | 전역 변수 값을 레지스터에 로드 |
| `OpCall <nArgs>` | `CALL R(A), B, C` | B-1개 인수로 함수 호출, C-1개 값 반환 |

### 4.3 새로운 명령어 예제

```cpp
enum OpCode {
    OP_LOADK,      // 상수를 레지스터에 로드: R(A) = K(Bx)
    OP_MOVE,       // 레지스터 간 값 이동: R(A) = R(B)
    OP_ADD,        // 더하기: R(A) = R(B) + R(C)
    OP_SUB,        // 빼기: R(A) = R(B) - R(C)
    OP_MUL,        // 곱하기: R(A) = R(B) * R(C)
    OP_DIV,        // 나누기: R(A) = R(B) / R(C)
    OP_GETGLOBAL,  // 전역 변수 가져오기: R(A) = Global[K(Bx)]
    OP_SETGLOBAL,  // 전역 변수 설정: Global[K(Bx)] = R(A)
    OP_GETLOCAL,   // 지역 변수 가져오기: R(A) = R(B)
    OP_SETLOCAL,   // 지역 변수 설정: R(A) = R(B)
    OP_JMP,        // 점프: pc += sBx
    OP_EQ,         // 같음 비교: if ((R(B) == R(C)) ~= A) then pc++
    OP_LT,         // 작음 비교: if ((R(B) < R(C)) ~= A) then pc++
    OP_CALL,       // 함수 호출: R(A), ... ,R(A+B-2) = R(A)(R(A+1), ... ,R(A+B-1))
    OP_RETURN,     // 반환: return R(A), ... ,R(A+B-2)
    OP_CLOSURE,    // 클로저 생성: R(A) = closure(K(Bx))
    // 기타 명령어...
};
```

---

## 5. 컴파일러 수정 방안

컴파일러는 AST를 레지스터 기반 바이트코드로 변환하기 위해 다음과 같은 변경이 필요합니다:

### 5.1 레지스터 할당기 도입

```cpp
class RegisterAllocator {
public:
    int allocate();            // 새 레지스터 할당
    void free(int reg);        // 레지스터 해제
    int maxRegisters() const;  // 최대 사용된 레지스터 수 확인
    
private:
    std::set<int> freeRegisters;
    int nextRegister = 0;
};
```

### 5.2 표현식 컴파일 변경

표현식 평가 결과가 특정 레지스터에 저장되도록 컴파일 로직 수정:

```cpp
// 컴파일러에 결과 레지스터 추적 기능 추가
struct CompileResult {
    int resultReg;  // 표현식 결과가 저장된 레지스터
    bool isTemp;    // 임시 레지스터인지 여부
};

CompileResult Compiler::compileExpression(const Expression& expr, int targetReg = -1) {
    switch (expr.type) {
        case INTEGER_LITERAL: {
            int constIdx = addConstant(createInteger(expr.intValue));
            int reg = (targetReg >= 0) ? targetReg : allocator.allocate();
            emit(OP_LOADK, reg, constIdx);
            return {reg, targetReg < 0};
        }
        
        case INFIX_EXPRESSION: {
            if (expr.operator == "+") {
                CompileResult left = compileExpression(expr.left);
                CompileResult right = compileExpression(expr.right);
                
                int resultReg = (targetReg >= 0) ? targetReg : left.resultReg;
                emit(OP_ADD, resultReg, left.resultReg, right.resultReg);
                
                // 임시 레지스터 해제
                if (right.isTemp) allocator.free(right.resultReg);
                if (left.isTemp && left.resultReg != resultReg) allocator.free(left.resultReg);
                
                return {resultReg, targetReg < 0 && !left.isTemp};
            }
            // 기타 연산자 처리...
        }
        // 기타 표현식 처리...
    }
}
```

### 5.3 변수 및 함수 처리

```cpp
// 변수 선언 컴파일
void Compiler::compileLetStatement(const LetStatement& stmt) {
    CompileResult value = compileExpression(stmt.value);
    
    Symbol symbol = symbolTable.define(stmt.name);
    if (symbol.scope == GlobalScope) {
        int nameIdx = addConstant(createString(stmt.name));
        emit(OP_SETGLOBAL, value.resultReg, nameIdx);
    } else {
        // 지역 변수는 특정 레지스터에 직접 할당
        int localReg = symbol.index;
        if (value.resultReg != localReg) {
            emit(OP_MOVE, localReg, value.resultReg);
            if (value.isTemp) allocator.free(value.resultReg);
        }
    }
}

// 함수 컴파일
void Compiler::compileFunctionLiteral(const FunctionLiteral& func) {
    // 새 함수 환경 설정
    enterScope();
    RegisterAllocator funcAllocator;
    
    // 매개변수를 처음 n개 레지스터에 할당
    for (const auto& param : func.parameters) {
        symbolTable.defineLocal(param, funcAllocator.allocate());
    }
    
    // 함수 본문 컴파일
    compileBlockStatement(func.body);
    
    // 명시적 반환이 없는 경우 nil 반환 추가
    if (!lastInstructionIs(OP_RETURN)) {
        int nilReg = funcAllocator.allocate();
        emit(OP_LOADNIL, nilReg);
        emit(OP_RETURN, nilReg, 1);
    }
    
    // 컴파일된 함수 객체 생성
    int maxRegs = funcAllocator.maxRegisters();
    auto instructions = leaveScope();
    
    CompiledFunction fn(instructions, maxRegs, func.parameters.size());
    int funcIndex = addConstant(fn);
    
    // 현재 스코프에서 클로저 생성
    int destReg = allocator.allocate();
    emit(OP_CLOSURE, destReg, funcIndex);
    
    return {destReg, true};
}
```

---

## 6. 가상 머신 구현 변경

레지스터 기반 VM의 핵심 실행 로직은 다음과 같이 변경됩니다:

### 6.1 VM 구조체 변경

```cpp
class VM {
public:
    VM(int registerSize = 256);
    
    void execute(const ByteCode& bytecode);
    Object* getResult() const;
    
private:
    std::vector<Object*> registers;
    std::vector<Object*> constants;
    std::vector<Object*> globals;
    
    std::vector<Frame> frames;
    int framesIndex = 0;
    
    // 레지스터 접근 헬퍼
    Object* getRegister(int reg) const;
    void setRegister(int reg, Object* value);
    
    // 현재 실행 중인 프레임 접근
    Frame& currentFrame();
    
    // 명령어 실행 함수들
    void executeLoadK(int a, int bx);
    void executeAdd(int a, int b, int c);
    void executeCall(int a, int b, int c);
    // 기타 실행 함수...
};
```

### 6.2 실행 루프 구현

```cpp
void VM::execute(const ByteCode& bytecode) {
    constants = bytecode.constants;
    
    // 메인 프레임 설정
    frames[0] = Frame(bytecode.mainFunction, 0);
    framesIndex = 0;
    
    // 메인 실행 루프
    while (framesIndex >= 0) {
        Frame& frame = currentFrame();
        const auto& instructions = frame.instructions();
        
        frame.ip++;
        if (frame.ip >= instructions.size()) {
            break;
        }
        
        uint32_t instruction = instructions[frame.ip];
        uint8_t opcode = GET_OPCODE(instruction);
        uint8_t a = GET_ARG_A(instruction);
        uint8_t b = GET_ARG_B(instruction);
        uint8_t c = GET_ARG_C(instruction);
        
        switch (opcode) {
            case OP_LOADK: {
                uint16_t bx = GET_ARG_Bx(instruction);
                executeLoadK(a, bx);
                break;
            }
            case OP_ADD: {
                executeAdd(a, b, c);
                break;
            }
            case OP_CALL: {
                executeCall(a, b, c);
                break;
            }
            // 기타 명령어 처리...
        }
    }
}
```

### 6.3 명령어 실행 구현 예시

```cpp
void VM::executeLoadK(int a, int bx) {
    setRegister(a, constants[bx]);
}

void VM::executeAdd(int a, int b, int c) {
    Object* left = getRegister(b);
    Object* right = getRegister(c);
    
    if (left->type() == INTEGER && right->type() == INTEGER) {
        int result = static_cast<IntegerObject*>(left)->value + 
                    static_cast<IntegerObject*>(right)->value;
        setRegister(a, new IntegerObject(result));
    } else if (left->type() == STRING && right->type() == STRING) {
        std::string result = static_cast<StringObject*>(left)->value + 
                            static_cast<StringObject*>(right)->value;
        setRegister(a, new StringObject(result));
    } else {
        throw RuntimeError("타입 오류: 덧셈 연산은 정수 또는 문자열만 지원합니다");
    }
}

void VM::executeCall(int a, int b, int c) {
    Object* callee = getRegister(a);
    
    if (callee->type() != CLOSURE) {
        throw RuntimeError("호출 가능한 객체가 아닙니다");
    }
    
    Closure* closure = static_cast<ClosureObject*>(callee)->closure;
    
    // 새 프레임 생성
    int registerBase = registers.size();
    int numArgs = b - 1;
    
    // 레지스터 공간 확장
    registers.resize(registerBase + closure->maxRegisters);
    
    // 인수 복사
    for (int i = 0; i < numArgs; i++) {
        registers[registerBase + i] = getRegister(a + i + 1);
    }
    
    // 프레임 스택에 추가
    frames[++framesIndex] = Frame(closure, registerBase);
}
```

---

## 7. 최적화 기회

레지스터 기반 VM으로 전환하면 다음과 같은 최적화 기회가 생깁니다:

### 7.1 명령어 병합

스택 VM에서 여러 명령어로 표현되는 작업을 단일 명령어로 병합할 수 있습니다:

```
// 스택 기반 (3개 명령어)
LOAD_CONST 0    // 상수 0 로드
LOAD_CONST 1    // 상수 1 로드
ADD             // 두 값 더하기

// 레지스터 기반 (1개 명령어)
ADD R0, K0, K1  // 상수 0과 1을 직접 더해서 R0에 저장
```

### 7.2 레지스터 재사용

동일한 레지스터를 여러 연산에서 재사용하여 레지스터 개수를 최소화할 수 있습니다:

```cpp
// 예: a + b + c
// 비효율적: 3개 레지스터 사용
LOADK R0, K(a)
LOADK R1, K(b)
ADD R2, R0, R1  // R2 = a + b
LOADK R3, K(c)
ADD R4, R2, R3  // R4 = (a + b) + c

// 최적화: 2개 레지스터만 사용
LOADK R0, K(a)
LOADK R1, K(b)
ADD R0, R0, R1  // R0 = a + b (R0 재사용)
LOADK R1, K(c)
ADD R0, R0, R1  // R0 = (a + b) + c (R0 다시 재사용)
```

### 7.3 상수 폴딩

컴파일 시간에 상수 표현식을 평가하여 명령어 수를 줄일 수 있습니다:

```
// 최적화 전 (상수 1 + 2)
LOADK R0, K(1)
LOADK R1, K(2)
ADD R0, R0, R1

// 최적화 후 (컴파일러가 미리 계산)
LOADK R0, K(3)  // 상수 3 직접 로드
```

### 7.4 불필요한 이동 제거

```
// 최적화 전
LOADK R1, K(a)
MOVE R0, R1     // 불필요한 이동

// 최적화 후
LOADK R0, K(a)  // 직접 대상 레지스터에 로드
```

---

## 8. 구현 예시

### 8.1 피보나치 함수 예제

```
// 피보나치 함수 (스택 기반)
function fibonacci(n) {
    if (n <= 1) { return n; }
    return fibonacci(n - 1) + fibonacci(n - 2);
}
```

### 8.2 레지스터 기반 바이트코드 변환

```
// fibonacci 함수의 레지스터 기반 바이트코드 (의사 코드)
// R0: n (매개변수)
// R1-R3: 임시 레지스터

0  LOADK    R1, K(1)    // R1 = 1
1  LE       R2, R0, R1  // R2 = (R0 <= R1)
2  JMP      R2, +2      // R2가 true면 +2 점프 (5번으로)
3  LOADK    R3, K(0)    // R3 = 0 (결과 초기화) 
4  JMP      +9          // 13번으로 점프 (else 분기)

// then 분기: return n
5  MOVE     R3, R0      // R3 = R0 (n을 결과로 사용)
6  RETURN   R3, 1       // R3 반환

// else 분기: return fibonacci(n-1) + fibonacci(n-2)
7  LOADK    R1, K(1)    // R1 = 1
8  SUB      R1, R0, R1  // R1 = R0 - R1 (n-1)
9  GETGLOBAL R2, K("fibonacci")
10 CALL     R2, 1, 2    // R2 = fibonacci(R1)
11 MOVE     R1, R2      // R1 = R2 (임시 저장)

12 LOADK    R2, K(2)    // R2 = 2
13 SUB      R2, R0, R2  // R2 = R0 - R2 (n-2)
14 GETGLOBAL R3, K("fibonacci")
15 CALL     R3, 1, 2    // R3 = fibonacci(R2)

16 ADD      R3, R1, R3  // R3 = R1 + R3 (fibonacci(n-1) + fibonacci(n-2))
17 RETURN   R3, 1       // R3 반환
```

---

## 9. 성능 비교 고려사항

### 9.1 레지스터 기반 VM의 장점

1. **명령어 수 감소**: 동일한 작업에 대해 더 적은 수의 명령어 필요
2. **명령어 디스패치 오버헤드 감소**: 명령어 실행 루프 반복 횟수 감소
3. **로컬 변수 접근 최적화**: 스택 조작 없이 직접 접근 가능
4. **명령어 세트 확장성**: 다양한 특수 명령어 추가 용이

### 9.2 레지스터 기반 VM의 단점

1. **명령어 크기 증가**: 피연산자 레지스터 지정 필요로 명령어 크기 증가
2. **구현 복잡성**: 레지스터 할당 및 관리 로직 필요
3. **컴파일러 복잡성**: 레지스터 할당 알고리즘 구현 필요

### 9.3 벤치마크 고려사항

1. **작은 함수 vs 큰 함수**: 함수 크기에 따른 성능 차이 비교
2. **반복문 집약적 코드**: 루프 실행에서의 성능 이점 측정
3. **재귀 함수**: 함수 호출 오버헤드 비교
4. **객체 조작**: 객체 속성 접근 및 수정 성능 비교
5. **메모리 사용량**: 바이트코드 크기 및 실행 중 메모리 사용량 비교

레지스터 기반으로 전환 시 일반적으로 20-30%의 성능 향상을 기대할 수 있으나, 구현 세부 사항과 최적화 수준에 따라 달라질 수 있습니다. 