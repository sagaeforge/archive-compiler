# Kern Compiler — M1 Implementation Workflow

> 생성일: 2026-03-01
> 상태: Plan
> 목표: **피보나치 재귀 함수를 컴파일하여 macOS에서 실행**

---

## 수용 기준 (Acceptance Criteria)

```kern
fn fib(n: i64) -> i64 {
    if n <= 1 { n }
    else { fib(n - 1) + fib(n - 2) }
}

fn main() -> i64 {
    fib(10)
}
```

위 코드를 `kernc`에 입력하면:
- [x] 렉싱 → 파싱 → 의미분석 → IR → 코드젠 파이프라인 동작
- [x] macOS (x86-64) 네이티브 바이너리 생성
- [x] 실행 시 `fib(10)` = 55를 exit code로 반환
- [x] 재귀 함수 호출 정상 동작

---

## 전체 페이즈 개요

```
Phase 0: 프로젝트 부트스트랩           ██░░░░░░░░  CMake + 디렉토리 + 테스트 프레임워크
Phase 1: Support 라이브러리            ███░░░░░░░  Arena, Diagnostic, SourceLocation
Phase 2: Lexer                        ████░░░░░░  토큰화 + 에러 수집
Phase 3: Parser                       █████░░░░░  AST 구축 (재귀 하강 + Pratt)
Phase 4: Sema (최소)                   ██████░░░░  타입 검사 (i64만)
Phase 5: IR 생성                       ███████░░░  AST → SSA IR
Phase 6: CodeGen                      ████████░░  IR → x86 어셈블리
Phase 7: 드라이버 + 통합               █████████░  kernc CLI, nasm+ld 호출
Phase 8: 검증 + 정리                   ██████████  E2E 테스트, 리팩토링
```

---

## Phase 0: 프로젝트 부트스트랩

### 목표
CMake 빌드 시스템, 디렉토리 구조, 테스트 프레임워크 설정

### 태스크

| # | 태스크 | 산출물 | 의존성 |
|---|--------|--------|--------|
| 0.1 | 루트 `CMakeLists.txt` 생성 (C++20, 경고 플래그) | `CMakeLists.txt` | — |
| 0.2 | 디렉토리 구조 생성 | `include/kern/`, `lib/`, `tools/`, `tests/` | — |
| 0.3 | GoogleTest 통합 (`FetchContent`) | `cmake/` + 테스트 타겟 | 0.1 |
| 0.4 | 빈 `kernc/main.cpp` + 빌드 확인 | `tools/kernc/main.cpp` | 0.1, 0.2 |
| 0.5 | CI-free smoke test: `cmake --build . && ./kernc --help` | 빌드 성공 | 0.4 |

### 체크포인트
- `cmake -B build && cmake --build build` 성공
- `./build/kernc` 실행 가능 (아무것도 안 해도 OK)
- GoogleTest 샘플 테스트 통과

---

## Phase 1: Support 라이브러리

### 목표
모든 컴파일러 단계에서 사용하는 공통 인프라 구축

### 태스크

| # | 태스크 | 산출물 | 의존성 |
|---|--------|--------|--------|
| 1.1 | `SourceLocation` 구현 (file, line, col) | `include/kern/support/SourceLocation.h` | 0.2 |
| 1.2 | `Diagnostic` + `DiagnosticEngine` 구현 | `include/kern/support/Diagnostic.h`, `lib/Support/Diagnostic.cpp` | 1.1 |
| 1.3 | `Arena` 범프 할당자 구현 | `include/kern/support/Arena.h`, `lib/Support/Arena.cpp` | 0.2 |
| 1.4 | Support 유닛 테스트 | `tests/unit/support/` | 1.1–1.3 |

### 설계 결정
- `SourceLocation`: 간단한 struct `{uint32_t line; uint32_t col; const char* filename;}`
- `Arena`: 4096바이트 블록 단위 할당, `make<T>(args...)` 템플릿
- `DiagnosticEngine`: Error/Warning/Note 레벨, 위치 정보 포함, stderr 출력

### 체크포인트
- Arena에서 100개 객체 할당 → 해제 시 크래시 없음
- Diagnostic으로 에러 보고 → 포맷팅된 출력 확인

