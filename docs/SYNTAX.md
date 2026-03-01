# Kern Language — Syntax Specification

> 생성일: 2026-03-01
> 상태: Draft v2 (어노테이션 제거, 전면 자동 추론 모델)

---

## 1. 설계 원칙

| 원칙 | 설명 |
|------|------|
| **코드만 작성한다** | 프로그래머는 로직만 작성. 메타데이터는 전부 컴파일러가 추론 |
| **읽기 쉬운 코드** | 비전의 핵심. 코드가 의도를 명확히 전달해야 함 |
| **Kotlin 베이스** | 전체적 느낌과 인체공학은 Kotlin에서 출발 |
| **어노테이션 없음** | `@` 자체가 언어에 존재하지 않음. 순수성, 이펙트, 재귀 전부 컴파일러가 판단 |
| **LSP 중심 경험** | 추론된 모든 속성을 LSP가 인레이 힌트, 색상 구분, 호출 체인으로 시각화 |
| **파이프로 흐름** | `|>` 연산자로 데이터 체인을 자연스럽게 표현 |
| **패턴 매칭 중심** | 함수 정의 수준 + match 표현식 모두 지원 |
| **모나드는 값** | 모나드(Result, Maybe)는 성공/실패를 담는 독립적 타입. 이펙트와 무관 |
| **명시적 포인터** | 역참조 `*ptr`, 참조 `&x` 모두 명시적. 자동 역참조 없음 |
| **읽기 쉬운 논리** | `and`/`or`/`not` 키워드. `&&`/`||`/`!` 없음 |
| **모호하면 에러** | 파서/컴파일러가 추측하지 않음. 모호한 코드는 컴파일 에러 + 상세한 에러 메시지로 해결 유도 |

---

## 2. 기본 문법 요소

### 2.1 바인딩 — val / var

```kern
val x: i64 = 42          // 불변 바인딩
var y: i64 = 0            // 가변 바인딩 — 함수가 불순해짐
y = y + 1                 // OK — var는 재할당 가능
```

- `val` — 불변. 한 번 바인딩되면 변경 불가
- `var` — 가변. 재할당 가능. **var가 있으면 그 함수는 순수하지 않음**
- 타입은 항상 명시적: `이름: 타입 = 값`
- `var` 사용 시 컴파일 **경고** (val + 재귀로 리팩터링 제안)

### 2.2 블록 구조

```kern
// 중괄호 { } 블록
fn example() -> i64 {
    val a: i64 = 10
    val b: i64 = 20
    a + b                  // 마지막 표현식이 반환값
}
```

### 2.3 세미콜론

```kern
val x: i64 = 42           // 줄바꿈이 문장 구분
val a: i64 = 1; val b: i64 = 2   // 한 줄에 여러 문장일 때만 ; 사용
```

---

## 3. 함수

### 3.1 함수 선언 — 컴파일러가 모든 속성을 자동 추론

```kern
fn add(a: i64, b: i64) -> i64 {
    a + b
}
// 컴파일러 추론: pure, distributable
// LSP: "pure distributable"

fn fib(n: i64) -> i64 {
    if n <= 1 { n }
    else { fib(n - 1) + fib(n - 2) }
}
// 컴파일러 추론: pure, recursive, tail-rec 가능하면 자동 최적화
// LSP: "pure recursive"
```

**프로그래머는 순수성, 이펙트, 재귀 등 어떤 속성도 선언하지 않음.**
컴파일러가 함수 본문을 분석해서 전부 알아낸다.

### 3.2 불순 함수 — var 또는 외부 상호작용

```kern
// var 사용 → 컴파일러가 불순으로 추론 + 경고
fn counter() -> i64 {
    var x: i64 = 0         // WARNING: var makes function impure
    x = x + 1
    x
}
// 컴파일러 추론: impure (local mutation), not distributable
// LSP: "impure(mut) warning"

// 외부 상호작용 → 컴파일러가 IO로 추론
fn initSerial(port: u16) -> Unit {
    outb(port + 1, 0x00)   // outb는 intrinsic (IO)
    outb(port + 3, 0x80)
}
// 컴파일러 추론: impure (IO), pinned
// LSP: "io pinned"
```

