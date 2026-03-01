# Kern Compiler v2 — 아키텍처 혁신 설계

> 생성: 2026-03-01
> 최종 수정: 2026-03-01
> 상태: **Phase 0~4a 완료 (784 unit + 116 E2E)** — Phase 4b/4c/5 착수 가능
> 전제: M5 완료 ✅. 기존 `structural-refactoring-requirements.md`의 결정사항을 포함하되, 아키텍처를 근본적으로 재설계

---

## 1. 왜 "개선"이 아니라 "혁신"인가

### 현재 아키텍처의 근본적 한계

현재 파이프라인 (M5 완료 시점 — 517 unit + 113 E2E, 37 opcodes):
```
Source → Lexer → Parser → [TypeChecker 주석] → IRBuilder → CodeGen → NASM 텍스트
                  AST ─────────────────────────→ IR ──────→ 문자열
```
지원 타입: i8-i64, u8-u64, f32/f64, bool, Unit, struct, enum, union, Ptr<T>, Ptr<var T>, String

이것은 **2-레벨 아키텍처**다. AST에서 IR로의 점프가 너무 크고,
CodeGen은 IR에서 곧바로 텍스트를 뱉는다.

| 문제 | 근본 원인 |
|------|----------|
| 클로저 변환 넣을 곳 없음 | AST→IR 사이에 변환 레이어 부재 |
| 단형화(monomorphization) 불가 | 제네릭 AST를 복제할 레이어 부재 |
| 함수를 값으로 전달 불가 | IR callee가 문자열 이름, 간접호출 없음 |
| 패턴 매칭 최적화 불가 | 파서에서 이미 if/else로 desugar, decision tree 불가 |
| 루프 추가 시 SSA 깨짐 | back-edge phi를 처리할 메커니즘 없음 |
| 레지스터 직접 이름으로 할당 | Virtual register 개념 없음, 타겟 추상화 0 |
| 새 패스 추가하려면 main.cpp 수정 | 패스 매니저 없음, 하드코딩된 순서 |

**기존 리팩토링(StringPool, TypeId, IRInstr 정리)은 같은 2-레벨 위에서 방 청소하는 것.**
필요한 건 **방을 새로 짓는 것**이다.

---

## 2. 새 아키텍처: 4-레벨 파이프라인

```
Source
  │ Lexer
  ▼
Tokens
  │ Parser
  ▼
┌─────────────────────────────────────────────────┐
│  AST  (untyped, raw syntax)                     │
│  - 파서가 만든 그대로. 디슈가링 없음.           │
│  - pipe |> 도 AST에 PipeExpr로 보존             │
│  - fn 패턴 오버로드도 개별 FnDecl로 보존         │
└─────────────────────────────────────────────────┘
  │ TypeChecker + Desugarer → HIR Builder
  ▼
┌─────────────────────────────────────────────────┐
│  HIR  (High-level IR — typed, desugared)        │
│  - 모든 노드에 TypeId 내장                       │
│  - pipe → call, fn patterns → match 디슈가링     │
│  - match → decision tree                         │
│  - 함수 타입, 클로저 타입 표현 가능              │
│  - 아직 SSA 아님. 변수 이름 기반.                │
│                                                  │
│  ┌─── PassManager (HIR 패스들) ──────────────┐  │
│  │  ClosureConversionPass   (M6)             │  │
│  │  MonomorphizationPass    (M6)             │  │
│  │  PurityVerificationPass                    │  │
│  │  ExhaustivenessCheckPass                   │  │
│  │  TailCallAnalysisPass                      │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
  │ HIR → LIR Lowering
  ▼
┌─────────────────────────────────────────────────┐
│  LIR  (Low-level IR — SSA, virtual registers)   │
│  - SSA form with block parameters (phi 대체)     │
│  - Virtual registers (%v0, %v1, ...)            │
│  - 타겟 중립 opcode (Add, Load, Store, Call)     │
│  - 간접호출: CallIndirect opcode                 │
│  - 메모리: AddrOf, Load, Store, FieldPtr         │
│                                                  │
│  ┌─── PassManager (LIR 패스들) ──────────────┐  │
│  │  TCOPass (tail call → jump 변환)           │  │
│  │  DeadCodeEliminationPass                   │  │
│  │  ConstantFoldingPass                       │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
  │ Instruction Selection (LIR → MachIR)
  ▼
┌─────────────────────────────────────────────────┐
│  MachIR  (Machine IR — x86-64 specific)         │
│  - x86 opcode enum (Mov, Add, Cmp, Jmp, ...)   │
│  - virtual registers → physical via RegAlloc     │
│  - ABI 호출 규약 구체화                          │
│  - callee-saved, stack frame 계산                │
│                                                  │
│  ┌─── RegisterAllocator ─────────────────────┐  │
│  │  Liveness analysis                         │  │
│  │  Linear scan allocation                    │  │
│  │  Spill code insertion                      │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
  │ Emitter
  ▼
NASM assembly text → nasm → ld → binary
```

### 핵심: 각 레벨이 해결하는 문제

| 레벨 | 핵심 역할 | 변환 예시 |
|------|----------|----------|
| **AST** | 문법 그대로 보존 | `a \|> f` = PipeExpr (desugar 안 함) |
| **HIR** | 타입이 붙은 의미론적 표현. 고수준 변환의 무대 | pipe→call, closure→lifted fn + env struct, generic→concrete instantiation |
| **LIR** | SSA. 타겟 중립. 저수준 최적화의 무대 | tail call→jump, dead code 제거, constant folding |
| **MachIR** | x86-64 전용. 물리 레지스터 할당 | vreg→physical, spill, ABI 준수 |

---

## 3. 컴포넌트 상세 설계

### 3.1 인프라 레이어 (기존 WS-1, WS-2 계승)

#### StringPool
```cpp
// include/kern/support/StringPool.h
class StringPool {
    Arena& arena_;
    std::unordered_set<std::string_view> table_;
public:
    explicit StringPool(Arena& arena);
    std::string_view intern(std::string_view s);
    std::string_view intern(std::string_view a, std::string_view b);  // concat + intern
    std::string_view intern(std::string_view a, std::string_view b, std::string_view c);
};
```

#### TypeSystem (TypeId + TypeTable)
```cpp
// include/kern/support/TypeSystem.h  — Support 레이어! Sema가 아님
//   모든 레이어(HIR, LIR, MachIR)가 TypeId를 사용하므로 최하위에 배치

using TypeId = uint32_t;

enum class TypeKind : uint8_t {
    Primitive,   // i8..i64, u8..u64, f32, f64, bool, unit
    Struct,      // user-defined struct
    Enum,        // named integer enum
    Union,       // tagged union (ADT)
    Ptr,         // Ptr<T>
    PtrMut,      // Ptr<var T>
    Fn,          // (T1, T2) -> R
    Array,       // [T; N]  (미래)
    TypeVar,     // <T> generic param (M6)
};

struct TypeInfo {
    TypeKind kind;
    union {
        struct { PrimitiveKind prim; } primitive;
        struct { StringView name; FieldInfo* fields; uint32_t field_count;
                 uint32_t size; uint32_t align; } struct_;
        struct { StringView name; VariantInfo* variants; uint32_t variant_count; } union_;
        struct { TypeId pointee; bool is_mutable; } ptr;
        struct { TypeId* params; uint32_t param_count; TypeId return_type; } fn;
        // ...
    };
};

class TypeTable {
    Arena& arena_;
    std::vector<TypeInfo*> types_;  // TypeId → TypeInfo*
    // canonical type 중복 제거용 맵
public:
    TypeTable(Arena& arena);

    // 원시형: 생성 시 0~12 예약
    static constexpr TypeId I8 = 0, I16 = 1, /* ... */ Unit = 11, Error = 12;

    TypeId intern(TypeInfo info);       // canonical type 반환
    const TypeInfo& get(TypeId id) const;

    // 편의 메서드
    TypeId makePtr(TypeId pointee, bool is_mutable);
    TypeId makeFn(std::span<TypeId> params, TypeId ret);
    TypeId makeStruct(std::string_view name, std::span<FieldInfo> fields);

    uint32_t sizeOf(TypeId id) const;
    uint32_t alignOf(TypeId id) const;
    bool isFloat(TypeId id) const;
    bool isSigned(TypeId id) const;
    uint32_t bitWidth(TypeId id) const;
};
```

**핵심 결정: TypeSystem은 Support 레이어에 배치.** Sema가 아니라 전 파이프라인 공유 인프라.

#### CompilationContext (전역 공유 상태)
```cpp
// include/kern/support/CompilationContext.h
struct CompilationContext {
    Arena arena;
    StringPool strings;
    TypeTable types;
    DiagnosticEngine diag;

    CompilationContext();  // arena 생성, strings/types에 arena 전달, 원시형 등록
};
```

모든 스테이지가 `CompilationContext&`를 받음. main.cpp의 post-hoc 머징 같은 어색함 제거.

---

### 3.2 AST (파서 출력 — 문법 보존)

**기존과의 차이: 디슈가링을 하지 않는다.**

```cpp
// 현재: Parser가 |> 를 CallExpr로 desugar
// v2: PipeExpr { Expr* lhs; Expr* rhs; } 를 AST에 보존

// 현재: Parser가 fn 패턴 오버로드를 단일 FnDecl + MatchExpr로 desugar
// v2: 개별 PatternFnDecl을 그대로 보존. 디슈가링은 HIR Builder가 담당
```

왜? **AST에서 디슈가링하면 에러 메시지가 원본 소스와 괴리된다.** HIR에서 디슈가링하면 원본 AST를 참조하여 정확한 에러 위치를 보고할 수 있다.

Module 확장 (기존 WS-9):
```cpp
struct Module {
    std::string_view name;         // M6 모듈 시스템용
    FnDecl** functions;
    uint32_t fn_count;
    StructDecl** structs;
    uint32_t struct_count;
    EnumDecl** enums;              // enum + union
    uint32_t enum_count;
    SourceLocation loc;
};
```

---

### 3.3 HIR (High-level IR)

```
include/kern/hir/
├── HIR.h          — HIR 노드 정의
├── HIRBuilder.h   — AST → HIR 변환 (TypeCheck + Desugar 통합)
├── HIRDump.h      — HIR 텍스트 출력
└── HIRPass.h      — Pass 인터페이스 + PassManager
```

#### HIR 노드 설계

HIR은 AST와 다르게 **모든 노드에 TypeId가 내장**되어 있고,
디슈가링이 완료된 상태다.

```cpp
// include/kern/hir/HIR.h

// HIR은 AST처럼 Arena 할당, 포인터 기반 트리
struct HIRExpr {
    enum Kind : uint8_t {
        IntLit, FloatLit, BoolLit, StringLit,
        Ident,               // 변수 참조
        FnRef,               // 함수를 값으로 참조 (M6)
        BinOp, UnaryOp,
        Call,                // 직접 호출
        CallIndirect,        // 간접 호출 (함수 포인터, M6)
        If,
        Match,               // decision tree로 변환된 match
        Block,
        Return,
        StructLit,           // M5
        FieldAccess,         // M5
        AddrOf,              // M5c
        Deref,               // M5c
        Lambda,              // M6
    };

    Kind kind;
    TypeId type;             // ← 핵심: 모든 HIR 노드에 타입
    SourceLocation loc;      // 원본 AST 노드의 위치 (에러 보고용)
};

struct HIRCallExpr : HIRExpr {
    std::string_view callee;     // interned
    HIRExpr** args;
    uint32_t arg_count;
};

struct HIRCallIndirectExpr : HIRExpr {
    HIRExpr* callee_expr;        // 함수 값 (변수, 람다 등)
    HIRExpr** args;
    uint32_t arg_count;
};

// Match 변환: 2단계 접근
//
// Phase 2 초기: nested if/else (현재 buildMatchChain과 동일한 전략)
//   → M5까지의 패턴(IntLit, BoolLit, Enum, Union, Wildcard)에 충분
//   → HIR 노드: HIRMatchExpr { HIRMatchArm* arms; } (순차 비교)
//
// M6 이후: decision tree로 전환 (구조 분해, 중첩 패턴, guard 도입 시)
//   → 알고리즘: Maranget 2008 ("Compiling Pattern Matching to Good Decision Trees")
//   → 중복 패턴 검출, 최적 분기 순서 계산
//
// 현재 시점에서는 nested if/else로 구현하되, HIR 노드 구조를
// 향후 decision tree로 교체할 수 있도록 인터페이스를 추상화.

struct HIRMatchArm {
    HIRPattern* pattern;     // 패턴 (IntLit, BoolLit, Enum, Union, Variable, Wildcard)
    HIRExpr* guard;          // guard 조건 (M6, nullptr if none)
    HIRExpr* body;
    SourceLocation loc;
};

struct HIRMatchExpr : HIRExpr {
    HIRExpr* scrutinee;      // match 대상
    HIRMatchArm* arms;
    uint32_t arm_count;
    // Phase 2: arms를 순차 비교하는 nested if/else로 LIR에 lowering
    // M6+: decision tree로 변환 후 lowering
};

// M6에서 추가될 decision tree 노드 (현재는 미사용, 예약)
struct HIRDecisionTree {
    enum Kind { Leaf, Switch, Guard };
    // Leaf: 바디 실행
    // Switch: 값 테스트 → 분기 (bool 2-way, int N-way, constructor tag)
    // Guard: 조건 → true/false 분기
};

struct HIRFnDecl {
    std::string_view name;       // interned
    HIRParam* params;
    uint32_t param_count;
    TypeId return_type;
    HIRExpr* body;               // 또는 nullptr (intrinsic)

    // 메타데이터 (패스가 채움)
    Purity purity = Purity::Unknown;
    bool is_recursive = false;
    bool is_tail_recursive = false;
    bool is_intrinsic = false;

    // 모듈 시스템 예약 (M6+)
    enum class Visibility : uint8_t { Private, Public };
    Visibility visibility = Visibility::Public;  // 현재는 모두 public
};

struct HIRImportDecl {
    std::string_view module_path;   // "std/io", "mylib/math" (interned)
    std::string_view alias;         // import alias (interned, empty = none)
    std::string_view* symbols;      // 선택적 심볼 목록 (nullptr = 전체)
    uint32_t symbol_count;
    SourceLocation loc;
};

struct HIRModule {
    std::string_view name;           // 모듈 이름 (현재 빈 문자열, M6에서 활성화)
    HIRFnDecl** functions;
    uint32_t fn_count;
    HIRStructDecl** structs;
    uint32_t struct_count;
    HIREnumDecl** enums;             // enum 선언
    uint32_t enum_count;
    HIRUnionDecl** unions;           // union 선언
    uint32_t union_count;

    // 모듈 시스템 예약 (M6+, 현재는 count=0)
    HIRImportDecl* imports;
    uint32_t import_count;
};
```

#### HIRBuilder (AST → HIR)

```cpp
// include/kern/hir/HIRBuilder.h
class HIRBuilder {
    CompilationContext& ctx_;
    // fn_table_ 역할을 ctx_.types + 내부 심볼 테이블이 대체
public:
    explicit HIRBuilder(CompilationContext& ctx);

    // AST → HIR 변환 (TypeCheck + Desugar를 동시에 수행)
    // 현재의 TypeChecker + IRBuilder 역할을 통합
    HIRModule* build(const Module* ast);

private:
    HIRExpr* buildExpr(const Expr* ast, std::optional<TypeId> ctx);
    // pipe → call 디슈가링
    // fn pattern → match 디슈가링
    // 타입 추론 + 검사
};
```

**핵심 결정: TypeChecker와 HIRBuilder를 통합.** 현재는 TypeChecker가 주석만 달고 IRBuilder가 별도로 IR을 만드는데, HIR에서는 타입 검사와 HIR 생성이 동시에 일어난다. Swift의 SILGen + type checking 통합 모델.

#### HIRBuilder 에러 보고 전략

통합으로 인한 에러 품질 저하를 방지하기 위한 설계 원칙:

1. **원본 AST SourceLocation 보존**: 모든 HIR 노드는 원본 AST 노드의 `SourceLocation`을 상속. 디슈가링으로 생성된 합성 노드에는 `synthetic = true` 플래그 + 원본 AST 포인터를 보존.

2. **디슈가링 전 타입 체크**: `buildExpr()` 내부에서 디슈가링 전에 타입 검사를 먼저 수행. 타입 에러는 항상 원본 AST 기준으로 보고.
   ```
   buildExpr(PipeExpr) {
       // 1. lhs, rhs 타입 체크 (원본 AST 기준 에러 보고)
       // 2. 타입이 맞으면 → Call로 디슈가링하여 HIR 생성
       // 3. 타입이 안 맞으면 → 원본 PipeExpr의 loc으로 에러
   }
   ```

3. **다중 에러 보고 유지**: `DiagnosticEngine`이 에러를 수집하되 HIR 생성을 계속 시도. 타입 에러가 있는 표현식은 `TypeId::Error`를 부여하고 진행. 에러 전파: Error 타입이 포함된 연산은 추가 에러를 보고하지 않음 (cascade 방지).

