# Kern 언어 백서

> **버전**: 1.0
> **작성일**: 2026-03-02
> **대상 독자**: 소프트웨어 엔지니어, 시스템 프로그래머, 기술 투자자

---

# 목차

1. [요약 (Executive Summary)](#1-요약)
2. [왜 Kern인가](#2-왜-kern인가)
3. [설계 철학 — 세 기둥](#3-설계-철학--세-기둥)
4. [언어 가이드](#4-언어-가이드)
5. [이펙트 시스템](#5-이펙트-시스템)
6. [Unsafe 없는 커널 개발](#6-unsafe-없는-커널-개발)
7. [컴파일러 아키텍처](#7-컴파일러-아키텍처)
8. [커널 코드 예제](#8-커널-코드-예제)
9. [표준 라이브러리](#9-표준-라이브러리)
10. [도구 생태계](#10-도구-생태계)
11. [벤치마크](#11-벤치마크)
12. [언어 비교](#12-언어-비교)
13. [로드맵](#13-로드맵)
14. [BNF 문법 (부록)](#14-bnf-문법-부록)

---

# 1. 요약

**Kern**은 운영체제 커널 개발을 위한 순수 함수형 언어다.

C의 **제로코스트 성능**, Haskell의 **타입 안전성**, 그리고 Kotlin의 **가독성**을 단일 언어에 통합하여, 기존 어떤 언어도 점유하지 못한 교차점을 채운다.

### 핵심 차별점

| # | 차별점 | 설명 |
|---|--------|------|
| 1 | **어노테이션 제로** | 순수성, 이펙트, 재귀, 꼬리호출 — 모두 컴파일러가 자동 추론. 프로그래머는 로직만 작성 |
| 2 | **이펙트 시스템** | `with io, atomic` 컴파일 타임 어노테이션. IO 모나드 없이 순수성 보장. LIR에서 완전 소거 |
| 3 | **Unsafe 불필요** | 흐름 감응 타입, 의도 표현 연산자(`+%`, `+|`), 타입된 하드웨어 추상화. `asm {}`이 유일한 탈출구 |
| 4 | **4단계 IR 파이프라인** | AST → HIR → LIR → MachIR. 각 단계가 독립 라이브러리. 21,898줄 C++20 |

### 현재 상태 (2026-03-02)

| 항목 | 수치 |
|------|------|
| 단위 테스트 | 636개 통과 |
| 통합(E2E) 테스트 | 259개 |
| C++ 코드 (lib/ + include/) | 21,898줄 |
| LIR 오피코드 | 57종 |
| X86 기계어 오피코드 | 57종 |
| 토큰 종류 | 75종 (33개 키워드) |
| 기본 타입 | 13종 (i8~i64, u8~u64, f32/f64, bool, Unit, Error) |
| 컴파일 속도 (fib(35)) | ~58ms (소스 → 네이티브 바이너리) |
| 타겟 아키텍처 | x86-64 macOS (NASM + ld) |

---

# 2. 왜 Kern인가

## 2.1 커널 개발의 딜레마

운영체제 커널은 소프트웨어에서 가장 까다로운 영역이다. 하드웨어를 직접 제어해야 하고, 한 줄의 버그가 전체 시스템을 멈출 수 있다. 그런데 2026년 현재, 커널 개발자들이 사용할 수 있는 선택지는 근본적으로 제한되어 있다.

**C** — 50년 된 언어로 대부분의 운영체제가 작성되어 있다. 성능은 뛰어나지만, 타입 시스템이 약하고 메모리 안전성을 보장하지 못한다. 버퍼 오버플로우, use-after-free, 정수 오버플로우가 일상적이다.

**Rust** — 소유권 시스템으로 메모리 안전성을 보장하지만, `'a mut Vec<Box<dyn Trait + 'b>>` 같은 라이프타임 어노테이션이 코드 가독성을 크게 떨어뜨린다. `unsafe` 블록 없이는 커널 코드를 작성할 수 없다.

**Haskell** — 강력한 타입 시스템과 순수 함수형 패러다임을 갖추고 있지만, GC(가비지 컬렉터) 의존성과 런타임 오버헤드 때문에 커널 개발에는 부적합하다.

**Zig** — C의 대안으로 설계되었지만, 함수형 프로그래밍 지원이 미약하고 타입 시스템이 C++보다 단순하다.

## 2.2 빈 교차점

| 속성 | C | Rust | Haskell | Zig | **Kern** |
|------|---|------|---------|-----|----------|
| 가독성 | X | △ | O | O | **O** |
| 제로코스트 추상화 | O | O | X (GC) | O | **O** |
| 순수 함수형 | X | X | O | X | **O** |
| OS 커널 개발 가능 | O | O | X | O | **O** |

네 가지 속성을 **모두** 만족하는 언어는 존재하지 않았다. Kern이 이 빈 교차점을 채운다.

## 2.3 Kern의 핵심 아이디어

> **프로그래머는 로직만 작성한다. 나머지는 컴파일러가 알아낸다.**

```kern
fn fib(n: i64) -> i64 {
    if n <= 1 { n }
    else { fib(n - 1) + fib(n - 2) }
}
```

이 코드에서 프로그래머가 선언한 것은 **함수 이름, 매개변수, 반환 타입, 로직** 뿐이다. 컴파일러가 자동으로 추론하는 것:

- **순수성**: 외부 상태를 읽거나 쓰지 않으므로 `pure`
- **재귀**: 자기 자신을 호출하므로 `recursive`
- **분배 가능성**: 순수하고 캡처가 없으므로 `distributable`

## 2.4 시장 기회

커널 개발 시장은 Linux, Windows, macOS, 그리고 점증하는 임베디드/IoT 생태계를 포함한다. 안전한 시스템 프로그래밍 언어에 대한 수요는 Rust의 성공이 증명한다(Linux 커널의 Rust 모듈 도입, Android/ChromeOS의 Rust 채택).

그러나 Rust의 학습 곡선(라이프타임, 소유권 규칙의 복잡성)은 여전히 채택의 장벽이다. Kern은 **동일한 안전성을 더 읽기 쉬운 문법으로** 제공함으로써, Rust가 만든 안전한 시스템 언어 시장에서 더 넓은 개발자 기반을 확보할 수 있다.

---

# 3. 설계 철학 — 세 기둥

Kern의 모든 설계 결정은 세 가지 원칙에서 파생된다.

## 3.1 가독성 — 코드가 의도를 드러냄

프로그래머가 작성하는 코드는 **무엇을** 의도하는지 전달해야 한다. 언어의 제약을 우회하는 방법을 전달해서는 안 된다.

### 설계 원칙

| 원칙 | 설명 |
|------|------|
| **코드만 작성한다** | 프로그래머는 로직만 작성. 메타데이터는 전부 컴파일러가 추론 |
| **읽기 쉬운 코드** | 비전의 핵심. 코드가 의도를 명확히 전달해야 함 |
| **Kotlin 베이스** | 전체적 느낌과 인체공학은 Kotlin에서 출발 |
| **어노테이션 없음** | `@` 자체가 언어에 존재하지 않음 |
| **파이프로 흐름** | `\|>` 연산자로 데이터 체인을 자연스럽게 표현 |
| **읽기 쉬운 논리** | `and`/`or`/`not` 키워드. `&&`/`\|\|`/`!` 없음 |
| **모호하면 에러** | 컴파일러가 추측하지 않음. 상세한 에러 메시지로 해결 유도 |

### C vs Kern — 가독성 비교

```c
// C: 의도가 문법에 묻힘
int *p = &arr[0];
if (p != NULL && (*p > 0 || flag)) {
    int result = (*p) * factor;
}
```

```kern
// Kern: 의도가 명확히 드러남
val p: Ptr<i64> = &arr[0]
if *p > 0 or flag {
    val result: i64 = *p * factor
}
```

### Rust vs Kern — 가독성 비교

```rust
// Rust: 라이프타임 어노테이션이 로직을 가림
fn process<'a, 'b>(data: &'a mut Vec<Box<dyn Handler + 'b>>)
    -> Result<&'a [u8], Box<dyn Error>>
where 'b: 'a
{
    // ...
}
```

```kern
// Kern: 로직만 보임
fn process(data: var [Handler]) -> Result<[u8], Error> with mut {
    // ...
}
```

## 3.2 제로코스트 추상화 — 컴파일러가 더 많이 봄

프로그래머가 사용하는 모든 추상화는 컴파일 타임에 해소되어 런타임 비용이 없어야 한다.

| 추상화 | 컴파일 타임 처리 | 런타임 비용 |
|--------|-----------------|------------|
| `\|>` 파이프 | 함수 호출로 디슈가 | 제로 |
| `with io` 이펙트 | 타입 검사 후 소거 | 제로 |
| 제네릭 `<T>` | 단형화(monomorphization) | 제로 |
| 클로저 (캡처 없음) | 함수 포인터로 리프팅 | 제로 |
| 클로저 (캡처 있음) | 구조체 패킹 (스택 할당) | 힙 할당 없음 |
| `Result<T, E>` | Tagged union | 태그 1바이트 + union 크기 |
| 함수 패턴 매칭 | 디시전 트리 직접 생성 | 비교 명령어만 |

### 파이프 디슈가 예시

```kern
// 소스 코드
val result: i64 = 10 |> add(3) |> multiply(2)

// 컴파일러가 변환하는 것 (AST 수준)
multiply(add(10, 3), 2)
```

`|>` 연산자는 파서가 AST를 구성할 때 함수 호출로 직접 변환한다. 클로저 생성도, 가상 디스패치도, 힙 할당도 없다.

### 이펙트 소거 예시

```kern
fn read_port(port: u16) -> u8 with io {
    volatile_read(port as Ptr<u8>)
}
```

`with io`는 컴파일 타임에만 존재한다. 생성되는 어셈블리는:

```nasm
; 이펙트 어노테이션 유무와 관계없이 동일한 코드
mov al, [rdi]
```

## 3.3 실수 방지 — 잘못된 코드는 컴파일 불가

버그를 런타임에 잡는 것이 아니라, **구조적으로 불가능**하게 만든다. 이것이 컴파일러의 주요 임무다.

```kern
// 순수 함수에서 IO 함수 호출 → 컴파일 에러
fn bad_calc(x: i64) -> i64 {
    serial_write(COM1, 0x41)
    // ERROR: 'serial_write' requires effect 'io',
    //        but 'bad_calc' has no effects.
    //        Add 'with io' to 'bad_calc' or remove the call.
    x + 1
}
```

```kern
// 이동된 값 사용 → 컴파일 에러
val b = Buffer.new(1024)
destroy(own b)
read_header(b)    // ERROR: 'b' was moved at line N
```

```kern
// 불변 값 재할당 → 컴파일 에러
val x: i64 = 42
x = 10            // ERROR: cannot reassign immutable binding 'x'
```

---

# 4. 언어 가이드

## 4.1 바인딩 — val / var

```kern
val x: i64 = 42          // 불변 바인딩 — 한 번 바인딩되면 변경 불가
var y: i64 = 0            // 가변 바인딩 — 재할당 가능
y = y + 1                 // OK
```

- `val` — 불변. Kotlin/Scala의 `val`, Rust의 `let`에 해당
- `var` — 가변. 재할당 가능. **var가 있으면 해당 함수는 `mut` 이펙트를 가짐**
- 타입은 항상 명시적: `이름: 타입 = 값`
- `var` 사용 시 컴파일 **경고** (val + 재귀로 리팩터링 제안)

## 4.2 함수

### 기본 함수 선언

```kern
fn add(a: i64, b: i64) -> i64 {
    a + b
}
```

마지막 표현식이 반환값이다. `return` 키워드는 선택적.

컴파일러가 자동 추론하는 속성:

| 속성 | 의미 | 판단 기준 |
|------|------|----------|
| `pure` | 순수 함수 | var 미사용 + 이펙트 있는 함수 미호출 |
| `impure(mut)` | 로컬 변이 | var 사용 |
| `impure(io)` | 외부 I/O | intrinsic/IO 함수 호출 |
| `recursive` | 재귀 함수 | 자기 자신 호출 |
| `tail-rec` | 꼬리 재귀 | 재귀 호출이 꼬리 위치 → 자동 최적화 |
| `distributable` | 분배 가능 | 순수 + 캡처 없음 |

### 함수 정의 수준 패턴 매칭

```kern
fn fib(0) -> i64 { 0 }
fn fib(1) -> i64 { 1 }
fn fib(n: i64) -> i64 {
    fib(n - 1) + fib(n - 2)
}
```

동일한 이름의 여러 함수 정의가 하나의 디시전 트리로 합쳐진다.

### 람다 / 익명 함수

```kern
val double: (i64) -> i64 = { x: i64 => x * 2 }

// 타입 추론 가능한 경우
val double = { x => x * 2 }

// 여러 매개변수
val add = { a: i64, b: i64 => a + b }
```

### 고차 함수

```kern
fn apply(f: (i64) -> i64, x: i64) -> i64 {
    f(x)
}

fn compose(f: (i64) -> i64, g: (i64) -> i64) -> (i64) -> i64 {
    { x => f(g(x)) }
}
```

### Intrinsic (내장 함수)

하드웨어 접근 등 컴파일러가 본문을 분석할 수 없는 함수. 컴파일러가 이것들을 **이펙트의 원천**으로 인식한다.

```kern
fn outb(port: u16, val: u8) -> Unit = intrinsic
fn inb(port: u16) -> u8 = intrinsic
```

이 함수를 호출하는 모든 함수는 자동으로 해당 이펙트가 추론된다.

## 4.3 타입 시스템

### 기본 타입 (13종)

| 카테고리 | 타입 | 크기 |
|----------|------|------|
| 부호 있는 정수 | `i8`, `i16`, `i32`, `i64` | 1, 2, 4, 8 바이트 |
| 부호 없는 정수 | `u8`, `u16`, `u32`, `u64` | 1, 2, 4, 8 바이트 |
| 부동소수점 | `f32`, `f64` | 4, 8 바이트 |
| 불리언 | `bool` | 1 바이트 |
| 유닛 | `Unit` | 0 바이트 (반환값 없음) |

```kern
val a: i8 = 127
val b: i32 = 100000
val c: u64 = 18446744073709551615
val pi: f64 = 3.141592653589793
val flag: bool = true
```

**엄격한 타입**: 암시적 변환 없음. `i32`와 `i64` 사이에도 명시적 `as` 캐스트 필요.

### 포인터

```kern
val ptr: Ptr<u8> = &some_value       // 불변 포인터
val mptr: Ptr<var u8> = &some_var    // 가변 포인터

val value: u8 = *ptr                 // 역참조 — 명시적 * 연산자
(*mptr) = 42                        // 포인터를 통한 쓰기
```

규칙:
- `*ptr` — 역참조 (단항 접두사 연산자)
- `&x` — 참조 획득 (단항 접두사 연산자)
- `ptr.field` — 불가. 자동 역참조 없음. 항상 `(*ptr).field`

### 배열

```kern
val arr: [i64; 4] = [1, 2, 3, 4]    // 고정 크기 배열
val x: i64 = arr[0]                  // 인덱스 접근
```

### 함수 타입

```kern
val f: (i64) -> i64 = { x => x * 2 }
val g: (i64, i64) -> bool = { a, b => a > b }
val h: () -> Unit = { => print_str("hello") }
```

### 구조체

```kern
struct Point {
    x: i64,
    y: i64
}

val p: Point = Point { x: 10, y: 20 }
val px: i64 = p.x
```

`packed` 및 `aligned` 속성 지원:

```kern
@packed
struct PackedHeader {
    magic: u16,
    size: u32
}

@aligned(64)
struct CacheLine {
    data: [u8; 64]
}
```

### 열거형 (Enum)

```kern
enum Color { Red, Green, Blue }

fn describe(c: Color) -> i64 {
    match c {
        Red => 1,
        Green => 2,
        Blue => 3
    }
}
```

### 유니온 (Tagged Union / ADT)

```kern
union Option<T> { None, Some(T) }
union Result<T, E> { Ok(T), Err(E) }

fn safe_div(a: i64, b: i64) -> Result<i64, i64> {
    if b == 0 { Err(0) }
    else { Ok(a / b) }
}
```

### 제네릭

```kern
fn identity<T>(x: T) -> T { x }

struct Pair<A, B> {
    first: A,
    second: B
}

fn swap<A, B>(p: Pair<A, B>) -> Pair<B, A> {
    Pair { first: p.second, second: p.first }
}
```

제네릭은 **단형화(monomorphization)**로 처리된다. `identity<i64>`와 `identity<bool>`은 각각 별도의 함수로 생성되어 런타임 비용이 제로다.

터보피시(`::<>`) 문법은 없다. 컴파일러가 문맥에서 타입을 추론하며, 추론 불가 시 컴파일 에러와 함께 해결 방법을 제시한다.

### Const 제네릭

```kern
struct Buffer<T, const N: u64> {
    data: [T; N]
}

val buf: Buffer<u8, 1024> = ...
```

타입 매개변수 위치에 `const` 키워드로 컴파일 타임 상수를 전달할 수 있다.

### 트레이트

```kern
trait Printable {
    fn to_string(self) -> String
}

impl Printable for Point {
    fn to_string(self) -> String {
        "Point"
    }
}
```

트레이트는 타입 클래스(Haskell)나 인터페이스(Java)와 유사한 개념으로, 다형성을 구현한다.

## 4.4 제어 흐름

### if 표현식

`if`는 문장이 아닌 **표현식**. 값을 반환한다.

```kern
val max: i64 = if a > b { a } else { b }

val label: String = if n > 0 { "positive" }
    else if n < 0 { "negative" }
    else { "zero" }
```

### match 표현식

```kern
fn describe(n: i64) -> String {
    match n {
        0 => "zero",
        1 => "one",
        n if n < 0 => "negative",
        _ => "other"
    }
}
```

패턴 종류:
- 정수 리터럴: `0`, `1`, `42`
- 불리언 리터럴: `true`, `false`
- 와일드카드: `_`
- 변수 바인딩: `n`
- 가드: `n if n < 0`
- 열거형/유니온 분해: `Some(v)`, `Err(e)`

### loop

```kern
fn gcd(a: i64, b: i64) -> i64 {
    var x: i64 = a
    var y: i64 = b
    loop {
        if y == 0 { break x }
        val t: i64 = y
        y = x % y
        x = t
    }
}
```

`loop`는 명시적 반복 구조. `break`로 탈출하며, `break` 뒤의 표현식이 루프의 반환값이 된다.

### 재귀 (루프 대신)

순수 함수형 스타일에서 반복은 재귀로 표현한다. 꼬리 재귀는 컴파일러가 자동으로 반복문으로 최적화한다.

```kern
fn sum(n: i64, acc: i64) -> i64 {
    match n {
        0 => acc,
        n => sum(n - 1, acc + n)   // 꼬리 위치 → 자동 최적화
    }
}
```

## 4.5 파이프 연산자 |>

데이터 흐름을 왼쪽에서 오른쪽으로 표현한다.

```kern
val result: i64 = 10
    |> add(3)          // add(10, 3) → 13
    |> multiply(2)     // multiply(13, 2) → 26
    |> subtract(1)     // subtract(26, 1) → 25
```

**규칙**: `a |> f(b)` 는 `f(a, b)`로 변환. 좌변의 값이 첫 번째 인자로 삽입된다.

파이프는 파서 수준에서 함수 호출로 디슈가되므로 런타임 비용이 제로다.

## 4.6 클로저와 고차 함수

### 캡처 없는 클로저 (제로 코스트)

```kern
val double = { x: i64 => x * 2 }
// → 일반 함수로 리프팅. 함수 포인터로 전달
```

### 캡처 있는 클로저

```kern
fn make_adder(n: i64) -> (i64) -> i64 {
    { x => x + n }    // n을 캡처
}

val add5: (i64) -> i64 = make_adder(5)
val result: i64 = add5(10)    // 15
```

캡처 있는 클로저는 캡처된 변수를 **구조체로 패킹**하여 스택에 할당한다. 힙 할당이 없다.

### 고차 함수 반환

```kern
fn compose(f: (i64) -> i64, g: (i64) -> i64) -> (i64) -> i64 {
    { x => f(g(x)) }
}

val double_then_add5 = compose(make_adder(5), { x => x * 2 })
val result = double_then_add5(10)    // (10 * 2) + 5 = 25
```

## 4.7 모듈

```kern
module Math

fn square(x: i64) -> i64 { x * x }
fn cube(x: i64) -> i64 { x * x * x }
```

```kern
import Math

fn main() -> i64 {
    Math.square(6)    // 36
}
```

`module` 선언으로 네임스페이스를 정의하고, `import`로 다른 모듈을 가져온다.

## 4.8 타입 별칭과 뉴타입

```kern
// 타입 별칭 — 기존 타입의 다른 이름 (호환 가능)
type Byte = u8

// 뉴타입 — 동일한 메모리 표현이지만 타입적으로 구분 (호환 불가)
newtype PhysAddr = u64
newtype VirtAddr = u64

val pa: PhysAddr = PhysAddr(0x1000)
val va: VirtAddr = VirtAddr(0xFFFF0000)
// pa + va → 컴파일 에러! PhysAddr와 VirtAddr는 다른 타입
```

## 4.9 ? 연산자

`Result` 타입의 에러 전파를 간결하게 표현한다.

```kern
fn process() -> Result<i64, i64> {
    val a: i64 = parse_int("42")?    // Err면 즉시 반환
    val b: i64 = parse_int("10")?
    Ok(a + b)
}
```

`expr?`는 다음과 같이 디슈가된다:

```kern
match expr {
    Ok(v) => v,
    Err(e) => return Err(e)
}
```

## 4.10 논리 연산자

```kern
if a > 0 and b > 0 { ... }
if x or y { ... }
if not done { ... }
```

Kern은 `&&`, `||`, `!`를 사용하지 않는다. 영어 키워드 `and`, `or`, `not`을 사용하여 코드가 자연어처럼 읽힌다.

## 4.11 인라인 어셈블리

```kern
fn switch_context(old_sp: Ptr<var u64>, new_sp: u64) -> Unit {
    asm volatile {
        "mov [rdi], rsp",
        "mov rsp, rsi",
        "ret"
    }
}
```

`asm` 블록은 x86-64 어셈블리를 직접 삽입한다. `volatile` 키워드로 컴파일러 재배치를 방지한다.

---

# 5. 이펙트 시스템

## 5.1 모나드는 개념이지, 값이 아니다

Haskell은 `IO a`를 값으로 취급한다 — 구성하고, 전달하고, `>>=`로 합성한다. 이는 런타임 비용(thunk, 힙 할당)을 발생시키고 모나드 배관 코드로 의도를 가린다.

Kern은 이펙트를 **함수에 대한 어노테이션**으로 취급한다 — 타입 시스템에만 존재하고 컴파일 타임에 완전히 소거된다.

```haskell
-- Haskell: IO 모나드 (Kern이 거부하는 방식)
readPort :: Word16 -> IO Word8
readPort p = do
    x <- volatileRead (castPtr p)
    pure x
```

```kern
// Kern: 이펙트는 컴파일 타임 개념
fn read_port(port: u16) -> u8 with io {
    volatile_read_u8(port as Ptr<u8>)
}
// "with io"는 컴파일 타임 어노테이션
// 생성되는 코드: mov al, [rdi]
// IO 래퍼 없음, thunk 없음, 힙 할당 없음
```

### 이 철학의 결과

- `IO<T>` 래퍼 타입 없음
- `bind`/`flatMap`/`>>=` 연산자 없음 (이펙트 용도)
- 모나드 트랜스포머 없음
- 이펙트 핸들러 없음 (이펙트를 가로채거나 구체화할 수 없음)
- 이펙트 합성은 단순한 집합 합집합: `with io, atomic`

## 5.2 네 가지 이펙트

```kern
effect mut         // 가변 상태: var 바인딩, 필드 변이
effect mem         // 포인터/힙 메모리 접근
effect io          // 하드웨어 I/O, volatile, asm, intrinsic
effect atomic      // 원자적 연산: lock cmpxchg, lock xadd, fence
```

이펙트는 언어가 정의하는 **고정 집합**이다. 사용자 정의 이펙트는 v1에서 지원하지 않는다.

내부 구현은 4비트 비트마스크:

```
EffectSet = uint8_t
  Mut    = 0b0001 (1)
  Mem    = 0b0010 (2)
  IO     = 0b0100 (4)
  Atomic = 0b1000 (8)
  Pure   = 0b0000 (0)
```

## 5.3 `with` 절 문법

```kern
// 명시적 이펙트 어노테이션
fn read_port(port: u16) -> u8 with io {
    volatile_read_u8(port as Ptr<u8>)
}

// 복수 이펙트
fn cas_loop(ptr: Ptr<var u64>, old: u64, new: u64) -> bool with atomic, mem {
    atomic_cas(ptr, old, new) == old
}

// 순수 함수 (기본값 — 어노테이션 불필요)
fn add(a: i64, b: i64) -> i64 {
    a + b
}
```

## 5.4 순수성 자동 추론

프로그래머가 `with` 절을 작성하지 않아도, 컴파일러가 함수 본문을 분석하여 이펙트를 자동 추론한다.

```kern
fn counter() -> i64 {
    var x: i64 = 0         // var 사용 → 자동으로 mut 추론
    x = x + 1
    x
}
// 컴파일러 추론: with mut (경고: val + 재귀로 리팩터링 제안)
```

### 추론 전파 규칙

```
함수 A가 함수 B를 호출하면:
  - B가 io   → A도 io (전파됨)
  - B가 mem  → A도 mem (전파됨)
  - B가 atomic → A도 atomic (전파됨)
  - B가 mut  → A에는 전파되지 않음
```

**핵심 인사이트**: `mut` 이펙트는 **전파되지 않는다**. 함수 내부의 `var`는 로컬 뮤테이션이므로, 외부에서 관찰할 수 없다. 따라서 `mut` 함수를 호출하는 함수는 여전히 `pure`일 수 있다.

```kern
fn sum_imperative(n: i64) -> i64 {
    var acc: i64 = 0       // impure(mut)
    // ...
    acc
}

fn double(n: i64) -> i64 {
    sum_imperative(n) * 2
}
// 컴파일러: double은 pure
// 이유: sum_imperative의 mut는 로컬이므로 외부에서 보면 순수하게 동작
```

## 5.5 이펙트 강제 규칙

1. **기본값은 순수.** `with` 절이 없는 함수는 이펙트가 없다.
2. **호출 시 이펙트 필요.** 함수 `f`가 이펙트 `E`를 가지면, `f`의 호출자도 `E`를 가져야 한다(또는 상위집합).
3. **위반은 컴파일 에러.** 경고가 아니다.

```kern
fn pure_fn() -> i64 {
    read_port(0x60)
    // ERROR: 'read_port' requires effect 'io',
    //        but 'pure_fn' has no effects.
    //        Add 'with io' to 'pure_fn' or remove the call.
}
```

## 5.6 이펙트 다형성

> **[설계 완료]** 이펙트 다형성은 설계가 완료되었으며, 7단계 로드맵의 Stage 1에서 구현 예정입니다.

고차 함수는 콜백의 이펙트를 자동으로 상속한다:

```kern
fn map<T, U>(list: [T], f: fn(T) -> U) -> [U] {
    // f의 이펙트가 map에 자동 전파
    // ...
}

// 순수 콜백 → map도 순수
val doubled = [1, 2, 3] |> map({ x => x * 2 })

// IO 콜백 → map이 io 상속
val data = ports |> map({ p => read_port(p) })
```

구현 방식: `Fn` 타입 매개변수에 명시적 이펙트가 없으면, 컴파일러가 **이펙트 변수** `?E`를 할당한다. 각 호출 사이트에서 실제 인자의 이펙트와 통합(unify)한다.

## 5.7 트레이트 이펙트 반공변

> **[설계 완료]** 트레이트 이펙트 반공변은 설계가 완료되었으며, 7단계 로드맵의 Stage 1에서 구현 예정입니다.

트레이트 메서드가 이펙트를 선언하면, 구현체는 **더 적은** 이펙트를 가질 수 있다:

```kern
trait BlockDevice {
    fn read_block(block: u64, buf: var [u8]) with io
}

// 프로덕션: io 사용 (트레이트와 일치)
impl BlockDevice for VirtioBlock {
    fn read_block(block: u64, buf: var [u8]) with io {
        volatile_read(...)
    }
}

// 테스트 모의: 순수 (더 적은 이펙트 — 허용)
impl BlockDevice for MockBlock {
    fn read_block(block: u64, buf: var [u8]) {
        buf[0] = self.test_data[block]
    }
}
```

이 설계로 이펙트 핸들러 없이도 테스트 가능성을 확보한다.

## 5.8 이펙트 소거

이펙트는 HIR → LIR 변환 시 **완전히 제거**된다. LIR에는 이펙트 정보가 없다. 생성되는 어셈블리는 이펙트 어노테이션 유무와 관계없이 동일하다.

이것은 최적화가 아니라 **구조적으로 제로코스트**다.

## 5.9 Haskell IO 모나드와의 비교

| 측면 | Haskell (`IO a`) | Kern (`with io`) |
|------|-------------------|------------------|
| IO 표현 | 값 타입 (`IO Word8`) | 컴파일 타임 어노테이션 |
| 합성 방법 | `>>=`, `do` 표기법 | 일반 함수 호출 |
| 런타임 비용 | thunk, 힙 할당 | 제로 (완전 소거) |
| 모나드 트랜스포머 | 필요 (IO + State + Error) | 불필요 (이펙트 집합 합집합) |
| 가독성 | `liftIO`, `MonadIO m =>` | `with io` |
| 순수성 보장 | O (타입 레벨) | O (타입 레벨) |
| 타입 래핑/언래핑 | 필요 | 불필요 |

---

# 6. Unsafe 없는 커널 개발

## 6.1 철학

`unchecked`/`unsafe` 블록은 언어가 프로그래머의 의도를 표현하는 데 실패했음을 의미한다. 탈출구를 제공하는 대신, Kern은 타입 시스템을 충분히 표현력 있게 만들어 탈출이 불필요하도록 한다.

## 6.2 흐름 감응 타입 (Flow-Sensitive Typing)

> **[설계 완료]** 흐름 감응 타입은 설계가 완료되었으며, 7단계 로드맵의 Stage 3에서 구현 예정입니다.

배열 경계 검사를 비활성화하는 대신, **불필요함을 증명**한다:

```kern
// 정적: 상수 인덱스는 컴파일 타임에 검증
val arr: [i64; 4] = [1, 2, 3, 4]
val x = arr[3]    // OK: 3 < 4 (컴파일 타임 검증)
val y = arr[4]    // ERROR: index 4 out of bounds for [i64; 4]

// 동적: 조건문이 범위를 좁힘
fn get(arr: [i64; 512], i: u64) -> Option<i64> {
    if i < 512 {
        Some(arr[i])   // 안전: 컴파일러가 이 분기에서 i < 512임을 알고 있음
    } else {
        None
    }
}

// 루프: 범위가 제한됨
for i in 0..arr.len {
    arr[i]   // 안전: 구조적으로 i < arr.len
}
```

컴파일러가 조건문과 루프 범위를 통해 값의 범위를 추적한다. 범위가 증명 가능하면 런타임 검사가 삽입되지 않는다.

## 6.3 의도 표현 연산자

> **[설계 완료]** 래핑/포화 연산자는 설계가 완료되었으며, 7단계 로드맵의 Stage 3에서 구현 예정입니다.

오버플로우 검사를 비활성화하는 대신, **다른 연산자**로 다른 의도를 표현한다:

| 연산자 | 의미 | 용도 |
|--------|------|------|
| `+` | 트래핑 덧셈 | 기본값 — 오버플로우는 버그 |
| `+%` | 래핑 덧셈 | 해시 함수, 암호화 |
| `+\|` | 포화 덧셈 | 센서 값, 클램핑 |

```kern
val x: u8 = 255
val a = x + 1      // 트랩 (디버그) 또는 정의된 에러 (릴리즈)
val b = x +% 1     // 0 — 래핑이 의도적
val c = x +| 1     // 255 — 포화가 의도적
```

이것들은 "검사를 끄는" 연산이 아니다. 프로그래머의 의도를 **코드에서 바로 읽을 수 있는** 서로 다른 연산이다.

동일 패턴이 뺄셈(`-`, `-%`, `-|`)과 곱셈(`*`, `*%`)에도 적용된다.

## 6.4 손실 캐스트 메서드

> **[설계 완료]** 손실 캐스트 메서드는 설계가 완료되었으며, 7단계 로드맵의 Stage 3에서 구현 예정입니다.

```kern
val big: i64 = 1000

// 확대: 항상 안전
val wide: i64 = small_i32.widen()

// 축소: 명시적으로 동작 선택 필요
val a = big.truncate<u8>()     // 232로 잘림 — "데이터 손실 감수"
val b = big.try_narrow<u8>()   // Option<u8> — 범위 초과 시 None
val c = big.clamp<u8>()        // 255 — 최대값으로 클램프
val d = big as u8              // ERROR: potentially lossy cast
```

## 6.5 타입된 하드웨어 추상화

> **[설계 완료]** Port\<T\>, MmioReg, PhysAddr/VirtAddr는 설계가 완료되었으며, 7단계 로드맵의 Stage 4에서 구현 예정입니다.

원시 포인터 산술 대신, 타입된 추상화를 사용한다:

```kern
// MMIO 레지스터
struct MmioReg<const ADDR: u64, T> {}

fn read_reg<const ADDR: u64, T>(reg: MmioReg<ADDR, T>) -> T with io {
    volatile_read(ADDR as Ptr<T>)
}

// I/O 포트
struct Port<T> { number: u16 }

fn inb(port: Port<u8>) -> u8 with io {
    asm { "in al, dx" }
}

// 주소 공간 타입
newtype PhysAddr = u64
newtype VirtAddr = u64

// PhysAddr + u64 = PhysAddr   (OK)
// PhysAddr + VirtAddr          (ERROR: 의미 없는 연산)
// PhysAddr는 역참조 불가 — 먼저 매핑해야 함
```

## 6.6 인라인 어셈블리 — 유일한 "탈출구"

`asm {}` 블록은 "검사를 끄는" 것이 아니다 — 다른 언어(어셈블리)로의 **다리**다. `io` 이펙트를 요구한다.

```kern
@naked fn switch_context(old_sp: Ptr<var u64>, new_sp: u64) with io {
    asm {
        "mov [rdi], rsp",
        "mov rsp, rsi",
        "ret"
    }
}
```

이것이 허용되는 이유:
- 의도가 명확: "이것은 CPU 수준 코드"
- `io` 이펙트 필요: 호출자가 이펙트를 인지해야 함
- **가시적**이고 **감사 가능**: `asm {`을 grep하면 모든 사용처를 찾을 수 있음

---

# 7. 컴파일러 아키텍처

## 7.1 4단계 IR 파이프라인

```
┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐
│  Source   │───▶│  Lexer   │───▶│  Parser  │───▶│   AST    │
│  (.kern) │    │          │    │          │    │ (untyped)│
└──────────┘    └──────────┘    └──────────┘    └────┬─────┘
                                                      │
                                                 HIRBuilder
                                                      │
┌──────────┐    ┌──────────┐    ┌──────────┐    ┌────▼─────┐
│  Binary  │◀───│  NASM+ld │◀───│  MachIR  │◀───│   HIR    │
│          │    │          │    │ (x86-64) │    │ (typed)  │
└──────────┘    └──────────┘    └────▲─────┘    └────┬─────┘
                                     │               │
                                  Backend        LIRBuilder
                                     │               │
                                ┌────┴─────┐    ┌────▼─────┐
                                │ RegAlloc │    │   LIR    │
                                │ InsSel   │    │  (SSA)   │
                                └──────────┘    └──────────┘
```

| IR 레벨 | 특징 | 주요 역할 |
|---------|------|----------|
| **AST** | 비타입, 문법 보존 | 파이프, match, fn-패턴 모두 보존 |
| **HIR** | 타입 부여, 디슈가, 고수준 연산 | 파이프→호출, fn-패턴→match. 모든 노드에 TypeId |
| **LIR** | SSA, VReg, 저수준 연산, 블록 인자 | phi 노드 없음. 이펙트 소거됨 |
| **MachIR** | x86-64 물리 레지스터 | NASM 출력 직전. 프레임 설정/해제 |

## 7.2 레이어별 상세

### Lexer (어휘 분석기)

- 수제(hand-written), 제로카피(string_view로 소스 참조)
- 최대 탐욕(maximal munch) 토크나이징
- 키워드 해시 룩업
- 75종 토큰, 33개 키워드

토큰은 소스 텍스트의 `string_view`를 보유한다. 소스가 모든 소비자보다 오래 살아야 하는 이유다.

### Parser (구문 분석기)

- 하이브리드: 재귀 하강(Recursive Descent) + Pratt 파싱
- Arena 할당 AST (discriminated union)
- AST: 26 Expr종 + 9 Stmt종 + 6 Pattern종

Pratt 파서가 `*`(역참조 vs 곱셈)과 `&`(참조 획득)의 단항/이항 모호성을 결합력(binding power)으로 자동 해결한다.

### HIR (High-level IR)

- 모든 노드에 `TypeId` 부여 (uint32_t, TypeTable에서 공유)
- 디슈가: 파이프 → 호출, 함수 패턴 → match
- 인라인 타입 검사 (v1의 TypeChecker 역할 통합)
- 27 ExprKind + 7 StmtKind + 6 PatternKind
- HIR 패스: PurityAnalysisPass, TailCallAnalysisPass, EffectAnalysisPass

### LIR (Low-level IR)

- SSA (Static Single Assignment) 형식
- 블록 인자(block arguments) — phi 노드 대신
- VReg (가상 레지스터) 기반
- 57 오피코드: 산술, 비교, 메모리, 제어 흐름, 원자적 연산, 펜스, per-CPU

### MachIR (Machine IR)

- x86-64 물리 레지스터: 14 GPR + 16 XMM + RSP/RBP
- 57 X86Op: Mov, Add, IMul, LockCmpxchg, Mfence 등
- 3개 의사 명령어: ParallelMove, FrameSetup, FrameDestroy
- System V AMD64 ABI: rdi, rsi, rdx, rcx, r8, r9 → rax 반환

### 어셈블리 + 링킹

- NASM (nasm -f macho64)
- ld (macOS 링커): `-platform_version macos 14.0.0 14.0.0 -arch x86_64 -lSystem`
- `_start` 엔트리 포인트에서 `_main` 호출 후 `exit` 시스템 콜

## 7.3 공유 인프라

### CompilationContext

모든 단계가 공유하는 컨텍스트:

```
CompilationContext
├── Arena          — 범프 할당자 (4096바이트 블록). 모든 AST/HIR/LIR/MachIR 노드 할당
├── StringPool     — 모든 식별자 인터닝. 문자열 비교 = 포인터 비교
├── TypeTable      — TypeId 관리. makePtr/makeFn/makeStruct/makeEnum/makeUnion
└── DiagnosticEngine — 에러/경고 메시지 수집
```

### Arena 할당

모든 IR 노드는 Arena에 할당된다. `new`를 직접 사용하지 않는다. Arena는 4096바이트 블록 단위로 범프 할당하며, 프로그램 수명 동안 해제하지 않는다(컴파일 완료 시 전체 해제).

### TypeId 시스템

`uint32_t` TypeId로 모든 타입을 표현한다. 기본 타입 13종은 사전 등록되어 있다.

```
TypeId 0-12: I8, I16, I32, I64, U8, U16, U32, U64, F32, F64, Bool, Unit, Error
TypeId 13+:  포인터, 함수, 구조체, 열거형, 유니온 (동적 할당)
```

## 7.4 레이어 의존성 매트릭스

```
Support → Lexer → Parser → HIR → LIR → Backend
                            ↑               ↑
                           IDE          (역방향 불가)
```

| 레이어 | 의존할 수 있는 레이어 |
|--------|---------------------|
| **Support** | (없음 — 기반 레이어) |
| **Lexer** | Support |
| **Parser** | Support, Lexer |
| **HIR** | Support, Lexer, Parser |
| **LIR** | Support, HIR |
| **Backend** | Support, LIR |
| **Pipeline** | 모든 라이브러리 (오케스트레이터) |

역방향 의존은 금지된다. 예를 들어, LIR이 Backend를 참조하거나, Parser가 HIR을 참조할 수 없다.

## 7.5 데이터 흐름

```
소스 텍스트 (.kern)
    │
    ▼ [Lexer]
Token Stream (string_view → 소스 참조)
    │
    ▼ [Parser]
AST (Arena 할당, 26 Expr + 9 Stmt + 6 Pattern)
    │
    ▼ [HIRBuilder] — 인라인 타입 검사 + 디슈가
HIR (모든 노드에 TypeId, 27 Expr + 7 Stmt)
    │
    ├──▶ [PurityAnalysisPass] — 순수성 추론 (Kahn's 역위상정렬)
    ├──▶ [TailCallAnalysisPass] — 꼬리호출 분석
    ├──▶ [EffectAnalysisPass] — 이펙트 추론 + 검증
    │
    ▼ [LIRBuilder] — 이펙트 소거
LIR (SSA, VReg, 57 opcodes, 블록 인자)
    │
    ▼ [InstructionSelector] — 패턴 매칭으로 LIR → MachIR
MachIR (x86-64, 57 X86Ops, PhysReg)
    │
    ├──▶ [RegisterAllocator] — 선형 스캔, 칼리 세이브드 추적
    │
    ▼ [NASMEmitter]
NASM 어셈블리 (.asm)
    │
    ▼ [nasm + ld]
네이티브 바이너리 (x86-64 Mach-O)
```

---

# 8. 커널 코드 예제

이 장의 예제들은 Kern의 모든 설계 결정이 커널 코드에서 어떻게 합쳐지는지 보여준다.

## 8.1 시리얼 포트 초기화

```kern
module Kernel.Serial

fn outb(port: u16, val: u8) -> Unit = intrinsic
fn inb(port: u16) -> u8 = intrinsic

fn baud_to_divisor(baud: u32) -> u16 {
    (115200 / baud) as u16
}
// 컴파일러 추론: pure, distributable

fn init_serial(port: u16, baud: u32) -> Unit with io {
    val divisor: u16 = baud_to_divisor(baud)
    outb(port + 1, 0x00)      // 인터럽트 비활성화
    outb(port + 3, 0x80)      // DLAB 설정
    outb(port + 0, divisor)   // 보 레이트 하위
    outb(port + 1, 0x00)      // 보 레이트 상위
    outb(port + 3, 0x03)      // 8N1
    outb(port + 2, 0xC7)      // FIFO 활성화
}
// 컴파일러 추론: impure(io), pinned

fn write_serial(port: u16, data: u8) -> Unit with io {
    outb(port, data)
}
// 컴파일러 추론: impure(io), pinned
```

## 8.2 스핀락 (Atomics)

```kern
fn acquire(lock: Ptr<var u64>) -> Unit with atomic {
    loop {
        val old: u64 = atomic_cas(lock, 0, 1)
        if old == 0 { break }
    }
}

fn release(lock: Ptr<var u64>) -> Unit with atomic {
    atomic_store(lock, 0)
    mfence()
}
```

`atomic_cas`는 x86-64의 `lock cmpxchg`로, `mfence`는 직접 `mfence` 명령어로 컴파일된다.

## 8.3 페이지 테이블 엔트리

```kern
struct PageEntry {
    frame: u64,
    flags: u16
}

fn make_entry(frame: u64, flags: u16) -> PageEntry {
    PageEntry { frame: frame, flags: flags }
}
// 순수 — 이펙트 없음

newtype PhysAddr = u64
newtype VirtAddr = u64
// PhysAddr + VirtAddr = 컴파일 에러 (의미 없는 연산)
```

## 8.4 드라이버 인터페이스 (트레이트 + 이펙트)

```kern
trait BlockDevice {
    fn read_block(block: u64, buf: var [u8]) with io
    fn write_block(block: u64, buf: [u8]) with io
    fn capacity() -> u64    // pure
}
```

트레이트가 이펙트를 선언하므로, 구현체는 이펙트를 반드시 가져야 한다. 단, 테스트용 모의 구현은 더 적은 이펙트(순수)를 가질 수 있다.

## 8.5 해시 함수 (래핑 연산)

> **[설계 완료]** 래핑 연산자(`+%`, `*%`, `^%`)는 설계가 완료되었으며 구현 예정입니다.

```kern
fn fnv1a(data: [u8]) -> u64 {
    var hash: u64 = 0xCBF29CE484222325
    for i in 0..data.len {
        hash = hash ^% data[i].widen()   // XOR 래핑
        hash = hash *% 0x100000001B3     // 곱셈 래핑
    }
    hash
}
```

`^%`와 `*%`는 의도적 래핑을 표현한다. 코드를 읽는 사람이 "이 오버플로우는 의도적"임을 즉시 알 수 있다.

## 8.6 인터럽트 핸들러

```kern
@naked fn timer_handler() -> Unit with io {
    asm volatile {
        "push rax",
        "push rcx",
        "push rdx",
        "push rsi",
        "push rdi",
        "push r8",
        "push r9",
        "push r10",
        "push r11"
    }
    // 인터럽트 처리 로직
    asm volatile {
        "pop r11",
        "pop r10",
        "pop r9",
        "pop r8",
        "pop rdi",
        "pop rsi",
        "pop rdx",
        "pop rcx",
        "pop rax",
        "iretq"
    }
}
```

`@naked` 속성은 컴파일러가 프롤로그/에필로그를 생성하지 않도록 한다. 인터럽트 핸들러처럼 레지스터 상태를 직접 제어해야 하는 경우에 사용한다.

## 8.7 인라인 어셈블리

```kern
fn read_cr3() -> u64 with io {
    asm volatile { "mov rax, cr3" }
}

fn write_cr3(val: u64) -> Unit with io {
    asm volatile { "mov cr3, rdi" }
}
```

실제 E2E 테스트로 검증된 인라인 어셈블리 예제:

```kern
fn asm_add(a: i64, b: i64) -> i64 {
    asm { "add rdi, rsi", "mov rax, rdi" }
}

fn main() -> i64 {
    asm_add(20, 22)    // 42
}
```

---

# 9. 표준 라이브러리

Kern의 표준 라이브러리(`stdlib/core.kern`)는 최소한의 핵심 타입과 인트린직을 정의한다.

## 9.1 Option\<T\>

```kern
union Option<T> { None, Some(T) }

fn is_some<T>(opt: Option<T>) -> bool {
    match opt { None => false, Some(_) => true }
}

fn is_none<T>(opt: Option<T>) -> bool {
    match opt { None => true, Some(_) => false }
}

fn unwrap_or<T>(opt: Option<T>, fallback: T) -> T {
    match opt { None => fallback, Some(v) => v }
}
```

## 9.2 Result\<T, E\>

```kern
union Result<T, E> { Ok(T), Err(E) }

fn is_ok<T, E>(r: Result<T, E>) -> bool {
    match r { Ok(_) => true, Err(_) => false }
}

fn is_err<T, E>(r: Result<T, E>) -> bool {
    match r { Ok(_) => false, Err(_) => true }
}

fn unwrap_ok<T, E>(r: Result<T, E>, fallback: T) -> T {
    match r { Ok(v) => v, Err(_) => fallback }
}

fn unwrap_err<T, E>(r: Result<T, E>, fallback: E) -> E {
    match r { Ok(_) => fallback, Err(e) => e }
}
```

## 9.3 유틸리티 함수

```kern
fn min(a: i64, b: i64) -> i64 { if a < b { a } else { b } }
fn max(a: i64, b: i64) -> i64 { if a > b { a } else { b } }
fn abs(x: i64) -> i64 { if x < 0 { 0 - x } else { x } }
fn clamp(x: i64, lo: i64, hi: i64) -> i64 {
    if x < lo { lo } else if x > hi { hi } else { x }
}
```

## 9.4 인트린직 카탈로그

### I/O 인트린직

| 함수 | 시그니처 | 설명 |
|------|---------|------|
| `print_i64` | `(i64) -> Unit` | 정수 출력 |
| `print_str` | `(String) -> Unit` | 문자열 출력 |

### 메모리 인트린직

| 함수 | 시그니처 | 설명 |
|------|---------|------|
| `mem_copy` | `(Ptr<var u8>, Ptr<u8>, u64) -> Unit` | 메모리 복사 |
| `mem_set` | `(Ptr<var u8>, u8, u64) -> Unit` | 메모리 설정 |
| `mem_cmp` | `(Ptr<u8>, Ptr<u8>, u64) -> i64` | 메모리 비교 |
| `ptr_add_bytes` | `(Ptr<u8>, u64) -> Ptr<u8>` | 포인터 가산 |
| `ptr_sub_bytes` | `(Ptr<u8>, u64) -> Ptr<u8>` | 포인터 감산 |

### 원자적 연산 인트린직

| 함수 | 시그니처 | x86-64 명령어 |
|------|---------|--------------|
| `atomic_load` | `(Ptr<u64>) -> u64` | `mov` (aligned) |
| `atomic_store` | `(Ptr<var u64>, u64) -> Unit` | `mov` (aligned) |
| `atomic_cas` | `(Ptr<var u64>, u64, u64) -> u64` | `lock cmpxchg` |
| `atomic_fetch_add` | `(Ptr<var u64>, u64) -> u64` | `lock xadd` |

### 펜스 인트린직

| 함수 | 시그니처 | x86-64 명령어 |
|------|---------|--------------|
| `mfence` | `() -> Unit` | `mfence` |
| `sfence` | `() -> Unit` | `sfence` |
| `lfence` | `() -> Unit` | `lfence` |
| `compiler_barrier` | `() -> Unit` | (컴파일러 재배치 방지) |

### Per-CPU 데이터 인트린직

| 함수 | 시그니처 | 주소 지정 |
|------|---------|----------|
| `percpu_load` | `(u64) -> u64` | GS 세그먼트 |
| `percpu_store` | `(u64, u64) -> Unit` | GS 세그먼트 |

### Volatile I/O 인트린직

| 함수 | 크기 | 설명 |
|------|------|------|
| `volatile_read_u8/u16/u32/u64` | 1/2/4/8 바이트 | volatile 메모리 읽기 |
| `volatile_write_u8/u16/u32/u64` | 1/2/4/8 바이트 | volatile 메모리 쓰기 |

전체 22개 인트린직으로 커널 개발에 필요한 모든 저수준 연산을 커버한다.

---

# 10. 도구 생태계

## 10.1 kernc — 컴파일러

메인 컴파일러 드라이버. 소스 파일을 받아 네이티브 바이너리를 생성한다.

```bash
kernc source.kern -o output
```

### 덤프 플래그

파이프라인의 각 단계를 시각화할 수 있는 덤프 플래그:

| 플래그 | 출력 |
|--------|------|
| `--dump-tokens` | 토큰 스트림 |
| `--dump-ast` | AST (S-expression) |
| `--dump-hir` | HIR (타입 부여된 S-expression) |
| `--dump-purity` | 함수별 순수성/재귀 정보 |
| `--dump-lir` | LIR (SSA, VReg) |
| `--dump-machir` | MachIR (x86-64, PhysReg) |
| `--dump-ir` | 레거시 IR (v1 호환) |

## 10.2 kern-lsp — LSP 서버

Language Server Protocol 서버. IDE에서 다음 기능을 제공한다:

- 실시간 진단 (에러/경고)
- 인레이 힌트 (추론된 타입, 순수성, 이펙트)
- 코드 내비게이션

## 10.3 kern-fmt — 코드 포매터

Kern 소스 코드를 일관된 스타일로 자동 포매팅한다.

## 10.4 kern-dbg — 디버거

> 스켈레톤 단계. 기본 프레임워크가 구축되어 있으며, 상세 기능은 개발 예정.

## 10.5 kern-repl — REPL

> 스켈레톤 단계. 대화형 실행 환경 프레임워크가 구축되어 있으며, 상세 기능은 개발 예정.

## 10.6 kern-pkg — 패키지 매니저

> 스켈레톤 단계. 패키지 관리 프레임워크가 구축되어 있으며, 상세 기능은 개발 예정.

---

# 11. 벤치마크

## 11.1 테스트 환경

| 항목 | 값 |
|------|-----|
| 기계 | Apple Silicon (arm64) macOS |
| 실행 환경 | Rosetta 2 (x86-64 바이너리 에뮬레이션) |
| 벤치마크 프로그램 | `fib(35)` — 재귀 피보나치 |
| 결과값 | 9,227,465 (종료 코드: 201 = 9227465 % 256) |

## 11.2 컴파일 시간

전체 파이프라인: 소스 → 렉서 → 파서 → HIR → LIR → MachIR → NASM → ld

| 실행 | 시간 |
|------|------|
| 1 | 60ms |
| 2 | 59ms |
| 3 | 56ms |
| **평균** | **~58ms** |

58ms 안에 소스 코드에서 네이티브 바이너리가 생성된다. NASM 어셈블링과 ld 링킹 시간을 포함한 수치다.

v1 파이프라인 대비: v1의 ~54ms에서 v2는 ~58ms. 4단계 IR 파이프라인 추가에도 불구하고 컴파일 시간 증가가 미미하다(+7%).

## 11.3 실행 시간

| 실행 | Kern (v2) | C (gcc -O2) |
|------|-----------|-------------|
| 콜드 스타트 | 552ms | 401ms |
| 웜 1 | 95ms | 29ms |
| 웜 2 | 94ms | 29ms |
| **웜 평균** | **~95ms** | **~29ms** |

### 분석

- **콜드 스타트**: Rosetta 2 번역 캐시 워밍업 포함. 네이티브 x86-64에서는 이 오버헤드가 없다.
- **웜 실행**: Kern은 gcc -O2 대비 약 3.3배 느리다. 이는 Kern 컴파일러가 아직 최적화 패스(인라이닝, 상수 전파 등)를 구현하지 않았기 때문이다.
- **비교 관점**: 최적화 패스 없이 3.3배는 합리적인 수준. gcc -O0과 비교하면 유사한 성능대.

## 11.4 생성 코드 품질

`fib(35)` 함수의 MachIR 출력:

```nasm
fn @fib (stack=64):
.entry_0:
    mov [rbp-56], rdi           ; 매개변수 저장
    mov rcx, 1
    xor rdx, rdx
    cmp [rbp-56], rcx           ; n <= 1 비교
    setle dl
    test dl, dl
    jne .if_then_1              ; 기저 사례
    jmp .if_else_2              ; 재귀 사례
.if_then_1:
    mov r11, [rbp-56]           ; return n
    ...
.if_else_2:
    mov rsi, [rbp-56]
    sub rsi, 1                  ; n - 1
    mov rdi, rsi
    call _fib                   ; fib(n-1)
    mov rbx, rax                ; 결과 보존 (callee-saved)
    mov r13, [rbp-56]
    sub r13, 2                  ; n - 2
    mov rdi, r13
    call _fib                   ; fib(n-2)
    add rbx, rax                ; fib(n-1) + fib(n-2)
    ...
```

`main` 함수에서 `fib(35)` 호출은 꼬리호출 최적화(TCO)가 적용되어 `jmp` 명령어로 컴파일된다:

```nasm
fn @main (stack=56):
.entry_0:
    mov rax, 35
    mov rdi, rax
    frame_destroy
    jmp _fib                    ; TCO — call 대신 jmp
```

## 11.5 제로코스트 증명: 파이프 디슈가

```kern
// 파이프 사용
val r1: i64 = 10 |> add(3) |> multiply(2)

// 직접 호출
val r2: i64 = multiply(add(10, 3), 2)
```

두 코드는 **완전히 동일한** IR/MachIR/어셈블리를 생성한다. 파이프는 파서 수준에서 디슈가되므로 HIR 단계에서 이미 구분이 불가능하다.

## 11.6 벤치마크 재현 방법

```bash
# Kern 벤치마크
echo 'fn fib(n: i64) -> i64 { if n <= 1 { n } else { fib(n-1) + fib(n-2) } } fn main() -> i64 { fib(35) }' > /tmp/bench_fib.kern
time build/tools/kernc/kernc /tmp/bench_fib.kern -o /tmp/bench_fib
time /tmp/bench_fib

# C 벤치마크
cat > /tmp/bench_fib.c << 'EOF'
#include <stdlib.h>
long fib(long n) { return n <= 1 ? n : fib(n-1) + fib(n-2); }
int main() { return fib(35) % 256; }
EOF
gcc -O2 -o /tmp/bench_fib_c /tmp/bench_fib.c
time /tmp/bench_fib_c
```

---

# 12. 언어 비교

## 12.1 15개 카테고리 비교 테이블

| 카테고리 | Kern | C | Rust | Haskell | Zig |
|----------|------|---|------|---------|-----|
| **패러다임** | 순수 함수형 | 절차적 | 멀티패러다임 | 순수 함수형 | 절차적 |
| **타입 시스템** | 강타입, 정적 | 약타입, 정적 | 강타입, 정적 | 강타입, 정적 | 강타입, 정적 |
| **메모리 관리** | 수동 + 소유권 | 수동 | 소유권 + 빌림 | GC | 수동 |
| **이펙트 추적** | 컴파일 타임 이펙트 | 없음 | 없음 | IO 모나드 | 없음 |
| **Unsafe** | 없음 (asm만) | 전체 | `unsafe` 블록 | `unsafePerformIO` | `@intToPtr` |
| **제네릭** | 단형화 | 없음 (매크로) | 단형화 | 타입 소거 | 컴프타임 |
| **패턴 매칭** | O (exhaustive) | 없음 | O (exhaustive) | O (exhaustive) | switch 문 |
| **꼬리호출 최적화** | 자동 | 보장 안됨 | 보장 안됨 | O | 보장 안됨 |
| **파이프 연산자** | O (`\|>`) | 없음 | 없음 | 없음 (`$` 유사) | 없음 |
| **GC 의존** | 없음 | 없음 | 없음 | 필수 | 없음 |
| **인라인 어셈블리** | O | O (GCC 확장) | O | 없음 | O |
| **커널 개발** | 설계 목표 | 주력 | 가능 | 불가 | 가능 |
| **가독성** | 높음 | 중간 | 낮음-중간 | 높음 (익숙하면) | 높음 |
| **학습 곡선** | 낮음 | 중간 | 높음 | 높음 | 중간 |
| **에코시스템 성숙도** | 초기 | 매우 성숙 | 성숙 | 성숙 | 성장 중 |

## 12.2 동일 알고리즘 — 4개 언어 비교

### 피보나치

**C:**
```c
long fib(long n) {
    return n <= 1 ? n : fib(n-1) + fib(n-2);
}
```

**Rust:**
```rust
fn fib(n: i64) -> i64 {
    if n <= 1 { n } else { fib(n-1) + fib(n-2) }
}
```

**Haskell:**
```haskell
fib :: Integer -> Integer
fib n
  | n <= 1    = n
  | otherwise = fib (n-1) + fib (n-2)
```

**Kern:**
```kern
fn fib(n: i64) -> i64 {
    if n <= 1 { n }
    else { fib(n - 1) + fib(n - 2) }
}
```

네 언어 모두 간결하지만, Kern과 Rust가 가장 유사한 문법을 가진다. Kern은 Rust에서 라이프타임 시스템의 복잡성을 제거한 형태로 볼 수 있다.

### 하드웨어 I/O

**C:**
```c
void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
// 어디서든 호출 가능 — 컴파일러가 제한하지 않음
```

**Rust:**
```rust
unsafe fn outb(port: u16, val: u8) {
    core::arch::asm!("out dx, al", in("dx") port, in("al") val);
}
// unsafe 블록 필요
```

**Kern:**
```kern
fn outb(port: u16, val: u8) -> Unit = intrinsic
// with io 이펙트 자동 추론
// 순수 함수에서 호출하면 컴파일 에러
```

C는 아무 제한 없이 I/O 함수를 호출할 수 있어 위험하다. Rust는 `unsafe` 블록으로 위험 영역을 표시하지만 "어떤 종류의 위험인지"는 구분하지 않는다. Kern은 이펙트 시스템으로 "이것은 I/O다"를 명확히 표현하고, 순수 함수에서의 호출을 구조적으로 금지한다.

### 논리 연산

**C / Rust:**
```c
if (a > 0 && b > 0 || !flag) { ... }
```

**Kern:**
```kern
if a > 0 and b > 0 or not flag { ... }
```

`and`, `or`, `not` 키워드는 코드를 자연어처럼 읽을 수 있게 한다.

## 12.3 가독성 비교 — IO 처리

**Haskell (IO 모나드):**
```haskell
processInput :: IO ()
processInput = do
    line <- getLine
    let n = read line :: Int
    putStrLn (show (fib n))
```

**Kern (이펙트 어노테이션):**
```kern
fn process_input() -> Unit with io {
    val line: String = read_line()
    val n: i64 = parse_i64(line)
    print_i64(fib(n))
}
```

Haskell의 `do` 표기법과 `<-` 바인딩이 Kern에서는 일반적인 `val` 바인딩으로 대체된다. `with io`가 같은 순수성 보장을 제공하면서도, 코드는 명령형 언어만큼 직관적으로 읽힌다.

---

# 13. 로드맵

## 13.1 완료된 마일스톤

| 마일스톤 | 내용 | 테스트 |
|----------|------|--------|
| **M1** | Lexer + Parser + TypeChecker + IRBuilder + CodeGen. fib(35) 동작 | 35 unit + 4 E2E |
| **M2** | 강타입 시스템 (13 primitives), 순수성 추론, Purity dump | 255 unit + 44 E2E |
| **M3** | f32/f64 지원, 컨텍스트 기반 타입 추론, XMM 레지스터 | 추가 unit + E2E |
| **M4** | 파이프 `\|>`, match 표현식, 함수 패턴 매칭, intrinsic | 360 unit + 78 E2E |
| **M5** | 에러 경로 커버리지 100% | 517 unit + 113 E2E |
| **TCO** | 일반 꼬리호출 제거 (자기/상호 재귀) | 추가 14 unit + 8 E2E |
| **v2 Phase 0** | CMake 인프라, Coverage, StringPool, TypeSystem, CompilationContext | 신규 인프라 |
| **v2 Phase 1** | StringPool, TypeTable, Arena, DiagnosticEngine 통합 | 추가 unit |
| **v2 Phase 2** | HIR 레이어: HIRBuilder, HIRDump, HIRPasses | 637 unit + 114 E2E |
| **v2 Phase 3** | LIR 레이어: SSA, VReg, 블록 인자, 57 opcodes | 추가 unit + E2E |
| **v2 Phase 4a** | Backend: InstructionSelector, RegisterAllocator, NASMEmitter | 784 unit + 116 E2E |
| **v2 Phase 5** | LSP, IDE DiagnosticProvider, kern-fmt, kern-lsp | 추가 tool tests |
| **v2 Phase 6** | 디버거 스켈레톤 (kern-dbg) | - |
| **v2 Phase 7** | 패키지 매니저 스켈레톤 (kern-pkg), REPL 스켈레톤 (kern-repl) | - |
| **OS Phase A** | 비트연산, mod, loop, 배열, 인라인 asm, 포인터 산술, 캐스트, volatile, noreturn | 추가 E2E |
| **OS Phase B** | packed/aligned 구조체, 타입 별칭, 뉴타입, 슬라이스, 모듈, 링커 스크립트, @section | 추가 E2E |
| **OS Phase C** | 제네릭(단형화), 함수 포인터, naked/interrupt, const fn, 트레이트, 클로저, ? 연산자 | 추가 E2E |
| **OS Phase D** | Atomics, fences, per-CPU(GS 세그먼트), const 제네릭 | 추가 E2E |
| **이펙트 Stage 1** | EffectSet 비트마스크, `with` 절 파싱, EffectAnalysisPass, 이펙트 강제 | 636 unit + 259 E2E |
| **v1 삭제** | sema/, ir/, codegen/ 코드 + 테스트 제거 (-9,286줄) | - |

## 13.2 미래 로드맵 — 7단계

### Stage 1: 이펙트 시스템 고도화

이펙트 다형성(고차 함수의 이펙트 변수 통합), 트레이트 이펙트 반공변 검사, 이펙트 소거 검증.

### Stage 2: 오너십 라이트 (Ownership Lite)

Rust보다 단순한 소유권 모델. 세 가지 전달 모드(기본 빌림, `var` 가변 빌림, `own` 소유권 이전). 라이프타임 어노테이션 없음. 사용 후 이동 감지, 빌림 탈출 감지, 가변 빌림 앨리어싱 금지.

### Stage 3: 컴파일 타임 검증

래핑 연산자(`+%`, `-%`, `*%`), 포화 연산자(`+|`, `-|`), 손실 캐스트 메서드, 흐름 감응 범위 좁히기, 루프 경계 범위 추론.

### Stage 4: 타입된 하드웨어 추상화

`Port<T>`, `MmioReg<ADDR, T>`, `PhysAddr`/`VirtAddr` 뉴타입. `Ptr<T>` 생성을 `mem` 이펙트 함수로 제한.

### Stage 5: OS 인프라

전역 변수 (static val/var → .data/.bss), 적응적 디스패치 (자동 정적/동적), 강화된 모듈 시스템, 모듈 시그니처 파일(.kerni).

### Stage 6: 최적화 파이프라인

LIR 패스: 함수 인라이닝, 상수 전파, 공통 부분식 제거(CSE), 클로저 구조체 제거, 탈가상화(devirtualization), 루프 불변 코드 이동, HOF 체인 루프 융합.

### Stage 7: 멀티 아키텍처

Backend 추상화 인터페이스, AArch64 InstructionSelector/RegisterAllocator/Emitter, 조건부 컴파일(`@cfg`), 교차 컴파일(`--target` 플래그).

## 13.3 기능 구현 상태 매트릭스

| 기능 | 상태 | 관련 Stage |
|------|------|-----------|
| val/var 바인딩 | 구현 완료 | - |
| 13 기본 타입 | 구현 완료 | - |
| 함수 + 자동 추론 | 구현 완료 | - |
| 파이프 `\|>` | 구현 완료 | - |
| 패턴 매칭 (match + fn-level) | 구현 완료 | - |
| 클로저 / HOF / 반환 | 구현 완료 | - |
| 제네릭 (단형화) | 구현 완료 | - |
| const 제네릭 | 구현 완료 | - |
| 트레이트 | 구현 완료 | - |
| 구조체 / 열거형 / 유니온 | 구현 완료 | - |
| 배열 / 슬라이스 | 구현 완료 | - |
| 포인터 / 참조 | 구현 완료 | - |
| 인라인 어셈블리 | 구현 완료 | - |
| naked / interrupt 함수 | 구현 완료 | - |
| loop / break / continue | 구현 완료 | - |
| 모듈 / import | 구현 완료 | - |
| Atomics (lock cmpxchg/xadd) | 구현 완료 | - |
| Fences (mfence/sfence/lfence) | 구현 완료 | - |
| Per-CPU (GS 세그먼트) | 구현 완료 | - |
| Volatile I/O | 구현 완료 | - |
| 순수성 자동 추론 | 구현 완료 | - |
| 꼬리호출 최적화 (TCO) | 구현 완료 | - |
| EffectSet 비트마스크 | 구현 완료 | Stage 1 |
| `with` 절 파싱 | 구현 완료 | Stage 1 |
| EffectAnalysisPass | 구현 완료 | Stage 1 |
| ? 연산자 | 구현 완료 | - |
| 타입 별칭 / 뉴타입 | 구현 완료 | - |
| `own` 키워드 파싱 | 구현 완료 | Stage 2 |
| 사용 후 이동 감지 | 구현 완료 | Stage 2 |
| 이펙트 다형성 | **설계 완료** | Stage 1 |
| 트레이트 이펙트 반공변 | **설계 완료** | Stage 1 |
| 오너십 라이트 (전체) | **설계 완료** | Stage 2 |
| 래핑 연산자 `+%`/`-%`/`*%` | **설계 완료** | Stage 3 |
| 포화 연산자 `+\|`/`-\|` | **설계 완료** | Stage 3 |
| 흐름 감응 타입 | **설계 완료** | Stage 3 |
| 손실 캐스트 메서드 | **설계 완료** | Stage 3 |
| Port\<T\> / MmioReg | **설계 완료** | Stage 4 |
| PhysAddr / VirtAddr 분리 | **설계 완료** | Stage 4 |
| 대수적 이펙트 | **향후 계획** | 미정 |
| ARM64 백엔드 | **향후 계획** | Stage 7 |
| 선형 타입 | **향후 계획** | 미정 |

---

# 14. BNF 문법 (부록)

Phase D까지의 전체 BNF 문법.

```bnf
program         = { top_level_decl }

top_level_decl  = fn_decl
                | struct_decl
                | enum_decl
                | union_decl
                | type_alias
                | newtype_decl
                | trait_decl
                | impl_block
                | module_decl
                | import_decl

(* ── 모듈 ── *)
module_decl     = "module" IDENT { "::" IDENT }
import_decl     = "import" IDENT { "::" IDENT }

(* ── 타입 별칭 / 뉴타입 ── *)
type_alias      = "type" IDENT [ type_params ] "=" type
newtype_decl    = "newtype" IDENT [ type_params ] "=" type

(* ── 트레이트 / impl ── *)
trait_decl      = "trait" IDENT [ type_params ] "{" { trait_method } "}"
trait_method    = "fn" IDENT "(" param_list ")" "->" type [ effect_clause ]
impl_block      = "impl" IDENT [ type_params ] "for" type_ref "{"
                    { fn_decl }
                  "}"

(* ── 함수 ── *)
fn_decl         = [ fn_attrs ] "fn" IDENT [ type_params ]
                  "(" param_or_pattern_list ")" "->" type
                  [ effect_clause ] ( block | "=" "intrinsic" )

fn_attrs        = { "@" IDENT }           (* @packed, @aligned(N),
                                              @naked, @interrupt, @section("name") *)

effect_clause   = "with" effect_name { "," effect_name }
effect_name     = "pure" | "io" | "atomic" | "mut" | "mem"

type_params     = "<" type_param { "," type_param } ">"
type_param      = IDENT
                | "const" IDENT ":" type   (* const 제네릭 *)

param_or_pattern_list = [ param_or_pattern { "," param_or_pattern } ]
param_or_pattern      = param | pattern_param

param           = IDENT ":" [ passing_mode ] type
passing_mode    = "var" | "own"

pattern_param   = INT_LIT | BOOL_LIT

(* ── 구조체 / 열거형 / 유니온 ── *)
struct_decl     = [ struct_attrs ] "struct" IDENT [ type_params ] "{"
                    field { "," field }
                  "}"
struct_attrs    = { "@" IDENT [ "(" INT_LIT ")" ] }

field           = IDENT ":" type

enum_decl       = "enum" IDENT [ type_params ] "{"
                    IDENT { "," IDENT }
                  "}"

union_decl      = "union" IDENT [ type_params ] "{"
                    variant { "," variant }
                  "}"
variant         = IDENT [ "(" type ")" ]

(* ── 타입 ── *)
type            = type_ref
type_ref        = IDENT [ "<" type_arg_list ">" ]
                | "[" type [ ";" const_expr ] "]"    (* 배열 *)
                | "(" type_list ")" "->" type         (* 함수 타입 *)
                | "Ptr" "<" [ "var" ] type ">"        (* 포인터 *)

type_arg_list   = type_arg { "," type_arg }
type_arg        = type | INT_LIT                      (* const 제네릭 인자 *)
type_list       = [ type { "," type } ]

(* ── 문장 ── *)
block           = "{" { statement } [ expr ] "}"
statement       = val_decl | var_decl | assignment | return_stmt
                | expr_stmt | break_stmt | continue_stmt

val_decl        = "val" IDENT ":" type "=" expr
var_decl        = "var" IDENT ":" type "=" expr
assignment      = IDENT "=" expr
                | IDENT "." IDENT "=" expr            (* 필드 할당 *)
                | "*" expr "=" expr                   (* 역참조 할당 *)
                | expr "[" expr "]" "=" expr          (* 인덱스 할당 *)
return_stmt     = "return" [ expr ]
break_stmt      = "break" [ expr ]
continue_stmt   = "continue"
expr_stmt       = expr

(* ── 표현식 ── *)
expr            = if_expr | match_expr | loop_expr | pipe_expr
                | lambda | asm_expr | try_expr

pipe_expr       = binary_expr { "|>" ( call_expr | IDENT ) }

binary_expr     = unary_expr { binop unary_expr }
binop           = "+" | "-" | "*" | "/" | "%"
                | "==" | "!=" | "<" | ">" | "<=" | ">="
                | "and" | "or"
                | "|" | "^" | "~" | "<<" | ">>"

unary_expr      = [ "-" | "not" | "*" | "&" | "~" ] postfix_expr
postfix_expr    = primary { "." IDENT [ "(" arg_list ")" ]     (* 필드/메서드 *)
                          | "[" expr "]"                        (* 인덱스 *)
                          | "as" type                           (* 캐스트 *)
                          | "?"                                 (* try *)
                          }

primary         = INT_LIT | FLOAT_LIT | STRING_LIT | BOOL_LIT
                | IDENT [ "(" arg_list ")" ]          (* 변수 또는 호출 *)
                | IDENT "::" IDENT [ "(" arg_list ")" ] (* 열거/유니온 접근 *)
                | IDENT "{" field_init_list "}"        (* 구조체 리터럴 *)
                | "[" expr_list "]"                    (* 배열 리터럴 *)
                | "(" expr ")"                         (* 그룹핑 *)
                | block
                | "sizeof" "(" type ")"
                | "alignof" "(" type ")"

call_expr       = IDENT "(" arg_list ")"
arg_list        = [ expr { "," expr } ]
expr_list       = [ expr { "," expr } ]
field_init_list = field_init { "," field_init }
field_init      = IDENT ":" expr

(* ── 제어 흐름 ── *)
if_expr         = "if" expr block [ "else" ( if_expr | block ) ]

match_expr      = "match" expr "{" { match_arm } "}"
match_arm       = pattern [ "if" expr ] "=>" expr [ "," ]

loop_expr       = "loop" block

(* ── 패턴 ── *)
pattern         = "_"                                  (* 와일드카드 *)
                | INT_LIT                              (* 정수 리터럴 *)
                | BOOL_LIT                             (* 불리언 리터럴 *)
                | IDENT                                (* 변수 바인딩 *)
                | IDENT "(" pattern ")"                (* 열거/유니온 분해 *)

(* ── 람다 ── *)
lambda          = "{" [ lambda_params "=>" ] expr "}"
lambda_params   = lambda_param { "," lambda_param }
lambda_param    = IDENT [ ":" type ]

(* ── 인라인 어셈블리 ── *)
asm_expr        = "asm" [ "volatile" ] "{"
                    STRING_LIT { "," STRING_LIT }
                  "}"

(* ── Try ── *)
try_expr        = postfix_expr "?"

(* ── 상수 표현식 ── *)
const_expr      = INT_LIT | IDENT                      (* sizeof, const 제네릭 *)

(* ── 리터럴 ── *)
INT_LIT         = DIGIT { DIGIT }
                | "0x" HEX_DIGIT { HEX_DIGIT }
                | "0b" BIN_DIGIT { BIN_DIGIT }
FLOAT_LIT       = DIGIT { DIGIT } "." DIGIT { DIGIT } [ "f" ]
STRING_LIT      = '"' { char | escape } '"'
BOOL_LIT        = "true" | "false"
IDENT           = ALPHA { ALPHA | DIGIT | "_" }
```

### 키워드 목록 (33개)

```
fn    val    var    match   return  if     else    and     or
not   true   false  struct  enum    union  loop    break   continue
as    asm    volatile noreturn type  newtype module import  sizeof
alignof trait impl  const   with    own
```

### 연산자 우선순위 (낮음 → 높음)

| 우선순위 | 연산자 | 결합성 |
|----------|--------|--------|
| 1 | `\|>` | 좌 |
| 2 | `or` | 좌 |
| 3 | `and` | 좌 |
| 4 | `==`, `!=` | 좌 |
| 5 | `<`, `>`, `<=`, `>=` | 좌 |
| 6 | `\|` | 좌 |
| 7 | `^` | 좌 |
| 8 | `&` (비트) | 좌 |
| 9 | `<<`, `>>` | 좌 |
| 10 | `+`, `-` | 좌 |
| 11 | `*`, `/`, `%` | 좌 |
| 12 | 단항 `-`, `not`, `*`, `&`, `~` | 우 |
| 13 | 후위 `.`, `[]`, `as`, `?` | 좌 |

---

> **Kern** — 커널을 위한 순수 함수형 언어.
> C의 성능, Haskell의 안전성, 그리고 읽을 수 있는 코드.
