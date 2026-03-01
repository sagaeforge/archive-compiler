# Kern OS 개발 기능 로드맵 (Phase A~D)

## Context

Kern은 순수 함수형 커널 개발 언어로서, 고수준 타입 시스템(ADT, 패턴 매칭, 순수성 추론, TCO)은 완성되었으나 OS 개발에 필요한 저수준 기능이 거의 없다. 이 계획은 v2 아키텍처(Backend/MachIR→NASM) 완성 후, 4단계에 걸쳐 OS 개발 가능 수준까지 언어를 확장하는 로드맵이다.

## 전제 조건

**v2 Backend 완성** (Phase 4)이 선행되어야 한다:
- `lib/Backend/InstructionSelector.cpp` — LIR→MachIR 변환 (헤더만 존재)
- `lib/Backend/RegisterAllocator.{h,cpp}` — 선형 스캔 레지스터 할당
- `lib/Backend/ABI.{h,cpp}` — System V AMD64 호출 규약
- `lib/Backend/Emitter.{h,cpp}` — MachIR→NASM 텍스트 출력
- `lib/Backend/X86Backend.{h,cpp}` — InstructionSelector→RegAlloc→Emitter 오케스트레이터
- `tools/kernc/main.cpp` — v2 파이프라인으로 바이너리 생성 경로 연결

**v2 Backend 완성 후**, 아래 Phase A~D의 모든 기능은 v2 파이프라인(HIR→LIR→Backend)에서만 구현한다.

## 설계 원칙

1. **No unsafe 블록** — 저수준 연산은 기존 순수성 시스템으로 흡수 (asm→impure(io), volatile→impure(mem), ptr 산술→impure(mem))
2. **loop + break val** — while/for 없이, Rust 스타일 `loop { ... break value }` 제공
3. **v2 cascade** — 모든 기능은 Token→Lexer→AST→Parser→HIR→LIR→InstructionSelector→Emitter 순서로 구현
4. **단계별 검증** — 각 Phase 완료 시 실제 구동 가능한 마일스톤 바이너리

---

## Phase A: "Hello Hardware" — 최소 하드웨어 접근

**목표**: freestanding x86-64 바이너리가 시리얼 포트에 문자를 출력한다.

### A1: 비트 연산자 (& | ^ ~ << >>)

C 기호 그대로 사용. 기존 토큰과의 충돌은 문맥(단항/이항)과 lookahead로 해결.

**토큰 충돌 해결**:
- `&`: 단항 = addr-of (기존), 이항 = bitwise AND (Pratt 파서가 자동 구분)
- `|`: `|>` = pipe (2글자 우선 매칭), `|` 단독 = bitwise OR
- `^` `~`: 새 토큰, 충돌 없음
- `<<` `>>`: `<`+`<` / `>`+`>` lookahead. 타입 위치의 `<T>`와는 파서 문맥이 다름

**우선순위 (Pratt BP)** — 전체 재배치, 10단위 여유:
```
Pipe     {10,11}
or       {20,21}
and      {30,31}
Eq/NotEq {40,41}
Cmp      {50,51}
|        {60,61}   ← bitwise OR (비교보다 높음! C 버그 회피)
^        {70,71}   ← bitwise XOR
&        {80,81}   ← bitwise AND
Add/Sub  {90,91}
<< >>    {100,101} ← shift
Mul/Div  {110,111}
Prefix   125       (- not * & ~)
as       {130,131} ← cast (A7)
Dot      {200,201}
```
`a == b & c`는 `a == (b & c)` — 직관적 우선순위.

**수정 파일**:
| 파일 | 변경 |
|------|------|
| `include/kern/lexer/Token.h` | `BitOr, BitXor, Tilde, Shl, Shr` 추가. `Ampersand` 기존 유지 (이항 시 bitAND) |
| `lib/Lexer/Lexer.cpp` | `|` lookahead (`>`이면 Pipe, 아니면 BitOr), `<<`/`>>` 2글자 매칭, `^`/`~` 단일 문자 |
| `include/kern/parser/AST.h` | `BinOpKind::BitAnd, BitOr, BitXor, Shl, Shr` + `UnaryOpKind::BitNot` |
| `lib/Parser/Parser.cpp` | `infixBP` 전체 재배치 (10단위), `Ampersand`를 infix에도 등록 {80,81}, `parsePrimary`(Tilde prefix) |
| `include/kern/hir/HIR.h` | `HIRBinOp::BitAnd/BitOr/BitXor/Shl/Shr` + `HIRUnaryOp::BitNot` |
| `lib/HIR/HIRBuilder.cpp` | 타입 체크: 정수 타입만 허용, float→에러 |
| `include/kern/lir/LIR.h` | `LIROp::BAnd, BOr, BXor, Shl, Shr, BNot` |
| `lib/LIR/LIRBuilder.cpp` | HIR→LIR 매핑 |
| `lib/Backend/InstructionSelector.cpp` | `BAnd→X86Op::And`, `BOr→X86Op::Or`, `BXor→X86Op::Xor`, `BNot→X86Op::Not` |
| `include/kern/backend/MachIR.h` | `X86Op::Shl, Shr` 추가 (shift 명령어) |
| `lib/Backend/Emitter.cpp` | `shl`/`shr` + CL 레지스터 처리 |

