# Kern 컴파일러 구조적 리팩토링 — 요구사항 명세

> 생성: 2026-03-01
> 최종 수정: 2026-03-01
> 상태: **대체됨** — `architecture-v2-design.md`가 이 문서를 계승 + 확장
> 전제: M5 개발 완료 후 전체 계획 업데이트 → 리팩토링 진행
>
> ⚠️ **참고**: 이 문서의 요구사항(WS-1~WS-10)은 `architecture-v2-design.md`의 4-레벨 파이프라인
> 설계에 통합되었습니다. Section 7 "기존 리팩토링 요구사항과의 대응" 참조.
> 이 문서는 의사결정 히스토리 보존 목적으로 유지됩니다.

---

## 1. 동기 및 목표

### 동기
- M4까지 4,028줄 구현이 단일 책임 원칙 없이 성장 → 확장 한계
- M5(struct/enum/pointer) + M6(closures/generics/modules)을 위한 구조적 기반이 부재
- 현재 아키텍처의 기술부채가 M5+ 기능 개발 속도를 제약

### 목표
- **M5/M6에 필요한 확장 포인트를 미리 확보**
- 각 파이프라인 스테이지의 관심사를 명확히 분리
- 테스트 가능성과 재사용성 향상
- E2E 테스트를 순수 출력 기반 앵커로 강화 (정확성 보장)

### E2E 테스트 철학
> **E2E는 아웃풋에만 의존하며, 내부 변화에 절대 흔들리지 않아야 한다.**

현재 78개 E2E 중 **55개 exit code + 14개 error**는 이미 안정적이지만,
**9개 dump 테스트**(`dump_ast`, `dump_ir`, `dump_tokens`, `dump_purity` 등)는
내부 IR/AST 텍스트 포맷에 의존하여 리팩토링 시 깨질 수 있다.

**원칙**:
- E2E = 컴파일러의 **외부 계약** (입력 .kern → exit code / error message) 만 검증
- 내부 표현(IR 포맷, AST 포맷, 토큰 포맷) 검증은 **unit 테스트의 영역**
- dump 기능의 정상 동작 자체는 unit 테스트로 보장, E2E에서는 제외하거나 최소화

**리팩토링 시 조치**:
- 9개 dump E2E를 별도 카테고리(`tests/integration/dump/`)로 분리
- 이들은 "변경 가능한 테스트"로 취급 (리팩토링 시 포맷 변경에 맞춰 업데이트 허용)
- 나머지 69개(exit code + error)는 **불변 앵커** — 절대 깨져선 안 됨

---

## 2. 확정된 결정 사항

| 항목 | 결정 | 근거 |
|------|------|------|
| 접근 방식 | **빅뱅 리팩토링** (M5 완료 후) | 일관성 높은 인터페이스, 중간 상태 양립 불필요 |
| Type 시스템 | **TypeId + TypeTable** (Arena 기반) | 개방형, struct/enum/ptr 확장 가능, 포인터 비교 가능 |
| IR 설계 | **opcode별 tagged union 분리** | 메모리 효율, 확장성, callee_name 힙 할당 제거 |
| CodeGen 분리 | **클래스 분리** (RegisterAllocator, ABIHandler, InstructionEmitter) | 관심사 분리, 독립 테스트 가능 |
| String 처리 | **Arena 기반 StringPool** | 합성 이름 수명 관리, 해시→포인터 비교 전환 |
| RegAlloc | **Linear Scan 정식 구현** | liveness 분석 + live interval → 레지스터 해제 정상화 |
| AST Dump | **lib/Parser/ASTDump.cpp** 분리 | Parser.cpp 946줄 감소, 관심사 분리 |
| 드라이버 | **CompilerPipeline 클래스** 추상화 | main.cpp 단순화, 테스트에서 파이프라인 재사용 |
| 테스트 전략 | **E2E 앵커 + unit 재작성** | 78 E2E 불변, unit은 새 인터페이스에 맞춰 전면 재작성 |
| 타이밍 | M5 개발 완료 → 전체 계획 업데이트 → 리팩토링 실행 | 충돌 최소화 |

---

## 3. 리팩토링 워크스트림 (10개)

### WS-1: StringPool 인프라
**파일**: `include/kern/support/StringPool.h`, `lib/Support/StringPool.cpp`

- Arena 기반 문자열 인터닝 (bump alloc + 해시맵)
- `StringPool::intern(string_view) → string_view` (Arena에 복사, 같은 문자열은 같은 포인터)
- 합성 이름 생성: `intern(concat("Shape", "::", "Circle"))` 등
- 기존 string_view (source text)는 그대로 동작 (interning은 선택적)
- IR의 `std::string callee_name` → `string_view` (인터닝된) 전환 포함