4. **Side-table HIR 대응**:
   | 현재 TypeChecker side-table | HIR 대응 |
   |---|---|
   | `struct_defs_` | `HIRModule.structs` (HIRStructDecl 배열) |
   | `enum_defs_` | `HIRModule.enums` (HIREnumDecl 배열) |
   | `union_defs_` | `HIRModule.unions` (HIRUnionDecl 배열) |
   | `expr_types_` | HIR 노드의 `TypeId type` 필드 (side-table 불필요) |
   | `expr_struct_names_` / `local_struct_names_` | HIR 노드의 `TypeId`로 TypeTable 조회 |
   | `expr_ptr_info_` / `local_ptr_info_` | `TypeId`의 `TypeKind::Ptr/PtrMut` + `TypeInfo.ptr.pointee` |
   | scope stack | HIRBuilder 내부 `SymbolTable` (name → VReg + TypeId) |

#### HIR Pass 인터페이스

```cpp
// include/kern/hir/HIRPass.h
class HIRPass {
public:
    virtual ~HIRPass() = default;
    virtual std::string_view name() const = 0;
    virtual void run(HIRModule& module, CompilationContext& ctx) = 0;
};

class HIRPassManager {
    std::vector<std::unique_ptr<HIRPass>> passes_;
public:
    template<typename T, typename... Args>
    void add(Args&&... args);

    void run(HIRModule& module, CompilationContext& ctx);
    // 순서: 등록 순서대로 실행 (topo sort는 v3에서)
};
```

초기 HIR 패스:
1. **PurityAnalysisPass** — call graph 분석, purity 추론 (현재 PurityChecker)
2. **TailCallAnalysisPass** — tail position 분석, is_tail_recursive 마킹
3. **ExhaustivenessCheckPass** — match 완전성 검사 (현재 TypeChecker 내장 → 분리)

M6에 추가될 패스:
4. **ClosureConversionPass** — 캡처 분석, 람다→리프팅된 함수 + 환경 구조체
5. **MonomorphizationPass** — 제네릭 함수 인스턴스화

---

### 3.4 LIR (Low-level IR)

현재 `KernIR.h`의 역할을 계승하되, 완전히 새로 설계.

```
include/kern/lir/
├── LIR.h          — LIR 노드 정의 (SSA, VReg, tagged union)
├── LIRBuilder.h   — HIR → LIR lowering
├── LIRDump.h      — LIR 텍스트 출력
└── LIRPass.h      — LIR 패스 인터페이스
```

#### LIR 노드 (기존 WS-3 확장)

```cpp
// include/kern/lir/LIR.h

using VReg = uint32_t;  // Virtual Register
constexpr VReg INVALID_VREG = UINT32_MAX;

enum class LIROp : uint8_t {
    // Constants
    ConstInt, ConstFloat, ConstBool,
    ConstString,   // 문자열 리터럴 → GlobalData 인덱스 참조
    GlobalRef,     // 글로벌 데이터 주소 로드 (float 상수 풀 등)

    // Integer arithmetic
    Add, Sub, Mul, Div, Mod,

    // Float arithmetic
    FAdd, FSub, FMul, FDiv,

    // Comparison (결과: bool vreg)
    ICmpEq, ICmpNe, ICmpLt, ICmpLe, ICmpGt, ICmpGe,
    FCmpEq, FCmpNe, FCmpLt, FCmpLe, FCmpGt, FCmpGe,

    // Unary
    Neg, FNeg, Not,

    // Memory (M5)
    AddrOf,       // &x → ptr
    Load,         // *ptr → value
    Store,        // *ptr = value
    FieldPtr,     // ptr + offset → ptr (GEP-like)
    StructAlloc,  // stack alloc for struct

    // Control
    Branch,       // unconditional jump
    CondBranch,   // conditional jump
    Ret,

    // Call
    Call,          // direct call by name
    CallIndirect,  // indirect call via vreg (M6)

    // Phi-like (block parameter)
    BlockArg,
};

// Tagged union 설계 (기존 WS-3 계승)
struct LIRInstr {
    LIROp op;
    VReg result;         // 결과 virtual register (INVALID_VREG if void)
    TypeId type;         // 결과 타입
    SourceLocation loc;  // 디버그용 원본 위치

    union {
        struct { int64_t value; }                  const_int;
        struct { double value; }                   const_float;
        struct { bool value; }                     const_bool;
        struct { uint32_t global_index; }          const_string;  // GlobalData 인덱스
        struct { uint32_t global_index; }          global_ref;    // GlobalData 인덱스
        struct { VReg lhs; VReg rhs; }             bin;
        struct { VReg operand; }                   unary;
        struct { std::string_view callee;
                 VReg* args; uint32_t arg_count;
                 bool is_tail; }                   call;
        struct { VReg callee_vreg;
                 VReg* args; uint32_t arg_count; } call_indirect;
        struct { uint32_t target; }                branch;
        struct { VReg cond;
                 uint32_t true_target;
                 uint32_t false_target; }          cond_branch;
        struct { VReg value; }                     ret;
        struct { VReg ptr; }                       load;
        struct { VReg ptr; VReg value; }           store;
        struct { VReg base; uint32_t offset; }     field_ptr;
        struct { uint32_t size; uint32_t align; }  struct_alloc;
        struct { VReg source; }                    addr_of;
        struct { uint32_t index; }                 block_arg;
    };
};

struct LIRBlock {
    std::string_view label;         // interned
    TypeId* param_types;            // block parameter types (phi 대체)
    uint32_t param_count;
    LIRInstr* instrs;               // Arena 할당 배열
    uint32_t instr_count;
};

struct LIRFunction {
    std::string_view name;          // interned
    TypeId* param_types;
    uint32_t param_count;
    TypeId return_type;
    LIRBlock* blocks;               // Arena 할당 배열
    uint32_t block_count;
    VReg next_vreg;                 // vreg 할당 카운터

    // 메타데이터
    Purity purity;
    bool is_tail_recursive;

    // 모듈 시스템 + 링킹 예약 (M6+)
    enum class Linkage : uint8_t { Internal, External };
    Linkage linkage = Linkage::External;  // 현재는 모두 External
};

// 글로벌 데이터 (문자열 리터럴, float 상수 등 .rodata 배치 대상)
struct GlobalData {
    enum Kind : uint8_t { StringLit, FloatConst, RawBytes };
    Kind kind;
    uint32_t index;              // 모듈 내 고유 인덱스
    std::string_view label;      // NASM 라벨 (interned)
    union {
        struct { const char* data; uint32_t length; }  string_lit;
        struct { double value; bool is_f32; }          float_const;
        struct { const uint8_t* data; uint32_t size; } raw_bytes;
    };
};

struct LIRModule {
    LIRFunction* functions;
    uint32_t fn_count;

    // 글로벌 데이터 섹션 (.rodata에 emit됨)
    GlobalData* globals;
    uint32_t global_count;
};
```

**글로벌 데이터 흐름**: HIRBuilder가 문자열/float 리터럴을 만나면 `LIRModule.globals`에 등록하고 `ConstString`/`GlobalRef` opcode에서 인덱스로 참조. Backend의 NASMEmitter가 `.rodata` 섹션을 생성할 때 GlobalData 배열을 순회하여 `db`, `dd`, `dq` 디렉티브 생성.

#### LIR 패스

초기 패스:
1. **TCOPass** — tail call → argument overwrite + jump (현재 CodeGen 내장 로직 → LIR 패스로 승격)
2. (선택) **ConstantFoldingPass** — `ConstInt(3) + ConstInt(4)` → `ConstInt(7)`
3. (선택) **DeadCodeEliminationPass** — 사용되지 않는 vreg 정의 제거

---

### 3.5 MachIR + Backend (기존 WS-4, WS-7 계승)

```
include/kern/backend/
├── MachIR.h           — x86-64 machine instruction 표현
├── InstructionSelector.h  — LIR → MachIR 변환
├── RegisterAllocator.h    — vreg → physical reg (linear scan)
├── ABI.h              — System V AMD64 호출 규약
├── Emitter.h          — MachIR → NASM 텍스트
└── X86Backend.h       — 전체 backend 오케스트레이션
```

#### MachIR

```cpp
// include/kern/backend/MachIR.h

enum class PhysReg : uint8_t {
    RAX, RBX, RCX, RDX, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15,
    XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7,
    XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
    RSP, RBP,
    NONE,
};

struct MachOperand {
    enum Kind : uint8_t { Reg, Imm, Stack, Label, None };
    Kind kind;
    bool is_physical;        // true = PhysReg 확정, false = VReg (RegAlloc 전)
    union {
        uint32_t vreg;
        PhysReg phys;
        int64_t imm;
        int32_t stack_offset;
        std::string_view label;
    };

    // Pre-colored register 지원 (ABI 강제 레지스터)
    // InstructionSelector가 ABI 인자/반환 레지스터를 pre-color로 생성
    // RegisterAllocator는 pre-colored operand를 고정(fixed interval)으로 처리
    static MachOperand precolored(PhysReg r) {
        return {Kind::Reg, true, {.phys = r}};
    }
    static MachOperand virt(uint32_t v) {
        return {Kind::Reg, false, {.vreg = v}};
    }
};

enum class X86Op : uint8_t {
    Mov, MovZX, MovSX,
    Add, Sub, IMul, IDiv, Xor,
    Cmp, Test,
    Setcc,       // + condition code
    Jmp, Jcc,    // + condition code
    Call, Ret,
    Push, Pop,
    Lea,
    Cqo,         // rdx:rax sign extend (implicit operands)
    Rep_Movsb,   // memcpy: rcx bytes from [rsi] to [rdi] (implicit)
    Nop,         // alignment padding
    // SSE
    Movss, Movsd, Addsd, Subsd, Mulsd, Divsd, Ucomisd,
    // Pseudo-instructions (RegAlloc/Emitter가 확장)
    Pseudo_ParallelMove,   // 다중 레지스터 동시 이동 (cycle-break 포함)
    Pseudo_FrameSetup,     // prologue placeholder (RegAlloc 후 확정)
    Pseudo_FrameDestroy,   // epilogue placeholder (RegAlloc 후 확정)
};

// 가변 길이 operand 배열로 multi-register 명령어 지원
// 대부분 명령어는 operand_count ≤ 3이므로 인라인 최적화
static constexpr uint8_t MACH_INLINE_OPERANDS = 3;

struct MachInstr {
    X86Op op;
    uint8_t condition;       // for Setcc, Jcc
    uint8_t width;           // 8, 16, 32, 64
    uint8_t operand_count;   // 실제 operand 개수
    union {
        MachOperand inline_ops[MACH_INLINE_OPERANDS];  // 3개 이하: 인라인
        MachOperand* heap_ops;                          // 4개 이상: Arena 할당
    };

    // 편의 접근자
    MachOperand& dst()  { return operand(0); }
    MachOperand& src1() { return operand(1); }
    MachOperand& src2() { return operand(2); }
    MachOperand& operand(uint8_t i) {
        return (operand_count <= MACH_INLINE_OPERANDS)
            ? inline_ops[i] : heap_ops[i];
    }
};

// Pseudo-instruction 예시:
// Pseudo_ParallelMove: operands = [dst0, src0, dst1, src1, ...]
//   → Emitter가 cycle-break 알고리즘으로 mov 시퀀스 생성
// Pseudo_FrameSetup/Destroy: operands = [] (RegAlloc이 callee-saved 확정 후 확장)
// Cqo, Rep_Movsb: operands = [] (implicit register가 고정이므로 operand 불필요)
//   → Emitter가 해당 명령어 텍스트를 직접 생성

struct MachBlock {
    std::string_view label;
    MachInstr* instrs;
    uint32_t instr_count;
};

struct MachFunction {
    std::string_view name;
    MachBlock* blocks;
    uint32_t block_count;
    uint32_t stack_size;
    bool callee_saved[16];   // 어떤 callee-saved reg가 사용되었는지
};
```

#### Instruction Selector (LIR → MachIR)
```cpp
class InstructionSelector {
    CompilationContext& ctx_;
public:
    // LIR의 virtual register를 그대로 유지하며 x86 opcode로 변환
    MachFunction* select(const LIRFunction& fn);
private:
    // 패턴 매칭: LIR opcode → 1개 이상의 MachInstr
    // 예: LIR::Add(vreg, vreg) → X86::Mov(dst, src1) + X86::Add(dst, src2)
    // 예: LIR::ICmpLt + CondBranch → X86::Cmp + X86::Jcc(lt)
};
```

#### Register Allocator (Linear Scan)
```cpp
class RegisterAllocator {
public:
    struct LiveInterval {
        VReg vreg;
        TypeId type;
        uint32_t start;     // instruction index
        uint32_t end;       // last use instruction index
        PhysReg hint;       // 선호 물리 레지스터 (ABI 등, NONE=힌트 없음)
        bool is_fixed;      // true = pre-colored, 물리 레지스터 변경 불가
    };

    // Phase 1: liveness analysis → live intervals
    // pre-colored operand → is_fixed=true + hint=해당 PhysReg
    std::vector<LiveInterval> computeIntervals(const MachFunction& fn);

    // Phase 2: linear scan → vreg→PhysReg/StackSlot 매핑
    // Fixed interval: 해당 PhysReg를 점유 기간 동안 예약 (다른 vreg 할당 불가)
    // Hint interval: 가능하면 hint 레지스터에 할당, 불가 시 다른 레지스터
    struct Allocation {
        std::unordered_map<VReg, PhysReg> reg_map;
        std::unordered_map<VReg, int32_t> spill_map;
        uint32_t stack_size;
        bool callee_saved_used[16];
    };

    Allocation allocate(std::vector<LiveInterval>& intervals);

    // Phase 3: rewrite MachInstrs to use physical regs
    // + Pseudo_FrameSetup/Destroy를 실제 push/pop/sub rsp로 확장
    void rewrite(MachFunction& fn, const Allocation& alloc);
};
```

#### ABI Handler
```cpp
class SystemVABI {
public:
    // 호출 준비: 인자를 어떤 레지스터/스택에 배치할지 계산
    struct ArgPlacement {
        enum Kind { Reg, Stack, RegPair };  // RegPair = 9-16B struct
        PhysReg reg;
        int32_t stack_offset;
    };
    std::vector<ArgPlacement> planArgs(std::span<TypeId> param_types,
                                        const TypeTable& types);

    // parallel-move 알고리즘 (1곳에서만 구현!)
    std::vector<MachInstr> emitParallelMoves(
        std::span<std::pair<MachOperand, MachOperand>> moves);

    // _start wrapper 생성
    MachFunction* emitEntryPoint(std::string_view main_fn);

    // prologue/epilogue 생성
    std::vector<MachInstr> emitPrologue(const MachFunction& fn);
    std::vector<MachInstr> emitEpilogue(const MachFunction& fn);
};
```

#### Emitter (MachIR → NASM text)
```cpp
class NASMEmitter {
    std::ostream& out_;
public:
    void emit(const MachFunction& fn);
    void emitRodata(const GlobalData* globals, uint32_t global_count);

private:
    // 단순한 1:1 변환: MachInstr → NASM 텍스트 줄
    void emitInstr(const MachInstr& instr);
    std::string_view regName(PhysReg reg, uint8_t width);
};
```

---

### 3.6 Kern ABI 명세 (System V AMD64 기반)

> 모든 레이어(HIR, LIR, Backend)가 참조하는 단일 ABI 규격.
> Kern은 C ABI/외부 연결성을 배제하므로 System V를 기반으로 하되 struct 전달을 자체 최적화.

#### 레지스터 규약

| 용도 | 레지스터 | 비고 |
|------|---------|------|
| 정수 인자 | rdi, rsi, rdx, rcx, r8, r9 (순서대로) | 최대 6개 |
| Float 인자 | xmm0-xmm7 (순서대로) | 최대 8개 |
| 정수 반환 | rax (1 word), rax+rdx (2 words) | |
| Float 반환 | xmm0 (1 float), xmm0+xmm1 (2 floats) | |
| Callee-saved | rbx, r12, r13, r14, r15, rbp | 함수가 사용 시 push/pop |
| Caller-saved | rax, rcx, rdx, rsi, rdi, r8-r11 | 호출 전후 보존 안 됨 |
| 스택 포인터 | rsp | 16-byte aligned at `call` |
| Float scratch | xmm8-xmm15 | callee-saved 아님 |

#### 인자 분류 (Argument Classification)

각 인자는 타입에 따라 분류된 후 레지스터/스택에 배치된다.

```
classify(type) → class:
    i8..i64, u8..u64, bool, Ptr<T>, Ptr<var T>, enum
        → INTEGER (GPR 1개)
    f32, f64
        → FLOAT (XMM 1개)
    struct (≤8B, 모든 필드가 INTEGER)
        → INTEGER (GPR 1개, 필드들을 pack)
    struct (≤8B, float 필드 포함)
        → FLOAT (XMM 1개)
    struct (9-16B)
        → 각 8B 청크를 별도 분류 (필드 타입에 따라 GPR 또는 XMM)
        → 2개 레지스터 (GPR+GPR, GPR+XMM, XMM+XMM 등)
    struct (>16B)
        → MEMORY (caller가 스택에 복사, 포인터를 GPR로 전달)
    String (= 16B fat pointer)
        → GPR 2개 (ptr=rdi계열, len=rsi계열)
    Unit
        → 무시 (레지스터 소비 없음)
```

