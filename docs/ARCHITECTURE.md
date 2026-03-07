# Kern Compiler — System Architecture

이 문서는 Kern 컴파일러를 처음부터 다시 구현할 수 있도록 설계 의도, 자료구조, 데이터 흐름을 기술한다.

## 목차

1. [개요](#1-개요)
2. [핵심 인프라 (Support)](#2-핵심-인프라)
3. [Lexer](#3-lexer)
4. [Parser / AST](#4-parser--ast)
5. [HIR (High-level IR)](#5-hir)
6. [LIR (Low-level IR)](#6-lir)
7. [Backend (MachIR → NASM)](#7-backend)
8. [Pipeline 오케스트레이션](#8-pipeline)
9. [모듈 시스템](#9-모듈-시스템)
10. [IDE / Tooling](#10-ide--tooling)
11. [핵심 불변식](#11-핵심-불변식)
12. [ABI 규약](#12-abi-규약)

---

## 1. 개요

Kern은 OS 커널 개발을 위한 순수 함수형 언어이다. 컴파일러는 C++20으로 작성되며, x86-64 macOS 네이티브 바이너리(NASM + ld)를 생성한다.

### 1.1 파이프라인 개관

```
Source
  → Lexer        (문자열 → Token 스트림)
  → Parser        (Token → AST, untyped)
  → HIRBuilder    (AST → HIR, typed + desugared, 타입검사 내장)
  → HIR Passes    (Purity, Effect, Ownership, TailCall, Lint)
  → LIRBuilder    (HIR → LIR, SSA + VReg + block arguments)
  → LIR Passes    (ConstFold, ConstProp, DCE, CSE, Inline, StrengthReduce, LICM, GlobalDCE)
  → ISel          (LIR → MachIR, x86-64 명령 선택)
  → RegAlloc      (VReg → PhysReg, linear scan)
  → Peephole      (물리 레지스터 수준 최적화)
  → NASMEmitter   (MachIR → NASM 어셈블리 텍스트)
  → nasm          (.asm → .o)
  → ld            (.o → 실행파일)
```

### 1.2 디렉토리 구조

| 경로 | 역할 |
|------|------|
| `include/kern/support/` | Arena, StringPool, TypeTable, DiagnosticEngine, CompilationContext |
| `include/kern/lexer/` | Token, Lexer |
| `include/kern/parser/` | AST 노드, Parser |
| `include/kern/hir/` | HIR 노드, HIRBuilder, HIR Passes, MonomorphizationPass |
| `include/kern/lir/` | LIR opcodes, LIRBuilder, LIR Passes |
| `include/kern/backend/` | MachIR, ISel, RegAlloc, Emitter, X86Backend, Peephole |
| `include/kern/pipeline/` | CompilerPipeline, CompileOptions |
| `include/kern/ide/` | Completion, Definition, Hover, References, Diagnostics, SemanticTokens |
| `include/kern/fmt/` | Formatter (AST → 소스 정렬) |
| `include/kern/debug/` | DebugInfo, SourceMap, ValueInspector |
| `lib/<Name>/` | 각 헤더에 대응하는 구현 |
| `tools/kernc/main.cpp` | 컴파일러 드라이버 |
| `tests/unit/` | GoogleTest 단위 테스트 |
| `tests/integration/` | `.kern` + `.expected` E2E 테스트 |

### 1.3 레이어 의존성 (엄격)

```
Support ← Lexer ← Parser ← HIR ← LIR ← Backend
                              ↑
                             IDE (Support, Lexer, Parser, HIR)
                              ↑
                           Debug (Support only)
                           Fmt   (Support, Lexer, Parser)
                           Pkg   (Support only)
```

Pipeline은 모든 레이어에 의존하는 오케스트레이터이다.

---

## 2. 핵심 인프라

모든 파이프라인 단계는 `CompilationContext` 하나를 공유한다.

```cpp
struct CompilationContext {
    Arena arena;             // 범프 포인터 할당자
    StringPool strings;      // 문자열 인터닝
    TypeTable types;         // 타입 레지스트리
    DiagnosticEngine diag;   // 에러/경고 수집
};
```

### 2.1 Arena

4096바이트 블록 단위 범프 포인터 할당자. AST/HIR/LIR/MachIR의 **모든** 노드는 Arena에 할당한다. 개별 해제 없음 — `CompilationContext` 소멸 시 전체 해제.

```cpp
class Arena {
    template<typename T, typename... Args>
    T* make(Args&&... args);       // 단일 객체 생성

    template<typename T>
    T* makeArray(size_t count);    // 배열 생성
};
```

**설계 의도**: 컴파일러 노드는 생성 후 변경이 거의 없고, 수명이 컴파일 단위와 동일하다. GC나 ref-counting 없이 최대 성능을 달성한다.

### 2.2 StringPool

Arena에 문자열을 인터닝한다. 동일 문자열 → 동일 `string_view` 포인터. 문자열 비교가 포인터 비교로 대체된다.

```cpp
class StringPool {
    StringPool(Arena& arena);
    std::string_view intern(std::string_view s);
    std::string_view intern(std::string_view a, std::string_view b);  // 연결 + 인터닝
};
```

### 2.3 TypeTable

모든 IR 단계에서 공유하는 타입 레지스트리. 타입은 `TypeId` (`uint32_t`) 핸들로 참조한다.

#### 기본 타입 (인덱스 0–15, 컴파일러 부팅 시 등록)

```
I8(0) I16(1) I32(2) I64(3) U8(4) U16(5) U32(6) U64(7)
F32(8) F64(9) Bool(10) Unit(11) Error(12) Never(13) Isize(14) Usize(15)
```

#### TypeKind

```cpp
enum class TypeKind : uint8_t {
    Primitive, Struct, Enum, Union, Ptr, PtrMut, Fn, Array, TypeVar, Never, DynTrait
};
```

#### TypeInfo 변형

| Kind | 데이터 | 주요 필드 |
|------|--------|----------|
| `Struct` | `StructData` | name, fields (`FieldInfo[]` + offsets + bitfield), size/align, packed/repr_c |
| `Enum` | `EnumData` | name, discriminant values/names, backing_size |
| `Union` | `UnionData` | name, variants + payload types, repr_c, tag_size |
| `Ptr/PtrMut` | `PtrData` | pointee TypeId, is_mutable |
| `Fn` | `FnData` | param TypeIds, return TypeId, EffectSet |
| `Array` | `ArrayData` | element TypeId, count |
| `DynTrait` | `DynTraitData` | trait name, method names (vtable slot 순서) |

#### 주요 API

```cpp
class TypeTable {
    TypeId add(TypeInfo info);
    const TypeInfo& get(TypeId id) const;       // const ref, 포인터 아님
    TypeId makePtr(TypeId pointee, bool mutable);
    TypeId makeFn(span<TypeId> params, TypeId ret, EffectSet effects);
    TypeId makeStruct(name, fields, packed, align, repr_c);
    TypeId makeOpaqueStruct(name);              // 자기참조 구조체용 전방선언
    void updateStruct(TypeId, fields, ...);     // opaque 채우기
    TypeId makeEnum(name, variants, values, backing_size);
    TypeId makeUnion(name, variants, repr_c, tag_size);
    TypeId makeArrayType(element, count);
    TypeId makeDynTrait(trait_name, method_names);

    uint32_t sizeOf(TypeId), alignOf(TypeId), bitWidth(TypeId);
    int32_t offsetOf(TypeId, field_name);
    bool isFloat(TypeId), isSigned(TypeId), isInteger(TypeId);
    IntRange intRange(TypeId);                  // {min, max}
};
```

**설계 의도**: `get()`이 포인터가 아닌 `const&`를 반환하는 이유 — null 체크 실수를 방지하고, 존재하지 않는 TypeId는 즉시 abort.

### 2.4 DiagnosticEngine

```cpp
enum class DiagLevel { Error, Warning, Note };

class DiagnosticEngine {
    void error(SourceLocation, std::string message);
    void warning(SourceLocation, std::string message);
    bool hasErrors() const;
    void printAll(std::ostream& out) const;     // 소스 위치 + 캐럿 출력
    void setSource(string_view source);         // 캐럿 출력용 원본 소스 설정
};
```

### 2.5 SourceLocation

```cpp
struct SourceLocation {
    uint32_t line = 1;
    uint32_t col = 1;
    std::string_view filename;
};
```

모든 Token, AST 노드, HIR 노드, LIR 명령, MachIR 명령이 보유한다.

### 2.6 이펙트 시스템

```cpp
enum class Effect : uint8_t { Mut, Mem, IO, Atomic };
using EffectSet = uint8_t;
// EFFECT_NONE=0, EFFECT_MUT=1, EFFECT_MEM=2, EFFECT_IO=4, EFFECT_ATOMIC=8
```

HIR 단계에서 추론/검증되며, LIR 이전에 소거된다.

---

## 3. Lexer

핸드라이튼, 제로카피. 원본 소스에 대한 `string_view`를 토큰에 보유한다.

### 3.1 Token

```cpp
struct Token {
    TokenKind kind;
    SourceLocation loc;
    std::string_view text;    // 원본 소스로의 string_view (제로카피)
};
```

#### TokenKind (주요 분류)

| 분류 | 예시 |
|------|------|
| 리터럴 | `IntLit`, `FloatLit`, `StringLit`, `CStringLit`, `FStringLit`, `CharLit` |
| 키워드 | `KwFn`, `KwVal`, `KwVar`, `KwMatch`, `KwReturn`, `KwIf`, `KwElse`, `KwAnd`, `KwOr`, `KwNot`, `KwStruct`, `KwEnum`, `KwUnion`, `KwLoop`, `KwAs`, `KwAsm`, `KwVolatile`, `KwModule`, `KwImport`, `KwTrait`, `KwImpl`, `KwPub`, `KwExtern`, `KwFor`, `KwWhile`, `KwDefer`, `KwWhere`, `KwDyn`, `KwOwn`, `KwConst`, `KwNull`, `KwUninit` 등 |
| 연산자 | `Plus(+)`, `PlusWrap(+%)`, `PlusSat(+\|)`, `Minus`, `Star`, `Slash`, `Percent`, `Pipe(\|>)`, `Arrow(->)`, `FatArrow(=>)`, `DotDot(..)`, `Question(?)` 등 |
| 구분자 | `LParen`, `RParen`, `LBrace`, `RBrace`, `LBracket`, `RBracket` |
| 특수 | `Newline`, `Eof`, `Error`, `Label('ident)` |

### 3.2 Lexer API

```cpp
class Lexer {
    Lexer(string_view source, string_view filename, DiagnosticEngine& diag);
    Token nextToken();

    // Parser lookahead 용 스냅샷
    struct Snapshot { ... };
    Snapshot save() const;
    void restore(const Snapshot& s);
};
```

**설계 결정**:
- 한 번에 하나의 토큰만 생성 (`nextToken()`) — 메모리 효율적
- `save()/restore()` — Parser의 백트래킹 지원 (예: 타입 파싱 vs 표현식 파싱 모호성)
- 줄바꿈이 토큰(`Newline`) — `*`, `/`, `&` 같은 이항 연산자가 줄 시작에 오면 Pratt 파서의 infix 루프 종료

---

## 4. Parser / AST

Recursive Descent + Pratt Parsing. AST는 전부 Arena 할당, tagged struct 방식.

### 4.1 TypeRef (소스 레벨 타입 참조)

타입 해결(resolution) 이전의 구문적 타입 표현.

```cpp
struct TypeRef {
    enum class Kind { Named, Ptr, Fn, Never, Array, ConstVal, Dyn, Tuple };
    Kind kind;
    string_view name;
    TypeRef* pointee;           // Ptr<T>
    bool is_ptr_var;            // Ptr<var T> (mutable pointer)
    TypeRef* array_element;
    uint32_t array_size;
    TypeRef* fn_params;         // Fn(A, B) -> C
    uint32_t fn_param_count;
    TypeRef* fn_return;
    TypeRef* type_args;         // 제네릭 타입 인자
    uint32_t type_arg_count;
    int64_t const_value;        // const generic value
};
```

### 4.2 패턴 (Pattern)

Match 표현식과 함수 레벨 패턴 매칭에 사용.

```
IntLitPattern(int64_t value)
BoolLitPattern(bool value)
WildcardPattern(_)
VariablePattern(name, optional type)
EnumPattern(variant_name)
UnionPattern(variant_name, inner payload or field bindings)
RangePattern(lo, hi, inclusive)
```

### 4.3 표현식 (Expr)

Tagged union. `Kind` 열거형으로 구분.

| Kind | 설명 | 핵심 필드 |
|------|------|----------|
| `IntLit` | 정수 리터럴 | value, suffix (u8, i32 등) |
| `FloatLit` | 부동소수점 | value, is_f32 |
| `BoolLit` | true/false | value |
| `StringLit` | 문자열 | text |
| `CStringLit` | C 문자열 (null-terminated) | text |
| `NullLit` | null 포인터 | — |
| `Ident` | 식별자 참조 | name |
| `BinOp` | 이항 연산 | op (Add/Sub/.../And/Or/BitAnd/.../Shl/Shr), lhs, rhs |
| `UnaryOp` | 단항 연산 | op (Neg/Not/BNot/Deref/AddrOf/AddrOfMut), operand |
| `Call` | 함수 호출 | callee name, args[], type_args[] |
| `ExprCall` | 간접 호출 (표현식을 통한) | callee Expr*, args[] |
| `Cast` | 타입 캐스트 | expr, target type |
| `If` | 조건 분기 | cond, then_branch, else_branch |
| `Block` | 블록 | stmts[], result expr |
| `Return` | 반환 | value |
| `Match` | 패턴 매칭 | scrutinee, arms[] (pattern + guard + body) |
| `StructLit` | 구조체 리터럴 | type_name, fields[] |
| `FieldAccess` | 필드 접근 | object, field_name |
| `EnumAccess` | 열거형 변형 접근 | type_name, variant_name |
| `UnionVariant` | 유니온 생성 | type_name, variant_name, value |
| `Loop` | 무한 루프 | bindings[], stmts[], result, label |
| `ForRange` | 범위 루프 | var_name, start, end, stmts[] |
| `WhileLoop` | 조건 루프 | condition, stmts[], label |
| `InlineAsm` | 인라인 어셈블리 | lines, outputs[], inputs[], clobbers[] |
| `ArrayLit` | 배열 리터럴 | elements[] |
| `ArrayRepeat` | 배열 반복 | element, count |
| `IndexAccess` | 배열 인덱싱 | array, index |
| `SliceExpr` | 슬라이스 | array, start, end |
| `Lambda` | 람다/클로저 | params[], return_type, body |
| `Try` | ? 연산자 | inner expr |
| `Sizeof/Alignof/Offsetof` | 타입 메타 | type or type+field |
| `ConstIf` | 컴파일 타임 분기 | cfg_key, cfg_value, then, else |
| `StringInterp` | f-string 보간 | parts[] |

### 4.4 문장 (Stmt)

```
ValDecl(name, type?, init)       — 불변 바인딩
VarDecl(name, type?, init)       — 가변 바인딩
ExprStmt(expr)                   — 표현식 문장
Assign(name, value)              — 변수 대입
FieldAssign(object, field, value)
DerefAssign(ptr_expr, value)     — 포인터 역참조 대입
IndexAssign(array, index, value)
TupleDestruct(names[], expr)     — 튜플 분해
Break(label?, value?)
Continue(label?)
Defer(body)                      — 지연 실행
```

### 4.5 선언 (Declaration)

#### FnDecl

```cpp
struct FnDecl {
    string_view name;
    Param* params;    uint32_t param_count;
    TypeRef* return_type;
    BlockExpr* body;  // nullptr for intrinsic/extern
    TypeParam* type_params;  // 제네릭
    WhereClause* where_clauses;

    // 어노테이션 플래그
    bool is_pub, is_extern, is_variadic;
    bool is_naked, is_interrupt, is_noreturn;
    bool is_weak, is_cold, is_hot;
    bool is_constructor, is_destructor;  // + priority
    bool is_no_mangle, is_panic_handler;
    bool is_hidden, is_protected, is_must_use;
    string_view link_name, section_name;
    string_view* effect_names;    // 선언된 이펙트
};
```

#### StructDecl

```cpp
struct StructDecl {
    string_view name;
    FieldDecl* fields;  // name, type, bitfield_width
    TypeParam* type_params;
    bool is_packed, is_repr_c, is_pub;
    uint32_t explicit_align;
};
```

#### EnumDecl, UnionDecl, TraitDecl, ImplDecl, GlobalDecl, ImportDecl 등

각각 대응하는 언어 구문을 표현하는 Arena 할당 구조체.

### 4.6 Module (최상위 컨테이너)

```cpp
struct Module {
    string_view module_name;
    ImportDecl** imports;       uint32_t import_count;
    FnDecl** functions;         uint32_t fn_count;
    StructDecl** structs;       uint32_t struct_count;
    EnumDecl** enums;           uint32_t enum_count;
    UnionDecl** unions;         uint32_t union_count;
    TraitDecl** traits;         uint32_t trait_count;
    ImplDecl** impls;           uint32_t impl_count;
    GlobalDecl** globals;       uint32_t global_count;
    TypeAliasDecl** type_aliases; ...
    NewtypeDecl** newtypes;     ...
    StaticAssertDecl** static_asserts; ...
};
```

### 4.7 Parser

```cpp
class Parser {
    Parser(Lexer& lexer, Arena& arena, DiagnosticEngine& diag);

    void setCfg(string_view key, string_view value);  // 조건부 컴파일
    void addKnownStruct(string_view name);  // 임포트된 타입 등록
    void addKnownEnum(string_view name);
    void addKnownUnion(string_view name);
    void copyNamesFrom(const Parser& other);

    Module* parseModule();
};
```

**Pratt Parsing**: `parseExpr(uint8_t minBP)` — `prefixBP()`와 `infixBP()`로 연산자 우선순위 제어.

**설계 결정**:
- `addKnownStruct/Enum/Union`: 모듈 간 컴파일 시, 임포트된 타입 이름을 Parser에 알려 타입 이름과 표현식을 구분
- Newline 토큰이 `*`, `/`, `&` 뒤에 오면 infix 루프 종료 — 줄바꿈이 문장 구분자 역할

---

## 5. HIR (High-level IR)

타입이 부여되고 탈설탕(desugar)된 중간 표현. 모든 노드에 `TypeId`가 있다.

### 5.1 AST → HIR 변환 (탈설탕)

| AST 구문 | HIR 변환 |
|----------|---------|
| 파이프 `\|>` | 일반 `Call` (인자 재배치) |
| 함수 레벨 패턴 매칭 | 단일 매개변수 + `Match` 표현식 |
| 람다/클로저 | 캡처 구조체 + 리프팅된 함수 + `Call` |
| `?` 연산자 | `Match Ok(v)=>v, Err(e)=>Return(Err(e))` |
| 제네릭 함수 | MonomorphizationPass에서 구체화 복제 |

### 5.2 HIR 노드

#### HIRExpr (Kind 열거형)

```
IntLit, FloatLit, BoolLit, StringLit, CStringLit,
Ident, BinOp, UnaryOp, Call, If, Match, Block, Return,
StructLit, FieldAccess, EnumAccess, UnionVariant,
AddrOf, Deref, Cast, Loop, Break, Continue,
ArrayLit, IndexAccess, InlineAsm,
FnRef,          // 함수 이름 → 함수 포인터
CallIndirect,   // 함수 포인터를 통한 간접 호출
```

모든 `HIRExpr`은 `TypeId type`과 `SourceLocation loc`을 보유한다.

#### HIRFnDecl

```cpp
struct HIRFnDecl {
    string_view name;
    HIRParam* params;   uint32_t param_count;
    TypeId return_type;
    HIRExpr* body;      // intrinsic/extern이면 nullptr

    uint8_t purity;     // Pure, ImpureMut, ImpureIo, ImpureMem
    EffectSet declared_effects, inferred_effects;

    bool is_recursive, is_tail_recursive;
    bool is_intrinsic, is_pub, is_extern;
    // ... 모든 어노테이션 플래그 미러링 ...
};
```

### 5.3 HIRBuilder

단일 패스 — `Module*` (AST)를 받아 `HIRModule*`을 생성한다. **타입검사를 인라인으로 수행**.

```cpp
class HIRBuilder {
    HIRBuilder(CompilationContext& ctx);
    HIRModule* build(const Module* ast);

    // 모듈 간 심볼 주입 (multi-file 컴파일용)
    void registerExports(const Module* ast, string_view module_path);
    void injectFnSig(string_view name, const vector<TypeId>& params, TypeId ret);
    void injectNamedType(string_view name, TypeId tid);
    void injectGenericStruct(string_view name, const StructDecl* decl);
    void injectGlobalType(string_view name, TypeId tid);
};
```

**내부 상태**:
- `fn_table_`: 함수 시그니처 맵 (name → param types + return type)
- `local_vars_`, `mutable_vars_`: 현재 스코프의 바인딩
- 제네릭 템플릿 레지스트리 (struct, union, type alias, fn)
- 트레이트/impl 테이블, vtable 추적
- 클로저 캡처 추적 + 람다 리프팅 누적기

**설계 의도**: v1에서는 별도 TypeChecker를 두었으나, AST→HIR 변환과 타입검사를 분리하면 노드를 두 번 순회해야 한다. v2에서는 HIRBuilder가 타입검사를 인라인 수행하여 단일 패스로 통합.

### 5.4 MonomorphizationPass

제네릭 함수를 구체 타입별로 복제하고, 호출 사이트를 맹글링된 특화 이름으로 교체.

3단계: 제네릭 수집 → 특화 (deep copy + 타입 치환) → 호출 사이트 패치.

### 5.5 HIR Passes

모든 패스는 `HIRPass` 인터페이스 구현:

```cpp
class HIRPass {
    virtual string_view name() const = 0;
    virtual void run(HIRModule& module, CompilationContext& ctx) = 0;
};
```

실행 순서:

| # | 패스 | 역할 |
|---|------|------|
| 1 | `PurityAnalysisPass` | 함수 순수성 분석 (Pure/ImpureMut/ImpureMem/ImpureIo), 호출자로 전파 |
| 2 | `EffectAnalysisPass` | 이펙트 추론, 선언과 비교 검증, 호출 시 이펙트 검사 |
| 3 | `OwnershipCheckPass` | use-after-move (own 파라미터), borrow 탈출, 동시 mutable borrow |
| 4 | `TailCallAnalysisPass` | 꼬리 호출 마킹, is_recursive/is_tail_recursive 설정 |
| 5 | `ConstOverflowPass` | 상수 정수 오버플로 감지 |
| 6 | `LossyCastPass` | 축소 캐스트 경고 (explicit truncate 없이) |
| 7 | `BorrowEscapePass` | 로컬 참조 반환 감지 |
| 8 | `MutBorrowAliasPass` | 같은 변수의 mutable borrow가 여러 파라미터에 전달되는 경우 감지 |
| 9 | `UnusedBindingPass` | 미사용 val/var 경고 (`_` 접두사 면제) |
| 10 | `MustUseCheckPass` | `@must_use` 반환값 무시 경고 |

---

## 6. LIR (Low-level IR)

SSA 형식. 블록 인자(block arguments)로 phi 노드를 대체한다.

### 6.1 핵심 타입

```cpp
using VReg = uint32_t;
constexpr VReg INVALID_VREG = UINT32_MAX;

enum class MemOrder : uint8_t { Relaxed, Acquire, Release, AcqRel, SeqCst };
```

### 6.2 LIR Opcodes

| 분류 | Opcodes |
|------|---------|
| 상수 | `ConstInt`, `ConstFloat`, `ConstBool`, `ConstString`, `ConstCString`, `GlobalRef` |
| 정수 산술 | `Add`, `Sub`, `Mul`, `Div`, `Mod` |
| 래핑/포화 산술 | `AddWrap`, `SubWrap`, `MulWrap`, `AddSat`, `SubSat` |
| 비트 연산 | `BAnd`, `BOr`, `BXor`, `Shl`, `Shr` |
| 부동소수점 | `FAdd`, `FSub`, `FMul`, `FDiv`, `FNeg` |
| 정수 비교 | `ICmpEq`, `ICmpNe`, `ICmpLt`, `ICmpLe`, `ICmpGt`, `ICmpGe` |
| 부동소수점 비교 | `FCmpEq`, `FCmpNe`, `FCmpLt`, `FCmpLe`, `FCmpGt`, `FCmpGe` |
| 단항 | `Neg`, `Not`, `BNot` |
| 캐스트 | `Cast` |
| 메모리 | `AddrOf`, `Load`, `Store`, `FieldPtr`, `StructAlloc`, `Alloca` |
| 제어 흐름 | `Branch`, `CondBranch`, `Ret`, `Switch` |
| 호출 | `Call`, `CallIndirect`, `FnRef` |
| 블록 인자 | `BlockArg` (인덱스로 블록 파라미터 로드 — phi 대체) |
| 원자적 | `AtomicLoad`, `AtomicStore`, `AtomicCas`, `AtomicFetchAdd/Sub/And/Or/Xor`, `AtomicCas128` |
| 펜스 | `Fence`, `CompilerFence` |
| Per-CPU | `PercpuLoad`, `PercpuStore` |
| 전역 | `LoadGlobal`, `StoreGlobal` |
| 비트 조작 | `Clz`, `Ctz`, `Popcnt`, `Bswap` |
| I/O | `PortIn`, `PortOut` |
| 기타 | `InlineAsm`, `Trap` (ud2), `VaStart`, `VaArg`, `TlsLoad`, `TlsStore` |

### 6.3 LIR 명령 구조

```cpp
struct LIRInstr {
    LIROp op;
    VReg result;
    TypeId type;
    SourceLocation loc;

    // 페이로드 (tagged union)
    union {
        LIRConstInt const_int;          // {int64_t value}
        LIRBinPayload bin;              // {VReg lhs, rhs}
        LIRCallPayload call;            // {callee, callee_module, VReg* args, arg_count, is_tail}
        LIRBranchPayload branch;        // {target_block, VReg* args, arg_count}
        LIRCondBrPayload cond_br;       // {cond VReg, true_target, false_target, hint}
        LIRSwitchPayload switch_;       // {scrutinee, default_block, cases[], min/max}
        LIRAtomicCasPayload atomic_cas; // {ptr, expected, desired, MemOrder}
        // ... 기타 ...
    };
};
```

### 6.4 블록과 함수

```cpp
struct LIRBlock {
    string_view label;
    TypeId* param_types;    // 블록 파라미터 타입 (phi 대체)
    uint32_t param_count;
    LIRInstr* instrs;
    uint32_t instr_count;
};

struct LIRFunction {
    string_view name;
    TypeId* param_types;    uint32_t param_count;
    TypeId return_type;
    LIRBlock* blocks;       uint32_t block_count;
    VReg next_vreg;         // 다음 할당할 VReg 번호
    // ... 메타데이터 플래그들 ...
};
```

### 6.5 GlobalData (전역 데이터)

```cpp
struct GlobalData {
    enum Kind : uint8_t { StringLit, FloatConst, Variable, VTable };
    Kind kind;
    string_view label;

    // Variable: init_value, size, is_mutable, init_bytes[], relocs[]
    // VTable: method_labels[], method_count, self_size
    // StringLit: text
    // FloatConst: double_value, is_f32
};
```

### 6.6 LIRBuilder

```cpp
class LIRBuilder {
    LIRBuilder(CompilationContext& ctx);
    LIRModule* build(const HIRModule* hir);
    void setBoundsCheck(bool v);
};
```

**SSA 구축 전략**:
- `val` 바인딩: 이름 → VReg 직접 매핑 (`locals_`)
- `var` 바인딩: 스택 할당 주소 VReg → Load로 읽기, Store로 쓰기 (`var_addrs_`)
- 블록 파라미터: 제어 흐름 합류점에서 값 전달 (phi 대체)

### 6.7 LIR Passes

| # | 패스 | 역할 |
|---|------|------|
| 1 | `ConstFoldPass` | 상수 산술을 컴파일 타임에 폴딩 |
| 2 | `ConstPropPass` | 알려진 상수를 VReg을 통해 전파 |
| 3 | `DeadCodeElimPass` | 결과가 사용되지 않는 명령 제거 |
| 4 | `CSEPass` | 베이직 블록 내 공통 부분식 제거 |
| 5 | `InliningPass` | 작은 함수 인라이닝 |
| 6 | `StrengthReductionPass` | 2의 거듭제곱 나눗셈/나머지 → 시프트/AND |
| 7 | `LICMPass` | 루프 불변 코드 이동 |
| 8 | `GlobalDCEPass` | 호출되지 않고 외부 가시성 없는 함수 제거 |

---

## 7. Backend (MachIR → NASM)

### 7.1 물리 레지스터

```cpp
enum class PhysReg : uint8_t {
    RAX, RBX, RCX, RDX, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15,
    XMM0, XMM1, ..., XMM15,
    RSP, RBP, NONE
};
```

### 7.2 MachIR 구조

#### MachOperand (tagged union)

```cpp
struct MachOperand {
    enum { Reg, Imm, Stack, Label, None } kind;

    // Reg: vreg (가상) 또는 phys (물리), is_physical 플래그
    // Imm: int64_t 즉치값
    // Stack: int32_t RBP 오프셋
    // Label: string_view 레이블

    static MachOperand virt(uint32_t vreg);
    static MachOperand precolored(PhysReg reg);   // ISel이 강제 할당
    static MachOperand physical(PhysReg reg);      // RegAlloc 이후
    static MachOperand immediate(int64_t val);
    static MachOperand stack(int32_t offset);
    static MachOperand lbl(string_view label);
};
```

#### X86Op (x86-64 명령 opcode)

| 분류 | Opcodes |
|------|---------|
| 데이터 이동 | `Mov`, `MovZX`, `MovSX`, `MovLoad`, `MovStore`, `Lea`, `Push`, `Pop` |
| 정수 산술 | `Add`, `Sub`, `IMul`, `IDiv`, `Xor`, `And`, `Or`, `Shl`, `Shr`, `Sar`, `Neg`, `Not`, `Cqo` |
| 비교/조건 | `Cmp`, `Test`, `Setcc`, `Cmovcc`, `Jmp`, `Jcc`, `Call`, `Ret` |
| SSE 부동소수점 | `Movss/Movsd`, `FloatLoad/FloatStore`, `Addss/sd`, `Subss/sd`, `Mulss/sd`, `Divss/sd`, `Ucomisd/ss`, `Xorps/pd` |
| 변환 | `Cvttsd2si`, `Cvttss2si`, `Cvtsi2sd`, `Cvtsi2ss`, `Cvtsd2ss`, `Cvtss2sd` |
| Pseudo | `Pseudo_ParallelMove`, `Pseudo_FrameSetup`, `Pseudo_FrameDestroy` |
| 원자적 | `LockCmpxchg`, `LockXadd`, `LockCmpxchg16b`, `Xchg`, `Mfence/Sfence/Lfence` |
| Per-CPU/TLS | `GsLoad/GsStore`, `FsLoad/FsStore` |
| 전역 | `MovLoadGlobal`, `MovStoreGlobal`, `LeaGlobal` |
| 비트 | `Bsf`, `Bsr`, `Popcnt`, `Bswap` |
| I/O | `In`, `Out` |
| 기타 | `Ud2` (trap), `JmpTable`, `InlineAsm`, `Nop` |

#### CondCode

```
E(==), NE(!=), L(<), LE(<=), G(>), GE(>=)         // 부호 있는
B(<), BE(<=), A(>), AE(>=)                          // 부호 없는
O(overflow), NO(no overflow)
```

#### MachInstr

```cpp
struct MachInstr {
    X86Op op;
    CondCode cc;
    uint8_t width;          // 8/16/32/64
    uint8_t operand_count;
    bool is_volatile;
    int8_t branch_hint;     // +1=likely, -1=unlikely, 0=none

    union {
        MachOperand inline_ops[4];   // 4개 이하면 힙 할당 회피
        MachOperand* heap_ops;       // 5개 이상
    };

    SourceLocation loc;
};
```

### 7.3 Instruction Selector (ISel)

LIR → MachIR. 각 `LIROp`를 하나 이상의 `X86Op` 시퀀스로 변환.

```cpp
class InstructionSelector {
    InstructionSelector(CompilationContext& ctx, OutputFormat fmt);
    MachModule* select(const LIRModule& lir_mod);
};
```

**주요 처리**:
- **System V ABI 호출 규약**: GPR 6개 (RDI, RSI, RDX, RCX, R8, R9), XMM 8개 (XMM0–7)
- **구조체 전달**: ≤8B → 1 GPR, 9–16B → 2 GPR, >16B → 스택
- **>16B 구조체 반환**: RDI에 숨겨진 포인터 (hidden return pointer)
- **병렬 이동 (Pseudo_ParallelMove)**: 호출 인자 레지스터 충돌 시 순환 해소 (RAX 스크래치)
- **Jump Table**: `Switch` → `JmpTable` 명령 + 점프 테이블 데이터
- **Precoloring**: 특정 연산에 특정 레지스터 강제 (예: AtomicCas → RAX)

### 7.4 Register Allocator

Linear scan 할당. 3단계:

```
1. computeIntervals() — 라이브 인터벌 계산 (전역 명령 번호 부여)
2. allocate()         — 레지스터 할당 (GPR/XMM 분리, 스필/리로드)
3. rewrite()          — VReg → PhysReg 치환, 프롤로그/에필로그 삽입
```

```cpp
struct LiveInterval {
    uint32_t vreg;
    uint32_t start, end;     // 전역 명령 인덱스
    PhysReg hint;            // precolored 힌트
    bool is_fixed, is_float;
};

struct RegAllocation {
    unordered_map<uint32_t, PhysReg> reg_map;     // vreg → phys
    unordered_map<uint32_t, int32_t> spill_map;   // vreg → stack offset
    uint32_t stack_size;
    bool callee_saved_used[5];  // RBX, R12, R13, R14, R15
};
```

**설계 결정**:
- Precolored VReg은 `is_fixed=true`, 해당 PhysReg에 고정
- XMM과 GPR은 별도 풀에서 할당
- Callee-saved 레지스터 사용 시 자동으로 push/pop 삽입

### 7.5 Peephole Optimizer

레지스터 할당 후 물리 명령 수준에서 최적화.

```cpp
void peepholeOptimize(MachModule& mod);
```

### 7.6 NASM Emitter

MachModule + LIRModule(전역 데이터) → NASM 어셈블리 텍스트.

```cpp
class NASMEmitter {
    NASMEmitter(ostream& out, OutputFormat fmt);

    void emitModule(const MachModule& mod, const LIRModule& lir_mod, bool freestanding);

    const char* symPrefix() const;  // Mach-O: "_", ELF: ""
};
```

**출력 구조**:
1. `extern` 선언 (intrinsic, extern, 크로스 모듈)
2. `global` 선언 (pub 함수, weak 심볼)
3. `.rodata` (문자열 리터럴, float 상수, 전역 변수, VTable)
4. `.data` (스택 프로텍터 가드)
5. `.text` (함수 코드)
6. `_start` 래퍼 (freestanding이 아닌 경우)
7. dyn dispatch thunks (VTable 호출 트램펄린)
8. `.eh_frame` (스택 언와인딩)
9. `.init_array` / `.fini_array` (생성자/소멸자)

**병렬 이동 발출**: `Pseudo_ParallelMove`를 순환 분석하여 2-cycle은 `xchg`, 3+cycle은 R11 스크래치 레지스터 사용.

### 7.7 X86Backend (오케스트레이터)

```cpp
class X86Backend : public TargetBackend {
    void emit(const LIRModule& lir, ostream& out) override;
    // 내부: ISel → RegAlloc(per fn) → Peephole → NASMEmitter
};
```

---

## 8. Pipeline

### 8.1 CompileOptions

```cpp
struct CompileOptions {
    string input_file;
    vector<string> input_files;       // 멀티 파일
    string output_file = "a.out";
    OutputFormat format = OutputFormat::Macho64;

    bool asm_only;         // -S (어셈블리만 출력)
    bool compile_only;     // -c (오브젝트만 출력)
    bool freestanding;     // OS 없이 동작
    bool bounds_check;     // 배열 경계 검사
    bool stack_protector;  // 스택 보호
    bool incremental;      // 증분 컴파일
    bool pie, shared, relocatable;

    vector<pair<string,string>> cfg_flags;  // 조건부 컴파일
    vector<string> lib_paths, lib_names;
    vector<string> module_paths;            // 모듈 검색 경로
    vector<string> include_paths;           // @include 검색 경로

    // 디버그/덤프
    bool dump_tokens, dump_ast, dump_hir, dump_lir, dump_machir;
    bool debug_info, debug_locs;
};
```

### 8.2 CompilerPipeline

```cpp
class CompilerPipeline {
    CompilerPipeline(CompilationContext& ctx);

    // 단일 파일 컴파일 + 링크
    int run(const string& source, const CompileOptions& opts,
            ostream& out, ostream& err);

    // 단일 파일 → .o
    int compileToObject(const string& source, const CompileOptions& opts,
                        ostream& out, ostream& err);

    // .o 파일들만 링크
    int linkObjects(const CompileOptions& opts, ostream& out, ostream& err);

    // 멀티 파일 (각각 독립 컴파일 후 링크)
    int runMultiFile(const CompileOptions& opts, ostream& out, ostream& err);

    // 모듈 시스템 (의존성 DAG 기반)
    int runModular(const CompileOptions& opts, ostream& out, ostream& err);

    // @include 전처리
    static string preprocessIncludes(const string& source, const string& base_dir,
                                     const vector<string>& include_paths,
                                     ostream& err, bool& ok);
};
```

### 8.3 컴파일 흐름 (단일 파일)

```
1. preprocessIncludes()    — @include("path") 텍스트 확장
2. Lexer → Parser          — Token 스트림 → Module* (AST)
3. HIRBuilder::build()     — AST → HIRModule* (타입검사 포함)
4. MonomorphizationPass    — 제네릭 특화
5. HIRPassManager::run()   — 분석 + 린트 패스
6. LIRBuilder::build()     — HIR → LIRModule* (SSA)
7. LIRPassManager::run()   — 최적화 패스
8. X86Backend::emit()      — LIR → MachIR → NASM 텍스트
9. assemble()              — nasm -f macho64 → .o
10. link()                 — ld → 실행파일
```

### 8.4 외부 도구 호출

| 단계 | 명령 |
|------|------|
| 어셈블 | `nasm -f macho64 [-g -F dwarf] input.asm -o output.o` |
| 링크 (일반) | `ld obj.o -o a.out -e _start -platform_version macos 14.0.0 14.0.0 -arch x86_64 -lSystem` |
| 링크 (freestanding) | `ld obj.o -o a.out -e _main -platform_version macos 14.0.0 14.0.0 -arch x86_64 -static` |
| 링크 (ELF) | `ld obj.o -o a.out -e _start [-T linker.ld] [-lc]` |

---

## 9. 모듈 시스템

### 9.1 ModuleResolver

```cpp
class ModuleResolver {
    void addSearchPath(string_view path);
    bool resolve(const string& module_path, string& out_file);
    bool buildDependencyGraph(const vector<string>& entry_files);
    bool topologicalOrder(vector<string>& order);
};
```

모듈 경로(예: `kern.memory.page`) → 파일 경로(`kern/memory/page.kern`)로 해결. 순환 의존성 감지.

### 9.2 크로스 모듈 컴파일 흐름

```
1. ModuleResolver로 의존성 DAG 구축
2. 위상 정렬 순서로 컴파일
3. 각 모듈 컴파일 전:
   - HIRBuilder::registerExports() — 의존 모듈의 AST에서 pub 심볼 등록
   - injectFnSig() — 함수 시그니처 주입
   - injectNamedType() — 타입 주입 (struct, enum, union)
   - injectGenericStruct/Union() — 제네릭 타입 템플릿 주입
   - injectGlobalType() — 전역 변수 타입 주입
4. 각 모듈 → .o 파일
5. 전체 .o를 linkMultiple()로 링크
```

### 9.3 심볼 네이밍

- 모듈 내 함수: `_<module>__<fn_name>` (Mach-O 접두사 `_` 포함)
- `main` 함수: 항상 `_main` (모듈 접두사 없음)
- `@link_name("custom")`: 커스텀 심볼 이름
- `@no_mangle`: 모듈 접두사 제거

### 9.4 BuildCache (증분 컴파일)

```cpp
class BuildCache {
    string cacheKey(const string& source, const string& flags) const;  // FNV-1a 해시
    string lookup(const string& key) const;   // 캐시된 .o 경로 반환
    bool store(const string& key, const string& obj_file);
};
```

소스 내용 + 컴파일 플래그의 해시로 `.o` 파일을 캐싱.

---

## 10. IDE / Tooling

### 10.1 IDEContext

파일별 캐시. AST와 HIR을 지연 재빌드.

```cpp
class IDEContext {
    void openFile(string_view path, string_view content);
    void updateFile(string_view path, string_view content);
    const HIRModule* getHIR(string_view path);   // dirty이면 재빌드
    const Module* getAST(string_view path);
};
```

### 10.2 Provider들

| Provider | 반환 | 역할 |
|----------|------|------|
| `CompletionProvider` | `CompletionItem[]` | 자동완성 (함수, 변수, 타입, 필드, 키워드, 열거형 변형) |
| `HoverProvider` | `HoverResult?` | 마우스 오버 정보 (타입, 순수성, 정의 위치) |
| `DefinitionProvider` | `DefinitionResult?` | 정의로 이동 |
| `ReferencesProvider` | `ReferenceLocation[]` | 모든 참조 위치 |
| `DiagnosticProvider` | `IDEDiagnostic[]` | 파일의 에러/경고 |
| `SemanticTokensProvider` | `SemanticToken[]` | 시맨틱 하이라이팅 |

### 10.3 Formatter

AST 기반 소스 정렬기.

```cpp
class Formatter {
    Formatter(ostream& out, FormatOptions opts);  // indent_width=4, max_line_width=100
    void formatModule(const Module* mod);
};
```

### 10.4 Debug Tooling

| 컴포넌트 | 역할 |
|----------|------|
| `DebugInfoBuilder` | MachModule에서 DebugInfo 구축 + `.kern_debug` 직렬화 |
| `SourceMapBuilder` | 주소 → 소스 위치 매핑 |
| `ValueInspector` | 메모리 바이트를 Kern 타입 값으로 포맷 |

---

## 11. 핵심 불변식

이 컴파일러를 재구현할 때 반드시 지켜야 하는 불변식:

1. **TypeId 일관성**: `uint32_t` 핸들이 모든 IR 단계에서 동일 TypeTable을 참조. 기본 타입은 인덱스 0–15에 고정.

2. **Arena 할당**: AST/HIR/LIR/MachIR 노드에 `new` 금지. 반드시 `arena.make<T>()`. 개별 해제 없음.

3. **string_view 수명**: Token과 AST 노드의 `text`는 원본 소스 버퍼에 대한 `string_view`. 소스가 모든 소비자보다 오래 살아야 한다.

4. **StringPool 인터닝**: 모든 식별자는 인터닝. 문자열 비교 = 포인터 비교.

5. **레이어 의존성**: Support → Lexer → Parser → HIR → LIR → Backend. 역방향 의존 금지.

6. **블록 인자 (phi 대체)**: LIR에서 phi 노드 대신 블록 파라미터 + 분기 인자 사용.

7. **Newline 토큰**: `*`, `/`, `&`가 줄 시작에 오면 이항 연산자가 아닌 다음 문장의 시작으로 해석.

8. **Mach-O 심볼 접두사**: macOS에서 모든 심볼에 `_` 접두사. ELF에서는 없음.

---

## 12. ABI 규약

### 12.1 System V AMD64 Calling Convention

| 항목 | 규칙 |
|------|------|
| GPR 인자 | RDI, RSI, RDX, RCX, R8, R9 (최대 6개) |
| XMM 인자 | XMM0–XMM7 (최대 8개, float/double) |
| 반환값 | RAX (정수), XMM0 (부동소수점) |
| 스택 정렬 | RSP는 `call` 직전 16바이트 정렬 |
| Callee-saved | RBX, R12, R13, R14, R15, RBP |
| Caller-saved | RAX, RCX, RDX, RSI, RDI, R8–R11, XMM0–XMM15 |

### 12.2 구조체 전달

| 크기 | 전달 방식 |
|------|----------|
| ≤ 8B | GPR 1개 |
| 9–16B | GPR 2개 (lo + hi) |
| > 16B | 숨겨진 포인터 (호출자가 스택에 공간 할당, RDI로 주소 전달) |

### 12.3 구조체 반환

| 크기 | 반환 방식 |
|------|----------|
| ≤ 8B | RAX |
| 9–16B | RAX + RDX (precolored R11을 base pointer로 pack) |
| > 16B | 숨겨진 포인터 via RDI (caller allocated) |

### 12.4 클로저 ABI

```
__closure_N 구조체: { __fn: fn pointer, cap1, cap2, ... }
```

스택 할당 (힙 없음). 클로저 호출 시 구조체 포인터를 첫 번째 인자로 전달.

### 12.5 macOS 링킹

```
ld -e _start -platform_version macos 14.0.0 14.0.0 -arch x86_64 -lSystem
```

`_start` 래퍼: 스택 정렬 후 `_main` 호출, 반환값을 `exit` 시스템 콜로 전달.