**수용 기준**:
- 동일 문자열 intern 시 같은 포인터 반환
- IR callee_name이 string_view로 전환, 힙 할당 0

---

### WS-2: Type 시스템 재설계 (TypeId + TypeTable)
**파일**: `include/kern/sema/TypeSystem.h` (신규), `lib/Sema/TypeSystem.cpp` (신규)

- `TypeId = uint32_t`, 원시형 0~12 예약 (I8=0, ..., Unit=11, Error=12)
- `TypeInfo` 노드 (Arena 할당):
  ```
  TypeInfo { TypeKind kind; union { PrimitiveInfo, StructInfo, EnumInfo, UnionInfo, PtrInfo, FnInfo, ... } }
  ```
- `TypeTable`: TypeId → TypeInfo* 매핑, 중복 제거 (canonical types)
- 기존 `TypeChecker::Type` enum 사용처를 모두 `TypeId`로 교체
- `FnSig` 파라미터/리턴 타입 → `TypeId`
- `expr_types_: unordered_map<const Expr*, TypeId>`
- IRType도 TypeId 기반으로 통일 (IRType.h → TypeChecker.h 역방향 의존 제거)

**수용 기준**:
- 기존 13개 원시형이 TypeId 0~12로 정확히 매핑
- TypeTable에서 TypeId로 TypeInfo 조회 O(1)
- IR 레이어가 Sema 헤더에 의존하지 않음

---

### WS-3: IRInstr Tagged Union 재설계
**파일**: `include/kern/ir/KernIR.h` (수정)

- `IRInstr` → base fields (opcode, result, type, loc) + `union { ConstIntData, ConstFloatData, BinOpData, CallData, BranchData, ... }`
- 각 opcode 카테고리별 data struct:
  - `ConstIntData { int64_t value; }`
  - `ConstFloatData { double value; }`
  - `CallData { string_view callee; ValueId* args; uint32_t arg_count; bool is_tail_call; }`
  - `BranchData { uint32_t target; uint32_t false_target; ValueId cond; }` (CondBranch)
  - `UnaryData { ValueId operand; }`
  - `BinOpData { ValueId lhs; ValueId rhs; }`
  - `RetData { ValueId value; }`
  - (M5용 예약) `MemData { ValueId addr; ValueId value; uint32_t offset; }` (Load/Store/FieldAccess)
- `IRInstr.args` (현재 `vector<uint32_t>`) → Arena 할당 `ValueId*` + `arg_count`
- 전체 IRBuilder, CodeGen의 IRInstr 접근 패턴을 새 인터페이스에 맞춤

**수용 기준**:
- sizeof(IRInstr) 감소 (현재 대비)
- std::string 힙 할당 0
- M5 memory opcode 추가가 union variant 추가만으로 가능

---

### WS-4: CodeGen 클래스 분리
**파일**: 신규 헤더/구현 다수

- `RegisterAllocator` 클래스:
  - GPR + XMM 풀 관리
  - `allocReg(IRType) → Location`, `freeReg(Location)`, `spillToStack() → Location`
  - Live interval 계산 + linear scan 로직 포함 (WS-7과 통합)
- `ABIHandler` 클래스:
  - System V AMD64 호출 규약
  - 인자 배치 (reg vs stack), 반환값 처리
  - parallel-move 알고리즘 (1곳에만 구현, 중복 제거)
  - `_start` 래퍼 생성 (main.cpp에서 이동)
- `InstructionEmitter` 클래스:
  - x86-64 명령어 텍스트 생성 (NASM 문법)
  - 레지스터 이름 매핑 (regForWidth 등)
  - float 상수 풀 (.rodata 관리)
- `CodeGen` 클래스: 위 3개를 조합, `emitModule()` 최상위 오케스트레이션

**수용 기준**:
- parallel-move 코드가 1곳에만 존재
- `_start`가 CodeGen 내에서 생성됨
- 각 클래스가 독립 unit test 가능

---

### WS-5: ASTDump 분리
**파일**: `lib/Parser/ASTDump.cpp` (신규), `include/kern/parser/ASTDump.h` (신규)

- `dumpAST()`, `dumpExpr()`, `dumpStmt()`, `dumpPattern()` 등을 Parser.cpp에서 분리
- `ASTDumper` 클래스 또는 free function 세트
- Parser.cpp는 순수 파싱 로직만 남김
- `--dump-ast` 플래그 처리는 드라이버에서 ASTDumper 호출

**수용 기준**:
- Parser.cpp에 dump 관련 코드 0줄
- `--dump-ast` 기능 동일하게 동작

---

### WS-6: CompilerPipeline 클래스
**파일**: `include/kern/CompilerPipeline.h` (신규), `lib/Pipeline/CompilerPipeline.cpp` (신규)