#### 인자 배치 알고리즘

```
gpr_idx = 0    // 다음 사용할 GPR (0=rdi, 1=rsi, ..., 5=r9)
xmm_idx = 0   // 다음 사용할 XMM (0=xmm0, ..., 7=xmm7)
stack_offset = 0

for each param in params:
    cls = classify(param.type)
    if cls == INTEGER:
        if gpr_idx < 6:
            assign GPR[gpr_idx++]
        else:
            assign stack[stack_offset]; stack_offset += 8
    elif cls == FLOAT:
        if xmm_idx < 8:
            assign XMM[xmm_idx++]
        else:
            assign stack[stack_offset]; stack_offset += 8
    elif cls == RegPair(c1, c2):
        // 9-16B struct: 각 half에 대해 레지스터 할당 시도
        // 두 half 모두 레지스터에 들어갈 수 있어야 함, 아니면 MEMORY로 fallback
        assign_half(c1, gpr_idx, xmm_idx)
        assign_half(c2, gpr_idx, xmm_idx)
    elif cls == MEMORY:
        // caller가 스택에 복사, 해당 주소를 GPR로 전달
        if gpr_idx < 6:
            assign GPR[gpr_idx++]  // 포인터
        else:
            assign stack[stack_offset]; stack_offset += 8
```

**GPR 인덱스와 XMM 인덱스는 독립적.** `fn(i64, f64, i64)`는 rdi(i64) + xmm0(f64) + rsi(i64).

#### 반환값 규약

| 반환 타입 | 배치 |
|----------|------|
| 정수/bool/ptr/enum | rax |
| f32/f64 | xmm0 |
| struct ≤8B (INTEGER) | rax |
| struct ≤8B (FLOAT) | xmm0 |
| struct 9-16B | rax+rdx, xmm0+xmm1, 또는 rax+xmm0 (필드 타입에 따라) |
| struct >16B | caller가 rdi에 반환 버퍼 포인터 전달 → callee가 채움 → rax에 포인터 반환 |
| String (16B) | rax(ptr) + rdx(len) |
| Unit | 아무것도 반환하지 않음 |

#### 스택 프레임 레이아웃

```
높은 주소
┌──────────────────────┐
│  caller의 스택 인자들   │  [rbp + 16 + N]
├──────────────────────┤
│  return address       │  [rbp + 8]
├──────────────────────┤
│  saved rbp            │  [rbp]
├──────────────────────┤
│  callee-saved regs    │  [rbp - 8], [rbp - 16], ...
├──────────────────────┤
│  로컬 변수 / spill     │  [rbp - K]
├──────────────────────┤
│  outgoing stack args   │  [rsp], [rsp + 8], ...
└──────────────────────┘
낮은 주소
```

- `rsp`는 `call` 명령어 시점에 **16-byte aligned** 필수
- 홀수 개 push 시 `sub rsp, 8` 패딩 (현재 CodeGen의 C2 수정 사항)

#### >16B Struct 값 의미론 (Deep Copy)

```nasm
; callee 진입 — >16B struct 매개변수의 값 의미론 보장
; rdi = caller가 전달한 포인터
sub rsp, <struct_size>        ; 로컬 복사본 공간
lea rdi, [rsp]                ; dst = 로컬
; rsi = 원본 포인터 (caller가 전달)
mov rcx, <struct_size>
rep movsb                     ; deep copy
; 이후 [rsp]를 로컬 변수로 사용 (원본 참조 끊김)
```

---

### 3.7 PassManager 설계 (기존 3.6)

```cpp
// include/kern/pass/PassManager.h

// 범용 패스 인터페이스 (HIR, LIR 공통 패턴)
template<typename IRModule>
class Pass {
public:
    virtual ~Pass() = default;
    virtual std::string_view name() const = 0;
    virtual void run(IRModule& module, CompilationContext& ctx) = 0;
};

template<typename IRModule>
class PassManager {
    struct PassEntry {
        std::unique_ptr<Pass<IRModule>> pass;
        bool enabled = true;
    };
    std::vector<PassEntry> passes_;
public:
    template<typename T, typename... Args>
    void add(Args&&... args) {
        passes_.push_back({std::make_unique<T>(std::forward<Args>(args)...), true});
    }

    void setEnabled(std::string_view name, bool enabled);

    void run(IRModule& module, CompilationContext& ctx) {
        for (auto& entry : passes_) {
            if (entry.enabled) {
                entry.pass->run(module, ctx);
                if (ctx.diag.hasErrors()) return;  // 에러 시 중단
            }
        }
    }
};

using HIRPassManager = PassManager<HIRModule>;
using LIRPassManager = PassManager<LIRModule>;
```

---

### 3.8 CompilerPipeline (기존 WS-6 대체)

```cpp
// include/kern/CompilerPipeline.h
class CompilerPipeline {
    CompilationContext ctx_;
    HIRPassManager hir_passes_;
    LIRPassManager lir_passes_;

public:
    CompilerPipeline();

    // 패스 등록 (기본 패스 자동 등록 + 사용자 커스텀)
    HIRPassManager& hirPasses() { return hir_passes_; }
    LIRPassManager& lirPasses() { return lir_passes_; }

    // 전체 파이프라인
    struct Result {
        bool success;
        Module* ast;           // --dump-ast용
        HIRModule* hir;        // --dump-hir용
        LIRModule* lir;        // --dump-lir용
        std::string assembly;  // 최종 NASM 텍스트
    };

    Result compile(std::string_view source, std::string_view filename);

    // 개별 단계 (테스트용)
    Module* parse(std::string_view source, std::string_view filename);
    HIRModule* buildHIR(const Module* ast);
    LIRModule* lowerToLIR(const HIRModule* hir);
    std::string emitAssembly(const LIRModule* lir);

    CompilationContext& context() { return ctx_; }
};
```

---

## 4. 프로젝트 전체 디렉토리 구조 (v2)

> **설계 모델**: LLVM/Clang 모노레포 패턴 — Library-first + thin binaries.
> 모든 컴파일러 로직은 `lib/`에 라이브러리로 존재하고, `tools/`의 각 바이너리는
> 필요한 라이브러리 조각만 링크하는 thin wrapper.

```
kern/
├── include/kern/                    ═══ 공개 라이브러리 API (headers) ═══
│   ├── support/
│   │   ├── Arena.h
│   │   ├── Diagnostic.h
│   │   ├── SourceLocation.h
│   │   ├── StringPool.h             — NEW
│   │   └── TypeSystem.h             — NEW (TypeId, TypeTable, TypeInfo)
│   ├── lexer/
│   │   ├── Token.h
│   │   └── Lexer.h
│   ├── parser/
│   │   ├── AST.h                    — 수정 (디슈가링 안 함, Module 확장)
│   │   ├── Parser.h
│   │   └── ASTDump.h                — NEW (분리)
│   ├── hir/                         — NEW 전체
│   │   ├── HIR.h                    — HIR 노드 정의
│   │   ├── HIRBuilder.h             — AST → HIR (TypeCheck + Desugar 통합)
│   │   ├── HIRDump.h                — HIR 텍스트 출력
│   │   └── HIRPass.h                — HIR 패스 인터페이스
│   ├── lir/                         — NEW (기존 ir/ 대체)
│   │   ├── LIR.h                    — LIR 노드 (SSA, VReg, tagged union)
│   │   ├── LIRBuilder.h             — HIR → LIR lowering
│   │   ├── LIRDump.h                — LIR 텍스트 출력
│   │   └── LIRPass.h                — LIR 패스 인터페이스
│   ├── backend/                     — NEW (기존 codegen/ 대체)
│   │   ├── MachIR.h                 — x86-64 machine instructions
│   │   ├── InstructionSelector.h    — LIR → MachIR
│   │   ├── RegisterAllocator.h      — linear scan
│   │   ├── ABI.h                    — System V AMD64
│   │   ├── Emitter.h                — MachIR → NASM text
│   │   └── X86Backend.h             — backend 오케스트레이션
│   ├── ide/                         — NEW (IDE 전용 query 레이어)
│   │   ├── IDEContext.h             — IDE 세션 상태 관리
│   │   ├── CompletionProvider.h     — 자동완성 쿼리
│   │   ├── DiagnosticProvider.h     — 실시간 진단 (on-the-fly)
│   │   ├── HoverProvider.h          — 심볼 hover 정보
│   │   ├── DefinitionProvider.h     — go-to-definition
│   │   ├── ReferenceProvider.h      — find-all-references
│   │   ├── SymbolIndex.h            — 프로젝트 심볼 인덱스
│   │   └── SemanticTokens.h         — 시맨틱 하이라이팅 토큰
│   ├── debug/                       — NEW (디버그 정보 생성 + 런타임)
│   │   ├── DebugInfo.h              — 디버그 메타데이터 (소스맵, 심볼 테이블)
│   │   ├── DebugInfoBuilder.h       — MachIR → 디버그 정보 생성
│   │   ├── SourceMap.h              — instruction addr ↔ source location 매핑
│   │   └── ValueInspector.h         — 런타임 값 읽기 (메모리 레이아웃 해석)
│   ├── fmt/                         — NEW (포매터 엔진)
│   │   ├── Formatter.h              — AST pretty-printer
│   │   └── FormatStyle.h            — 포맷 옵션 (indent, braces 등)
│   ├── lint/                        — NEW (정적 분석 / 린터)
│   │   ├── LintPass.h              — 린트 규칙 인터페이스
│   │   ├── LintEngine.h            — 규칙 등록 + 실행 엔진
│   │   └── rules/                   — 개별 린트 규칙 헤더
│   ├── pass/                        — NEW
│   │   └── PassManager.h            — 범용 패스 매니저
│   └── CompilerPipeline.h           — NEW
│
├── lib/                             ═══ 라이브러리 구현 ═══
│   ├── Support/
│   │   ├── Arena.cpp
│   │   ├── Diagnostic.cpp
│   │   ├── StringPool.cpp           — NEW
│   │   └── TypeSystem.cpp           — NEW
│   ├── Lexer/
│   │   └── Lexer.cpp                — 최소 변경
│   ├── Parser/
│   │   ├── Parser.cpp               — 수정 (디슈가링 제거)
│   │   └── ASTDump.cpp              — NEW (분리)
│   ├── HIR/                         — NEW 전체
│   │   ├── HIRBuilder.cpp           — TypeChecker + 현재 IRBuilder 역할 통합
│   │   ├── HIRDump.cpp
│   │   └── passes/
│   │       ├── PurityAnalysis.cpp
│   │       ├── TailCallAnalysis.cpp
│   │       └── ExhaustivenessCheck.cpp
│   ├── LIR/                         — NEW (기존 IR/ 대체)
│   │   ├── LIRBuilder.cpp           — HIR → LIR lowering
│   │   ├── LIRDump.cpp
│   │   └── passes/
│   │       └── TCO.cpp
│   ├── Backend/                     — NEW (기존 CodeGen/ 대체)
│   │   ├── InstructionSelector.cpp
│   │   ├── RegisterAllocator.cpp
│   │   ├── ABI.cpp
│   │   ├── Emitter.cpp
│   │   └── X86Backend.cpp
│   ├── IDE/                         — NEW (IDE query 레이어)
│   │   ├── IDEContext.cpp
│   │   ├── CompletionProvider.cpp
│   │   ├── DiagnosticProvider.cpp
│   │   ├── HoverProvider.cpp
│   │   ├── DefinitionProvider.cpp
│   │   ├── ReferenceProvider.cpp
│   │   ├── SymbolIndex.cpp
│   │   └── SemanticTokens.cpp
│   ├── Debug/                       — NEW (디버그 정보)
│   │   ├── DebugInfoBuilder.cpp
│   │   ├── SourceMap.cpp
│   │   └── ValueInspector.cpp
│   ├── Fmt/                         — NEW (포매터)
│   │   └── Formatter.cpp
│   ├── Lint/                        — NEW (린터)
│   │   ├── LintEngine.cpp
│   │   └── rules/
│   │       ├── UnusedVariable.cpp
│   │       ├── PurityViolation.cpp
│   │       └── ShadowedBinding.cpp
│   └── Pipeline/                    — NEW
│       └── CompilerPipeline.cpp
│
├── tools/                           ═══ 바이너리 (thin wrappers) ═══
│   ├── kernc/                       — 컴파일러 드라이버 (기존)
│   │   ├── main.cpp                 — CLI 파싱 + CompilerPipeline 호출
│   │   └── CMakeLists.txt
│   ├── kern-lsp/                    — NEW: LSP 서버
│   │   ├── main.cpp                 — LSP 프로토콜 루프
│   │   ├── LSPServer.h/cpp          — JSON-RPC 처리, LSP 메서드 라우팅
│   │   ├── DocumentManager.h/cpp    — 열린 파일 관리, 변경 추적
│   │   └── CMakeLists.txt           — links: kern_ide, kern_hir, kern_parser, kern_lexer
│   ├── kern-dbg/                    — NEW: 전용 디버거
│   │   ├── main.cpp                 — REPL-style 디버거 CLI
│   │   ├── Debugger.h/cpp           — breakpoint, step, continue 엔진
│   │   ├── ProcessController.h/cpp  — ptrace/mach 기반 프로세스 제어
│   │   ├── StackWalker.h/cpp        — 콜 스택 해석
│   │   └── CMakeLists.txt           — links: kern_debug, kern_support
│   ├── kern-fmt/                    — NEW: 포매터
│   │   ├── main.cpp                 — CLI (파일/stdin → 포맷된 출력)
│   │   └── CMakeLists.txt           — links: kern_fmt, kern_parser, kern_lexer
│   ├── kern-lint/                   — NEW: 린터
│   │   ├── main.cpp                 — CLI (파일 → 경고/에러 출력)
│   │   └── CMakeLists.txt           — links: kern_lint, kern_hir, kern_parser
│   ├── kern-repl/                   — NEW: REPL (Read-Eval-Print Loop)
│   │   ├── main.cpp                 — 대화형 입력 루프
│   │   ├── REPLEngine.h/cpp         — 증분 파싱 + JIT-like 실행
│   │   └── CMakeLists.txt           — links: kern_pipeline (전체)
│   └── kern-pkg/                    — NEW: 패키지 매니저
│       ├── main.cpp                 — CLI (init, install, build, publish)
│       ├── PackageResolver.h/cpp    — 의존성 해석
│       ├── Manifest.h/cpp           — kern.toml 파싱
│       └── CMakeLists.txt           — links: kern_support
│
├── tests/                           ═══ 테스트 ═══
│   ├── unit/                        — GoogleTest 단위 테스트
│   │   ├── support/                 — Arena, StringPool, TypeSystem 테스트
│   │   ├── lexer/
│   │   ├── parser/
│   │   ├── hir/
│   │   ├── lir/
│   │   ├── backend/
│   │   ├── ide/                     — NEW: IDE query 레이어 테스트
│   │   ├── debug/                   — NEW: 디버그 정보 생성 테스트
│   │   ├── fmt/                     — NEW: 포매터 테스트
│   │   └── lint/                    — NEW: 린트 규칙 테스트
│   ├── integration/                 — E2E 테스트 (.kern + .expected)
│   │   ├── *.kern + *.expected      — 앵커 테스트 (exit code + error 기반)
│   │   ├── dump/                    — dump 테스트 (내부 포맷 의존, 변경 가능)
│   │   └── run_tests.sh
│   ├── tool/                        — NEW: 도구별 통합 테스트
│   │   ├── lsp/                     — LSP 프로토콜 테스트 (request→response)
│   │   ├── fmt/                     — 포매팅 전후 비교 테스트
│   │   ├── lint/                    — 린트 결과 테스트
│   │   └── dbg/                     — 디버거 시나리오 테스트
│   └── coverage/                    — NEW: 커버리지 인프라
│       ├── run_coverage.sh          — 커버리지 빌드 + 리포트 생성
│       └── coverage_gate.sh         — 98% 미달 시 exit 1 (CI 게이트)
│
├── runtime/                         ═══ NEW: 런타임 라이브러리 ═══
│   ├── kern_rt.asm                  — 런타임 (GC, allocator 등 미래용)
│   └── CMakeLists.txt
│
├── stdlib/                          ═══ NEW: 표준 라이브러리 (미래) ═══
│   ├── core/                        — 핵심 타입 (Result, Maybe, Ordering)
│   ├── io/                          — I/O 바인딩
│   └── collections/                 — 리스트, 맵 등
│
├── docs/                            — 문서 (변경 불가)
├── cmake/                           — NEW: 공유 CMake 모듈
│   ├── KernAddLibrary.cmake         — add_kern_library() 매크로
│   ├── KernAddTool.cmake            — add_kern_tool() 매크로
│   └── KernConfig.cmake.in          — 외부 프로젝트용 config
├── CMakeLists.txt                   — 루트 빌드 (lib/ + tools/ + tests/)
└── README.md
```

**삭제되는 것** (v2 전환 시):
- `include/kern/sema/` — TypeChecker.h, PurityChecker.h → HIR로 흡수
- `include/kern/ir/` — KernIR.h, IRBuilder.h, IRType.h, Metadata.h → LIR로 대체
- `include/kern/codegen/` — CodeGen.h → backend로 대체
- `lib/Sema/` → `lib/HIR/`로 흡수
- `lib/IR/` → `lib/LIR/`로 대체
- `lib/CodeGen/` → `lib/Backend/`로 대체