---

## Phase 2: Lexer

### 목표
Kern 소스 텍스트를 Token Stream으로 변환

### 태스크

| # | 태스크 | 산출물 | 의존성 |
|---|--------|--------|--------|
| 2.1 | `TokenKind` 열거형 + `Token` 구조체 | `include/kern/lexer/Token.h` | 1.1 |
| 2.2 | 키워드 해시맵 (`fn`, `val`, `var`, `if`, `else`, `return`, `and`, `or`, `not`) | `Token.h` 내 또는 `Lexer.cpp` | 2.1 |
| 2.3 | `Lexer` 클래스 구현 — 핸드라이튼 | `include/kern/lexer/Lexer.h`, `lib/Lexer/Lexer.cpp` | 2.1, 2.2 |
| 2.4 | 정수 리터럴 스캔 (10진, 0x 헥스) | `Lexer.cpp` | 2.3 |
| 2.5 | 식별자 + 키워드 인식 | `Lexer.cpp` | 2.3 |
| 2.6 | 연산자 스캔 (`+`, `-`, `*`, `/`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `->`, `=>`, `|>`) | `Lexer.cpp` | 2.3 |
| 2.7 | 구분자 스캔 (`(`, `)`, `{`, `}`, `:`, `,`, `;`) | `Lexer.cpp` | 2.3 |
| 2.8 | 주석 스킵 (`//` 한 줄, `/* */` 블록) | `Lexer.cpp` | 2.3 |
| 2.9 | 에러 토큰 발행 (인식 불가 문자) | `Lexer.cpp` | 2.3, 1.2 |
| 2.10 | Lexer 유닛 테스트 | `tests/unit/lexer/` | 2.3–2.9 |

### M1 토큰 범위

```
리터럴:    IntLit
식별자:    Ident
키워드:    fn val var if else return and or not
연산자:    + - * / == != < <= > >= -> => |> = &
구분자:    ( ) { } : , ;
특수:      Eof, Error
```

### 체크포인트
- `fn fib(n: i64) -> i64 { ... }` 를 토큰 스트림으로 변환
- 각 토큰의 `kind`, `text`, `loc` (line/col) 검증
- 에러 문자 입력 시 `Error` 토큰 발행 + 에러 수집

---

## Phase 3: Parser

### 목표
Token Stream을 AST로 변환

### 태스크

| # | 태스크 | 산출물 | 의존성 |
|---|--------|--------|--------|
| 3.1 | AST 노드 정의 (`Expr`, `FnDecl`, `Param`, `TypeRef`) | `include/kern/parser/AST.h` | 1.3 |
| 3.2 | `Parser` 클래스 스켈레톤 (peek, advance, expect) | `include/kern/parser/Parser.h`, `lib/Parser/Parser.cpp` | 2.3, 3.1 |
| 3.3 | 타입 파싱 (`i64`, `bool`, `Unit` 등 Named 타입) | `Parser.cpp` | 3.2 |
| 3.4 | 함수 선언 파싱 (`fn name(params) -> type { body }`) | `Parser.cpp` | 3.2, 3.3 |
| 3.5 | Pratt parser — 이항 연산자 (`+`, `-`, `*`, `/`, `==`, `!=`, `<`, `<=`, `>`, `>=`) | `Parser.cpp` | 3.2 |
| 3.6 | Pratt parser — 단항 연산자 (`-`, `not`) | `Parser.cpp` | 3.5 |
| 3.7 | Primary 표현식 (정수 리터럴, 식별자, 괄호) | `Parser.cpp` | 3.5 |
| 3.8 | 함수 호출 표현식 (`name(args)`) | `Parser.cpp` | 3.7 |
| 3.9 | if-else 표현식 (`if cond { } else { }`) | `Parser.cpp` | 3.2 |
| 3.10 | 블록 표현식 (`{ stmts; expr }`) | `Parser.cpp` | 3.2 |
| 3.11 | val 바인딩 (`val x: i64 = expr`) | `Parser.cpp` | 3.2, 3.3 |
| 3.12 | return 문 (선택적) | `Parser.cpp` | 3.2 |
| 3.13 | 모듈(프로그램) 파싱 — 함수 선언 리스트 | `Parser.cpp` | 3.4 |
| 3.14 | 논리 연산자 (`and`, `or`) — Pratt 바인딩 파워 | `Parser.cpp` | 3.5 |
| 3.15 | 에러 복구 — 동기화 포인트 (`fn` 키워드) | `Parser.cpp` | 3.2, 1.2 |
| 3.16 | Parser 유닛 테스트 | `tests/unit/parser/` | 3.4–3.14 |

