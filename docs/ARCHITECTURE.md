# Kern Compiler — Architecture Design

> 생성일: 2026-03-01
> 상태: Design Draft

---

## 1. 전체 파이프라인 개요

```
┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
│  Source   │───▶│  Lexer   │───▶│  Parser  │───▶│   Sema   │───▶│  IR Gen  │───▶│ CodeGen  │
│  (.kern) │    │          │    │          │    │          │    │          │    │          │
└──────────┘    └────┬─────┘    └────┬─────┘    └────┬─────┘    └────┬─────┘    └────┬─────┘
                     │               │               │               │               │
                 Token Stream       AST          Typed AST       Kern IR         .asm file
                                                                (SSA형식)          │
                                                                              ┌────▼─────┐
                                                                              │  NASM    │
                                                                              │  + ld    │
                                                                              └────┬─────┘
                                                                                   │
                                                                              Native Binary
```

**6단계 파이프라인:**

| 단계 | 입력 | 출력 | 설명 |
|------|------|------|------|
| 1. Lexer | 소스 텍스트 | Token Stream | 문자열 → 토큰 변환 |
| 2. Parser | Token Stream | AST | 구문 분석, 트리 구조화 |
| 3. Sema | AST | Typed AST | 타입 검사, 순수성 검증 |
| 4. IR Gen | Typed AST | Kern IR (SSA) | 중간 표현 생성 |
| 5. CodeGen | Kern IR | x86 어셈블리 텍스트 | 명령어 선택 + 레지스터 할당 |
| 6. Assemble | .asm | 네이티브 바이너리 | NASM + ld (외부 도구) |

---

## 2. 프로젝트 디렉토리 구조

```
kern/
├── CMakeLists.txt                  # 루트 CMake (C++20, 전역 설정)
├── cmake/                          # CMake 헬퍼 모듈
│
├── include/kern/                   # 공개 헤더 (단계별 분리)
│   ├── lexer/
│   │   ├── Token.h                 # TokenKind 열거형, Token 구조체
│   │   └── Lexer.h                 # Lexer 클래스
│   ├── parser/
│   │   ├── AST.h                   # AST 노드 정의
│   │   └── Parser.h               # Parser 클래스
│   ├── sema/
│   │   ├── TypeChecker.h           # 타입 검사기
│   │   └── PurityChecker.h         # 순수성 검증기
│   ├── ir/
│   │   ├── KernIR.h                # IR 노드 정의 (SSA)
│   │   ├── IRBuilder.h             # AST → IR 변환기
│   │   └── Metadata.h              # 순수성/분배 가능성 메타데이터
│   ├── codegen/
│   │   ├── InstrSelect.h           # 명령어 선택
│   │   ├── RegAlloc.h              # 레지스터 할당
│   │   └── AsmEmitter.h            # 어셈블리 출력
│   └── support/
│       ├── SourceLocation.h        # 파일/라인/컬럼 위치 정보
│       ├── Diagnostic.h            # 에러/경고 메시지 시스템
│       ├── Arena.h                 # 범프 할당자 (AST/IR 노드용)
│       └── StringInterner.h        # 문자열 인터닝 (식별자 중복 제거)
│
├── lib/                            # 구현부 (정적 라이브러리로 빌드)
│   ├── Lexer/
│   │   ├── CMakeLists.txt
│   │   └── Lexer.cpp
│   ├── Parser/
│   │   ├── CMakeLists.txt
│   │   └── Parser.cpp
│   ├── Sema/
│   │   ├── CMakeLists.txt
│   │   ├── TypeChecker.cpp
│   │   └── PurityChecker.cpp
│   ├── IR/
│   │   ├── CMakeLists.txt
│   │   └── IRBuilder.cpp
│   ├── CodeGen/
│   │   ├── CMakeLists.txt
│   │   ├── InstrSelect.cpp
│   │   ├── RegAlloc.cpp
│   │   └── AsmEmitter.cpp
│   └── Support/
│       ├── CMakeLists.txt
│       ├── Arena.cpp
│       ├── Diagnostic.cpp
│       └── StringInterner.cpp
│
├── tools/
│   └── kernc/                      # 컴파일러 드라이버 (실행 바이너리)
│       ├── CMakeLists.txt
│       └── main.cpp
│
├── tests/
│   ├── unit/                       # 단계별 유닛 테스트
│   │   ├── lexer/
│   │   ├── parser/
│   │   └── ir/
│   └── integration/                # .kern 소스 → 기대 출력 테스트
│       └── fib.kern
│
├── docs/
│   └── ARCHITECTURE.md             # 이 문서
│
├── REQUIREMENTS.md
└── README.md
```