**주의**: `&`의 이항/단항 구분 — Pratt 파서의 `parseExprInfix`에서 `Ampersand`가 infix BP {80,81}을 반환하면 이항, `parsePrimary`에서 prefix BP 125를 반환하면 단항. 기존 newline disambiguation 로직(`on_new_line && Ampersand → break`) 유지.

**테스트**: unit (HIR타입체크 + LIR매핑), E2E `bitwise_ops.kern` (42 & 0xFF == 42)

### A2: 모듈로 연산자 (mod)

LIR `Mod` 오피코드와 `InstructionSelector::selectDiv(instr, /*is_mod=*/true)` 이미 존재. AST/HIR/Parser 연결만 필요.

**수정 파일**: Token.h(`KwMod`), Lexer.cpp, AST.h(`BinOpKind::Mod`), Parser.cpp(`infixBP {22,23}`), HIR.h(`HIRBinOp::Mod`), HIRBuilder.cpp, LIRBuilder.cpp

### A3: loop + break/continue (축적자 패턴)

순수 함수형 정체성 유지를 위해, loop는 **축적자(accumulator) 패턴**을 지원한다.
`var` 없이 상태를 전달할 수 있으므로, loop를 사용해도 **pure** 함수 작성이 가능하다.

**구문**:
```kern
// 축적자 패턴 — var 불필요, pure 가능
val sum = loop(acc = 0, i = 0) {
    if i == 10 { break acc }
    continue(acc + arr[i], i + 1)
}

// 단순 무한 루프 (Unit 반환, 보통 -> ! 함수에서)
loop {
    asm { "hlt" }
}

// 단일 축적자
val found = loop(i = 0) {
    if arr[i] == target { break i }
    continue(i + 1)
}
```

loop은 expression. break는 결과값을 결정. continue는 축적자를 갱신하며 다음 반복으로.

**핵심 설계**: 축적자 바인딩(`acc`, `i`)은 loop header의 **블록 파라미터**로 모델링된다.
`continue(new_acc, new_i)`는 새 값으로 header 블록을 재진입하는 `Branch + BlockArg` 패턴이다.
이는 기존 LIR의 블록 파라미터(SSA phi 대체) 메커니즘을 **정확히 재사용**한다.

**AST**:
- `Expr::Kind::Loop` → `LoopExpr { LoopBinding* bindings; uint32_t binding_count; Stmt** stmts; uint32_t stmt_count; }`
- `LoopBinding { std::string_view name; Expr* init; }` — 축적자 초기값
- `Stmt::Kind::Break` → `BreakStmt { Expr* value; }` — 결과값
- `Stmt::Kind::Continue` → `ContinueStmt { Expr** args; uint32_t arg_count; }` — 축적자 갱신값

**HIR**: `HIRLoopExpr`, `HIRBreakStmt`, `HIRContinueStmt` 동일 구조. 타입 체크:
- 모든 `break` 값의 타입이 동일해야 함 (loop의 결과 타입)
- `continue` 인자 수 = 축적자 수, 각 타입이 대응 축적자와 일치
- 축적자 바인딩은 loop body 내에서 **immutable** (val과 동일)
- 축적자가 없는 loop: 결과 타입 = Unit, break 값 없이 가능

**순수성**: 축적자만 사용하면 **pure** 유지. var를 섞으면 impure(mut) — 기존 규칙 그대로.