### 3.3 함수 정의 수준 패턴 매칭

```kern
fn fib(0) -> i64 { 0 }
fn fib(1) -> i64 { 1 }
fn fib(n: i64) -> i64 {
    fib(n - 1) + fib(n - 2)
}
```

### 3.4 람다 / 익명 함수

```kern
val double: (i64) -> i64 = { x: i64 => x * 2 }

// 타입 추론이 가능한 경우 간결하게
val double = { x => x * 2 }

// 여러 매개변수
val add = { a: i64, b: i64 => a + b }
```

### 3.5 고차 함수

```kern
fn apply(f: (i64) -> i64, x: i64) -> i64 {
    f(x)
}

fn compose(f: (i64) -> i64, g: (i64) -> i64) -> (i64) -> i64 {
    { x => f(g(x)) }
}
```

### 3.6 Intrinsic (내장 함수)

하드웨어 접근 등 컴파일러가 본문을 분석할 수 없는 함수. 컴파일러가 이것들을 **불순의 원천**으로 인식.

```kern
fn outb(port: u16, val: u8) -> Unit = intrinsic
fn inb(port: u16) -> u8 = intrinsic
```

이 함수를 호출하는 모든 함수는 자동으로 불순 추론됨.

---

## 4. 파이프 연산자 |>

데이터 흐름을 왼쪽에서 오른쪽으로 표현. **컴퓨팅 체인의 문법적 기반.**

### 4.1 값 파이프

```kern
val result: i64 = 10
    |> add(3)          // add(10, 3) → 13
    |> multiply(2)     // multiply(13, 2) → 26
    |> subtract(1)     // subtract(26, 1) → 25
```

**규칙:** `a |> f(b)` 는 `f(a, b)` 로 변환. 좌변의 값이 첫 번째 인자로 삽입.

### 4.2 모나드 파이프

```kern
fn main() -> Result<Unit, Error> {
    readLine()
        |> bind(parseI64)
        |> bind({ n => print(fib(n)) })
}
```

### 4.3 복합 파이프라인

```kern
fn processData(data: List<i64>) -> i64 {
    data
        |> filter({ x => x > 0 })
        |> map({ x => x * 2 })
        |> fold(0, add)
}
```

---

## 5. 패턴 매칭

### 5.1 match 표현식

```kern
fn describe(n: i64) -> String {
    match n {
        0 => "zero"
        1 => "one"
        n if n < 0 => "negative"
        n => "other"
    }
}
```

### 5.2 구조 분해

```kern
fn first(pair: Pair<i64, i64>) -> i64 {
    match pair {
        Pair(a, _) => a
    }
}
```

### 5.3 중첩 패턴

```kern
fn eval(expr: Expr) -> i64 {
    match expr {
        Literal(n) => n
        Add(Literal(a), Literal(b)) => a + b
        Add(a, b) => eval(a) + eval(b)
    }
}
```

---

## 6. 컴파일러 자동 추론 시스템

**어노테이션 없음.** 모든 함수 속성은 컴파일러가 본문 분석으로 추론.

### 6.1 추론되는 속성들

| 속성 | 의미 | 판단 기준 |
|------|------|----------|
| `pure` | 순수 함수 | var 미사용 + 불순 함수 미호출 + 외부 상태 미접근 |
| `impure(mut)` | 로컬 변이 | var 사용 |
| `impure(io)` | 외부 상호작용 | intrinsic/IO 함수 호출 |
| `impure(mem)` | 메모리 조작 | 메모리 할당/해제 함수 호출 |
| `recursive` | 재귀 함수 | 자기 자신 호출 |
| `tail-rec` | 꼬리 재귀 | 재귀 호출이 꼬리 위치 → 자동 최적화 |
| `distributable` | 노드 분배 가능 | 순수 + 캡처 없음 |
| `pinned` | 현재 노드 고정 | impure(io) 또는 impure(mem) |