**설계 원칙:**
- 각 단계(Lexer, Parser, Sema, IR, CodeGen)는 **독립적인 정적 라이브러리**로 빌드
- `include/kern/`의 공개 헤더로만 단계 간 통신
- `support/`에 공통 인프라 (Arena, Diagnostic, StringInterner)

---

## 3. 단계별 상세 설계

### 3.1 Lexer — 핸드라이튼 토크나이저

**방식: 수동 작성 (generator 사용 안 함)**

rustc, Clang, Zig 등 모든 주요 현대 컴파일러가 사용하는 방식.
에러 복구, 소스 위치 추적, 성능 최적화에 대한 완전한 제어 가능.

#### Token 구조

```cpp
// include/kern/lexer/Token.h

enum class TokenKind : uint8_t {
    // --- Literals ---
    IntLit,             // 42, 0xFF
    FloatLit,           // 3.14
    StringLit,          // "hello"

    // --- Identifiers & Keywords ---
    Ident,              // foo, bar
    KwFn,               // fn
    KwVal,              // val
    KwVar,              // var
    KwMatch,            // match
    KwReturn,           // return (선택적 — 마지막 표현식도 반환값)
    KwIf,               // if
    KwElse,             // else
    KwAnd,              // and (논리 AND 키워드)
    KwOr,               // or  (논리 OR 키워드)
    KwNot,              // not (논리 NOT 키워드)

    // --- Operators ---
    Plus,               // +
    Minus,              // -
    Star,               // *
    Slash,              // /
    Eq,                 // =
    EqEq,               // ==
    NotEq,              // !=
    Lt, Gt, LtEq, GtEq, // < > <= >=
    Arrow,              // ->
    FatArrow,           // =>
    Colon,              // :
    Dot,                // .
    Pipe,               // |> (파이프 연산자)
    Ampersand,          // & (참조 획득)
    Comma,              // ,
    Semicolon,          // ;

    // --- Delimiters ---
    LParen, RParen,     // ( )
    LBrace, RBrace,     // { }
    LBracket, RBracket, // [ ]

    // --- Special ---
    Eof,
    Error,              // 에러 토큰 — 중단하지 않고 에러 수집
};

struct Token {
    TokenKind        kind;
    SourceLocation   loc;
    std::string_view text;  // 소스 버퍼에 대한 zero-copy 뷰
};
```

#### Lexer 핵심 패턴

```cpp
// lib/Lexer/Lexer.cpp

class Lexer {
    const char* start_;       // 현재 토큰 시작
    const char* current_;     // 현재 읽기 위치
    const char* end_;         // 소스 끝
    uint32_t    line_ = 1;
    uint32_t    col_  = 1;

public:
    explicit Lexer(std::string_view source);
    Token nextToken();       // 메인 디스패치 — switch 문으로 분기
};
```

**핵심 설계 결정:**
- **Zero-copy**: 토큰은 원본 소스 버퍼의 `string_view` — 할당 없음
- **Maximal munch**: `<=`는 하나의 토큰 (LtEq), `<` + `=` 아님
- **키워드 인식**: 식별자 스캔 후 해시맵 룩업으로 키워드 판별 (`fn`, `val`, `var`, `match`, `return`, `if`, `else`, `and`, `or`, `not`)
- **주석**: `//` 라인 주석 + `/* */` 블록 주석 지원
- **에러 수집**: `TokenKind::Error` 토큰을 발행하고 계속 진행

---

### 3.2 Parser — Recursive Descent + Pratt Parsing

**방식: 하이브리드**
- 선언/문장: **재귀 하강(Recursive Descent)**
- 표현식: **Pratt Parsing** (연산자 우선순위 자동 처리)

#### AST 노드 설계

Arena 할당 기반 discriminated union. 가상 함수 디스패치 없이 `Kind` 열거형으로 분기.