### 도구별 라이브러리 의존성 매트릭스

| 도구 | support | lexer | parser | hir | lir | backend | pipeline | ide | debug | fmt | lint |
|------|---------|-------|--------|-----|-----|---------|----------|-----|-------|-----|------|
| **kernc** | O | O | O | O | O | O | O | - | O* | - | - |
| **kern-lsp** | O | O | O | O | - | - | - | **O** | - | O | O |
| **kern-dbg** | O | - | - | - | - | - | - | - | **O** | - | - |
| **kern-fmt** | O | O | O | - | - | - | - | - | - | **O** | - |
| **kern-lint** | O | O | O | O | - | - | - | - | - | - | **O** |
| **kern-repl** | O | O | O | O | O | O | **O** | - | - | - | - |
| **kern-pkg** | O | - | - | - | - | - | - | - | - | - | - |

> *kernc의 debug 의존은 `-g` 플래그로 디버그 정보 생성 시에만 필요

**핵심**: 각 도구는 필요한 조각만 링크. LSP는 LIR/Backend를 모름, 디버거는 Parser를 모름.

---

## 5. 빌드 의존성 그래프

```
kern_support  (Arena, Diagnostic, StringPool, TypeSystem)
     │
     ├───────────────────────────────────────────────────┐
     │                                                   │
kern_lexer    (Lexer, Token)                        kern_debug  (DebugInfo, SourceMap, ValueInspector)
     │                                                   │
kern_parser   (Parser, AST, ASTDump)                     │
     │                                                   │
     ├──────────┬──────────┐                             │
     │          │          │                             │
kern_fmt    kern_hir    kern_lint                         │
 (Formatter)  │  (HIRBuilder, passes)  (LintEngine)      │
              │          │                               │
         kern_ide     kern_lir                            │
    (IDE queries)   │  (LIRBuilder, passes)              │
                    │                                    │
               kern_backend                              │
    (InstSel, RegAlloc, ABI, Emitter)                    │
                    │                                    │
               kern_pipeline  ──────────────────── (opt) │
                    │                                    │
     ┌──────────┬───┴───────┬────────┐                   │
     │          │           │        │                   │
   kernc    kern-repl   kern-lsp  kern-lint         kern-dbg
                                 kern-fmt           kern-pkg
```

**순방향 의존만 존재.** 역방향 의존 완전 제거.

**CMake 타겟 구조**:
```cmake
# cmake/KernAddLibrary.cmake
macro(add_kern_library name)
    add_library(kern_${name} STATIC ${ARGN})
    target_include_directories(kern_${name} PUBLIC ${PROJECT_SOURCE_DIR}/include)
    target_compile_features(kern_${name} PUBLIC cxx_std_20)
endmacro()

macro(add_kern_tool name)
    add_executable(${name} ${ARGN})
    target_compile_features(${name} PRIVATE cxx_std_20)
endmacro()
```

```cmake
# lib/CMakeLists.txt 예시
add_kern_library(support lib/Support/Arena.cpp lib/Support/StringPool.cpp ...)
add_kern_library(lexer lib/Lexer/Lexer.cpp)
target_link_libraries(kern_lexer PUBLIC kern_support)
# ...

add_kern_library(ide lib/IDE/IDEContext.cpp lib/IDE/CompletionProvider.cpp ...)
target_link_libraries(kern_ide PUBLIC kern_hir kern_parser kern_lexer)

add_kern_library(debug lib/Debug/DebugInfoBuilder.cpp lib/Debug/SourceMap.cpp ...)
target_link_libraries(kern_debug PUBLIC kern_support)
```

```cmake
# tools/kern-lsp/CMakeLists.txt 예시
add_kern_tool(kern-lsp main.cpp LSPServer.cpp DocumentManager.cpp)
target_link_libraries(kern-lsp PRIVATE kern_ide kern_fmt kern_lint)
```

---

## 6. 마이그레이션 전략 (병렬 실행 최적화)

> **원칙**: 의존성이 없는 작업은 병렬로 실행. 아래 ▸ 표시가 같은 Phase 내 병렬 가능 작업.

### 빅뱅 전략 + Phase별 검증

**빅뱅** = 기존 파이프라인(AST→IR→CodeGen)을 새 파이프라인(AST→HIR→LIR→MachIR→NASM)으로 전면 교체.
Phase 2~4 동안 **동작하는 컴파일러가 없는 "dark period"** 가 존재한다. 이를 수용하되, 각 Phase에서 레이어별 정확성을 독립 검증한다.

#### Phase별 검증 전략

| Phase | 검증 방법 | 앵커 E2E | 비고 |
|-------|----------|:---:|------|
| 2 (HIR) | **HIR interpreter** + unit tests | X | 간단한 tree-walk interpreter로 HIR 의미론 검증. 산술/비교/if/match/call 지원. 메모리/struct는 스킵 가능 |
| 3a (LIR) | **LIR interpreter** + unit tests | X | SSA 기반 interpreter. block arguments + VReg 해석. 기존 fib/arith 같은 핵심 테스트 케이스를 interpreter로 실행하여 exit code 비교 |
| 4a (Backend) | MachIR → NASM 후 **실제 실행** | **O** | 이 시점에서 처음으로 69개 앵커 E2E 테스트 가능 |
| 5a (통합) | **전체 파이프라인** E2E 검증 | **O** | 기존 파이프라인 삭제, 새 파이프라인으로 교체 |

#### HIR/LIR Interpreter 설계 (검증 전용, 프로덕션 코드 아님)

```cpp
// tests/unit/hir/HIRInterpreter.h — 테스트 전용
class HIRInterpreter {
    CompilationContext& ctx_;
    std::unordered_map<std::string_view, int64_t> globals_;  // fn name → callable
public:
    // HIRModule의 "main" 함수를 실행하고 반환값(i64 exit code)을 돌려줌
    int64_t run(const HIRModule& module);
private:
    int64_t evalExpr(const HIRExpr* expr, Env& env);
    int64_t evalCall(std::string_view fn, std::span<int64_t> args);
};
```

- **범위**: 정수 연산, 비교, if/else, match, 함수 호출, tail call만 지원
- **미지원**: float, struct, ptr, string (이들은 unit test로 개별 검증)
- **목적**: `tests/integration/`의 순수 정수 계산 E2E 케이스(fib, arith, comparison 등)를 파이프라인 연결 전에 HIR/LIR 단독으로 검증

이렇게 하면 Phase 2 완료 시점에 HIR interpreter로 ~30개 정수 기반 테스트를 돌릴 수 있고, Phase 3 완료 시점에 LIR interpreter로 같은 테스트를 돌릴 수 있다. Phase 4a에서 처음으로 실제 바이너리 E2E 검증.

### Phase 0: 준비 — ✅ 완료

- [ ] git tag `v0.5-m5` ← **M5 완료됨! 태그 생성 필요**
- [ ] E2E 앵커 테스트 분리 확인 ← **M5 완료됨! 113개 E2E 중 앵커/dump 분리 필요**
- [x] `cmake/` 공유 모듈 작성: `KernAddLibrary.cmake`, `KernAddTool.cmake`, `KernCoverage.cmake`
- [x] CMakeLists.txt에 `CMAKE_MODULE_PATH` + `KERN_COVERAGE` 옵션 추가
- [x] **스킬 12개 전부 작성 완료** (Phase별 점진 작성 계획에서 선행 완료로 변경):
  - `/kern:build`, `/kern:e2e-add`, `/kern:test` (Phase 0)
  - `/kern:add-pass` (Phase 2 예정이었으나 선행 완료)
  - `/kern:add-opcode`, `/kern:add-lint` (Phase 3 예정이었으나 선행 완료)
  - `/kern:pipeline-trace`, `/kern:layer-check`, `/kern:coverage` (Phase 4 예정이었으나 선행 완료)
  - `/kern:add-ast-node`, `/kern:add-tool`, `/kern:add-type` (Phase 5 예정이었으나 선행 완료)
- [x] **Rules 작성**: `rules/test-policy.md`, `rules/architecture-v2.md`, `rules/layer-boundaries.md`
- [x] **Hooks 작성** (비활성): `hooks/layer-guard.sh`, `hooks/anchor-protect.sh`
- [x] **Coverage 인프라**: `scripts/run_coverage.sh`, `scripts/coverage_gate.sh`
- [x] **CI**: `.github/workflows/ci.yml` coverage job 추가
- [x] `.gitignore` 업데이트: `build-cov/`, `*.profraw`, `*.profdata`

### Phase 1: 인프라 (바닥부터) — ✅ 완료

```
┌──────────────────────────────┐  ┌─────────────────────────┐
│ 1a. StringPool + 테스트  ✅   │  │ 1b. TypeSystem + 테스트 ✅│
│     (Arena 기반 문자열 인터닝) │  │     (TypeId, TypeTable)  │
└──────────────────────────────┘  └─────────────────────────┘
                    │                          │
                    └─────────┬────────────────┘
                              ▼
                 1c. CompilationContext 통합 ✅
                     (StringPool + TypeSystem + Arena + Diag)
```

**결과물:**
- [x] `include/kern/support/StringPool.h` + `lib/Support/StringPool.cpp` (13 unit tests)
- [x] `include/kern/support/TypeSystem.h` + `lib/Support/TypeSystem.cpp` (23 unit tests)
  - TypeId(uint32_t), TypeKind enum, TypeInfo tagged union, TypeTable (primitives 0-12 pre-registered)
  - makePtr, makeFn, makeStruct, makeEnum, makeUnion, sizeOf/alignOf/bitWidth 쿼리
- [x] `include/kern/support/CompilationContext.h` (6 unit tests)
- [x] `lib/Support/CMakeLists.txt` 수정 (+2 source files)
- [x] `tests/unit/CMakeLists.txt` 수정 (+3 test files, DISCOVERY_MODE PRE_TEST)
- **총 42개 신규 unit tests** (StringPool 13 + TypeSystem 23 + CompilationContext 6)
- 기존 코드 미변경 — 새 파일만 추가

### Phase 2: HIR 레이어 구축 — ✅ 완료 (660 unit + 115 E2E, commit f12364f)

```
2a. HIR.h 노드 정의 + HIRDump
         │
         ▼
2b. HIRBuilder (AST → HIR, TypeCheck + Desugar 통합)
         │
         ▼
   ┌─────┴──────────────────┐──────────────────────┐
   │ 2c. PurityAnalysisPass  │ 2d. TailCallAnalysis │ 2e. ExhaustivenessCheck │
   └────────────────────────┘──────────────────────┘
                              │
                              ▼
                    2f. --dump-hir 플래그 연결
```

▸ **2c + 2d + 2e 병렬**: 세 패스 모두 HIR 노드에만 의존. 서로 독립적.
▸ ~~**스킬 작성**: `/kern:add-pass`~~ ✅ Phase 0에서 선행 완료
▸ **에이전트 워크플로우**: 2c: `/kern:add-pass PurityAnalysis --level hir` → 구현 → `/kern:build`
▸ ~~**환경 작성**: `rules/layer-boundaries.md`~~ ✅ Phase 0에서 선행 완료

### Phase 3: LIR 레이어 — ✅ 완료 (695 unit + 115 E2E, commit 713e664)

```
┌────────────────────────────┐  ┌─────────────────────┐  ┌──────────────────┐
│ 3a. LIR.h 노드 정의         │  │ 3b. kern_fmt 라이브러리│  │ 3c. kern_lint 엔진 │
│     + LIRDump               │  │     (AST→pretty-print)│  │   (HIR 기반 분석)  │
│     + LIRBuilder            │  │     + kern-fmt CLI    │  │   + kern-lint CLI │
│     + TCO pass              │  │                      │  │                   │
│     + --dump-lir            │  │  ▸ Parser만 의존!     │  │  ▸ HIR만 의존!     │
└────────────────────────────┘  └─────────────────────┘  └──────────────────┘
              │
              ▼
```

▸ **3a + 3b + 3c 병렬**: LIR은 HIR→LIR lowering, fmt는 Parser만, lint는 HIR만 의존.
▸ ~~**스킬 작성**: `/kern:add-opcode`, `/kern:add-lint`~~ ✅ Phase 0에서 선행 완료
▸ **환경 활성화**: `hooks/layer-guard.sh` (작성 완료, settings.json에 등록 필요)
▸ **에이전트 워크플로우**:
  - 3a: `/kern:add-opcode <op>` 반복 → LIRBuilder 구현 → `/kern:build`
  - 3b: `/kern:add-tool kern-fmt` → Formatter 구현 → `/kern:test fmt`
  - 3c: `/kern:add-lint <rule>` 반복 → LintEngine 구현 → `/kern:test lint`

### Phase 4: Backend + Debug + IDE — ✅ 전체 완료 (4a: 784u+116e commit 25573fb, 4b/4c: commit 9ce95aa)

```
┌──────────────────────────────────┐  ┌──────────────────────┐  ┌─────────────────┐
│ 4a. MachIR + InstructionSelector │  │ 4b. kern_debug        │  │ 4c. kern_ide     │
│     + RegisterAllocator          │  │     (DebugInfoBuilder  │  │  (IDEContext,    │
│     + ABI + NASMEmitter          │  │      SourceMap,        │  │   Completion,    │
│     + X86Backend                 │  │      ValueInspector)   │  │   Hover, GoTo,   │
│                                  │  │                       │  │   References,    │
│                                  │  │  ▸ Support만 의존!    │  │   SemanticTokens)│
│                                  │  │                       │  │                  │
│  ▸ LIR만 의존!                    │  │                       │  │  ▸ HIR만 의존!    │
└──────────────────────────────────┘  └──────────────────────┘  └─────────────────┘
```

▸ **4a + 4b + 4c 병렬**: Backend는 LIR만, Debug는 Support만, IDE는 HIR만 의존. 완전 독립.
▸ ~~**스킬 작성**: `/kern:pipeline-trace`, `/kern:layer-check`, `/kern:coverage`~~ ✅ Phase 0에서 선행 완료
▸ **에이전트 워크플로우**:
  - 4a: InstructionSelector 구현 → `/kern:pipeline-trace fib.kern` 검증 → `/kern:build`
  - 4b: DebugInfoBuilder 구현 → `/kern:test debug` → `/kern:coverage` (debug만)
  - 4c: IDEContext 구현 → Completion/Hover/GoTo → `/kern:test ide`
  - 각 완료 후: `/kern:layer-check` (역방향 의존 전무 확인)

### Phase 5: 통합 + 도구 어셈블리 — ✅ 완료 (5a-5f, v1 삭제, LSP wired, tool tests)

```
5a. CompilerPipeline으로 전체 연결
    main.cpp 교체 (새 파이프라인 사용)
    **69개 앵커 E2E 테스트 통과 확인** ← 최우선 성공 기준
    기존 sema/, ir/, codegen/ 삭제
         │
         ▼
   ┌─────┴─────────────┐────────────────┐
   │ 5b. kern-lsp 연결  │ 5c. kern-fmt    │
   │  (IDE + fmt + lint │  (포매터 CLI)   │
   │   + LSP 프로토콜)   │                │
   └────────────────────┘────────────────┘
                              │
                              ▼
                    5d. unit 테스트 전면 재작성
                    5e. dump E2E 업데이트
                    5f. 도구별 통합 테스트 (tests/tool/)
```

> ⚠️ **kern-dbg, kern-repl은 Phase 7로 분리** (아래 참조).
> 이유: kern-dbg는 DWARF 생성 + macOS SIP 제약으로 난이도 극상.
> kern-repl은 증분 컴파일 모델이 필요하며 Arena generation (8.3) 설계 선행 필수.
> Phase 5의 스코프를 핵심 파이프라인 안정화에 집중한다.

▸ **5b + 5c 병렬**: 두 도구 모두 라이브러리 조립만 다름.
▸ **5d + 5e + 5f 병렬**: 테스트 작성은 서로 독립.
▸ ~~**스킬 작성**: `/kern:add-ast-node`, `/kern:add-tool`, `/kern:add-type`~~ ✅ Phase 0에서 선행 완료
▸ **환경 활성화**: `hooks/anchor-protect.sh` (작성 완료, settings.json에 등록 필요)
▸ **환경 완성**: CLAUDE.md 최종 교체, `rules/architecture.md` v1 삭제 → v2 교체
▸ **에이전트 워크플로우**:
  - 5a: CompilerPipeline 연결 → `/kern:build` → **69개 앵커 E2E 통과 확인** (최우선)
  - 5b: `/kern:add-tool kern-lsp` → LSP 프로토콜 구현 → `tests/tool/lsp/` 테스트
  - 5c: `/kern:add-tool kern-fmt` → 포매터 CLI 구현 → `tests/tool/fmt/` 테스트
  - 5d-f: `/kern:e2e-add` 반복 → 전체 테스트 보강
  - 최종: `/kern:coverage` → **전체 lib/ 98% 달성 확인** → `/kern:layer-check`

### Phase 6: 패키지 매니저 + 표준 라이브러리 — ✅ 완료 (6a: kern-pkg, 6b: stdlib/core.kern with Option/Result/utils/intrinsics)