**LIR 전략 — 블록 파라미터 패턴** (phi-slot이 아닌 진짜 블록 파라미터):
```
entry:
    StructAlloc 8B → result_slot     // break 결과 슬롯
    Branch → loop_header [init_acc, init_i]  // 초기값 전달

loop_header(acc: TypeId, i: TypeId):     // 블록 파라미터 = 축적자
    %acc = BlockArg $0
    %i   = BlockArg $1
    ... 루프 본문 ...
    // continue(acc + arr[i], i + 1):
    Branch → loop_header [%new_acc, %new_i]   // 백엣지 + 새 값
    // break acc:
    Store result_slot, %acc
    Branch → loop_exit

loop_exit:
    Load result_slot → result
```

**LIRBuilder 확장**:
- `struct LoopContext { uint32_t header_bb; uint32_t exit_bb; VReg result_slot; TypeId result_type; std::vector<std::string_view> acc_names; }`
- `std::vector<LoopContext> loop_stack_` — 중첩 루프 지원
- `lowerLoop()`: result_slot 할당 → header 블록 (param_types = 축적자 타입들) → body lowering → exit 블록
- `lowerBreak()`: Store result_slot + Branch to exit
- `lowerContinue()`: Branch to header with new acc values

**수정 파일**:
| 파일 | 변경 |
|------|------|
| `Token.h` | `KwLoop, KwBreak, KwContinue` |
| `Lexer.cpp` | 키워드 매핑 |
| `AST.h` | `LoopExpr`, `LoopBinding`, `BreakStmt`, `ContinueStmt` |
| `Parser.cpp` | `parsePrimary`(KwLoop + 축적자 파싱), `parseStmt`(KwBreak, KwContinue) |
| `HIR.h` | `HIRLoopExpr`, `HIRLoopBinding`, `HIRBreakStmt`, `HIRContinueStmt` |
| `HIRBuilder.cpp` | 축적자 타입 추론, break/continue 타입 일관성 체크, 루프 밖 break/continue 에러 |
| `LIRBuilder.h` | `LoopContext`, `loop_stack_` |
| `LIRBuilder.cpp` | `lowerLoop()`, `lowerBreak()`, `lowerContinue()` — 블록 파라미터 활용 |
| `HIRPasses.cpp` | Loop/Break/Continue 노드 순회 추가 (purity/tail-call) |

InstructionSelector 변경 없음 — loop은 기존 LIR 연산(StructAlloc, Store, Load, Branch, BlockArg)으로 표현됨.

### A4: 고정 크기 배열 [T; N]

**구문**:
```kern
val arr: [i64; 4] = [1, 2, 3, 4]
val x = arr[2]        // 인덱스 접근
var buf: [u8; 256]    // 0-초기화
buf[0] = 42           // var 배열만 인덱스 할당
```

**AST**:
- `TypeRef::Kind::Array` → `element: TypeRef*, size: uint32_t`
- `Expr::Kind::ArrayLit` → `ArrayLitExpr { Expr** elements; uint32_t count; }`
- `Expr::Kind::IndexAccess` → `IndexAccessExpr { Expr* array; Expr* index; }`
- `Stmt::Kind::IndexAssign` → `IndexAssignStmt { Expr* array; Expr* index; Expr* value; }`

**TypeTable**: `makeArray(element: TypeId, count: uint32_t) → TypeId` 추가. `ArrayData { TypeId element; uint32_t count; }` 이미 존재.

**LIR lowering**:
- 배열 리터럴: `StructAlloc(N * elem_size)` + N개의 `FieldPtr(base, i*elem_size)` → `Store`
- 상수 인덱스: `FieldPtr(base, const_offset)` → `Load`
- 변수 인덱스: `Mul(index, elem_size)` → `Add(base_ptr, byte_offset)` → `Load` — 새 `LIROp::PtrAdd` 필요 (A6과 공유)

**수정 파일**: Token.h(`LBracket/RBracket` 이미 존재), AST.h, Parser.cpp(배열 타입/리터럴/인덱스 파싱), TypeSystem.h(`makeArray`), HIR.h, HIRBuilder.cpp, LIR.h(`PtrAdd`), LIRBuilder.cpp

### A5: 인라인 어셈블리

**구문** (Phase A — 최소 버전, 입출력 제약 없음):
```kern
asm {
    "cli"
    "mov dx, 0x3F8"
    "mov al, 72"
    "out dx, al"
}
```

각 줄은 NASM 명령어 문자열. Emitter가 그대로 출력. 향후 Phase C에서 입출력 제약 추가.