- 전체 파이프라인을 캡슐화:
  ```cpp
  class CompilerPipeline {
      Arena arena_;
      StringPool strings_;
      TypeTable types_;
      DiagnosticEngine diag_;
  public:
      struct Result { bool success; IRModule ir; /* ... */ };
      Result compile(string_view source, string_view filename);
      // dump 옵션들
      void setDumpTokens(bool);
      void setDumpAST(bool);
      void setDumpIR(bool);
      void setDumpPurity(bool);
  };
  ```
- main.cpp → CLI 파싱 + CompilerPipeline 호출 + NASM/ld 실행
- Purity merge를 IRBuilder 내부로 이동 (PurityChecker 결과를 IRBuilder에 전달)
- 테스트 헬퍼에서 CompilerPipeline 재사용 가능

**수용 기준**:
- main.cpp 100줄 이하 (CLI + 외부 도구 호출만)
- unit 테스트에서 CompilerPipeline으로 full-pipeline 테스트 가능
- purity merge가 main.cpp에서 사라짐

---

### WS-7: Linear Scan Register Allocator
**파일**: WS-4의 RegisterAllocator 내부

- IR 기반 liveness analysis:
  - 각 ValueId의 정의(def) 위치와 마지막 사용(last use) 위치 계산
  - Live interval: `[def_point, last_use_point]`
- Linear scan:
  - Interval을 시작점 기준 정렬
  - 활성 interval 관리, 만료된 interval의 레지스터 해제
  - 스필: 가장 먼 last_use를 가진 interval을 스택으로
- GPR과 XMM 풀 분리 유지
- Callee-saved 레지스터 추적 (prologue/epilogue 생성에 사용)

**수용 기준**:
- 사용 종료된 레지스터가 즉시 해제됨
- 78 E2E 테스트 100% 통과 (레지스터 할당 변경에도 동일 결과)
- fib(35) 등 레지스터 압력 높은 케이스에서 스필 발생 시 정상 동작

---

### WS-8: IRType 통합 (Sema 역방향 의존 제거)
**파일**: `include/kern/ir/IRType.h` (수정)

- `irTypeFromSemaType()` 함수를 `IRType.h`에서 제거
- 변환 로직을 `IRBuilder.cpp`의 static 헬퍼로 이동
- IRType.h는 `kern/sema/` 헤더를 include하지 않음
- TypeId 도입 후: IRType을 TypeId로 통합할지 별도 유지할지 결정
  - 옵션 A: IRType 제거, IR에서도 TypeId 직접 사용 (TypeTable 참조)
  - 옵션 B: IRType을 CodeGen 전용 축소형으로 유지 (i64/f64/ptr 등 물리적 표현만)

**수용 기준**:
- `ir/IRType.h`가 `sema/` 헤더를 include하지 않음
- 빌드 시 레이어 순방향 의존만 존재: Support → Lexer → Parser → Sema → IR → CodeGen

---

### WS-9: Module 구조 확장
**파일**: `include/kern/parser/AST.h` (수정)

- Module struct 확장:
  ```cpp
  struct Module {
      string_view name;              // 모듈 이름 (M6용 예약, 현재 빈 문자열)
      FnDecl** functions;
      uint32_t fn_count;
      StructDecl** structs;          // M5a
      uint32_t struct_count;
      EnumDecl** enums;              // M5b (enum + union 통합 또는 분리)
      uint32_t enum_count;
      SourceLocation loc;
  };
  ```
- Parser가 top-level struct/enum 선언을 파싱하여 Module에 저장
- TypeChecker가 Module의 타입 선언을 먼저 등록 (two-pass: 1. 타입 등록 2. 함수 체크)
- **현재 리팩토링에서는 빈 배열 (count=0)** — M5에서 실제 파싱 추가

**수용 기준**:
- Module이 struct/enum 배열 필드를 가짐
- 기존 .kern 파일 파싱 시 count=0으로 정상 동작
- M5 코드가 Module.structs에 바로 채울 수 있는 구조

---

### WS-10: 테스트 체계 재구축
**파일**: `tests/unit/` 전체, `tests/integration/` 구조 변경

#### E2E 테스트 재구조화
- `tests/integration/` 디렉토리 구조 변경:
  ```
  tests/integration/
  ├── run_tests.sh          — 앵커 테스트만 실행 (기본)
  ├── run_all_tests.sh      — 앵커 + dump 테스트 모두 실행
  ├── *.kern + *.expected   — 앵커 테스트 (69개): exit code + error 검증만
  └── dump/                 — dump 테스트 (9개): 내부 포맷 의존, 변경 허용
      ├── dump_ast.*
      ├── dump_ir.*
      ├── dump_ir_tail.*
      ├── dump_tokens.*
      ├── dump_purity.*
      ├── dump_purity_impure.*
      ├── dump_purity_tailrec.*
      ├── intrinsic_propagation.*
      └── intrinsic_purity.*
  ```