```
┌───────────────────┐  ┌──────────────────────┐
│ 6a. kern-pkg       │  │ 6b. stdlib/core       │
│  (kern.toml 파싱,  │  │  (Result, Maybe 등   │
│   의존성 해석,      │  │   .kern 소스로 작성)  │
│   빌드 오케스트레이션)│  │                      │
└───────────────────┘  └──────────────────────┘
```

▸ **6a + 6b 병렬**: 패키지 매니저는 빌드 시스템, 표준 라이브러리는 언어 차원.
▸ **에이전트 워크플로우**:
  - 6a: `/kern:add-tool kern-pkg` → kern.toml 파서 → 의존성 해석 → `/kern:test pkg`
  - 6b: `/kern:add-type` (Result, Maybe 등) → stdlib .kern 소스 작성 → `/kern:build`

### Phase 7: 고난이도 도구 — 🟡 스켈레톤 완료 (kern-dbg CLI + kern-repl full-recompile)

> kern-dbg와 kern-repl은 선행 인프라가 충분히 성숙한 후에 착수.

```
┌───────────────────────┐  ┌──────────────────────┐
│ 7a. kern-dbg           │  │ 7b. kern-repl         │
│  선행: DWARF 생성 구현  │  │  선행: Arena gen (8.3) │
│  선행: SourceMap 완성   │  │  선행: 증분 파싱 모델   │
│  macOS: codesign 필요   │  │                       │
│  ptrace → mach API      │  │  compile→exec 루프    │
└───────────────────────┘  └──────────────────────┘
```

▸ **7a 선행 조건**:
  - `lib/Debug/DebugInfoBuilder.cpp`가 DWARF `.debug_info`/`.debug_line` 섹션을 NASM으로 emit
  - `lib/Debug/SourceMap.cpp`가 MachIR instruction → source location 매핑 제공
  - macOS에서 디버거 실행: `kern-dbg` 바이너리에 `com.apple.security.get-task-allow` entitlement + codesign
  - ptrace 대신 macOS Mach API (`task_for_pid`, `thread_get_state`) 사용
  - **현실적 스코프 축소**: 첫 버전은 breakpoint + continue + print 만 지원. step/backtrace는 후속

▸ **7b 선행 조건**:
  - Arena generation (Section 8.3) 구현 완료
  - 증분 파싱: 이전 세대의 심볼 테이블을 새 세대가 참조하는 모델
  - **현실적 스코프 축소**: 첫 버전은 매 표현식마다 full compile + exec. 증분 최적화는 후속

### 커버리지 게이트 (Phase별)

> 매 Phase 완료 시 **해당 Phase에서 추가된 코드**의 line coverage 98% 이상 필수.
> Phase 5 통합 후 **전체 프로젝트** line coverage 98% 이상 달성.

| Phase | 커버리지 대상 | 게이트 기준 | 측정 방법 |
|-------|-------------|-----------|----------|
| 0 | - | - | 기존 커버리지 베이스라인 측정 |
| 1 | `lib/Support/StringPool.cpp`, `TypeSystem.cpp` | 98% line | unit 테스트 |
| 2 | `lib/HIR/**` | 98% line | unit + AST→HIR 변환 테스트 |
| 3a | `lib/LIR/**` | 98% line | unit + HIR→LIR 변환 테스트 |
| 3b | `lib/Fmt/**` | 98% line | 포맷 전후 비교 테스트 |
| 3c | `lib/Lint/**` | 98% line | 린트 규칙별 positive/negative 테스트 |
| 4a | `lib/Backend/**` | 98% line | unit + E2E 앵커 |
| 4b | `lib/Debug/**` | 98% line | 디버그 정보 생성 unit 테스트 |
| 4c | `lib/IDE/**` | 98% line | IDE query unit 테스트 |
| 5 | **전체** (`lib/**`) | **98% line** | 전체 테스트 스위트 |

### 병렬 실행 요약

| Phase | 순차 (선행) | 병렬 가능 항목 | 병렬 수 | 검증 수단 | 상태 |
|-------|-----------|--------------|---------|----------|------|
| 0 | 전부 순차 | - | - | 기존 E2E | ✅ 완료 |
| 1 | 1c | 1a, 1b | 2 | unit tests | ✅ 완료 |
| 2 | 2a, 2b, 2f | 2c, 2d, 2e | 3 | **HIR interpreter** + unit | ✅ 완료 (660 unit + 115 E2E) |
| 3 | - | 3a, 3b, 3c | 3 | **LIR interpreter** + unit | ✅ 완료 (695 unit + 115 E2E) |
| 4 | - | 4a, 4b, 4c | 3 | **69 E2E 앵커** (처음!) | ✅ 완료 (4a: 784u+116e, 4b: Debug, 4c: IDE+DiagnosticProvider) |
| 5 | 5a | 5b, 5c / 5d, 5e, 5f | 2+3 | 전체 E2E + coverage | ✅ 완료 (v1 삭제, LSP wired, kern-fmt, tool tests) |
| 6 | - | 6a, 6b | 2 | pkg/stdlib 테스트 | ✅ 완료 (6a: kern-pkg, 6b: stdlib/core.kern Option/Result/utils) |
| 7 | - | 7a, 7b | 2 | 도구별 통합 테스트 | 🟡 스켈레톤 (kern-dbg CLI, kern-repl full-recompile) |

---

## 7. 기존 리팩토링 요구사항과의 대응

| 기존 WS | v2 대응 | 상태 |
|---------|---------|------|
| WS-1 StringPool | 그대로 포함 (Section 3.1) | 계승 |
| WS-2 TypeSystem | Support 레이어로 격상 (Section 3.1) | 계승+강화 |
| WS-3 IRInstr 재설계 | LIR tagged union (Section 3.4) | 대체 |
| WS-4 CodeGen 분리 | Backend 전체 분리 (Section 3.5) | 대체+확장 |
| WS-5 ASTDump | 그대로 포함 (Section 4) | 계승 |
| WS-6 Pipeline | CompilerPipeline (Section 3.7) | 계승+강화 |
| WS-7 RegAlloc | Backend RegisterAllocator (Section 3.5) | 계승 |
| WS-8 IRType 통합 | TypeId 전 파이프라인 공유로 해소 | 해소 |
| WS-9 Module 확장 | AST Module 확장 (Section 3.2) | 계승 |
| WS-10 테스트 | E2E 앵커 + unit 재작성 (Section 6 Phase 5) | 계승 |

**새로 추가된 것**: HIR 레이어, HIR 패스, MachIR 레이어, InstructionSelector, PassManager

---

## 8. 테스트 전략 — 커버리지 98% 달성 계획

### 8.1 목표

> **전체 `lib/` line coverage 98% 이상**. 새로 작성하는 코드는 처음부터 98%를 유지하며 쌓아가는 방식.

| 지표 | 목표 | 측정 도구 |
|------|------|----------|
| Line coverage | **≥ 98%** | llvm-cov (Clang source-based) |
| Branch coverage | ≥ 90% | llvm-cov |
| Function coverage | 100% | llvm-cov |
| E2E 앵커 통과율 | 100% | run_tests.sh |

### 8.2 Phase별 중간 레이어 테스트 전략

빅뱅 전환 중 "dark period"(Phase 2~3)에서 각 레이어의 정확성을 보장하기 위한 테스트 전략.

#### Phase 2 (HIR) 테스트

| 테스트 유형 | 대상 | 방법 |
|-----------|------|------|
| **Unit: HIRBuilder** | AST → HIR 변환 정확성 | 수동 구성 AST 입력 → HIR 출력 검증 (타입, 디슈가링) |
| **Unit: HIR Passes** | Purity, TailCall, Exhaustiveness | HIR 입력 구성 → 패스 실행 → 메타데이터 검증 |
| **Unit: HIRDump** | 텍스트 출력 정확성 | HIR → dump 텍스트 → 스냅샷 비교 |
| **Integration: HIR Interpreter** | 의미론 검증 | `.kern` 소스 → Parser → HIRBuilder → HIRInterpreter → exit code 비교 |

**HIR Interpreter 대상**: 순수 정수 계산 테스트 (fib, arith, comparison, match, tce 등 ~30개).
Float/struct/ptr는 unit test로만 검증 (interpreter 미지원).

#### Phase 3 (LIR) 테스트

| 테스트 유형 | 대상 | 방법 |
|-----------|------|------|
| **Unit: LIRBuilder** | HIR → LIR lowering 정확성 | HIR 입력 → LIR 출력 검증 (SSA form, VReg 할당) |
| **Unit: LIR Passes** | TCO, ConstantFolding, DCE | LIR 입력 → 패스 실행 → LIR 출력 검증 |
| **Unit: LIRDump** | 텍스트 출력 정확성 | LIR → dump 텍스트 → 스냅샷 비교 |
| **Integration: LIR Interpreter** | 의미론 검증 | `.kern` → ... → LIR → LIRInterpreter → exit code 비교 |

**LIR Interpreter 대상**: HIR interpreter와 동일한 ~30개 + SSA 특유의 block argument 테스트.

#### Phase 4 (Backend) 테스트

| 테스트 유형 | 대상 | 방법 |
|-----------|------|------|
| **Unit: InstructionSelector** | LIR → MachIR 패턴 | LIR opcode → MachInstr 검증 |
| **Unit: RegisterAllocator** | vreg → phys 매핑 | 수동 MachFunction → allocation 검증 |
| **Unit: NASMEmitter** | MachIR → NASM text | MachInstr → 텍스트 줄 비교 |
| **E2E: 전체 파이프라인** | 바이너리 정확성 | **69개 앵커 E2E 테스트 — 이 시점에서 처음 실행 가능** |

#### 테스트 헬퍼 구조

```cpp
// tests/unit/TestHelpers.h — 모든 레이어에서 재사용

// Source → AST (파싱만)
Module* parseSource(CompilationContext& ctx, std::string_view source);

// Source → HIR (파싱 + 타입 체크 + 디슈가링)
HIRModule* buildHIR(CompilationContext& ctx, std::string_view source);

// Source → LIR (파싱 + HIR + lowering)
LIRModule* buildLIR(CompilationContext& ctx, std::string_view source);

// HIR/LIR interpreter로 실행하고 exit code 반환
int64_t interpretHIR(CompilationContext& ctx, const HIRModule& module);
int64_t interpretLIR(CompilationContext& ctx, const LIRModule& module);
```

### 8.3 Arena 수명 관리 — IDE/REPL 시나리오

> Arena는 bump allocator로, 개별 해제가 불가능하다. 단일 파일 컴파일(kernc)에는 문제없지만,
> IDE(kern-lsp)와 REPL(kern-repl)에서는 반복적 재파싱/재분석이 필요하므로 별도 설계가 필요하다.

#### 문제

| 시나리오 | 문제점 |
|---------|--------|
| **kern-lsp**: 파일 수정 시 re-parse/re-check | 매번 새 Arena를 만들면 TypeId가 무효화됨 |
| **kern-repl**: 매 표현식마다 compile + execute | Arena가 무한 성장 |

#### 해결: Arena Generation (세대별 관리)

```cpp
// Phase 4c (IDE) 구현 시 도입
struct ArenaGeneration {
    Arena arena;                    // 이 세대의 bump allocator
    uint32_t generation_id;         // 세대 번호
};

struct IDECompilationContext {
    // 영구 데이터 (세대 간 공유)
    Arena permanent_arena;          // TypeTable, StringPool → 영구
    StringPool strings;             // 문자열은 영구 (새 문자열만 추가, 삭제 안 함)
    TypeTable types;                // 타입도 영구 (TypeId 안정성 보장)
    DiagnosticEngine diag;

    // 세대별 데이터 (파일 수정 시 폐기 + 재생성)
    ArenaGeneration* current_gen;   // AST, HIR 노드 → 현재 세대에 할당

    void resetGeneration() {
        // 현재 세대의 Arena를 폐기하고 새 세대 생성
        // TypeTable, StringPool은 보존 (TypeId 안정성)
        current_gen = new ArenaGeneration{Arena{}, next_gen_id_++};
    }
};
```

**핵심**: `StringPool`과 `TypeTable`은 영구 Arena에, AST/HIR 노드는 세대별 Arena에 할당.
파일 수정 시 세대별 Arena만 리셋하면 TypeId와 interned string은 보존된다.

**REPL**: 각 평가를 별도 세대로 관리. N번째 입력의 AST/HIR은 N번째 세대에 할당.
이전 세대의 결과(값)는 별도 결과 저장소에 복사.

**구현 시점**: Phase 4c (kern_ide 라이브러리) 착수 시. Phase 2~3에서는 단일 CompilationContext로 충분.

### 8.4 커버리지 인프라

#### CMake 커버리지 빌드 옵션
```cmake
# CMakeLists.txt 추가
option(KERN_COVERAGE "Enable code coverage" OFF)
if(KERN_COVERAGE)
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate)
endif()
```

#### 커버리지 측정 스크립트
```bash
# tests/coverage/run_coverage.sh
#!/bin/bash
set -e

# 1. 커버리지 빌드
cmake -B build-cov -DKERN_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-cov

# 2. unit + E2E 실행 (profraw 생성)
LLVM_PROFILE_FILE="build-cov/default.profraw" build-cov/tests/unit/kern_tests
LLVM_PROFILE_FILE="build-cov/e2e.profraw" bash tests/integration/run_tests.sh \
    build-cov/tools/kernc/kernc tests/integration

# 3. profraw → profdata 병합
llvm-profdata merge -sparse \
    build-cov/default.profraw build-cov/e2e.profraw \
    -o build-cov/merged.profdata

# 4. 리포트 생성
llvm-cov report \
    build-cov/tests/unit/kern_tests \
    -object build-cov/tools/kernc/kernc \
    -instr-profile=build-cov/merged.profdata \
    -ignore-filename-regex="(tests/|_deps/|tools/kernc/main.cpp)" \
    | tee build-cov/coverage-report.txt

# 5. HTML 리포트 (상세)
llvm-cov show \
    build-cov/tests/unit/kern_tests \
    -object build-cov/tools/kernc/kernc \
    -instr-profile=build-cov/merged.profdata \
    -ignore-filename-regex="(tests/|_deps/)" \
    -format=html -output-dir=build-cov/coverage-html

echo "HTML report: build-cov/coverage-html/index.html"
```

#### 커버리지 게이트 (CI 블로커)
```bash
# tests/coverage/coverage_gate.sh
#!/bin/bash
# 98% 미달 시 CI 실패
THRESHOLD=98.0
COVERAGE=$(llvm-cov report \
    build-cov/tests/unit/kern_tests \
    -object build-cov/tools/kernc/kernc \
    -instr-profile=build-cov/merged.profdata \
    -ignore-filename-regex="(tests/|_deps/|tools/)" \
    | grep "TOTAL" | awk '{print $NF}' | sed 's/%//')

echo "Coverage: ${COVERAGE}%  (threshold: ${THRESHOLD}%)"
if (( $(echo "$COVERAGE < $THRESHOLD" | bc -l) )); then
    echo "FAIL: Coverage below threshold!"
    # 미달 파일 목록 출력
    llvm-cov report \
        build-cov/tests/unit/kern_tests \
        -object build-cov/tools/kernc/kernc \
        -instr-profile=build-cov/merged.profdata \
        -ignore-filename-regex="(tests/|_deps/|tools/)" \
        -show-region-summary=false \
        | awk 'NR>2 && $NF+0 < 98 {print "  " $0}'
    exit 1
fi
echo "PASS"
```

### 8.5 테스트 3계층 구조

```
┌─────────────────────────────────────────────────────┐
│  Layer 3: E2E 앵커 테스트 (tests/integration/)       │
│  .kern → kernc → binary → exit code / error msg     │
│  ▸ 변경에 흔들리지 않는 불변 계약                      │
│  ▸ 69+ 앵커 (exit/error) + dump 테스트 분리           │
│  ▸ 커버리지: 전체 파이프라인 end-to-end 경로           │
├─────────────────────────────────────────────────────┤
│  Layer 2: 도구 통합 테스트 (tests/tool/)              │
│  LSP: JSON-RPC request → response 검증               │
│  Formatter: input.kern → expected_output.kern 비교    │
│  Linter: input.kern → expected diagnostics 비교       │
│  Debugger: scenario script → expected state 비교      │
│  ▸ 도구의 외부 인터페이스 계약 검증                     │
├─────────────────────────────────────────────────────┤
│  Layer 1: 단위 테스트 (tests/unit/)                   │
│  라이브러리별 GoogleTest                               │
│  ▸ 98% 커버리지의 주력                                 │
│  ▸ edge case, error path, boundary 집중              │
└─────────────────────────────────────────────────────┘
```

### 8.6 라이브러리별 테스트 전략