**AST**: `Expr::Kind::InlineAsm` → `InlineAsmExpr { StringLitExpr** lines; uint32_t line_count; }`
**HIR**: `HIRExpr::Kind::InlineAsm` → 타입 = Unit, purity = ImpureIo (무조건)
**LIR**: `LIROp::InlineAsm` → 새 payload `LIRInlineAsmPayload { const char** lines; uint32_t line_count; }`
**MachIR**: `X86Op::Pseudo_InlineAsm` → Emitter: 줄별로 `"    " + line + "\n"` 출력
**InstructionSelector**: `selectInlineAsm` → `Pseudo_InlineAsm` 발행

**순수성**: 함수 내 `asm` 블록 → 자동 `impure(io)`

### A6: 포인터 산술

**구문**: 이름 기반 내장함수 (제네릭 전이므로 구체 타입):
```kern
fn ptr_add_bytes(ptr: Ptr<u8>, n: u64) -> Ptr<u8> = intrinsic
fn ptr_sub_bytes(ptr: Ptr<u8>, n: u64) -> Ptr<u8> = intrinsic
```

**LIR**: `LIROp::PtrAdd` → `LIRPtrAddPayload { VReg ptr; VReg byte_offset; }`
**LIRBuilder**: `lowerCall`에서 callee == `"ptr_add_bytes"`/`"ptr_sub_bytes"` 인식 → `PtrAdd` 또는 `Neg`+`PtrAdd` 발행
**InstructionSelector**: `selectPtrAdd` → `X86Op::Add` 또는 `X86Op::Lea` (lea rX, [rP + rN])

**순수성**: **pure** (포인터 산술은 정수 덧셈과 동일 — 메모리 접근 아님. 실제 읽기/쓰기는 Load/Store에서 impure 처리됨)

### A7: 타입 캐스팅 (as)

**구문**:
```kern
val addr = ptr as u64        // 포인터 → 정수
val p = addr as Ptr<u8>      // 정수 → 포인터
val narrow = big as i32      // 정수 truncation
val wide = small as i64      // 정수 확장
```

**AST**: `Expr::Kind::Cast` → `CastExpr { Expr* operand; TypeRef target; }`
**Parser**: `as`를 infix 연산자로 추가, BP {27,28} (매우 높음, dot 바로 아래)
**HIR**: `HIRExpr::Kind::Cast` → `HIRCastExpr { HIRExpr* operand; TypeId target; }`

**HIRBuilder 타입 체크**:
- 정수→정수: 항상 허용 (truncate/extend)
- Ptr→u64 / u64→Ptr: 허용
- float→int / int→float: 허용 (Phase A)
- 그 외: 에러

**LIR**: `LIROp::Cast` → `LIRCastPayload { VReg src; }` (from/to 타입은 LIRInstr.type + src의 type으로)
**InstructionSelector**: width별 분기 — movzx (unsigned widen), movsx (signed widen), mov (truncate), nop (ptr↔int, same width)

### A8: volatile 읽기/쓰기

**구문**: 내장함수 방식 (파서 변경 없이 가장 단순):
```kern
val x = volatile_read(ptr)           // volatile 읽기
volatile_write(ptr, val)             // volatile 쓰기
```

**결정: 내장함수 방식 채택** — `volatile_read`/`volatile_write`를 HIRBuilder가 인식하는 빌트인 호출로 처리. LIR에서는 기존 Load/Store에 `is_volatile` 플래그 추가.
파서/AST 변경 불필요. 타입별 오버로드: `volatile_read_u8`, `volatile_read_u16`, ..., `volatile_read_u64` (제네릭 전이므로).

**LIR**: `LIRLoadPayload`에 `bool is_volatile` 추가, `LIRStorePayload`에도 동일
**InstructionSelector**: volatile Load/Store → 동일한 x86 명령어이나, 최적화 패스에서 재배치/제거 금지 (Phase A에서는 최적화 패스 미존재이므로 사실상 동일)
**Emitter**: 변경 없음 (x86 mov는 기본적으로 volatile; 중요한 건 컴파일러 최적화 방지)

### A9: Noreturn 타입 (-> !)

**구문**: `fn halt() -> ! { ... }`