- **앵커 테스트 (69개)**: 리팩토링 전후 100% 통과 필수. 실패 = 회귀 버그
- **dump 테스트 (9개)**: 리팩토링 시 내부 포맷 변경에 맞춰 업데이트 허용. 단, dump 기능 자체가 동작해야 함

#### Unit 테스트 재작성
- 새 인터페이스에 맞춰 360개 unit 테스트를 재작성
- WS-6 CompilerPipeline 활용하여 테스트 헬퍼 단순화
- 테스트 카테고리:
  - `support/`: Arena, Diagnostic, **StringPool** (신규)
  - `lexer/`: Lexer (변경 최소)
  - `parser/`: Parser, **ASTDump** (분리)
  - `sema/`: TypeChecker (**TypeId 기반**), PurityChecker
  - `ir/`: IRBuilder (**tagged union IRInstr**)
  - `codegen/`: **RegisterAllocator, ABIHandler, InstructionEmitter, CodeGen** (분리)
  - `pipeline/`: **CompilerPipeline** 통합 테스트 (신규)
- dump 포맷 검증은 unit 테스트 영역으로 이관 (ASTDump, IRDump 등)
- 기존 테스트의 assertion 의미는 최대한 보존 (같은 시나리오, 새 API)

**수용 기준**:
- 69개 앵커 E2E 테스트 100% 통과 (불변)
- 9개 dump E2E 테스트 업데이트 후 통과
- Unit 테스트 수 ≥ 360 (감소 불가)
- 모든 새 클래스(StringPool, TypeTable, RegisterAllocator 등)에 전용 테스트 존재
- dump 포맷 검증이 unit 테스트에 포함됨

---

## 4. 의존관계 및 실행 순서

```
WS-1 (StringPool)  ──┐
                      ├──→ WS-2 (TypeSystem) ──→ WS-8 (IRType 통합)
                      │                              │
WS-5 (ASTDump 분리) ──┤                              ├──→ WS-3 (IRInstr 재설계)
                      │                              │
WS-9 (Module 확장)  ──┤                              ├──→ WS-4 (CodeGen 분리)
                      │                                       │
                      │                                       ├──→ WS-7 (Linear Scan)
                      │                                       │
                      └──────────────────────────────────────→ WS-6 (Pipeline 클래스)
                                                              │
                                                              └──→ WS-10 (테스트 재작성)
```

**Critical Path**: WS-1 → WS-2 → WS-8 → WS-3 → WS-4 → WS-7 → WS-6 → WS-10

**병렬 가능**: WS-1, WS-5, WS-9는 독립적으로 동시 착수 가능

---

## 5. 리스크 및 완화 전략

| 리스크 | 확률 | 완화 |
|--------|------|------|
| E2E 테스트 깨짐 | 높음 | git tag로 M4 스냅샷 보존. 각 WS 완료마다 E2E 확인 |
| TypeId 전환 시 cascading 변경 | 높음 | WS-2에서 타입 alias (`using Type = TypeId`) 중간 단계 활용 |
| Linear Scan이 기존 출력과 다른 레지스터 할당 | 중간 | E2E는 exit code/출력만 검증 → 레지스터 변경 자체는 무관 |
| CodeGen 분리 시 상태 공유 복잡도 | 중간 | CodeGenContext 구조체로 공유 상태 명시적 전달 |
| M5 코드와의 머지 충돌 | 낮음 (M5 완료 후 시작) | M5 완료 → 리팩토링 시작 순서 확정 |

---

## 6. 미결정 사항 (Open Questions)

1. **IRType vs TypeId 통합**: IR에서 TypeId를 직접 쓸지, CodeGen 전용 축소형 IRType을 유지할지 → WS-8 착수 시 결정
2. **EnumDecl과 UnionDecl 분리 vs 통합**: M5b 설계에 따라 결정 → M5 완료 후 확정
3. **CompilerPipeline의 NASM/ld 호출 포함 여부**: Pipeline이 어셈블리 텍스트까지만 vs 바이너리까지 → WS-6 착수 시 결정
4. **callee_name interning 시점**: 파서에서 함수 이름 intern vs IRBuilder에서 intern → WS-1 + WS-3 설계 시 결정

---

## 7. 다음 단계

1. **M5 개발 완료** (현재 진행 중)
2. M5 완료 후 이 요구사항 문서를 M5 결과에 맞춰 업데이트
3. `/sc:design`으로 각 워크스트림의 상세 기술 설계
4. `/sc:workflow`로 구현 워크플로우 생성
5. 빅뱅 리팩토링 실행 (E2E 앵커 유지하며)