| 라이브러리 | 단위 테스트 초점 | 커버리지 핵심 영역 | 예상 테스트 수 |
|-----------|----------------|------------------|--------------|
| **kern_support** | Arena 경계, StringPool 중복/concat, TypeId intern/get, TypeKind 전체, sizeOf/alignOf | 모든 TypeKind 조합, Arena 블록 경계 | 60+ |
| **kern_lexer** | 토큰 분류 전체, 에러 토큰, 유니코드 엣지 | 모든 토큰 타입, 에러 경로 | 40+ |
| **kern_parser** | 모든 AST 노드 파싱, 에러 복구, 우선순위 | 모든 구문, 에러 메시지 경로 | 80+ |
| **kern_hir** | HIRBuilder 변환 (모든 AST→HIR 케이스), 디슈가링 (pipe, fn pattern, match→decision tree), 패스별 분석 | 디슈가링 분기, 타입 에러 경로, 패스 결과 | 120+ |
| **kern_lir** | LIRBuilder lowering (모든 HIR→LIR 케이스), TCO 변환, VReg 할당 | SSA 변환 정확성, block arg 전달, tail call 패턴 | 80+ |
| **kern_backend** | InstructionSelector (모든 LIR op→MachInstr), RegAlloc (spill, liveness), ABI (arg placement, parallel-move), Emitter (모든 MachInstr→NASM) | 레지스터 압력, spill 경로, ABI 엣지케이스 | 100+ |
| **kern_ide** | 자동완성 후보, hover 타입 정보, go-to-def 정확성, 시맨틱 토큰, 파일 변경 시 캐시 무효화 | 불완전한 코드에서의 동작, 캐시 dirty 경로 | 60+ |
| **kern_debug** | SourceMap 매핑, DebugInfo 직렬화/역직렬화, ValueInspector 타입별 해석 | 모든 타입의 메모리 레이아웃 해석, 빈 함수 엣지 | 40+ |
| **kern_fmt** | 모든 AST 노드의 pretty-print, 들여쓰기 규칙, pipe 정렬, match arm 정렬, 줄 길이 제한 | long line 분할, 중첩 구조 | 50+ |
| **kern_lint** | 규칙별 true positive/false negative, auto-fix 정확성 | 각 규칙의 경계 케이스, false positive 방지 | 30+ |
| **합계** | | | **660+** |

### 8.7 커버리지 98% 달성 핵심 기법

#### 1. TDD — 테스트 먼저, 코드 나중
```
실패하는 테스트 작성 → 최소 구현 → 통과 → 리팩토링
```
모든 Phase의 모든 컴포넌트에 적용. "코드를 쓴 뒤 테스트"가 아니라 "테스트가 코드를 이끌게".

#### 2. Error path 전수 테스트
```cpp
// 예: TypeSystem에서 잘못된 TypeId 접근
TEST(TypeSystem, GetInvalidTypeId) {
    TypeTable types(arena);
    EXPECT_THROW(types.get(99999), ...);  // 또는 assert
}

// 예: HIRBuilder에서 타입 불일치
TEST(HIRBuilder, TypeMismatchInBinOp) {
    // i64 + bool → 에러
    auto* ast = makeAddExpr(makeIntLit(1), makeBoolLit(true));
    EXPECT_TRUE(ctx.diag.hasErrors());
}
```

> 커버리지 갭의 80%는 에러 경로에서 발생. 모든 `if (error)` 분기에 대응하는 테스트 필수.

#### 3. Boundary value 테스트
```cpp
// Arena 블록 경계
TEST(Arena, ExactBlockBoundary) {
    Arena arena;
    arena.alloc(4096);     // 블록 꽉 참
    arena.alloc(1);        // 새 블록 할당
}

// 정수 타입 경계
TEST(TypeSystem, IntFitsInType) {
    EXPECT_TRUE(intFitsInType(127, TypeId::I8));
    EXPECT_FALSE(intFitsInType(128, TypeId::I8));
    EXPECT_TRUE(intFitsInType(255, TypeId::U8));
    EXPECT_FALSE(intFitsInType(256, TypeId::U8));
}
```

#### 4. 누락 커버리지 자동 탐지 (CI)
```yaml
# .github/workflows/ci.yml 확장
  coverage:
    runs-on: macos-13
    steps:
    - uses: actions/checkout@v4
    - name: Install tools
      run: brew install nasm llvm
    - name: Coverage build
      run: bash tests/coverage/run_coverage.sh
    - name: Coverage gate (98%)
      run: bash tests/coverage/coverage_gate.sh
    - name: Upload HTML report
      uses: actions/upload-artifact@v4
      with:
        name: coverage-report
        path: build-cov/coverage-html/
```

### 8.8 E2E 앵커 테스트 원칙 (재확인)

> **E2E는 아웃풋에만 의존. 내부 변경에 흔들리면 안 된다.**

| 분류 | 위치 | 검증 방법 | 변경 정책 |
|------|------|----------|----------|
| **앵커 (불변)** | `tests/integration/*.expected` | exit code, error substring | v2 전환 후에도 100% 통과 필수 |
| **dump (가변)** | `tests/integration/dump/*.expected` | stdout substring | 내부 포맷 변경 시 업데이트 허용 |
| **도구** | `tests/tool/{lsp,fmt,lint,dbg}/` | 도구별 프로토콜/포맷 | 도구 API 변경 시 업데이트 |

### 8.9 Phase별 테스트 진행 흐름

```
Phase 0: 베이스라인 측정 (현재 커버리지 확인)
    │     → coverage_gate.sh 인프라 구축
    │     → CI에 커버리지 job 추가
    ▼
Phase 1: StringPool 98% + TypeSystem 98%
    │     → 개별 라이브러리 커버리지 확인
    ▼
Phase 2: HIR 98% (unit 120+개)
    │     → HIR 레벨 dump 테스트 추가
    ▼
Phase 3: LIR 98% ∥ Fmt 98% ∥ Lint 98%
    │     → 도구 통합 테스트 시작 (tests/tool/)
    ▼
Phase 4: Backend 98% ∥ Debug 98% ∥ IDE 98%
    │     → E2E 앵커 재검증 (Backend 교체 후)
    ▼
Phase 5: 전체 통합 → **lib/ 전체 98% 달성**
    │     → CI 게이트 활성화 (98% 미달 = 빌드 실패)
    ▼
Phase 6+: 유지 (신규 코드도 98% 필수)
```

---

## 9. 리스크 및 완화

| 리스크 | 확률 | 영향 | 완화 |
|--------|------|------|------|
| 작업량 과다 | 높음 | M5 완료 후 추가 대규모 작업 | Phase별 E2E 앵커 검증. Phase 3 완료만으로도 현재보다 나은 아키텍처 |
| HIR + LIR 중복 | 중간 | 두 IR이 비슷해질 수 있음 | HIR = 이름 기반 + 고수준 변환용, LIR = SSA + 최적화용으로 역할 명확히 분리 |
| MachIR 과잉 설계 | 중간 | x86만 타겟하는데 추상화 레이어 | MachIR은 간단하게 유지. 핵심 가치는 VReg→PhysReg 분리 |
| 기존 버그 재발 | 중간 | 새 코드에서 M1-M4 버그 반복 | 69개 E2E 앵커 + 기존 버그 시나리오를 E2E로 보존 |
| HIRBuilder 복잡도 | 높음 | TypeCheck + Desugar 통합이 어려울 수 있음 | 초기에는 TypeCheck 분리 유지하고 HIR만 생성. 통합은 점진적 |
| IDE 증분 컴파일 복잡도 | 중간 | 파일 변경 시 HIR 재빌드 범위 결정 | 초기에는 파일 단위 전체 재빌드. 나중에 함수 단위 증분 |
| 디버거 플랫폼 의존성 | 높음 | macOS Mach ports + Rosetta 2 환경 | ptrace 추상화 레이어, x86 디버깅은 Rosetta 위에서 동작 확인 필요 |
| LSP JSON-RPC 구현 | 낮음 | 표준 프로토콜이지만 구현량 있음 | nlohmann/json 또는 simdjson 사용. LSP 스펙 서브셋부터 시작 |
| 도구 간 API 안정성 | 중간 | IDE/Debug 레이어 API가 자주 바뀔 수 있음 | 도구는 Phase 5에서 통합. 그 전까지 lib/IDE, lib/Debug API 안정화 |
| 커버리지 98% 유지 부담 | 중간 | 에러 경로/엣지케이스 테스트 작성 시간 증가 | TDD로 처음부터 커버리지 확보. Phase별 게이트로 누적 부채 방지 |
| 커버리지 도구 호환성 | 낮음 | llvm-cov + Rosetta 2 환경 이슈 | CI는 Intel macOS (macos-13)에서 실행. 로컬은 build-cov 별도 설정 |

---

## 9b. 미결정 사항 (갭 분석 반영)

> 아래 항목 중 ✅ 표시는 이번 갭 분석에서 결정된 것.

1. ✅ **HIRBuilder TypeCheck 통합**: 통합하되, 에러 보고 전략 명세 추가 (Section 3.3 참조). 디슈가링 전 타입 체크 수행, 다중 에러 보고 유지.
2. ✅ **Match decision tree 시점**: Phase 2에서는 nested if/else 유지. M6 이후 Maranget 알고리즘으로 전환 (Section 3.3 HIRMatchExpr 참조).
3. ✅ **MachIR operand 모델**: 가변 길이 operand 배열 + pseudo-instructions (Section 3.5 참조). Cqo/Rep_Movsb는 implicit operand.
4. **PassManager 의존성 DAG**: 초기에는 순서 고정. v3에서 선언적 의존성 도입 검토
5. ✅ **Purity metadata**: HIR.FnDecl에 내장 (Section 3.3 참조). 패스 순서: PurityAnalysisPass가 TailCallAnalysisPass보다 먼저 실행.
6. **디버그 정보 포맷**: DWARF 호환 방향 (lldb 재활용). Phase 7a 착수 시 DWARF subset 선정
7. **LSP JSON 라이브러리**: Phase 5b 착수 시 결정. nlohmann/json이 유력 (header-only, CMake 호환)
8. ✅ **REPL 실행 방식**: 초기에는 매번 full compile + exec. Arena generation (Section 8.3) 구현 후 증분 최적화
9. **패키지 매니페스트 포맷**: Phase 6a 착수 시 결정
10. **표준 라이브러리 언어**: Phase 6b 착수 시 결정. 핵심 타입은 Kern 소스, I/O는 intrinsic 바인딩이 유력
11. ✅ **모듈 시스템 확장 포인트**: HIRModule에 imports/visibility 예약 필드 추가, LIRFunction에 Linkage 예약 추가 (Section 3.3, 3.4 참조)
12. ✅ **Pre-colored register**: RegisterAllocator에 fixed interval 지원 추가 (Section 3.5 참조)
13. ✅ **kern-dbg/repl 스코프**: Phase 7로 분리. Phase 5는 핵심 파이프라인 안정화에 집중 (Section 6 Phase 7 참조)

### 메모리 모델 (향후 정의 필요)

> 커널 언어로서 메모리 순서(ordering)와 동시성 모델 정의가 필요하지만,
> 현재 Kern은 **단일 스레드 실행** 전제이므로 v2에서는 형식적 메모리 모델을 정의하지 않는다.

**현재 전제 (v2)**:
- 모든 메모리 접근은 **순차 일관성 (sequential consistency)** — 단일 스레드 내에서 프로그램 순서대로 실행
- `Ptr<var T>` 쓰기는 같은 스레드 내 후속 `Ptr<T>` 읽기에 즉시 반영
- 컴파일러는 `Ptr<var T>` 쓰기를 재배치하지 않음 (observable side effect)
- `Ptr<T>` 읽기만의 함수는 pure → 재배치/제거/메모이제이션 가능

**v3+ (멀티코어 커널 지원 시)**:
- `volatile Ptr<var T>` 또는 `atomic` 수식자 도입 필요
- 메모리 순서: Acquire/Release/SeqCst 모델 (C++11 memory order 참조)
- 이 시점에서 formal memory model 문서 작성

### 성능 벤치마크 계획

> 4-레벨 파이프라인은 2-레벨보다 컴파일 시간이 증가할 수 있다.
> 이를 측정하고 관리하기 위한 벤치마크 프레임워크.

**측정 대상**:
| 지표 | 현재 기준 (v1) | v2 목표 |
|------|:---:|:---:|
| fib(35) 컴파일 시간 | 측정 필요 | ≤ 2x v1 |
| 100개 함수 모듈 컴파일 시간 | 측정 필요 | ≤ 3x v1 |
| fib(35) 실행 시간 | 측정 필요 | ≤ 1.1x v1 (런타임 회귀 없음) |
| 컴파일러 메모리 사용량 (peak RSS) | 측정 필요 | ≤ 2x v1 |

**실행 계획**:
- Phase 0: v1 기준값 측정 (M5 완료 후, `time` + `getrusage`)
- Phase 5a: v2 기준값 측정 (파이프라인 통합 후)
- 이후: 각 Phase에서 회귀 확인

**벤치마크 스크립트** (`scripts/bench.sh`):
```bash
#!/bin/bash
# 컴파일 시간 측정
for f in tests/bench/*.kern; do
    echo "=== $(basename $f) ==="
    time build/tools/kernc/kernc "$f" -o /tmp/bench_out 2>/dev/null
done

# 실행 시간 측정 (Rosetta 2)
for f in tests/bench/*.kern; do
    build/tools/kernc/kernc "$f" -o /tmp/bench_out 2>/dev/null
    time /tmp/bench_out
done
```

### CMakeLists.txt Phase 1 통합 미비 수정 필요

> ⚠️ 현재 `lib/Support/CMakeLists.txt`에 `StringPool.cpp`와 `TypeSystem.cpp`가 누락되어 있음.
> Phase 1 완료로 표시되어 있지만 빌드 시스템에 통합되지 않은 상태.
> Phase 2 착수 전에 반드시 수정 필요:

```cmake
# lib/Support/CMakeLists.txt — 수정 필요
add_kern_library(support
    Arena.cpp
    Diagnostic.cpp
    StringPool.cpp      # ← 누락, 추가 필요
    TypeSystem.cpp       # ← 누락, 추가 필요
)
```

### M5 Open Questions 결정 상태

| OQ | 질문 | 상태 | 결정/방향 |
|:---:|------|:---:|----------|
| OQ-1 | 재귀 Union (Tree) | 미결정 | M6 제네릭과 함께 `Box<T>` (자동 heap 할당) 도입 예정 |
| OQ-2 | Struct 비교 (`==`, `!=`) | 미결정 | 구조적 동등성으로 가되, 도출 (derive) 메커니즘 필요. M6 이후 |
| OQ-3 | 포인터 산술 (`ptr + offset`) | 미결정 | 커널 필수. `unsafe` 블록 내에서만 허용하는 방향. v3 |
| OQ-4 | String 보간 | 미결정 | M6 (람다/클로저 기반 fmt 함수와 함께) |
| OQ-5 | with-copy 패턴 | 미결정 | M6에서 `val p2 = p with { x: 99 }` 문법 도입 검토 |
| OQ-6 | volatile 포인터 | 미결정 | v3 메모리 모델과 함께 |
| OQ-7 | 레이아웃 어노테이션 | 미결정 | v3 |
| OQ-8 | Ref<T> 통합 | 미결정 | M6 이후 소유권 시스템 설계 시 |
| OQ-9 | Variant 추론 실패 에러 | ✅ M5b에서 기본 구현 완료 | 에러 메시지 개선은 HIR 전환 시 |

---

## 10. 도구 생태계 상세 설계

### 10.1 IDE 레이어 (`lib/IDE/`)

> clang의 `lib/IDE/` 패턴. 컴파일러 내부를 IDE-friendly query API로 래핑.

```cpp
// include/kern/ide/IDEContext.h
// IDE 세션 = 열린 파일 + 증분 HIR 캐시
class IDEContext {
    CompilationContext& ctx_;
    // 파일별 캐시: source → AST → HIR (변경된 파일만 재빌드)
    std::unordered_map<std::string_view, CachedFileState> files_;

public:
    explicit IDEContext(CompilationContext& ctx);

    // 파일 열기/변경/닫기 (LSP의 textDocument/didOpen 등)
    void openFile(std::string_view path, std::string_view content);
    void updateFile(std::string_view path, std::string_view content);
    void closeFile(std::string_view path);

    // query API — 모든 도구가 이걸 통해 접근
    const HIRModule* getHIR(std::string_view path);  // lazy rebuild
    const Module* getAST(std::string_view path);
};

struct CachedFileState {
    std::string content;        // 현재 소스 텍스트
    Module* ast = nullptr;      // 캐시된 AST
    HIRModule* hir = nullptr;   // 캐시된 HIR
    uint64_t version = 0;       // 변경 카운터
    bool dirty = true;          // 재빌드 필요?
};
```

```cpp
// include/kern/ide/CompletionProvider.h
class CompletionProvider {
public:
    struct CompletionItem {
        std::string_view label;
        std::string_view detail;     // 타입 정보
        enum Kind { Function, Variable, Type, Field, Keyword };
        Kind kind;
    };

    // 커서 위치에서 자동완성 후보 반환
    std::vector<CompletionItem> complete(
        IDEContext& ctx, std::string_view path,
        uint32_t line, uint32_t column);
};

// include/kern/ide/HoverProvider.h
class HoverProvider {
public:
    struct HoverResult {
        std::string_view type_info;    // "fn(i64, i64) -> i64"
        std::string_view doc;          // 미래: doc comment
        Purity purity;                 // pure/impure 표시
        SourceLocation definition_loc; // go-to-definition 위치
    };

    std::optional<HoverResult> hover(
        IDEContext& ctx, std::string_view path,
        uint32_t line, uint32_t column);
};
```