### 6.2 추론 전파 규칙

```
함수 A가 함수 B를 호출하면:
  - B가 impure(io)  →  A도 impure(io)
  - B가 impure(mem) →  A도 impure(mem)
  - B가 impure(mut) →  A에는 전파되지 않음 (B의 로컬 뮤테이션은 B 내부 문제)

함수에 var가 있으면:
  - 해당 함수만 impure(mut)
  - 호출자에게는 전파되지 않음 (외부에서 보면 순수할 수 있음)
  - 단, 컴파일 경고 발생
```

**핵심 인사이트: var의 impure(mut)는 전파되지 않는다.**

```kern
// var 사용 → impure(mut) + 경고
fn sumImperative(n: i64) -> i64 {
    var acc: i64 = 0
    // ... (미래: 루프가 생기면)
    acc
}

// 이 함수를 호출하는 함수는?
fn double(n: i64) -> i64 {
    sumImperative(n) * 2
}
// 컴파일러: double은 pure (sumImperative의 mut는 전파 안 됨)
// 왜? sumImperative의 var는 로컬이므로 외부에서 보면 순수하게 동작
```

**반면 IO는 전파된다:**

```kern
fn outb(port: u16, val: u8) -> Unit = intrinsic  // impure(io)

fn initSerial(port: u16) -> Unit {
    outb(port + 1, 0x00)    // outb 호출
}
// 컴파일러: initSerial은 impure(io) — outb에서 전파됨
```

### 6.3 LSP 표시 예시

```
fn add(a: i64, b: i64) -> i64          ← pure, distributable
fn fib(n: i64) -> i64                  ← pure, recursive
fn counter() -> i64                    ← impure(mut) ⚠️
fn initSerial(port: u16) -> Unit       ← impure(io), pinned
fn readFile(path: String) -> Result<String, Error>  ← impure(io)
fn parse(s: String) -> Result<i64, Error>           ← pure
```

---

## 7. 모나드 시스템

모나드는 **독립적인 값 타입**. 이펙트와 무관. 순수 함수에서도 사용 가능.

### 7.1 Result<T, E>

성공 또는 실패를 담는 컨테이너.

```kern
fn parseI64(s: String) -> Result<i64, Error> {
    // 순수 함수지만 실패할 수 있음
    // ...
}

fn readFile(path: String) -> Result<String, Error> {
    // 불순 함수(IO) + 실패 가능
    // ...
}
```

### 7.2 Maybe<T>

값이 있거나 없음.

```kern
fn find(list: List<i64>, target: i64) -> Maybe<i64> {
    // 순수 — 값이 있을 수도 없을 수도
    // ...
}
```

### 7.3 모나드 체이닝 (|> bind)

```kern
fn processInput() -> Result<i64, Error> {
    readLine()
        |> bind(parseI64)
        |> bind({ n => ok(fib(n)) })
}
```

bind의 구체적 형태(내장 함수 vs 메서드)는 M2에서 결정.

---

## 8. 타입 시스템

### 8.1 기본 타입

```kern
// 정수 (크기 명시)
val a: i8 = 127
val b: i16 = 32000
val c: i32 = 100000
val d: i64 = 9999999999

// 부호 없는 정수
val e: u8 = 255
val f: u16 = 65535
val g: u32 = 4000000000
val h: u64 = 18446744073709551615

// 부동소수점
val pi: f32 = 3.14
val precise: f64 = 3.141592653589793

// 불리언
val flag: bool = true

// 유닛 (반환값 없음)
val nothing: Unit = ()
```

### 8.2 포인터 / 참조 (커널 개발용)

```kern
val ptr: Ptr<u8> = ...           // Raw 포인터
val ref: Ref<i64> = ...          // 안전한 참조 (미래 — 소유권 시스템)

// 역참조 — 명시적 * 연산자
val value: u8 = *ptr             // 역참조
val x: i64 = *ptr * 3            // 역참조 후 곱셈

// 참조 획득 — & 연산자
val r: Ref<i64> = &x             // 참조 획득

// 포인터 멤버 접근 — 명시적 역참조 필수
val name = (*ptr).name           // OK — 명시적 역참조
val name = ptr.name              // 컴파일 에러! Ptr<T>에는 name 필드 없음
```