### Pratt 바인딩 파워 테이블

```
우선순위 (낮은 → 높은):
  1. or                    → { 2,  3}
  2. and                   → { 4,  5}
  3. ==, !=                → { 6,  7}
  4. <, <=, >, >=          → { 8,  9}
  5. +, -                  → {10, 11}
  6. *, /                  → {20, 21}

단항 (접두사):
  -, not                   → rBP = 25
```

### 체크포인트
- 피보나치 소스 → AST 트리 생성
- AST 덤프 (pretty-print) 로 구조 시각 확인
- `fn main() -> i64 { fib(10) }` 파싱 성공

---

## Phase 4: Sema (최소 의미분석)

### 목표
AST의 타입 정합성 검증 (M1: i64 단일 타입)

### 태스크

| # | 태스크 | 산출물 | 의존성 |
|---|--------|--------|--------|
| 4.1 | 심볼 테이블 (함수 이름 → 시그니처 매핑) | `include/kern/sema/SymbolTable.h` | 3.1 |
| 4.2 | `TypeChecker` 구현 — 함수 본문 타입 검증 | `include/kern/sema/TypeChecker.h`, `lib/Sema/TypeChecker.cpp` | 4.1, 3.1 |
| 4.3 | 산술/비교 표현식 타입 검사 (`i64 op i64 → i64` / `bool`) | `TypeChecker.cpp` | 4.2 |
| 4.4 | 함수 호출 타입 검사 (인자 개수, 인자 타입, 반환 타입) | `TypeChecker.cpp` | 4.2, 4.1 |
| 4.5 | 미선언 식별자 에러 보고 | `TypeChecker.cpp` | 4.2, 1.2 |
| 4.6 | if-else 분기 타입 일치 확인 | `TypeChecker.cpp` | 4.2 |
| 4.7 | Sema 유닛 테스트 | `tests/unit/sema/` | 4.2–4.6 |

### M1 타입 규칙 (단순화)

```
- 모든 정수 리터럴: i64
- 산술 연산: i64 op i64 → i64
- 비교 연산: i64 cmp i64 → bool
- if 조건: bool 필수
- if-else 반환: 양쪽 타입 일치 필수
- 함수 반환: 본문 마지막 표현식 타입 == 선언 반환 타입
```

### 체크포인트
- 올바른 피보나치 코드 → 에러 없음
- `fn bad() -> i64 { true }` → 타입 불일치 에러
- `fn bad() -> i64 { unknown(1) }` → 미선언 함수 에러

---

## Phase 5: IR 생성

### 목표
Typed AST를 SSA 형태의 Kern IR로 변환

### 태스크

| # | 태스크 | 산출물 | 의존성 |
|---|--------|--------|--------|
| 5.1 | IR 데이터 구조 정의 (`IRModule`, `IRFunction`, `IRBlock`, `IRInstr`) | `include/kern/ir/KernIR.h` | — |
| 5.2 | `IRBuilder` 클래스 스켈레톤 | `include/kern/ir/IRBuilder.h`, `lib/IR/IRBuilder.cpp` | 5.1 |
| 5.3 | 상수 정수 → `ConstInt` IR 명령 | `IRBuilder.cpp` | 5.2 |
| 5.4 | 산술/비교 연산 → `Add`, `Sub`, `Mul`, `Div`, `ICmp*` | `IRBuilder.cpp` | 5.2 |
| 5.5 | 함수 호출 → `Call` IR 명령 | `IRBuilder.cpp` | 5.2 |
| 5.6 | if-else → `CondBranch` + 블록 분기 | `IRBuilder.cpp` | 5.2 |
| 5.7 | 함수 매개변수 → 블록 인자 매핑 | `IRBuilder.cpp` | 5.2 |
| 5.8 | `Ret` 명령 생성 | `IRBuilder.cpp` | 5.2 |
| 5.9 | IR 덤프 (텍스트 형식으로 출력) | `IRBuilder.cpp` 또는 별도 | 5.2 |
| 5.10 | IR 유닛 테스트 | `tests/unit/ir/` | 5.3–5.8 |