**수정 파일**:
| 파일 | 변경 |
|------|------|
| `Token.h` | `Exclaim` 토큰 (단독 `!`, `!=`과 구별) |
| `Lexer.cpp` | `!` 다음이 `=`이면 NotEq, 아니면 Exclaim |
| `AST.h` | `TypeRef::Kind::Never` |
| `Parser.cpp` | `parseType()`에서 `!` → Never |
| `TypeSystem.h` | `TypeId NEVER_TYPE` 추가 (프리미티브 13번) |
| `HIR.h` | `HIRFnDecl::is_noreturn` |
| `LIR.h` | `LIRFunction::is_noreturn` |
| `MachIR.h` | `MachFunction::is_noreturn` |
| `Emitter.cpp` | noreturn 함수: `ret` 생략, 끝에 `ud2` 발행 (안전장치) |

### A10: Freestanding 모드

**수정 파일**: `tools/kernc/main.cpp` 만

**플래그**: `--freestanding`
- `-lSystem` 제거
- `_start` 래퍼(exit syscall 호출) 생략
- `--entry <name>` 플래그로 커스텀 진입점 (기본: `start`)
- ld 명령: `-e _<entry> -static` + 유저 제공 오브젝트만 링크

**추가**: `--output-format` 플래그 (향후 ELF 지원 대비)
- `macho64` (기본, 현재)
- `elf64` (Phase B에서 추가)

### Phase A 의존성 순서

```
A2 (mod) ─────────┐
A9 (noreturn) ────┤
A10 (freestanding)┤
A1 (bitwise) ─────┤
A7 (cast/as) ─────┼──→ A6 (ptr arithmetic) ──→ A4 (arrays)
A8 (volatile) ────┤
A3 (loop+break) ──┘
A5 (inline asm) ──────────────────────────────────────────→ [검증]
```

### Phase A 검증 마일스톤

**주의**: Phase A 인라인 어셈블리는 입출력 제약(constraints)이 없으므로,
Kern 변수를 asm 블록에 전달할 수 없다. 검증은 두 가지 경로:

**경로 1 — intrinsic + 외부 asm** (현실적):
```kern
// intrinsic 선언 — 실제 구현은 외부 .asm 파일
fn outb(port: u16, val: u8) -> Unit = intrinsic

fn start() -> ! {
    outb(0x3F8, 72)   // 'H' to COM1
    outb(0x3F8, 101)  // 'e'
    outb(0x3F8, 108)  // 'l'
    outb(0x3F8, 108)  // 'l'
    outb(0x3F8, 111)  // 'o'
    loop { asm { "hlt" } }
}
```
+ 별도 `serial.asm`: `_outb: mov dx, di; mov al, sil; out dx, al; ret`
+ `kernc --freestanding --link serial.o hello.kern`

**경로 2 — 순수 asm** (하드코딩):
```kern
fn start() -> ! {
    asm {
        "mov dx, 0x3F8"
        "mov al, 72"
        "out dx, al"
        "cli"
        "hlt"
    }
}
```
변수 전달 없이 상수만 사용. 기능 검증에 충분.

`kernc --freestanding serial.kern` → freestanding x86-64 바이너리 → QEMU 실행

---

## Phase B: "Memory Manager" — 메모리 관리 기초

**목표**: 비트맵 물리 메모리 할당자를 Kern으로 구현.

### B1: packed struct + B2: aligned struct

**구문**:
```kern
@packed struct USBDesc { device_class: u8, subclass: u8, protocol: u8 }
@align(4096) struct PageTable { entries: [u64; 512] }
```

**구현**: `@` 어노테이션 파싱 인프라 구축 (Parser에 `parseAnnotation()` 범용 함수)
- `StructDecl::is_packed`, `StructDecl::explicit_align`
- `TypeTable::makeStruct`에 `is_packed`/`align` 파라미터 추가
- `sizeOf()`/`alignOf()` — packed일 때 패딩 없는 레이아웃 계산

### B3: 타입 별칭 + B4: 뉴타입

```kern
type PhysAddr = u64                   // 별칭 (투명)
newtype VirtAddr(u64)                 // 뉴타입 (불투명, 명시적 변환만)
```

- 별칭: `HIRBuilder::named_types_`에 이름→TypeId 등록, 타입 해석 시 치환
- 뉴타입: 단일 필드 struct로 내부 구현, 생성자 호출 `VirtAddr(0x1000)`

### B5: 슬라이스 (Slice<T>)

제네릭 전이므로 `TypeTable::makeSlice(element: TypeId) → TypeId`로 빌트인 구현.
내부 레이아웃: `{ data: Ptr<T>, len: u64 }` = 16바이트 (String과 동일).
`Slice<u8>`, `Slice<i64>` 등 구체 타입별 인스턴스화.