```cpp
// include/kern/parser/AST.h

// --- 표현식 ---
struct Expr {
    enum class Kind { IntLit, FloatLit, StringLit, Ident, BinOp, UnaryOp, Call, If, Match };
    Kind           kind;
    SourceLocation loc;

    // kind에 따른 데이터 (tagged union)
    union {
        struct { int64_t value; }                           intLit;
        struct { double value; }                            floatLit;
        struct { std::string_view value; }                  stringLit;
        struct { std::string_view name; }                   ident;
        struct { TokenKind op; Expr* lhs; Expr* rhs; }     binOp;
        struct { TokenKind op; Expr* operand; }             unaryOp;  // -, not, * (deref), & (ref)
        struct { Expr* callee; Expr** args; uint32_t argc; } call;
    };
};

// --- 선언 ---
struct Decl {
    enum class Kind { Fn, Val, Var };
    Kind kind;
    SourceLocation loc;
};

struct FnDecl : Decl {
    std::string_view  name;
    Param*            params;      // Arena 할당 배열
    uint32_t          param_count;
    TypeRef           return_type;
    Expr*             body;        // 함수 본문 (블록 표현식)
};

struct Param {
    std::string_view name;
    TypeRef          type;
};

// --- 타입 참조 ---
struct TypeRef {
    enum class Kind { Named, Ptr, Fn };
    Kind kind;
    std::string_view name;        // "i64", "bool", etc.
};
```

**함수 정의 수준 패턴 매칭 매개변수 규칙:**
- `IDENT ":" type` — 일반 매개변수 (타입 필수)
- `literal` — 패턴 매칭 (리터럴만 허용, IDENT 단독 불가 → 컴파일 에러: "parameter missing type")

#### Pratt Parsing 핵심 — 바인딩 파워

```cpp
// 바인딩 파워 테이블
struct BindingPower { uint8_t left, right; };

static BindingPower infixBP(TokenKind op) {
    switch (op) {
        case TokenKind::Plus:
        case TokenKind::Minus:   return {10, 11};  // 좌결합
        case TokenKind::Star:
        case TokenKind::Slash:   return {20, 21};  // 좌결합, 높은 우선순위
        case TokenKind::EqEq:
        case TokenKind::NotEq:   return {5, 6};    // 비교
        case TokenKind::Lt:
        case TokenKind::Gt:
        case TokenKind::LtEq:
        case TokenKind::GtEq:    return {7, 8};    // 비교
        default:                 return {0, 0};    // 중위 연산자 아님
    }
}

// 파서 핵심 루프
Expr* Parser::parseExpr(uint8_t minBP) {
    Expr* lhs = parsePrimary();     // 리터럴, 식별자, 괄호, 접두사

    while (true) {
        auto [lBP, rBP] = infixBP(peek().kind);
        if (lBP < minBP) break;

        Token op = advance();
        Expr* rhs = parseExpr(rBP); // 재귀 — rBP로 결합성 결정
        lhs = arena_.make<BinOpExpr>(op, lhs, rhs);
    }

    return lhs;
}
```

---

### 3.3 Sema — 의미 분석 (Semantic Analysis)

M1에서는 최소한의 타입 검사만 수행. M2에서 순수성 검증 추가.

```cpp
// include/kern/sema/TypeChecker.h

class TypeChecker {
    DiagnosticEngine& diag_;

public:
    // AST를 순회하며 타입 정보를 채움
    // 실패 시 Diagnostic으로 에러 보고
    bool check(FnDecl* fn);

private:
    TypeRef inferExpr(Expr* expr);
    bool    unify(TypeRef a, TypeRef b, SourceLocation loc);
};
```

```cpp
// include/kern/sema/PurityChecker.h  (M2에서 구현)

class PurityChecker {
    DiagnosticEngine& diag_;

public:
    // 함수가 순수 선언인데 부수 효과가 있으면 에러
    bool checkPurity(FnDecl* fn);
};
```

**M1 범위의 Sema:**
- 함수 반환 타입과 본문의 타입 일치 확인
- 함수 호출 시 인자 개수/타입 확인
- 미선언 식별자 검출

---

### 3.4 IR — Kern IR (SSA, 블록 인자 방식)

**SSA + 블록 인자 (Block Arguments)** — LLVM의 phi 노드 대신 Cranelift/MLIR 스타일.

#### 왜 SSA인가?
- 순수성 분석이 단순해짐: SSA에서 순수 함수 = 외부 메모리 쓰기 없음
- 분배 가능성 분석: 입력이 모두 값이면 분배 가능
- 표준 최적화(상수 접기, 죽은 코드 제거) 직접 적용 가능

#### IR 데이터 구조