### IR 생성 핵심 패턴

```
AST 순회 방식: 재귀적 visit

visitFnDecl(fn):
  새 IRFunction 생성
  entry 블록 생성
  매개변수 → entry 블록 인자로 등록
  visitExpr(fn.body)
  Ret 명령 추가

visitExpr(expr):
  match expr.kind:
    IntLit  → ConstInt 명령, ValueId 반환
    BinOp   → lhs/rhs visit → Add/Sub/... 명령
    Call    → 인자 visit → Call 명령
    If      → 조건 visit → CondBranch → then/else 블록 → merge 블록
    Ident   → 심볼 테이블에서 ValueId 조회
```

### 체크포인트
- 피보나치 AST → IR 텍스트 덤프가 ARCHITECTURE.md의 예시와 구조적으로 일치
- entry, base_case, recurse 블록이 올바르게 생성됨
- SSA 속성 유지: 각 ValueId가 정확히 한 번 정의됨

---

## Phase 6: CodeGen

### 목표
Kern IR을 x86-64 어셈블리(NASM)로 변환

### 태스크

| # | 태스크 | 산출물 | 의존성 |
|---|--------|--------|--------|
| 6.1 | `MachineInstr`, `Operand` 데이터 구조 | `include/kern/codegen/InstrSelect.h` | — |
| 6.2 | 명령어 선택 — IR opcode → x86 명령 매핑 | `lib/CodeGen/InstrSelect.cpp` | 6.1, 5.1 |
| 6.3 | 함수 프롤로그/에필로그 생성 (`push rbp`, `mov rbp, rsp`, ...) | `InstrSelect.cpp` | 6.2 |
| 6.4 | 함수 호출 규약 구현 (System V AMD64 ABI) | `InstrSelect.cpp` | 6.2 |
| 6.5 | `CondBranch` → `cmp` + `jle`/`je`/... | `InstrSelect.cpp` | 6.2 |
| 6.6 | 재귀 호출 → `call` + callee-saved 레지스터 보존 | `InstrSelect.cpp` | 6.4 |
| 6.7 | Linear Scan 레지스터 할당 구현 | `include/kern/codegen/RegAlloc.h`, `lib/CodeGen/RegAlloc.cpp` | 6.1 |
| 6.8 | 스택 spill/reload 처리 | `RegAlloc.cpp` | 6.7 |
| 6.9 | `AsmEmitter` — NASM 텍스트 출력 | `include/kern/codegen/AsmEmitter.h`, `lib/CodeGen/AsmEmitter.cpp` | 6.1 |
| 6.10 | macOS 특이사항: `_main` 심볼, `_` 접두사 | `AsmEmitter.cpp` | 6.9 |
| 6.11 | CodeGen 유닛 테스트 | `tests/unit/codegen/` | 6.2–6.9 |

### x86-64 호출 규약 (System V AMD64 ABI)

```
인자 전달:    rdi, rsi, rdx, rcx, r8, r9 (순서대로)
반환값:       rax
Caller-saved: rax, rcx, rdx, rsi, rdi, r8-r11
Callee-saved: rbx, r12-r15, rbp
스택 정렬:    16바이트 정렬 필수 (call 전)
```

### 피보나치 목표 어셈블리

```nasm
section .text
global _main

_fib:
    push rbp
    mov  rbp, rsp
    push rbx                 ; callee-saved

    mov  rbx, rdi            ; n 보존

    cmp  rbx, 1
    jle  .L_fib_base

    ; fib(n-1)
    lea  rdi, [rbx - 1]
    call _fib
    push rax                 ; fib(n-1) 결과 보존

    ; fib(n-2)
    lea  rdi, [rbx - 2]
    call _fib
    pop  rcx
    add  rax, rcx

    jmp  .L_fib_done

.L_fib_base:
    mov  rax, rbx

.L_fib_done:
    pop  rbx
    pop  rbp
    ret

_main:
    push rbp
    mov  rbp, rsp

    mov  rdi, 10             ; fib(10)
    call _fib

    ; exit(rax) via syscall
    mov  rdi, rax            ; exit code = fib(10)
    mov  rax, 0x02000001     ; macOS exit syscall
    syscall
```