### B6: 모듈 시스템 (기본)

**구문**:
```kern
module kern.memory
import kern.types (PhysAddr)
```

**구현 단계**:
1. `ModuleRegistry` (Support 레이어) — 모듈명→컴파일된 HIRModule 매핑
2. `kernc` 드라이버: 여러 `.kern` 파일 입력, 의존성 순서로 HIR 빌드
3. 심볼 가시성: 기본 public, `@internal` 어노테이션으로 모듈 내부 제한
4. 링킹: 각 모듈을 개별 `.o`로 컴파일, ld로 최종 링크

**수정 파일**: Token.h(`KwModule, KwImport`), AST.h(`ImportDecl`), Parser.cpp, HIR.h(`HIRModule`에 `imports`/`import_count` 필드 추가 — 현재 없음, 신규), LIR.h(`LIRFunction`에 `Linkage` 필드 추가 — 현재 없음, 신규), `Support/ModuleRegistry.{h,cpp}` 신규, main.cpp(다중 파일 컴파일 루프)

### B7: 링커 스크립트 + B8: 커스텀 섹션

```kern
@section(".boot") fn kernel_entry() -> ! { ... }
```

- B7: `main.cpp`에 `--linker-script <file>` 플래그 → ld에 `-T` 전달
- B8: `@section("name")` 어노테이션 → `LIRFunction::section` → Emitter: `section <name>` 디렉티브

### B9: 컴파일타임 assertion

```kern
static_assert(sizeof(PageTableEntry) == 8, "PTE must be 8 bytes")
```

`sizeof` / `alignof` 내장 함수 + 상수 평가기(ConstantFolder). HIRBuilder에서 상수 표현식 평가 → false이면 `diag.error()`.

### Phase B 검증 마일스톤
```kern
module kern.memory

@packed @align(4096) struct BitmapAllocator {
    var bitmap: Ptr<var u8>
    var total_frames: u64
    var used_frames: u64
}

fn alloc_frame(alloc: Ptr<var BitmapAllocator>) -> u64 {
    loop {
        // bitmap 스캔 (band, shl, ptr_add_bytes 사용)
        // 비트 찾으면 break frame_addr
    }
}
```

---

## Phase C: "Kernel Abstractions" — 타입 시스템 강화

**목표**: 타입 안전한 IDT 초기화, SpinLock<T>, Vec<T, A> 구현.

### C1: 제네릭 (단형화)

```kern
fn identity<T>(x: T) -> T { x }
struct Vec<T> { data: Ptr<T>, len: u64, cap: u64 }
```

**구현**: `MonomorphizationPass` (HIR 패스)
1. 다형 함수/구조체를 TypeVar로 HIR 빌드
2. 모든 호출/사용 지점에서 구체 타입 수집
3. 각 (함수, 타입 인자 집합)에 대해 HIR 트리 딥카피 + TypeVar→구체 TypeId 치환
4. 맹글 이름: `identity_i64`, `Vec_u8` 등
5. 호출 지점 패치

**단계적 구현**:
- C1a: 제네릭 함수 (구조체 없이)
- C1b: 제네릭 구조체
- C1c: 타입 추론 (호출 시 타입 인자 생략)

**복잡도**: 매우 높음. HIR 노드 딥카피 + 치환이 핵심 난이도.

### C2: 트레이트/인터페이스

```kern
trait Allocator {
    fn alloc(self: Ptr<var Self>, size: u64) -> Ptr<u8>
    fn dealloc(self: Ptr<var Self>, ptr: Ptr<u8>, size: u64) -> Unit
}
impl Allocator for BitmapAllocator { ... }
```

**구현**: 정적 디스패치 (단형화). 동적 디스패치(vtable)는 Phase D 이후.
- `TraitDecl` → 메서드 시그니처 집합
- `ImplDecl` → 구체 타입의 메서드 구현
- HIRBuilder: 트레이트 바운드 체크, `Self` 타입 치환
- 단형화 시 트레이트 제약 해소

### C3: 클로저/람다

```kern
val double = { x: i64 => x * 2 }
[1, 2, 3] |> map({ x => x + 1 })
```

**구현**: `ClosureConversionPass` (HIR 패스)
- 캡처 분석: 자유 변수 탐지
- 캡처 없음 → 일반 함수로 리프팅 (제로 비용)
- 캡처 있음 → 캡처 구조체 생성, 함수 리프팅 + 환경 포인터
- 함수 타입: `(i64) -> i64` = `{ fn_ptr: Ptr<fn>, env: Ptr<u8> }` (2-word fat pointer)