**규칙:**
- `*ptr` — 역참조 (단항 접두사 연산자)
- `&x` — 참조 획득 (단항 접두사 연산자)
- `ptr.field` — **불가**. 자동 역참조 없음. 항상 `(*ptr).field`
- `->` — 반환 타입 전용. C 스타일 `ptr->field` 없음

### 8.3 제네릭 타입 매개변수

```kern
// 타입 위치 — <> 사용
val ptr: Ptr<u8> = ...
val result: Result<i64, Error> = ...
fn identity<T>(x: T) -> T { x }

// 표현식 위치 — 컴파일러가 추론
val x = identity(42)             // T = i64 추론
print(identity(42))              // 문맥상 i64 추론

// 추론 불가 시 — val에 타입 명시로 유도
val x: i64 = identity(42)       // T = i64 명시적 유도
```

**타입 컨텍스트 규칙:**
- `:` 뒤, `->` 뒤, `fn` 이름 뒤에서 `<`는 항상 제네릭으로 해석
- **표현식 위치에서 제네릭 명시 불가** — 터보피시(`::<>`) 없음
- 컴파일러가 인자 타입, 반환 타입, 바인딩 타입 등 문맥에서 추론
- **모호하면 에러:** 추론 불가 시 추측하지 않고 컴파일 에러 + 해결 방법 제시

```kern
// 추론 불가 → 컴파일 에러
val x = identity(42)
// error: cannot infer type parameter T for 'identity'
//   help: specify the type: val x: i64 = identity(42)
```

### 8.4 함수 타입

```kern
val f: (i64) -> i64 = ...
val g: (i64, i64) -> bool = ...
val h: () -> Unit = ...
```

### 8.5 논리 연산자

```kern
// 키워드 기반 논리 연산
if a > 0 and b > 0 { ... }
if x or y { ... }
if not done { ... }

// != 비교 연산자는 유지
if a != b { ... }
```

**규칙:**
- `and` — 논리 AND (`&&` 없음)
- `or` — 논리 OR (`||` 없음)
- `not` — 논리 NOT (`!` 없음)
- `!=` — 비교 연산자 (유지)
- `&` — 참조 획득 전용 (논리 AND와 분리)

### 8.6 제네릭 (미래)

```kern
fn identity<T>(x: T) -> T { x }
```

---

## 9. 제어 흐름

### 9.1 if 표현식

`if`는 문장이 아닌 **표현식**. 값을 반환.

```kern
val max: i64 = if a > b { a } else { b }
```

### 9.2 match 표현식

```kern
val label: String = match statusCode {
    200 => "OK"
    404 => "Not Found"
    _ => "Unknown"
}
```

### 9.3 재귀 (루프 대신)

순수 함수형 스타일에서 반복은 재귀로 표현. 꼬리 재귀는 컴파일러가 자동 최적화.

```kern
fn sum(n: i64, acc: i64) -> i64 {
    match n {
        0 => acc
        n => sum(n - 1, acc + n)   // 꼬리 위치 → 자동 최적화
    }
}
```

---

## 10. 종합 예제 — 피보나치 (M1 목표)

```kern
fn fib(n: i64) -> i64 {
    if n <= 1 { n }
    else { fib(n - 1) + fib(n - 2) }
}

fn main() -> i64 {
    fib(10)
}
```

패턴 매칭 버전:

```kern
fn fib(0) -> i64 { 0 }
fn fib(1) -> i64 { 1 }
fn fib(n: i64) -> i64 { fib(n - 1) + fib(n - 2) }

fn main() -> i64 { fib(10) }
```

---

## 11. 종합 예제 — 커널 코드 (미래 비전)