### M1 CodeGen 단순화 전략

M1은 정확성 우선. 다음 단순화를 적용:

1. **레지스터 할당 단순화**: 피보나치는 변수가 적으므로 Linear Scan으로 충분
2. **스택 관리**: 재귀 호출 간 값 보존은 push/pop 으로 처리
3. **최적화 없음**: 상수 접기, 죽은 코드 제거 등 M2 이후
4. **i64만**: 모든 값이 64비트 정수 → 레지스터 크기 고정

### 체크포인트
- IR → NASM 출력 파일 생성
- 생성된 어셈블리가 사람이 읽을 수 있는 수준
- `nasm -f macho64 output.asm -o output.o` 성공

---

## Phase 7: 드라이버 + 통합

### 목표
`kernc` CLI로 전체 파이프라인을 실행하여 네이티브 바이너리 생성

### 태스크

| # | 태스크 | 산출물 | 의존성 |
|---|--------|--------|--------|
| 7.1 | `kernc` CLI 인자 파싱 (입력 파일, -o 출력) | `tools/kernc/main.cpp` | 0.4 |
| 7.2 | 파이프라인 오케스트레이션 (Lexer → Parser → Sema → IR → CodeGen) | `main.cpp` | Phase 2–6 |
| 7.3 | 외부 도구 호출: `nasm -f macho64` | `main.cpp` | 7.2 |
| 7.4 | 외부 도구 호출: `ld -o output output.o -lSystem` | `main.cpp` | 7.3 |
| 7.5 | 에러 처리 — 각 단계 실패 시 적절한 에러 메시지 + exit code | `main.cpp` | 7.2 |
| 7.6 | `--dump-tokens`, `--dump-ast`, `--dump-ir` 디버그 플래그 | `main.cpp` | 7.2 |
| 7.7 | 통합 테스트 스크립트 | `tests/integration/` | 7.4 |

### CLI 사용 예시

```bash
# 기본 사용
$ kernc fib.kern -o fib
$ ./fib; echo $?
55

# 디버그 모드
$ kernc fib.kern --dump-tokens    # 토큰 스트림 출력
$ kernc fib.kern --dump-ast       # AST 트리 출력
$ kernc fib.kern --dump-ir        # IR 텍스트 출력
$ kernc fib.kern -S               # 어셈블리만 생성 (.asm)
```

### macOS 링킹 참고

```bash
# macOS에서의 링킹 (Apple ld)
nasm -f macho64 output.asm -o output.o
ld output.o -o output -lSystem -syslibroot $(xcrun --show-sdk-path) -e _main

# 또는 최소 링킹 (syscall 직접 사용 시)
ld output.o -o output -static -e _main
```

### 체크포인트
- `kernc fib.kern -o fib && ./fib; echo $?` → 55
- 각 `--dump-*` 플래그가 적절한 중간 결과 출력
- 잘못된 소스 입력 시 에러 메시지 출력 + 비정상 종료 코드

---

## Phase 8: 검증 + 정리

### 목표
E2E 테스트, 엣지 케이스 검증, 코드 정리

### 태스크

| # | 태스크 | 산출물 | 의존성 |
|---|--------|--------|--------|
| 8.1 | E2E 테스트: `fib.kern` (피보나치) | `tests/integration/fib.kern` | 7.4 |
| 8.2 | E2E 테스트: 단순 함수 (상수 반환) | `tests/integration/const.kern` | 7.4 |
| 8.3 | E2E 테스트: 중첩 함수 호출 | `tests/integration/nested_call.kern` | 7.4 |
| 8.4 | 에러 케이스 테스트: 구문 에러 | `tests/integration/errors/` | 7.5 |
| 8.5 | 에러 케이스 테스트: 타입 에러 | `tests/integration/errors/` | 7.5 |
| 8.6 | 에러 메시지 품질 검토 (help 메시지 포함) | — | 8.4, 8.5 |
| 8.7 | 코드 정리 + 주석 보강 | — | 전체 |
| 8.8 | README 업데이트 (빌드 방법, 사용법) | `README.md` | 8.1 |