**LIR**: `LIROp::CallIndirect` (간접 호출) 추가

### C4: Result<T,E> / Maybe<T>

제네릭(C1) 위에 라이브러리 타입으로 구현. `?` 연산자는 HIR desugar:
```kern
val x = expr?
// → match expr { Ok(v) => v, Err(e) => return Err(e) }
```

### C5: const fn

`const fn`은 HIR 인터프리터로 평가. Phase B의 `static_assert` 상수 평가기를 확장.

### C6: 함수 포인터

```kern
val f: fn(i64) -> i64 = add_one
val result = f(42)              // 간접 호출
```

`TypeTable::makeFn` 이미 존재. `LIROp::CallIndirect` (C3과 공유). `InstructionSelector`: `call [vreg]`.

### C7: naked 함수 + C8: interrupt 호출규약

```kern
@naked fn isr_stub_32() -> ! { asm { "push rbp" "jmp isr_common" } }
@interrupt fn keyboard_handler(frame: Ptr<InterruptFrame>) -> ! { ... }
```

- naked: 프롤로그/에필로그 생략 (FrameSetup/FrameDestroy 스킵)
- interrupt: `iretq` 반환, 전 레지스터 저장/복원, 에러 코드 처리

`MachFunction::calling_conv: enum { Standard, Naked, Interrupt }`

### Phase C 검증 마일스톤
```kern
struct SpinLock<T> { var locked: Atomic<u64>, var data: T }
fn with_lock<T>(lock: Ptr<var SpinLock<T>>, f: fn(Ptr<var T>) -> Unit) -> Unit { ... }

// IDT 초기화
val idt: [IdtEntry; 256] = init_idt()
fn init_idt() -> [IdtEntry; 256] { ... }
```

---

## Phase D: "SMP Kernel" — 동시성과 멀티코어

**목표**: 2-CPU SMP 부팅 + 라운드로빈 스케줄러.

### D1: 아토믹 타입 (Atomic<T>)

```kern
val counter: Atomic<u64> = Atomic(0)
atomic_store(&var counter, 1, MemOrder.Release)
val v = atomic_load(&counter, MemOrder.Acquire)
atomic_cas(&var counter, 0, 1, MemOrder.SeqCst)
```

**LIR**: `LIROp::AtomicLoad, AtomicStore, AtomicCas, AtomicFetchAdd`
**MachIR**: `X86Op::LockPrefix` (pseudo) + 기존 Mov/Cmpxchg/Xadd
**Emitter**: `lock cmpxchg`, `lock xadd`, `xchg` (항상 atomic)

### D2: 메모리 오더링

빌트인 enum `MemOrder { Relaxed, Acquire, Release, AcqRel, SeqCst }`.
`LIROp::Fence` → `mfence`/`sfence`/`lfence`/no-op.

### D3: 컴파일러 배리어 + D4: 하드웨어 펜스

`compiler_barrier()` → `LIROp::CompilerFence` (no-op in emission, barrier for optimizer)
`mfence()`, `sfence()`, `lfence()` → `= intrinsic` → `LIROp::Fence`

### D5: Per-CPU 데이터

```kern
@percpu val current_task: Ptr<Task> = null_ptr()
```

GS-세그먼트 상대 주소 사용. `LIROp::PercpuLoad/PercpuStore`.
Emitter: `mov rax, [gs:offset]`.

### D6: const 제네릭

```kern
struct Buffer<T, const N: u64> { data: [T; N] }
```

C1 단형화 확장 — 상수 파라미터도 단형화 대상.

### Phase D 검증 마일스톤
```kern
// AP(Application Processor) 부팅 + 라운드로빈
fn scheduler_tick() -> Unit {
    val next = atomic_fetch_add(&var run_queue_idx, 1, MemOrder.Relaxed)
    switch_to(tasks[next mod task_count])
}
```

---

## 전체 의존성 그래프