```kern
module Kernel.Serial

fn outb(port: u16, val: u8) -> Unit = intrinsic
fn inb(port: u16) -> u8 = intrinsic

fn baudToDivisor(baud: u32) -> u16 {
    (115200 / baud) |> toU16
}
// 컴파일러: pure, distributable

fn initSerial(port: u16, baud: u32) -> Unit {
    val divisor: u16 = baudToDivisor(baud)
    outb(port + 1, 0x00)
    outb(port + 3, 0x80)
    outb(port + 0, divisor.low())
    outb(port + 1, divisor.high())
    outb(port + 3, 0x03)
    outb(port + 2, 0xC7)
}
// 컴파일러: impure(io), pinned

fn writeSerial(port: u16, data: u8) -> Unit {
    waitUntil({ => isTransmitEmpty(port) })
    outb(port, data)
}
// 컴파일러: impure(io), pinned

fn readSerial(port: u16) -> u8 {
    waitUntil({ => isDataReady(port) })
    inb(port)
}
// 컴파일러: impure(io), pinned
```

---

## 12. 문법 요약표

| 요소 | Kern 문법 | 비고 |
|------|----------|------|
| 불변 바인딩 | `val x: i64 = 42` | Kotlin 스타일 |
| 가변 바인딩 | `var y: i64 = 0` | 함수가 impure(mut)가 됨, 경고 |
| 함수 | `fn name(args) -> Type { }` | 모든 속성 컴파일러 추론 |
| 반환 타입 | `-> Type` | Rust 스타일 |
| 블록 | `{ }` | 중괄호, 마지막 표현식이 반환값 |
| 세미콜론 | 선택적 | 한 줄 복수 문장일 때만 |
| 파이프 | `a \|> f(b)` = `f(a, b)` | 데이터 흐름 |
| 패턴 매칭 | `match x { pat => expr }` | 표현식 |
| 함수 패턴 | `fn fib(0) -> i64 { 0 }` | 함수 정의 수준 |
| if | `if cond { } else { }` | 표현식 |
| 람다 | `{ x => x * 2 }` | Kotlin 스타일 |
| return | `return expr` | 선택적 — 마지막 표현식도 유효 |
| 주석 | `// 한 줄`, `/* 블록 */` | 둘 다 지원 |
| 논리 연산 | `and`, `or`, `not` | 키워드. `&&`/`||`/`!` 없음 |
| 역참조 | `*ptr` | 단항 접두사. `(*ptr).field` 명시적 |
| 참조 획득 | `&x` | 단항 접두사. `&`는 참조 전용 |
| 제네릭 | `Ptr<u8>`, `Result<T, E>` | 타입 위치만 `<>`. 표현식은 추론 전용 |
| 모나드 | `Result<T, E>`, `Maybe<T>` | 독립적 값 타입 |
| Intrinsic | `fn name(args) -> Type = intrinsic` | 불순의 원천 |
| 어노테이션 | **없음** | 모든 속성 컴파일러 자동 추론 |

---

## 13. BNF 문법 (M1 범위)