```cpp
// include/kern/ir/Metadata.h

// 순수성 수준 (가장 순수 → 가장 불순)
enum class Purity : uint8_t {
    Pure      = 0,    // 부수 효과 없음, 동일 입력 → 동일 출력
    ReadOnly  = 1,    // 전역 상태 읽기만 함
    Effectful = 2,    // 부수 효과 있음 (모나드로 감싸짐)
};

// 분배 가능성 (컴퓨팅 체인 대비)
enum class Distributable : uint8_t {
    Yes       = 0,    // 완전 분배 가능 (순수, 캡처 없음)
    NodeLocal = 1,    // 같은 머신 내 코어 간 이동 가능
    Pinned    = 2,    // 특정 노드 고정 (I/O 등)
};

struct FunctionMeta {
    Purity        purity  = Purity::Effectful;    // 보수적 기본값
    Distributable dist    = Distributable::Pinned;
    bool          is_recursive = false;
    bool          is_tailrec   = false;
};
```

```cpp
// include/kern/ir/KernIR.h

using ValueId = uint32_t;  // SSA 값 식별자

enum class IROpcode : uint8_t {
    // 산술
    Add, Sub, Mul, Div,
    // 비교
    ICmpEq, ICmpLt, ICmpLe, ICmpGt, ICmpGe,
    // 제어 흐름
    Branch, CondBranch, Ret,
    // 호출
    Call,
    // 상수
    ConstInt,
};

struct IRInstr {
    IROpcode               op;
    ValueId                result;      // 이 명령이 생산하는 SSA 값
    std::vector<ValueId>   operands;    // 입력 SSA 값들
    SourceLocation         loc;         // 디버그용 원본 위치

    // Call 전용 메타
    struct CallInfo {
        std::string_view callee_name;
        Purity           callee_purity;
    };
    std::optional<CallInfo> call_info;
};

struct IRBlock {
    std::string              label;       // "entry", "then", "else", "merge"
    std::vector<ValueId>     params;      // 블록 인자 (phi 노드 대체)
    std::vector<IRInstr>     instrs;
};

struct IRFunction {
    std::string              name;
    FunctionMeta             meta;
    std::vector<IRBlock>     blocks;
    std::vector<ValueId>     params;      // 함수 매개변수의 SSA 값
    // 타입 정보
    std::vector<TypeRef>     param_types;
    TypeRef                  return_type;
};

struct IRModule {
    std::vector<IRFunction>  functions;
};
```

#### 피보나치의 IR 표현 (예시)

```
@pure @distributable
fn fib(n: i64) -> i64 {
  entry(%n: i64):
    %cond = icmp_le %n, 1
    condbr %cond, base_case(%n), recurse(%n)

  base_case(%val: i64):
    ret %val

  recurse(%n: i64):
    %n1  = sub %n, 1
    %n2  = sub %n, 2
    %r1  = call @fib(%n1)    ; @pure
    %r2  = call @fib(%n2)    ; @pure
    %sum = add %r1, %r2
    ret %sum
}
```

---

### 3.5 CodeGen — x86/amd64 어셈블리 생성

3단계 코드 생성:

```
  Kern IR  ──▶  명령어 선택  ──▶  레지스터 할당  ──▶  어셈블리 출력
              (IR → MachineInstr)  (가상 → 물리 레지스터)  (텍스트 .asm)
```

#### 3.5.1 명령어 선택 (Instruction Selection)

```cpp
// include/kern/codegen/InstrSelect.h

enum class MachineOp : uint8_t {
    MOV, ADD, SUB, IMUL, IDIV,
    CMP, JMP, JE, JNE, JL, JLE, JG, JGE,
    CALL, RET,
    PUSH, POP,
    LEA,
};

enum class x86Reg : uint8_t {
    RAX, RBX, RCX, RDX, RSI, RDI, RSP, RBP,
    R8, R9, R10, R11, R12, R13, R14, R15,
};

struct Operand {
    enum class Kind { VReg, PReg, Imm, Mem, Label };
    Kind kind;
    union {
        uint32_t vreg;
        x86Reg   preg;
        int64_t  imm;
        struct { x86Reg base; int32_t disp; } mem;
    };
    std::string_view label;   // Label kind일 때 사용
};

struct MachineInstr {
    MachineOp              op;
    std::vector<Operand>   operands;  // dst, src1, src2 ...
};
```