```
[v2 Backend 완성] ──→ Phase A ──→ Phase B ──→ Phase C ──→ Phase D
                      │                        │
                      │ A1 bitwise              │ C1 generics ←── Phase B all
                      │ A2 mod                  │ C2 traits  ←── C1
                      │ A3 loop+break           │ C3 closures ←── C6
                      │ A7 cast/as              │ C4 Result   ←── C1, A3
                      │ A8 volatile             │ C5 const fn ←── B9
                      │ A9 noreturn             │ C6 fn ptr
                      │ A10 freestanding        │ C7 naked   ←── A5
                      │ A6 ptr arith ←── A7     │ C8 interrupt ←── C7
                      │ A4 arrays ←── A6        │
                      │ A5 inline asm           │
                      └─[시리얼 출력]            └─[IDT+SpinLock]
```

## 상대적 복잡도 추정

| 기능 | 복잡도 | 참조 기준 (1.0x = Ptr<T> 구현) |
|------|--------|------|
| A2 mod | 0.1x | LIR 이미 있음 |
| A10 freestanding | 0.2x | main.cpp만 |
| A9 noreturn | 0.3x | 메타데이터 플래그 |
| A1 bitwise | 0.5x | BinOpKind 6개 추가 |
| A8 volatile | 0.5x | 플래그 추가 |
| A7 cast/as | 0.7x | 새 Pratt 연산자 + 타입별 분기 |
| A6 ptr arith | 0.5x | 새 LIROp + Lea |
| A3 loop+break | 1.5x | CFG 백엣지 + 컨텍스트 스택 |
| A4 arrays | 1.5x | TypeTable + 변수 인덱스 |
| A5 inline asm | 1.5x | 새 LIROp + Emitter 패스스루 |
| B1-B2 packed/align | 0.5x | 레이아웃 계산 |
| B3 type aliases | 0.3x | resolveType |
| B4 newtypes | 0.4x | struct 위 sugar |
| B5 slices | 1.0x | 파라메트릭 빌트인 |
| B6 modules | 4.0x | 다중 파일 + 이름 해석 |
| B7-B8 linker/sections | 0.4x | 메타데이터 전파 |
| B9 static_assert | 1.0x | 상수 평가기 |
| C1 generics | 8.0x | **최고 난이도** — HIR 딥카피+단형화 |
| C2 traits | 4.0x | 트레이트 해석 + 디스패치 |
| C3 closures | 4.0x | 캡처 분석 + 함수 리프팅 |
| C4 Result/? | 1.5x | C1 위 sugar |
| C5 const fn | 3.0x | HIR 인터프리터 |
| C6 fn ptr | 1.0x | CallIndirect |
| C7 naked | 0.5x | 플래그 |
| C8 interrupt | 1.0x | 호출규약 |
| D1 atomics | 2.0x | lock prefix + CAS |
| D2 ordering | 0.8x | fence 매핑 |
| D3-D4 barriers | 0.3x | intrinsic |
| D5 percpu | 2.0x | GS 세그먼트 |
| D6 const generics | 2.0x | 단형화 확장 |

## 검증 방법

각 Phase 완료 시:
1. `cmake -B build && cmake --build build` — 전체 빌드 성공
2. `build/tests/unit/kern_tests` — 모든 단위 테스트 통과
3. `bash tests/integration/run_tests.sh build/tools/kernc/kernc tests/integration` — 모든 E2E 통과
4. Phase별 검증 바이너리 실제 실행 (QEMU 또는 Rosetta)

## 핵심 수정 파일 목록

**모든 Phase에서 반복 수정되는 파일** (v2 cascade):
- `include/kern/lexer/Token.h` — 새 키워드/토큰
- `lib/Lexer/Lexer.cpp` — 키워드 매핑
- `include/kern/parser/AST.h` — 새 노드 타입
- `lib/Parser/Parser.cpp` — 파싱 로직
- `include/kern/hir/HIR.h` — HIR 노드 타입
- `lib/HIR/HIRBuilder.cpp` — AST→HIR + 타입 체크
- `lib/HIR/HIRPasses.cpp` — 새 노드 순회
- `include/kern/lir/LIR.h` — LIR 오피코드
- `lib/LIR/LIRBuilder.cpp` — HIR→LIR lowering
- `lib/Backend/InstructionSelector.cpp` — LIR→MachIR
- `lib/Backend/Emitter.cpp` — MachIR→NASM

**Phase별 신규 파일**:
- Phase A: 없음 (기존 파일 확장만)
- Phase B: `include/kern/support/ModuleRegistry.h`, `lib/Support/ModuleRegistry.cpp`
- Phase C: `lib/HIR/MonomorphizationPass.cpp`, `lib/HIR/ClosureConversionPass.cpp`
- Phase D: 없음 (기존 파일 확장)