```bnf
program     = { fn_decl }

fn_decl     = "fn" IDENT "(" param_list ")" "->" type block
            | "fn" IDENT "(" pattern_list ")" "->" type block
            | "fn" IDENT "(" param_list ")" "->" type "=" "intrinsic"

param_list  = [ param { "," param } ]
param       = IDENT ":" type                   (* 매개변수 — 타입 필수 *)
            | literal                          (* 패턴 매칭용 — 리터럴만 *)
            (* IDENT만 단독 사용 불가 → 컴파일 에러: "parameter missing type" *)

type        = IDENT                            (* i64, u8, bool, Unit, ... *)
            | "Ptr" "<" type ">"               (* 포인터 *)
            | "(" type_list ")" "->" type      (* 함수 타입 *)
type_list   = [ type { "," type } ]

block       = "{" { statement } [ expr ] "}"
statement   = val_decl | var_decl | assignment | return_stmt | expr
return_stmt = "return" [ expr ]

val_decl    = "val" IDENT ":" type "=" expr
var_decl    = "var" IDENT ":" type "=" expr
assignment  = IDENT "=" expr

expr        = if_expr | match_expr | pipe_expr
pipe_expr   = binary_expr { "|>" ( call_expr | IDENT ) }
binary_expr = unary_expr { binop unary_expr }
unary_expr  = [ "-" | "not" | "*" | "&" ] primary
primary     = INT_LIT | FLOAT_LIT | STRING_LIT | IDENT
            | call_expr | "(" expr ")" | lambda | block

call_expr   = IDENT "(" arg_list ")"
arg_list    = [ expr { "," expr } ]

if_expr     = "if" expr block [ "else" ( if_expr | block ) ]

match_expr  = "match" expr "{" { match_arm } "}"
match_arm   = pattern [ "if" expr ] "=>" expr

pattern     = "_" | INT_LIT | IDENT | IDENT "(" pattern_list ")"
pattern_list= [ pattern { "," pattern } ]

lambda      = "{" [ lambda_params "=>" ] expr "}"
lambda_params = IDENT { "," IDENT }
            | IDENT ":" type { "," IDENT ":" type }

binop       = "+" | "-" | "*" | "/" | "==" | "!=" | "<" | ">" | "<=" | ">="
            | "and" | "or"
```

---

## 14. 제로 코스트 추상화 규칙

모든 추상화는 **컴파일 타임에 해소**되어 런타임 비용이 없어야 한다.

### 14.1 `|>` 파이프 — 문법 치환 (desugar)

런타임 비용: **제로**. 클로저 생성 없음.

```
규칙:
  a |> f(b, c)  →  f(a, b, c)    // 첫 번째 인자로 삽입
  a |> f        →  f(a)           // 인자 없으면 단일 호출
```

```kern
// 소스
val r: i64 = 10 |> add(3) |> multiply(2)

// 컴파일러가 변환하는 것
multiply(add(10, 3), 2)
```

제약: `|>` 뒤에는 반드시 **함수 호출** 또는 **함수명**만 올 수 있음.

### 14.2 함수 수준 패턴 매칭 — 디시전 트리 직접 생성

런타임 비용: **비교 명령어만** (match desugar 없이 IR에서 바로 분기 트리).

```kern
fn fib(0) -> i64 { 0 }
fn fib(1) -> i64 { 1 }
fn fib(n: i64) -> i64 { fib(n - 1) + fib(n - 2) }
```

컴파일러가 이것을 **하나의 함수 + 디시전 트리**로 합침:
- 정수 리터럴 패턴: `cmp` + `je`/`jne` 명령어
- 변수 패턴: 폴백(fallback)
- 중첩 패턴: 트리 분기 확장
- match로 desugar하지 않음 — IR에서 직접 최적 분기 생성

### 14.3 var의 순수성 규칙 — 단순, 증명 불필요

```
규칙:
  1. var 있으면 → 해당 함수 impure(mut) + 컴파일 경고
  2. impure(mut)는 호출자에게 전파 안 함
  3. 복잡한 탈출 분석(escape analysis) 불필요 — 규칙 자체가 판단의 전부
```

var는 로컬 뮤테이션이므로 외부에서 관찰 불가능 → 호출자는 pure로 간주 가능.

### 14.4 Result/Maybe — Tagged Union

런타임 비용: **태그 1바이트 + union 크기**. 힙 할당 없음.

```
메모리 레이아웃: [tag: u8][padding][value: union { T | E }]

반환 규칙:
  - 작은 타입 (≤ 8바이트): 레지스터 2개 반환 (rax=tag, rdx=value)
  - 큰 타입 (> 8바이트): 호출자가 스택 버퍼 포인터 전달
```

### 14.5 줄 연속 — 연속 토큰 규칙

**연속 토큰** 목록으로 줄이 시작하면 이전 줄의 연속:

```
연속 토큰: |>  .  +  -  *  /  ==  !=  <  >  <=  >=  =>  ,  and  or  else
```