#### 3.5.2 레지스터 할당 — Linear Scan

M1에서는 **Linear Scan** 알고리즘 사용. 구현이 단순하고 합리적인 코드 품질.

```
알고리즘:
1. 각 가상 레지스터의 생존 구간(live interval) 계산: [first_def, last_use]
2. 시작점 기준으로 구간 정렬
3. 순서대로 처리:
   a. 끝난 구간의 물리 레지스터 해제
   b. 남은 물리 레지스터 있으면 할당
   c. 없으면 가장 늦게 끝나는 구간을 스택에 spill
```

**x86-64 System V AMD64 ABI 레지스터 규칙:**

| 분류 | 레지스터 | 용도 |
|------|---------|------|
| Caller-saved (volatile) | rax, rcx, rdx, rsi, rdi, r8-r11 | 임시값 우선 할당 |
| Callee-saved | rbx, r12-r15, rbp | 호출 경계를 넘는 값 |
| 함수 인자 | rdi, rsi, rdx, rcx, r8, r9 | 순서대로 |
| 반환값 | rax | 정수 반환 |
| 예약 | rsp, rbp | 스택/프레임 포인터 |

#### 3.5.3 어셈블리 출력 (NASM Intel Syntax)

```cpp
// include/kern/codegen/AsmEmitter.h

class AsmEmitter {
    std::ostream& out_;

public:
    void emitModule(const IRModule& module);

private:
    void emitFunction(const IRFunction& fn);
    void emitPrologue(const std::string& name, uint32_t stack_size);
    void emitEpilogue();
    void emitInstr(const MachineInstr& mi);
    std::string formatOperand(const Operand& op);
};
```

**피보나치 목표 출력 (NASM):**

```nasm
section .text
global fib

fib:
    push rbp
    mov  rbp, rsp
    push rbx                ; callee-saved 보존

    mov  rbx, rdi           ; n 저장

    cmp  rbx, 1
    jle  .base_case

    ; fib(n-1)
    lea  rdi, [rbx - 1]
    call fib
    push rax                ; 결과 저장

    ; fib(n-2)
    lea  rdi, [rbx - 2]
    call fib
    pop  rcx                ; fib(n-1) 복원
    add  rax, rcx           ; fib(n-1) + fib(n-2)
    jmp  .done

.base_case:
    mov  rax, rbx

.done:
    pop  rbx
    pop  rbp
    ret
```

---

### 3.6 Support — 공통 인프라

#### Arena (범프 할당자)

모든 AST/IR 노드는 Arena에서 할당. 컴파일 단위 종료 시 일괄 해제.

```cpp
// include/kern/support/Arena.h

class Arena {
    static constexpr size_t BLOCK_SIZE = 4096;
    std::vector<char*> blocks_;
    char*    ptr_     = nullptr;
    char*    end_     = nullptr;

public:
    ~Arena();  // 모든 블록 해제

    template <typename T, typename... Args>
    T* make(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    void* allocate(size_t size, size_t align);
};
```

#### StringInterner (문자열 인터닝)

식별자, 키워드 등 동일 문자열의 중복 제거. 비교를 포인터 비교로 최적화.

```cpp
// include/kern/support/StringInterner.h

class StringInterner {
    std::unordered_set<std::string> pool_;

public:
    std::string_view intern(std::string_view str);
};
```

#### Diagnostic (진단 메시지)

```cpp
// include/kern/support/Diagnostic.h

enum class DiagLevel { Error, Warning, Note };

struct Diagnostic {
    DiagLevel      level;
    SourceLocation loc;
    std::string    message;
};

class DiagnosticEngine {
    std::vector<Diagnostic> diags_;
    bool has_errors_ = false;

public:
    void report(DiagLevel level, SourceLocation loc, std::string message);
    bool hasErrors() const { return has_errors_; }
    void printAll(std::ostream& out) const;
};
```

---

## 4. 컴파일러 드라이버