**핵심**: IDE 레이어는 **Backend를 모른다**. AST→HIR까지만 접근하면 모든 IDE 기능이 가능하다.

### 10.2 LSP 서버 (`tools/kern-lsp/`)

```cpp
// tools/kern-lsp/LSPServer.h
// LSP 프로토콜 핸들러 — 얇은 레이어
class LSPServer {
    IDEContext ide_ctx_;
    CompletionProvider completion_;
    HoverProvider hover_;
    DiagnosticProvider diagnostics_;
    DefinitionProvider definition_;
    SemanticTokens semantic_tokens_;
    // ...

public:
    // stdin/stdout JSON-RPC 루프
    void run();

private:
    // LSP 메서드 → IDE query 라우팅
    json handleInitialize(const json& params);
    json handleTextDocumentCompletion(const json& params);
    json handleTextDocumentHover(const json& params);
    json handleTextDocumentDefinition(const json& params);
    json handleTextDocumentDidOpen(const json& params);
    json handleTextDocumentDidChange(const json& params);
    json handleTextDocumentSemanticTokens(const json& params);
    // ...
};
```

**LSP가 링크하는 라이브러리**: `kern_ide`, `kern_fmt` (onType formatting), `kern_lint` (diagnostics 보강)

### 10.3 전용 디버거 (`tools/kern-dbg/`)

```cpp
// include/kern/debug/DebugInfo.h
// 컴파일러가 생성하는 디버그 메타데이터
struct DebugInfo {
    struct FunctionInfo {
        std::string_view name;
        SourceLocation source_loc;     // 함수 정의 위치
        uint64_t code_start;           // 바이너리 내 시작 주소
        uint64_t code_end;
        std::vector<LocalVar> locals;  // 로컬 변수 + 스택 위치
    };

    struct LocalVar {
        std::string_view name;
        TypeId type;
        int32_t stack_offset;          // [rbp + offset]
        uint32_t scope_start_line;
        uint32_t scope_end_line;
    };

    // instruction address → source location 매핑
    struct SourceMapping {
        uint64_t addr;
        uint32_t line;
        uint32_t column;
        std::string_view file;
    };

    std::vector<FunctionInfo> functions;
    std::vector<SourceMapping> source_map;  // sorted by addr
};

// include/kern/debug/DebugInfoBuilder.h
class DebugInfoBuilder {
public:
    // MachIR + CompilationContext → DebugInfo
    // 컴파일 시 -g 플래그로 활성화
    DebugInfo build(const MachFunction* fns, uint32_t fn_count,
                    CompilationContext& ctx);

    // .kern_debug 섹션으로 직렬화 (NASM .data 또는 별도 파일)
    void serialize(const DebugInfo& info, std::ostream& out);
    DebugInfo deserialize(std::istream& in);
};
```

```cpp
// tools/kern-dbg/Debugger.h
class Debugger {
    DebugInfo debug_info_;         // 바이너리에서 로드
    ProcessController process_;    // 대상 프로세스 제어

public:
    // 바이너리 로드 + 디버그 정보 파싱
    bool load(const std::string& binary_path);

    // 브레이크포인트 (소스 레벨)
    bool setBreakpoint(std::string_view file, uint32_t line);
    bool removeBreakpoint(std::string_view file, uint32_t line);

    // 실행 제어
    void run();
    void stepLine();       // source-level step
    void stepInstruction(); // asm-level step
    void continueExec();
    void stop();

    // 상태 조회
    SourceLocation currentLocation() const;
    std::vector<StackFrame> backtrace() const;
    std::string inspectVariable(std::string_view name) const;

    struct StackFrame {
        std::string_view function_name;
        SourceLocation loc;
        std::vector<std::pair<std::string_view, std::string>> locals;
    };
};

// tools/kern-dbg/ProcessController.h
class ProcessController {
public:
    // macOS: Mach ports + ptrace
    bool spawn(const std::string& binary_path);
    bool attach(pid_t pid);
    void resume();
    void suspend();
    int waitForStop();  // 브레이크포인트/시그널까지 대기

    // 메모리 읽기 (변수 값 조회용)
    bool readMemory(uint64_t addr, void* buf, size_t size);
    uint64_t readRegister(PhysReg reg);
    uint64_t getInstructionPointer();
};
```

### 10.4 포매터 (`lib/Fmt/` + `tools/kern-fmt/`)

```cpp
// include/kern/fmt/Formatter.h
class Formatter {
public:
    // AST → 포맷된 소스 텍스트
    std::string format(const Module* ast, const FormatStyle& style);

    // 부분 포맷 (커서 범위만)
    std::string formatRange(const Module* ast, const FormatStyle& style,
                            uint32_t start_line, uint32_t end_line);
};

// include/kern/fmt/FormatStyle.h
struct FormatStyle {
    uint32_t indent_width = 4;
    bool use_tabs = false;
    uint32_t max_line_length = 100;
    enum BraceStyle { KnR, Allman } brace_style = KnR;
    bool trailing_commas = true;
    // kern 특화: pipe 체인 들여쓰기, match arm 정렬 등
    bool align_pipe_chains = true;
    bool align_match_arms = true;
};
```

**핵심**: Formatter는 **Parser까지만 의존**. 타입 정보 없이 AST만으로 포맷.

### 10.5 린터 (`lib/Lint/` + `tools/kern-lint/`)

```cpp
// include/kern/lint/LintPass.h
class LintPass {
public:
    virtual ~LintPass() = default;
    virtual std::string_view name() const = 0;

    // HIR 수준에서 분석 (타입 정보 접근 가능)
    virtual void run(const HIRModule& module,
                     CompilationContext& ctx,
                     LintReporter& reporter) = 0;
};

// include/kern/lint/LintEngine.h
class LintEngine {
    std::vector<std::unique_ptr<LintPass>> passes_;
public:
    template<typename T, typename... Args>
    void addRule(Args&&... args);

    void run(const HIRModule& module, CompilationContext& ctx);

    struct LintDiagnostic {
        enum Severity { Warning, Error, Style };
        Severity severity;
        std::string_view rule_name;
        std::string message;
        SourceLocation loc;
        std::optional<std::string> fix_suggestion;  // auto-fix
    };
};
```

**초기 린트 규칙**:
| 규칙 | 설명 | HIR 접근 필요 |
|------|------|--------------|
| UnusedVariable | 사용되지 않는 val/var 경고 | O (HIR 스코프 분석) |
| PurityViolation | pure 함수에서 impure 호출 경고 | O (Purity 정보) |
| ShadowedBinding | 같은 이름의 바인딩이 상위 스코프 가림 | O (HIR 스코프) |
| UnreachableCode | match/if의 도달 불가 분기 | O (HIR 제어 흐름) |
| WildcardOnly | match에 wildcard만 있으면 의미 없음 | △ (AST로도 가능) |

### 10.6 REPL (`tools/kern-repl/`)

```cpp
// tools/kern-repl/REPLEngine.h
class REPLEngine {
    CompilerPipeline pipeline_;  // 전체 파이프라인 사용
    // 누적된 선언들 (fn, struct, enum 등)
    std::vector<std::string> declarations_;

public:
    struct EvalResult {
        int64_t exit_code;          // 표현식 결과값
        std::string type_info;      // 결과 타입
        std::string error;          // 에러 시
        bool is_declaration;        // fn/struct 선언 (실행 없음)
    };

    // 한 줄 입력 → 컴파일 + 실행 + 결과 반환
    EvalResult eval(std::string_view input);

private:
    // 입력을 main() 안에 래핑하여 컴파일
    std::string wrapAsProgram(std::string_view expr);
};
```

### 10.7 패키지 매니저 (`tools/kern-pkg/`) — 미래

```toml
# kern.toml 매니페스트 예시
[package]
name = "my-kernel"
version = "0.1.0"
kern-version = "0.5"

[dependencies]
kern-std = "0.1"

[build]
target = "x86_64-macos"
```

```cpp
// tools/kern-pkg/Manifest.h
struct Manifest {
    std::string name;
    std::string version;
    struct Dependency {
        std::string name;
        std::string version_req;
    };
    std::vector<Dependency> dependencies;
};
```

### 10.8 런타임 라이브러리 (`runtime/`)

```
runtime/
├── kern_rt.asm          — 기본 런타임 (현재의 _start 래퍼 → 여기로 이동)
├── allocator.asm        — 미래: 힙 할당자 (M5c Ptr<T>용)
└── panic.asm            — 미래: 패닉 핸들러
```

현재 `main.cpp`에 하드코딩된 `_start` 래퍼 코드를 `runtime/`으로 분리. `kern_rt.o`로 빌드하여 링크.

---

## 12. 에이전트 개발환경 (v2)

> v2 아키텍처는 컴포넌트 수가 6개 → 16개+로 증가한다.
> 에이전트가 이 복잡한 구조에서 효율적으로 작업하려면
> CLAUDE.md, rules, hooks, skills 전부 v2에 맞게 재설계해야 한다.

### 12.1 CLAUDE.md 업데이트 계획

현재 CLAUDE.md는 ~50줄로 v1 파이프라인 기준. v2에서의 핵심 변경:

- **Pipeline 섹션**: 6-stage → 4-level IR 표기
- **Library Architecture 추가**: 의존성 그래프 ASCII (에이전트가 스코프 파악)
- **Directory Structure**: 16개 모듈 + 6개 도구 반영
- **Agent Scope**: "MUST maintain 98% line coverage on changed files" 추가
- **Commit Format**: scope 필드 추가 (support|lexer|parser|hir|lir|backend|ide|debug|fmt|lint|pipeline|lsp|dbg|repl|pkg)
- **Rules 참조**: architecture-v2.md, test-policy.md, layer-boundaries.md

### 12.2 Rules 파일 재설계

현재 2개 → v2에서 4개:

| 파일 | 로드 조건 | 역할 |
|------|----------|------|
| `architecture-v2.md` | 매 세션 | 4-level 파이프라인 불변식, TypeId/StringPool/Arena 규칙, 순방향 의존 |
| `cpp-style.md` | *.cpp/*.h 작업 시 | 기존 유지 (변경 없음) |
| `test-policy.md` (NEW) | tests/ 또는 lib/ 작업 시 | 커버리지 98%, E2E 앵커 불변, 테스트 명명 규칙 |
| `layer-boundaries.md` (NEW) | lib/ 작업 시 | 레이어별 수정 가능 파일 범위 + 금지 의존 목록 |

#### `rules/architecture-v2.md` 핵심 내용
```
- 4-level: AST → HIR → LIR → MachIR → NASM
- AST에서 디슈가링 금지. 디슈가링은 HIRBuilder.
- HIR: 이름 기반 변수, TypeId 내장, 아직 SSA 아님
- LIR: SSA with block arguments (no phi), VReg 기반
- MachIR: x86-64 전용, physical register
- TypeId (uint32_t): Support 레이어. 모든 IR이 공유.
- 역방향 include 발견 시 즉시 수정
```

#### `rules/test-policy.md` 핵심 내용
```
- lib/ line coverage 98% 이상
- 새 코드는 대응 테스트와 함께 커밋
- error path 테스트 필수
- E2E 앵커 (exit/error) = 불변, dump = 가변
```

#### `rules/layer-boundaries.md` 핵심 내용
```
각 레이어: 수정 가능 파일, 허용 의존, 금지 의존 명시
예) kern_ide → 의존 허용: support, lexer, parser, hir
              의존 금지: lir, backend (절대 include 금지)
```

### 12.3 Hooks 재설계

현재 2개 → v2에서 5개:

| Hook | 트리거 | 역할 |
|------|--------|------|
| **protect-docs** (기존) | PreToolUse(Edit\|Write) | docs/, README.md 수정 차단 |
| **test-gate** (기존) | Stop | 테스트 미실행 시 종료 차단 |
| **layer-guard** (NEW) | PreToolUse(Edit\|Write) | 레이어 역방향 의존 #include 탐지 → 차단 |
| **anchor-protect** (NEW) | PreToolUse(Edit\|Write) | E2E 앵커 .expected 수정 차단 (dump/ 제외) |
| **coverage-check** (NEW) | PostToolUse(Bash) | 커버리지 빌드 후 98% 미달 시 경고 |

#### `hooks/layer-guard.sh` — 역방향 의존 자동 차단
```bash
#!/bin/bash
# 편집 내용에서 금지된 #include 패턴 탐지
input=$(cat)
file_path=$(echo "$input" | jq -r '.tool_input.file_path // empty')
new_content=$(echo "$input" | jq -r '.tool_input.new_string // .tool_input.content // empty')

case "$file_path" in
    */lib/LIR/*|*/include/kern/lir/*)
        echo "$new_content" | grep -q '#include.*kern/backend/' && {
            echo "BLOCKED: LIR → Backend 역방향 의존" >&2; exit 2; } ;;
    */lib/IDE/*|*/include/kern/ide/*)
        echo "$new_content" | grep -q '#include.*kern/\(lir\|backend\)/' && {
            echo "BLOCKED: IDE → LIR/Backend 역방향 의존" >&2; exit 2; } ;;
    */lib/HIR/*|*/include/kern/hir/*)
        echo "$new_content" | grep -q '#include.*kern/\(lir\|backend\)/' && {
            echo "BLOCKED: HIR → LIR/Backend 역방향 의존" >&2; exit 2; } ;;
esac
exit 0
```

#### `hooks/anchor-protect.sh` — E2E 앵커 보호
```bash
#!/bin/bash
input=$(cat)
file_path=$(echo "$input" | jq -r '.tool_input.file_path // empty')
case "$file_path" in
    */tests/integration/*.expected)
        [[ "$file_path" != */tests/integration/dump/* ]] && {
            echo "BLOCKED: E2E anchor test. Only dump/ tests modifiable." >&2
            exit 2; } ;;
esac
exit 0
```

### 12.4 커스텀 스킬 설계

v2 컴파일러 개발에 필요한 전용 스킬:

#### 빌드 & 검증 스킬

| 스킬 | 설명 | 사용 시점 |
|------|------|----------|
| **`/kern:build`** | cmake + build + unit + E2E 한 번에 | 모든 코드 변경 후 |
| **`/kern:coverage`** | 커버리지 빌드 + 리포트 + 98% 게이트 체크 | Phase 완료 시, PR 전 |
| **`/kern:test <scope>`** | 특정 모듈만 빌드+테스트 (예: `hir`, `backend`) | 개발 중 빠른 피드백 |
| **`/kern:layer-check`** | 전체 소스의 역방향 #include 의존 스캔 | 리팩토링 후 |
| **`/kern:pipeline-trace <file>`** | .kern 파일을 AST→HIR→LIR→MachIR→NASM 단계별 덤프 | 디버깅, 이해 |

#### 코드 생성 스킬 (보일러플레이트 자동화)

| 스킬 | 생성물 | 연쇄 수정 |
|------|--------|----------|
| **`/kern:add-pass <name> --level hir\|lir`** | Pass.h + Pass.cpp + Test.cpp | CMakeLists + PassManager 등록 |
| **`/kern:add-lint <name>`** | LintPass.h + LintPass.cpp + Test.cpp | LintEngine 등록 |
| **`/kern:add-opcode <name>`** | LIROp enum + union variant | LIRDump + InstSel + Emitter + 테스트 (7파일 연쇄) |
| **`/kern:add-ast-node <name>`** | AST Expr/Stmt + 파이프라인 전체 | Parser→HIR→LIR→Backend→테스트 (10+파일 연쇄) |
| **`/kern:add-type <name>`** | TypeKind + TypeInfo variant | sizeOf/alignOf + HIR/LIR 지원 + 테스트 |
| **`/kern:add-tool <name>`** | tools/<name>/ 스캐폴딩 | CMakeLists + main.cpp + 의존 설정 |
| **`/kern:e2e-add <name>`** | .kern + .expected 쌍 | run_tests.sh 동작 확인 |

#### 스킬 상세: `/kern:add-opcode` (7파일 연쇄 수정)

```
입력: /kern:add-opcode CallIndirect
→ 에이전트가 순서대로 수정할 파일 체크리스트:

  1. include/kern/lir/LIR.h         → LIROp::CallIndirect 추가
  2. include/kern/lir/LIR.h         → LIRInstr union에 call_indirect 필드
  3. lib/LIR/LIRBuilder.cpp         → HIR CallIndirect → LIR lowering
  4. lib/LIR/LIRDump.cpp            → 텍스트 출력: "call_indirect %v0(%v1, %v2)"
  5. lib/Backend/InstructionSelector.cpp → LIR::CallIndirect → MachIR (call reg)
  6. lib/Backend/Emitter.cpp         → MachIR → "call rax" 등 NASM
  7. tests/unit/lir/LIRTest.cpp      → 새 opcode unit 테스트
  8. tests/unit/backend/InstSelTest.cpp → instruction selection 테스트
```

#### 스킬 상세: `/kern:add-ast-node` (10+파일 파이프라인 전체)

```
입력: /kern:add-ast-node LambdaExpr
→ 전체 파이프라인 가이드:

  AST:     AST.h (Kind+구조체) → Parser.cpp (파싱) → ASTDump.cpp (출력)
  HIR:     HIR.h (Kind+구조체) → HIRBuilder.cpp (변환) → HIRDump.cpp (출력)
  LIR:     (필요 시 새 opcode) → LIRBuilder.cpp (lowering) → LIRDump.cpp
  Backend: (필요 시) InstructionSelector → Emitter
  Tests:   각 레이어 unit 테스트 + E2E .kern/.expected
```

#### 스킬 상세: `/kern:pipeline-trace` (파이프라인 시각화)

```
입력: /kern:pipeline-trace tests/integration/fib.kern
출력:
  === Source ===
  fn fib(n: i64) -> i64 { ... }

  === AST (--dump-ast) ===
  Module { FnDecl "fib" ... }

  === HIR (--dump-hir) ===
  fn fib(n: i64) -> i64 [pure, tail_recursive] { match n ... }

  === LIR (--dump-lir) ===
  fn fib: block0(%v0:i64): %v1 = ICmpLe %v0, #1 ...

  === NASM (--dump-asm) ===
  _fib: cmp rdi, 1 ...
```

### 12.5 settings.json v2

```json
{
  "hooks": {
    "PreToolUse": [
      {
        "matcher": "Edit|Write",
        "hooks": [
          { "type": "command", "command": ".claude/hooks/protect-docs.sh" },
          { "type": "command", "command": ".claude/hooks/anchor-protect.sh" },
          { "type": "command", "command": ".claude/hooks/layer-guard.sh" }
        ]
      }
    ],
    "Stop": [
      {
        "hooks": [
          { "type": "command", "command": ".claude/hooks/test-gate.sh" }
        ]
      }
    ]
  }
}
```

### 12.6 에이전트 워크플로우 (v2)

```
┌──────────────────────────────────────────────────────┐
│  계획 = 사용자                                        │
│  /sc:brainstorm → /sc:design → .claude/plans/         │
├──────────────────────────────────────────────────────┤
│  구현 = 에이전트                                      │
│                                                       │
│  새 기능:  /kern:add-ast-node → /kern:add-opcode      │
│           → 구현 → /kern:build → /kern:coverage       │
│                                                       │
│  새 패스:  /kern:add-pass → 구현 → /kern:build        │
│                                                       │
│  새 린트:  /kern:add-lint → 구현 → /kern:build        │
│                                                       │
│  새 도구:  /kern:add-tool → 구현 → /kern:build        │
│                                                       │
│  디버깅:   /kern:pipeline-trace → 문제 파악            │
│           → 수정 → /kern:build                         │
│                                                       │
│  검증:     /kern:layer-check (역방향 의존 없음?)       │
│           → /kern:coverage (98%?)                      │
│           → /sc:git (커밋)                              │
├──────────────────────────────────────────────────────┤
│  가드레일 (자동 — hooks + rules)                      │
│  hook: protect-docs     — docs/ 수정 차단             │
│  hook: anchor-protect   — E2E 앵커 수정 차단          │
│  hook: layer-guard      — 역방향 의존 차단            │
│  hook: test-gate        — 테스트 미실행 종료 차단      │
│  rule: architecture-v2  — 4-level 파이프라인 불변식    │
│  rule: test-policy      — 커버리지 98% + 테스트 정책   │
│  rule: layer-boundaries — 레이어별 파일 스코프 제한    │
│  rule: cpp-style        — 코딩 컨벤션                  │
└──────────────────────────────────────────────────────┘
```

### 12.7 Phase별 스킬 작성 + 에이전트 호출 계획

> **원칙**: 스킬은 해당 Phase의 실제 작업이 시작되기 **직전**에 작성.
> 에이전트는 스킬을 호출하여 보일러플레이트를 생성한 뒤, 실제 로직을 구현.

#### Phase 0 — 기반 스킬 3개 + 환경 초안

```
┌─────────────────────────────────────────────────────────────┐
│ 작성할 스킬                                                   │
│                                                              │
│  /kern:build       전체 빌드 + unit + E2E 자동화              │
│  /kern:e2e-add     .kern + .expected 쌍 생성                  │
│  /kern:test        모듈 단위 빌드 + 테스트                     │
│                                                              │
│ 작성할 환경                                                   │
│                                                              │
│  CLAUDE.md v2 초안                                            │
│  rules/test-policy.md                                        │
│  coverage 인프라 (cmake/KernCoverage.cmake, scripts)          │
│  CI job 추가 (.github/workflows/ci.yml)                      │
├─────────────────────────────────────────────────────────────┤
│ 에이전트 작업 순서                                            │
│                                                              │
│  1. /kern:build 스킬 구현 (commands.json + run_all.sh)        │
│  2. /kern:e2e-add 스킬 구현 (template .kern/.expected)        │
│  3. /kern:test 스킬 구현 (scope 인자 → ctest --label-regex)   │
│  4. CLAUDE.md v2, rules, hooks, CI 설정 → /kern:build 검증    │
└─────────────────────────────────────────────────────────────┘
```

#### Phase 1 — 인프라 개발 시 스킬 사용

```
┌──────────────────────────────────────────────────────────────┐
│ 에이전트 A (1a)                    에이전트 B (1b)             │
│                                                               │
│ StringPool 구현                    TypeSystem 구현             │
│   └→ /kern:test support            └→ /kern:test support      │
│   └→ 구현 반복                      └→ 구현 반복               │
│   └→ /kern:build (전체)             └→ /kern:build (전체)      │
│                                                               │
│              합류 → 1c. CompilationContext 통합                │
│                   → /kern:build                                │
│                                                               │
│ 환경: rules/architecture-v2.md 초안 작성                       │
└──────────────────────────────────────────────────────────────┘
```

#### Phase 2 — `/kern:add-pass` 스킬 도입

```
┌──────────────────────────────────────────────────────────────┐
│ 스킬 작성 (2a/2b 시작 전)                                     │
│                                                               │
│  /kern:add-pass <name> --level hir|lir                        │
│    → Pass.h + Pass.cpp + PassTest.cpp 생성                     │
│    → CMakeLists.txt 자동 등록                                  │
│    → PassManager에 등록 코드 추가                               │
│                                                               │
│ 환경: rules/layer-boundaries.md 초안                           │
├──────────────────────────────────────────────────────────────┤
│ 에이전트 작업 (2c + 2d + 2e 병렬)                              │
│                                                               │
│ 에이전트 A (2c)           에이전트 B (2d)          에이전트 C (2e)  │
│                                                               │
│ /kern:add-pass            /kern:add-pass           /kern:add-pass  │
│   PurityAnalysis            TailCallAnalysis         Exhaustiveness │
│   --level hir               --level hir              --level hir    │
│     ↓                         ↓                        ↓            │
│ 패스 로직 구현             패스 로직 구현            패스 로직 구현   │
│     ↓                         ↓                        ↓            │
│ /kern:test hir             /kern:test hir           /kern:test hir  │
│     ↓                         ↓                        ↓            │
│ /kern:build                /kern:build              /kern:build     │
└──────────────────────────────────────────────────────────────┘
```

#### Phase 3 — `/kern:add-opcode` + `/kern:add-lint` 스킬 도입

```
┌──────────────────────────────────────────────────────────────┐
│ 스킬 작성 (3a/3c 시작 전)                                     │
│                                                               │
│  /kern:add-opcode <name>     (7파일 연쇄 수정 가이드)          │
│  /kern:add-lint <name>       (린트 규칙 보일러플레이트)         │
│                                                               │
│ 환경: hooks/layer-guard.sh 활성화 (LIR→Backend 차단)          │
├──────────────────────────────────────────────────────────────┤
│ 에이전트 A (3a)           에이전트 B (3b)         에이전트 C (3c) │
│                                                               │
│ LIR 구축:                 Formatter:              Linter:      │
│ /kern:add-opcode IConst   /kern:add-tool kern-fmt /kern:add-lint │
│ /kern:add-opcode IAdd       ↓                     UnusedVar    │
│ /kern:add-opcode ICmp     AST pretty-printer      /kern:add-lint │
│ ... (전체 opcode)           ↓                     UncheckedResult│
│   ↓                       /kern:test fmt          ... (전체 규칙) │
│ LIRBuilder 구현             ↓                       ↓           │
│   ↓                       /kern:build             /kern:test lint│
│ /kern:test lir                                      ↓           │
│   ↓                                               /kern:build   │
│ /kern:build                                                     │
└──────────────────────────────────────────────────────────────┘
```

#### Phase 4 — `/kern:pipeline-trace` + `/kern:layer-check` + `/kern:coverage`

```
┌──────────────────────────────────────────────────────────────┐
│ 스킬 작성 (Phase 4 시작 전)                                   │
│                                                               │
│  /kern:pipeline-trace <file>   (전체 IR 덤프 시각화)           │
│  /kern:layer-check             (역방향 #include 스캔)          │
│  /kern:coverage                (커버리지 빌드+리포트+98%게이트) │
├──────────────────────────────────────────────────────────────┤
│ 에이전트 A (4a)           에이전트 B (4b)         에이전트 C (4c) │
│                                                               │
│ Backend:                  Debug:                  IDE:         │
│ InstSel + RegAlloc          DebugInfoBuilder       IDEContext  │
│ + Emitter 구현               SourceMap              Completion │
│   ↓                          ValueInspector         Hover      │
│ /kern:pipeline-trace           ↓                    GoTo       │
│   fib.kern (검증)           /kern:test debug         ↓         │
│   ↓                           ↓                  /kern:test ide│
│ /kern:test backend          /kern:coverage          ↓          │
│   ↓                          (debug만)           /kern:build   │
│ /kern:build                   ↓                               │
│   ↓                        /kern:build                        │
│ /kern:layer-check                                             │
│   (전체 역방향 의존 확인)                                       │
└──────────────────────────────────────────────────────────────┘
```

#### Phase 5 — 최종 스킬 3개 + 환경 완성

```
┌──────────────────────────────────────────────────────────────┐
│ 스킬 작성 (5a 시작 전)                                        │
│                                                               │
│  /kern:add-ast-node <name>   (10+파일 파이프라인 전체 연쇄)    │
│  /kern:add-tool <name>       (도구 스캐폴딩)                   │
│  /kern:add-type <name>       (타입 파이프라인 전체 추가)        │
│                                                               │
│ 환경 완성:                                                    │
│  hooks/anchor-protect.sh 활성화                                │
│  CLAUDE.md v1→v2 최종 교체                                     │
│  rules/architecture.md v1 삭제 → v2 교체                       │
│  모든 스킬 12개 동작 검증                                      │
├──────────────────────────────────────────────────────────────┤
│ 5a. CompilerPipeline 통합 (순차 — 최우선)                     │
│                                                               │
│ main.cpp 교체 → /kern:build → 69개 앵커 E2E 통과 확인         │
│ 기존 sema/, ir/, codegen/ 삭제                                 │
│         ↓                                                     │
│ 5b + 5c + 5d 병렬 도구 어셈블리                                │
│                                                               │
│ 에이전트 A (5b)          에이전트 B (5c)        에이전트 C (5d)  │
│                                                               │
│ /kern:add-tool kern-lsp  /kern:add-tool kern-dbg /kern:add-tool │
│   ↓                        ↓                     kern-repl     │
│ IDE + fmt + lint          Debug + 프로세스           ↓          │
│ + LSP 프로토콜 구현        제어 + 스택워크 구현     Pipeline 전체 │
│   ↓                        ↓                     + 증분 평가    │
│ tests/tool/lsp/          tests/tool/dbg/           ↓           │
│   ↓                        ↓                     tests/tool/repl│
│ /kern:build              /kern:build              /kern:build   │
│         ↓                                                     │
│ 5e + 5f + 5g 병렬 테스트 보강                                  │
│                                                               │
│ /kern:e2e-add 반복 → 전체 테스트 커버리지 보강                  │
│         ↓                                                     │
│ /kern:coverage → 전체 lib/ 98% 달성 확인                       │
│ /kern:layer-check → 역방향 의존 전무 확인                      │
│ 모든 스킬 12개 최종 동작 검증                                   │
└──────────────────────────────────────────────────────────────┘
```

#### Phase 6 — 미래 도구

```
┌──────────────────────────────────────────────────────────────┐
│ 에이전트 A (6a)                    에이전트 B (6b)             │
│                                                               │
│ /kern:add-tool kern-pkg            /kern:add-type Result       │
│   ↓                                /kern:add-type Maybe        │
│ kern.toml 파서                       ↓                         │
│ 의존성 해석                        stdlib .kern 소스 작성       │
│ 빌드 오케스트레이션                    ↓                         │
│   ↓                                /kern:build                 │
│ /kern:test pkg                                                │
│   ↓                                                           │
│ /kern:build                                                   │
└──────────────────────────────────────────────────────────────┘
```

### 12.8 스킬 작성 순서 요약

> ✅ **12개 스킬 전부 Phase 0에서 선행 작성 완료.** 원래 계획은 Phase별 점진 작성이었으나, M5 완료 전에 한꺼번에 구현함.

| # | 스킬 | 파일 | 상태 |
|---|------|------|------|
| 1 | `/kern:build` | `.claude/commands/kern-build.md` | ✅ |
| 2 | `/kern:e2e-add` | `.claude/commands/kern-e2e-add.md` | ✅ |
| 3 | `/kern:test` | `.claude/commands/kern-test.md` | ✅ |
| 4 | `/kern:add-pass` | `.claude/commands/kern-add-pass.md` | ✅ |
| 5 | `/kern:add-opcode` | `.claude/commands/kern-add-opcode.md` | ✅ |
| 6 | `/kern:add-lint` | `.claude/commands/kern-add-lint.md` | ✅ |
| 7 | `/kern:pipeline-trace` | `.claude/commands/kern-pipeline-trace.md` | ✅ |
| 8 | `/kern:layer-check` | `.claude/commands/kern-layer-check.md` | ✅ |
| 9 | `/kern:coverage` | `.claude/commands/kern-coverage.md` | ✅ |
| 10 | `/kern:add-ast-node` | `.claude/commands/kern-add-ast-node.md` | ✅ |
| 11 | `/kern:add-tool` | `.claude/commands/kern-add-tool.md` | ✅ |
| 12 | `/kern:add-type` | `.claude/commands/kern-add-type.md` | ✅ |

### 12.9 에이전트 세션 진입 시 스킬 자동 안내

> CLAUDE.md v2에 다음을 포함하여, 에이전트가 세션 시작 시 사용 가능한 스킬을 인지:

```markdown
## Available Skills (현재 Phase에 따라 가용)

Phase 0+: /kern:build, /kern:e2e-add, /kern:test <scope>
Phase 2+: /kern:add-pass <name> --level hir|lir
Phase 3+: /kern:add-opcode <name>, /kern:add-lint <name>
Phase 4+: /kern:pipeline-trace <file>, /kern:layer-check, /kern:coverage
Phase 5+: /kern:add-ast-node <name>, /kern:add-tool <name>, /kern:add-type <name>

> 작업 전 스킬로 보일러플레이트를 생성하고, 로직만 구현하세요.
```

---

## 13. 다음 단계

### 완료된 단계
- [x] ~~Phase 0 (준비)~~ — CMake 모듈 3개, 스킬 12개, Rules 3개, Hooks 2개, Coverage 인프라, CI job
- [x] ~~Phase 1 (인프라)~~ — StringPool (13 tests), TypeSystem (23 tests), CompilationContext (6 tests)
- [x] ~~M5 전체 완료~~ — **517 unit + 113 E2E** (M5a struct + M5b enum/union + M5c ptr + M5d string + 에러 경로 커버리지)
- [x] ~~Phase 2 (HIR)~~ — HIR.h, HIRBuilder, HIRPasses (Purity/TailCall/Exhaustiveness), --dump-hir. **660 unit + 115 E2E** (commit f12364f)
- [x] ~~Phase 3 (LIR)~~ — LIR.h, LIRBuilder (HIR→LIR SSA lowering), LIRDump, --dump-lir. **695 unit + 115 E2E** (commit 713e664)
- [x] ~~Phase 4a (Backend)~~ — MachIR, InstructionSelector, RegisterAllocator, NASMEmitter, X86Backend, --dump-machir. **784 unit + 116 E2E** (commit 25573fb)

### 다음 단계
1. Phase 4b — kern_debug (DebugInfoBuilder, SourceMap, ValueInspector) — Support만 의존
2. Phase 4c — kern_ide (IDEContext, Completion, Hover, GoTo, References) — HIR만 의존
3. Phase 5 — 통합: CompilerPipeline으로 v2 파이프라인 전환, **69개 앵커 E2E 통과 확인**, v1 sema/ir/codegen 제거
4. Phase 5b/c — kern-lsp + kern-fmt 도구 어셈블리
5. Phase 6 — kern-pkg + stdlib/core
6. Phase 7 — kern-dbg + kern-repl (고난이도, 별도 마일스톤)
7. Phase 5 완료 시 **전체 lib/ line coverage 98% 달성 확인**
8. CLAUDE.md 최종 교체, 에이전트 환경 v2 완성
9. CI에 커버리지 게이트 영구 활성화 (98% 미달 = 빌드 실패)