```kern
val r: i64 = 10
    |> add(3)        // |>로 시작 → 연속
    |> multiply(2)   // |>로 시작 → 연속

val sum: i64 = a
    + b              // +로 시작 → 연속
    + c              // +로 시작 → 연속
```

Lexer 수준에서 처리: 줄 시작 토큰이 연속 토큰이면 이전 줄의 implicit newline을 무시.

### 14.6 람다/클로저 — 함수 리프팅

런타임 비용: **캡처 없으면 제로** (함수 포인터). 캡처 있으면 구조체 패킹.

```
규칙:
  - 캡처 없음: 일반 함수로 리프팅 → 함수 포인터 전달
  - 캡처 있음: 캡처 변수를 구조체로 패킹 → 스택 할당, 힙 할당 없음
```

```kern
// 캡처 없음 → 함수 포인터 (제로 코스트)
val double = { x: i64 => x * 2 }

// 캡처 있음 → 구조체 패킹 (스택 할당)
fn makeAdder(n: i64) -> (i64) -> i64 {
    { x => x + n }    // n을 캡처 → struct { n: i64 } + 함수 포인터
}
```

---

### 14.7 제네릭 `<>` — 타입 컨텍스트 규칙

런타임 비용: **제로**. 제네릭은 단형화(monomorphization) 또는 타입 소거로 처리.

```
파싱 규칙:
  - 타입 위치 (: 뒤, -> 뒤, fn 이름 뒤): <는 항상 제네릭
  - 표현식 위치: 제네릭 명시 불가. 컴파일러가 문맥에서 추론
  - 추론 불가 시: 추측하지 않고 컴파일 에러 + help 메시지
  - 터보피시(::<>) 없음. :: 토큰 자체가 언어에 불필요
```

### 14.8 `*` 역참조 / `&` 참조 — Pratt parser 단항 처리

런타임 비용: **제로**. `*`는 메모리 읽기, `&`는 주소 계산 — 하드웨어 수준 연산.

```
파싱 규칙:
  - * 접두사: 역참조 (단항). 곱셈(이항)과 Pratt parser로 자동 구분
  - & 접두사: 참조 획득 (단항). 논리 AND 없음 (and 키워드 사용)
  - 자동 역참조 없음: ptr.field → 컴파일 에러 + help
```

---

## 15. 미결 사항

| # | 질문 | 상태 |
|---|------|------|
| SQ-1 | bind의 구체적 형태 (내장 함수 vs 메서드) | M2에서 결정 |
| SQ-2 | 구조체/레코드 타입 문법 | M2에서 설계 |
| SQ-3 | 열거형(enum/ADT) 문법 | M2에서 설계 |
| SQ-4 | 모듈/임포트 문법 세부 사항 | M2에서 설계 |
| SQ-5 | 연산자 오버로딩 허용 여부 | M2 이후 결정 가능 |
| SQ-6 | 문자열 리터럴과 보간 | M2에서 설계 |
| ~~SQ-7~~ | ~~블록 주석~~ | **결정됨: `//` + `/* */` 둘 다 지원** |
| ~~SQ-8~~ | ~~제네릭 문법~~ | **결정됨: `<>` + 타입 컨텍스트 규칙 + `::<>` 터보피시** |
| ~~SQ-9~~ | ~~역참조 연산자~~ | **결정됨: `*ptr` (C/Rust 스타일)** |
| ~~SQ-10~~ | ~~논리 연산자~~ | **결정됨: `and`/`or`/`not` 키워드** |
| ~~SQ-11~~ | ~~포인터 멤버 접근~~ | **결정됨: 명시적 역참조 필수 `(*ptr).field`** |
| ~~SQ-12~~ | ~~표현식 제네릭 호출~~ | **결정됨: 터보피시 없음. 타입 추론 전용** |

---

## 16. 다음 단계

1. REQUIREMENTS.md 최신화 (이 문서와 일치시킴)
2. ARCHITECTURE.md 토큰/AST 업데이트 (`val`/`var`/`|>`/`and`/`or`/`not`/`&` 추가)
3. M1 구현 워크플로우 작성