### E2E 테스트 케이스

```kern
// tests/integration/const.kern — 상수 반환
fn main() -> i64 { 42 }
// 기대: exit code 42

// tests/integration/add.kern — 기본 산술
fn add(a: i64, b: i64) -> i64 { a + b }
fn main() -> i64 { add(20, 22) }
// 기대: exit code 42

// tests/integration/nested_call.kern — 중첩 호출
fn double(x: i64) -> i64 { x + x }
fn quad(x: i64) -> i64 { double(double(x)) }
fn main() -> i64 { quad(5) }
// 기대: exit code 20

// tests/integration/fib.kern — 피보나치 재귀
fn fib(n: i64) -> i64 {
    if n <= 1 { n }
    else { fib(n - 1) + fib(n - 2) }
}
fn main() -> i64 { fib(10) }
// 기대: exit code 55
```

### 체크포인트
- 모든 E2E 테스트 통과
- 모든 유닛 테스트 통과
- 에러 메시지가 도움이 됨 (위치 + 설명 + help)

---

## 의존성 그래프

```
Phase 0 (부트스트랩)
  │
  ▼
Phase 1 (Support) ─────────────────────────────────┐
  │                                                │
  ▼                                                │
Phase 2 (Lexer)                                    │
  │                                                │
  ▼                                                │
Phase 3 (Parser) ◄─────────────────────────────────┘
  │
  ▼
Phase 4 (Sema)
  │
  ▼
Phase 5 (IR 생성)
  │
  ▼
Phase 6 (CodeGen)
  │
  ▼
Phase 7 (드라이버 + 통합)
  │
  ▼
Phase 8 (검증 + 정리)
```

**병렬화 가능 구간:**
- Phase 1의 각 Support 모듈은 독립적으로 개발 가능
- Phase 5(IR 데이터 구조)와 Phase 6(MachineInstr 데이터 구조)의 정의 단계는 병렬 가능
- Phase 8의 테스트 케이스 작성은 Phase 7과 병렬 가능

---

## 리스크 & 대응

| 리스크 | 영향 | 대응 |
|--------|------|------|
| macOS 링킹 이슈 (Apple ld 특이사항) | 바이너리 생성 실패 | Phase 7에서 조기 확인. syscall 직접 사용으로 libc 의존성 최소화 |
| 레지스터 할당 버그 (callee-saved 미보존) | 재귀 호출 시 값 손상 | 피보나치가 좋은 스트레스 테스트. 단계별 어셈블리 검증 |
| SSA 변환 복잡도 (if-else 합류점) | IR 생성 지연 | M1은 블록 인자 방식으로 단순화. phi 노드 없음 |
| Pratt parser 구현 오류 (우선순위) | 잘못된 AST | 바인딩 파워 테이블 유닛 테스트로 개별 검증 |

---

## 구현 순서 요약

```
0.1 → 0.2 → 0.3 → 0.4 → 0.5                    [Phase 0: 부트스트랩]
  → 1.1 → 1.2 → 1.3 → 1.4                       [Phase 1: Support]
    → 2.1 → 2.2 → 2.3 → 2.4~2.9 → 2.10         [Phase 2: Lexer]
      → 3.1 → 3.2 → 3.3~3.14 → 3.15 → 3.16    [Phase 3: Parser]
        → 4.1 → 4.2 → 4.3~4.6 → 4.7            [Phase 4: Sema]
          → 5.1 → 5.2 → 5.3~5.8 → 5.9 → 5.10  [Phase 5: IR]
            → 6.1 → 6.2~6.6 → 6.7~6.8 → 6.9~6.10 → 6.11  [Phase 6: CodeGen]
              → 7.1 → 7.2~7.6 → 7.7             [Phase 7: 드라이버]
                → 8.1~8.8                         [Phase 8: 검증]
```

---

## 다음 단계

워크플로우 승인 후:
1. `/sc:implement Phase 0` — 프로젝트 부트스트랩 실행
2. 각 Phase 완료 시 체크포인트 검증
3. Phase 단위로 커밋