```cpp
// tools/kernc/main.cpp — 전체 흐름 오케스트레이션

int main(int argc, char** argv) {
    // 1. 인자 파싱 (입력 파일, 출력 파일, 옵션)
    // 2. 소스 파일 읽기
    // 3. 파이프라인 실행:

    Arena arena;
    DiagnosticEngine diag;
    StringInterner interner;

    // Lexing
    Lexer lexer(source, interner);

    // Parsing
    Parser parser(lexer, arena, diag);
    auto* module = parser.parseModule();
    if (diag.hasErrors()) return 1;

    // Semantic Analysis
    TypeChecker typeChecker(diag);
    typeChecker.check(module);
    if (diag.hasErrors()) return 1;

    // IR Generation
    IRBuilder irBuilder(arena);
    auto irModule = irBuilder.build(module);

    // Code Generation
    std::ofstream asmFile(outputPath);
    AsmEmitter emitter(asmFile);
    emitter.emitModule(irModule);

    // 4. 외부 도구 호출: nasm + ld
    //    nasm -f macho64 output.asm -o output.o
    //    ld output.o -o output

    return 0;
}
```

---

## 5. 데이터 흐름 요약

```
Source Text
    │
    ▼
┌────────────┐
│   Lexer    │  소스 버퍼 → Token (string_view, zero-copy)
└─────┬──────┘
      │ Token Stream
      ▼
┌────────────┐
│   Parser   │  Recursive Descent + Pratt
│            │  Arena 할당 AST 노드
└─────┬──────┘
      │ AST (Expr, FnDecl, ...)
      ▼
┌────────────┐
│    Sema    │  타입 검사 (M1), 순수성 검증 (M2)
│            │  DiagnosticEngine으로 에러 보고
└─────┬──────┘
      │ Typed AST
      ▼
┌────────────┐
│  IRBuilder │  AST → Kern IR (SSA, 블록 인자)
│            │  FunctionMeta: Purity, Distributable
└─────┬──────┘
      │ IRModule (IRFunction, IRBlock, IRInstr)
      ▼
┌────────────┐
│  CodeGen   │  InstrSelect → RegAlloc (Linear Scan) → AsmEmit
│            │  NASM Intel 문법 출력
└─────┬──────┘
      │ .asm 파일
      ▼
┌────────────┐
│ NASM + ld  │  외부 어셈블러/링커
└─────┬──────┘
      │
      ▼
  Native Binary (Mach-O x86-64)
```

---

## 6. M1 구현 범위 (피보나치 재귀)

M1에서 **구현하는 것:**

| 컴포넌트 | 구현 범위 |
|---------|----------|
| Lexer | 정수 리터럴, 식별자, 기본 연산자, 키워드 (fn, val, var, if, else, return, and, or, not) |
| Parser | 함수 선언, 산술/비교 표현식, if-else, 함수 호출, return |
| Sema | 최소 타입 검사 (i64만), 함수 시그니처 확인 |
| IR | SSA 변환, 기본 블록, 분기, 함수 호출, ConstInt |
| CodeGen | x86 명령어 선택, Linear Scan 레지스터 할당, NASM 출력 |
| Support | Arena, DiagnosticEngine, SourceLocation |

M1에서 **구현하지 않는 것:**

| 컴포넌트 | 이유 |
|---------|------|
| PurityChecker | M2 범위 |
| IR 메타데이터 (Purity, Distributable) | 구조만 정의, 실제 분석은 M2 |
| StringInterner | M1에서는 string_view로 충분 |
| 최적화 패스 | M1은 정확성 우선 |

---

## 7. 핵심 설계 결정 요약

| 영역 | 결정 | 근거 |
|------|------|------|
| Lexer | 핸드라이튼, zero-copy | 완전한 제어, 무할당, 에러 복구 |
| Parser | Recursive Descent + Pratt | 선언은 자연스런 재귀, 표현식은 우선순위 자동 처리 |
| AST 할당 | Arena (범프 할당자) | 캐시 친화적, 일괄 해제 |
| IR 형식 | SSA + 블록 인자 | phi 노드보다 단순, 순수성 분석에 최적 |
| IR 메타데이터 | Purity + Distributable enum | 컴퓨팅 체인 대비, 확장 가능 |
| 레지스터 할당 | Linear Scan (M1) | 구현 단순, 합리적 코드 품질 |
| 어셈블리 출력 | NASM Intel 문법 텍스트 | 사람이 읽을 수 있음, 디버깅 용이 |
| 외부 도구 | NASM + ld | 오브젝트 파일 형식 복잡도 회피 |

---

## 8. 다음 단계

1. **`/sc:design`** — Kern 문법 프로토타입 (독자적 문법 설계)
2. **`/sc:workflow`** — M1 구현 워크플로우 (태스크 분해)
3. **`/sc:implement`** — CMake 프로젝트 초기 세팅 + Support 라이브러리 구현
