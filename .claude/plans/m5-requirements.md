# M5 Requirements Specification — 사용자 정의 타입 + 포인터

> 작성일: 2026-03-01
> 최종 수정: 2026-03-01
> 상태: **M5 전체 완료 ✅** (517 unit + 113 E2E)
> 선행 조건: M4 완료 (360 unit + 78 E2E)

---

## 1. 개요

M5는 Kern을 원시 타입 전용 언어에서 **복합 타입을 지원하는 시스템 프로그래밍 언어**로 전환하는 마일스톤.
4개의 서브 마일스톤으로 분리하여 점진적으로 구현한다.

**핵심 원칙:**
- Kern은 자체적으로 OS를 개발할 수 있는 언어. **C ABI/외부 연결성 배제**, Kern 자체 ABI로 고성능 최적화
- **부수효과 없음을 증명**하는 것이 최적화보다 중요. 변할 수 있는 메모리는 프로그래머가 명시적으로 선언
- 어노테이션은 필요 시 도입 가능 (purity 자동 추론과는 별개 영역)

**순서:** M5a (Struct) ✅ → M5b (Enum + Union) ✅ → M5c (Ptr\<T\>) ✅ → M5d (String) ✅ + 에러 경로 커버리지 ✅

---

## 2. 타입 선언 키워드 — 세 가지

```
struct  — 구조 (AND). 필드를 모두 가진다.
enum    — 상수 (OR).  이름 붙은 정수. 데이터 없음.
union   — 선택 (OR).  데이터를 담는 tagged union (ADT).
```

**분리 근거:** enum이 struct의 역할까지 가지려 하면 읽기 어려워진다.
상수 집합(enum)과 데이터를 담는 선택(union)은 본질적으로 다른 개념.

---

## 3. 확정된 설계 결정

### 3.1 전체 설계

| 항목 | 결정 | 근거 |
|------|------|------|
| 외부 연결성 | **배제** — Kern OS는 Kern만으로 개발 | C FFI 불필요, 자체 ABI 최적화 |
| Purity 기준 | **소스 수준 의미론만** — ABI 내부 포인터는 투명 | 컴파일러 구현 ≠ 언어 의미 |
| 어노테이션 | 필요 시 도입 가능 | purity 자동 추론과 별개 영역 |

### 3.2 Struct 설계

| 항목 | 결정 | 근거 |
|------|------|------|
| 키워드 | `struct` | Rust/C 스타일, Kern의 Kotlin-base 문법과 조화 |
| 인스턴스 생성 | `Name { field: val }` | 명시적, 가독성 우선 |
| 필드 접근 | `.` (dot) | 직관적. 포인터는 `(*ptr).x` 필수 |
| 필드 가변성 | **필드 수준 val/var** | 기본 val(생략 시 불변), var 명시로 변경 허용 |
| 변경 조건 | **바인딩 var + 필드 var** 둘 다 충족 시만 | 안전성 극대화 |
| 메모리 레이아웃 | 스택 할당 + **선언 순서 고정** | 커널 = 하드웨어 매핑 안전 우선 |
| 향후 레이아웃 | 재배치 어노테이션 추가 가능 | 최적화는 opt-in |
| Struct ABI | **Kern ABI** — ≤16B 레지스터, >16B 포인터 | 고성능, C ABI 배제 |

### 3.3 Enum + Union 설계

| 항목 | 결정 | 근거 |
|------|------|------|
| 상수 집합 | `enum` 키워드 | 데이터 없음, 이름 붙은 정수 |
| ADT | `union` 키워드 | 데이터를 담는 tagged union |
| enum 접근자 | `.` (dot) — `Color.Red` | 단순 접근 |
| union 접근자 | `::` — `Shape::Circle(...)` | "데이터 선택지" 신호 |
| union 원칙 | **타입을 감싸기만** — 구조 정의는 struct에게 | 역할 분리, 읽기 쉬움 |
| variant 추론 | variant 이름 = struct 이름이면 `{}` 추론 허용 | 장황함 제거 |
| 이름 불일치 시 | 명시 필요: `Shape::V(StructName { ... })` | 모호하면 에러 |
| match 자격 | 타입 생략 허용 (match 대상에서 추론) | 간결한 패턴 매칭 |
| 구조 분해 | match에서 variant 데이터 destructuring 지원 | ADT 핵심 기능 |
| 소진성 | 모든 variant 처리 필수 (또는 `_`) | 안전성 |

### 3.4 포인터 설계

| 항목 | 결정 | 근거 |
|------|------|------|
| 포인터 종류 | **2종** — `Ptr<T>`, `Ptr<var T>` | 단순하고 실용적 |
| `Ptr<T>` | 읽기 전용 → **pure** | 쓰기 불가, 부수효과 없음 |
| `Ptr<var T>` | 읽기/쓰기 → **impure(mem)** | 메모리 변경 |
| 주소 획득 | `&x` → `Ptr<T>` (val/var 모두 OK) | 읽기 전용 뷰 |
| 변경 가능 주소 | `&var x` → `Ptr<var T>` (var만) | 명시적 변경 의도 |
| 역참조 | `*ptr` (단항 접두사) | C/Rust 스타일 |
| 자동 역참조 | **없음** — `(*ptr).field` 필수 | Kern 명시성 원칙 |
| 매개변수 변경 | **Ptr\<var T\>로만** — 매개변수는 항상 val | 호출자가 `&var`로 의도 인지 |

### 3.5 Purity 체계

```
pure           부수효과 없음, 결정적. 메모이제이션/병렬화/제거 가능
impure(mut)    로컬 변이 (var 필드 변경). 호출자에게 전파 안 됨
impure(mem)    메모리 변경 (Ptr<var T> 통한 쓰기). 호출자에게 전파
impure(io)     하드웨어 상호작용 (intrinsic). 호출자에게 전파
```

**Purity 판단 원천:**

| 원천 | Purity | 전파 |
|------|--------|------|
| `var` 바인딩의 `var` 필드 변경 | impure(mut) | X |
| `Ptr<T>` 역참조 (읽기) | pure | — |
| `Ptr<var T>` 역참조/쓰기 | impure(mem) | O |
| `= intrinsic` 함수 호출 | impure(io) | O |
| 값 타입 필드 접근 (`s.x`) | pure | — |
| ABI 내부 포인터 (큰 struct 전달) | pure | — |

### 3.6 String 설계

| 항목 | 결정 | 근거 |
|------|------|------|
| 리터럴 | `"Hello, Kern!"` | 표준 문법 |
| 내부 표현 | Fat pointer (`Ptr<u8>` + `u64` len) | 안전, null 종료 불필요 |
| 배치 | `.rodata` 섹션 | 불변 리터럴 데이터 |
| ABI | 16B → 레지스터 2개 (rdi=ptr, rsi=len) | Kern ABI 규칙 따름 |

---

## 4. M5a — Struct (구조체) ✅ 완료 (398 unit + 86 E2E, commit 58d31d4)

### 4.1 기능 요구사항

**FR-1: Struct 선언 — 필드 수준 val/var**
```kern
struct Point {
    x: i64,                // val (기본) — 불변
    y: i64                 // val (기본) — 불변
}

struct ProcessControlBlock {
    pid: u64,              // val — 한번 설정되면 절대 안 바뀜
    var state: u8,         // var — 변경 가능
    var stack_ptr: Ptr<u8> // var — 변경 가능
}
```
- `struct` 키워드로 선언
- 필드는 `[var] name: Type` 형식, 쉼표로 구분
- 기본은 val (생략 시 불변) — Kern의 순수 함수형 기본값
- `var` 명시로 변경 가능 필드 선언
- 중첩 struct 지원
- 빈 struct 허용: `struct Unit {}`

**FR-2: Struct 인스턴스 생성**
```kern
val p: Point = Point { x: 10, y: 20 }
val pcb: ProcessControlBlock = ProcessControlBlock {
    pid: 1, state: 0, stack_ptr: ...
}
```
- `TypeName { field: expr, ... }` 문법
- 모든 필드 반드시 초기화 (부분 초기화 불가 → 컴파일 에러)
- 필드 순서는 선언 순서와 무관 (이름으로 매칭)

**FR-3: 필드 접근 + 변경**
```kern
// 읽기 — 항상 가능
val x: i64 = p.x
val nested: i64 = pixel.pos.x

// 변경 — var 바인딩 + var 필드일 때만
var pcb: PCB = PCB { pid: 1, state: 0, ... }
pcb.state = 1            // OK — 바인딩 var + 필드 var
// pcb.pid = 999         // 컴파일 에러! 필드가 val

val pcb2: PCB = PCB { pid: 2, state: 0, ... }
// pcb2.state = 1        // 컴파일 에러! 바인딩이 val
```

**변경 가능 조건 (두 조건 모두 충족):**

| 바인딩 | 필드 | 결과 |
|--------|------|------|
| val | val | 불변 |
| val | var | 불변 (바인딩이 잠금) |
| var | val | 불변 (필드가 잠금) |
| var | var | **변경 가능** → impure(mut) |

**FR-4: 함수 인자/반환값 (Kern ABI)**
```kern
fn distance(a: Point, b: Point) -> f64 { ... }
fn origin() -> Point { Point { x: 0, y: 0 } }
```

**Kern Struct ABI:**
- **≤8B (1 word):** 레지스터 1개 (rdi, rsi, ... / rax)
- **9~16B (2 words):** 레지스터 2개 (rdi+rsi / rax+rdx)
- **>16B:** caller가 스택 공간 할당 → 포인터 전달 → callee가 채움
- 값 의미론: 전달 시 항상 복사
- Float 필드 포함 시 XMM 레지스터 사용
- **매개변수는 항상 val** — 함수 내에서 인자의 var 필드도 변경 불가
- 변경하려면 `Ptr<var T>`로 전달 (M5c)

**FR-5: Purity 연동**
```kern
// 읽기만 → pure
fn get_state(pcb: PCB) -> u8 { pcb.state }

// 로컬 var + var 필드 변경 → impure(mut)
fn make_running() -> PCB {
    var pcb: PCB = PCB { pid: 1, state: 0, ... }
    pcb.state = 1
    pcb
}
```

### 4.2 비기능 요구사항

- **NFR-1:** 필드 메모리 배치는 선언 순서 고정 (하드웨어 매핑 안전)
- **NFR-2:** 구조체 크기/정렬은 컴파일 타임에 계산 (sizeOf/alignOf)
- **NFR-3:** Arena 할당: StructDecl AST 노드는 Arena에 할당
- **NFR-4:** 기존 360 unit + 78 E2E 테스트 회귀 없음
- **NFR-5:** ≤16B struct는 레지스터 전달로 함수 호출 오버헤드 최소화

### 4.3 파이프라인 변경

| 단계 | 변경 사항 |
|------|----------|
| **Lexer** | `struct` 키워드 토큰 추가 |
| **Parser** | StructDecl (필드 val/var), StructLitExpr, FieldAccessExpr, FieldAssignStmt |
| **TypeChecker** | struct 타입 등록, 필드 타입/가변성 검증, sizeOf/alignOf 계산, 변경 조건 검사 |
| **PurityChecker** | var 필드 변경 → impure(mut) 추론 |
| **IRBuilder** | StructAlloc, FieldStore, FieldLoad IR 명령어 |
| **CodeGen** | 스택 프레임에 struct 공간, mov 필드 읽기/쓰기, ABI별 레지스터/포인터 전달 |

**CodeGen 추가 — 큰 struct 매개변수 deep copy:**
>16B struct는 ABI 상 포인터로 전달되지만, 소스 수준은 값 의미론.
callee 진입 시 **로컬 스택에 deep copy** 하여 값 의미론을 보장해야 한다.
```
; callee 진입부 (>16B struct 매개변수)
; rdi = caller가 전달한 포인터
lea rsi, [rbp - <local_offset>]   ; 로컬 복사본 주소
mov rcx, <struct_size>
rep movsb                          ; deep copy
; 이후 [rbp - <local_offset>]을 로컬 변수로 사용
```
deep copy가 수행되면 원본 포인터를 더 이상 참조하지 않으므로:
- callee 내에서 val 바인딩 → 읽기만 (pure)
- callee 내에서 var 바인딩으로 받아 var 필드 변경 → impure(mut) (로컬 복사본만 변경)
- ABI 내부 포인터는 언어 의미론에 노출되지 않음 → **pure** 유지

### 4.4 수용 기준

- [ ] struct 선언 (val/var 필드), 인스턴스 생성, 필드 접근
- [ ] 중첩 struct 동작
- [ ] var 바인딩 + var 필드 변경 동작
- [ ] val 바인딩 또는 val 필드 변경 시 컴파일 에러
- [ ] struct 함수 인자/반환값 (Kern ABI)
- [ ] 미초기화 필드 / 존재하지 않는 필드 → 컴파일 에러
- [ ] var 필드 변경 시 impure(mut) 추론
- [ ] 기존 테스트 전체 통과
- [ ] 최소 20개 unit + 6개 E2E 추가

---

## 5. M5b — Enum + Union ✅ 완료 (433 unit + 93 E2E, commit a0bbe85)

### 5.1 Enum (상수 집합)

**FR-1: Enum 선언**
```kern
enum Color { Red, Green, Blue }
enum Direction { North, South, East, West }
```
- `enum` 키워드. 데이터 없음. 내부적으로 정수 (0, 1, 2, ...)
- 쉼표로 구분

**FR-2: Enum 사용**
```kern
val c: Color = Color.Red           // dot 접근

match c {
    Red => 0xFF0000,               // 타입 생략 허용
    Green => 0x00FF00,
    Blue => 0x0000FF
}
```

**FR-3: 메모리**
- 정수 1개 (variant 수에 따라 u8/u16/u32)

### 5.2 Union (Tagged Union / ADT)

**FR-4: Union 선언 — 타입을 감싸기만 한다**
```kern
struct Circle { radius: f64 }
struct Rect { width: f64, height: f64 }

union Shape {
    Circle(Circle),         // struct를 감쌈
    Rect(Rect),
    Empty                   // 데이터 없는 variant도 가능
}

// 원시 타입도 감쌀 수 있음
union Option {
    Some(i64),
    None
}
```
- `union` 키워드
- Variant는 타입을 하나 감싸거나 (데이터 있음), 비어있거나 (데이터 없음)
- Variant가 자체적으로 필드를 정의하지 않음 — 구조 정의는 struct에게

**FR-5: Union 인스턴스 생성**
```kern
// 기본 — 명시적
val s: Shape = Shape::Circle(Circle { radius: 3.14 })

// 추론 — variant 이름 = struct 이름이면 {} 직접 사용
val s: Shape = Shape::Circle { radius: 3.14 }
// 컴파일러: Circle variant가 Circle struct를 감쌈 → 추론

// 이름 불일치 시 명시 필요
struct CircleData { radius: f64 }
union Shape2 { Circ(CircleData), Empty }
val s2: Shape2 = Shape2::Circ(CircleData { radius: 3.14 })

// 원시 타입
val o: Option = Option::Some(42)
val n: Option = Option::None

// 데이터 없는 variant
val e: Shape = Shape::Empty
```

**추론 desugar 규칙:**
```
Shape::Circle { radius: 3.14 }
→ Shape::Circle(Circle { radius: 3.14 })
```
조건: variant 이름과 감싸는 struct 이름이 동일할 때만.

**FR-6: match에서 구조 분해**
```kern
fn area(s: Shape) -> f64 {
    match s {
        Circle { radius: r } => 3.14 * r * r,   // struct 필드 destructuring
        Rect { width: w, height: h } => w * h,
        Empty => 0.0
    }
}

fn maybe_double(o: Option) -> i64 {
    match o {
        Some(x) => x * 2,        // 원시 타입 destructuring
        None => 0
    }
}
```
- match에서 타입 자격(Shape::) 생략 허용 (대상 타입에서 추론)
- 소진성 검사: 모든 variant 처리 필수 (또는 `_`)

**FR-6a: match 패턴 이름 충돌 해결 규칙**

match 대상의 union 타입에서 variant 이름을 먼저 찾는다. 이름 충돌 시:
```kern
val x: i64 = 42
union Wrapper { x(i64) }

match Wrapper::x(10) {
    x(v) => v           // → union variant "x" (match 대상 타입 우선)
}
```
**우선순위:** match 대상 타입의 variant > 현재 스코프의 바인딩.
match 패턴 내 식별자는 항상 variant 이름으로 먼저 해석되며, 동일 이름의 지역 변수가 있어도 variant가 우선.
모호함을 피하려면 정규화된 이름 사용 가능: `Wrapper::x(v)`

**FR-7: Union 메모리 레이아웃**
```
[tag: u8 또는 적절한 크기][padding][union of variant data]
```
- tag: variant 식별 (0, 1, 2, ...)
- union: 가장 큰 variant 크기로 할당
- 전체 크기 = tag + padding + max(variant sizes)

### 5.3 접근자 구분

| 타입 | 접근자 | 예시 | 의미 |
|------|--------|------|------|
| struct | `.` | `p.x` | 필드 접근 |
| enum | `.` | `Color.Red` | 상수 접근 |
| union | `::` | `Shape::Circle(...)` | 데이터 선택지 |

`::` = "이건 데이터를 담는 variant다"라는 시각적 신호.

### 5.4 수용 기준

- [ ] enum 선언, 생성 (`.` 접근), match 동작
- [ ] union 선언, 생성 (`::` 접근), match + 구조 분해 동작
- [ ] variant 이름 = struct 이름일 때 `{}` 추론 동작
- [ ] 이름 불일치 시 명시적 생성만 허용
- [ ] 소진성 검사: 누락된 variant → 컴파일 에러
- [ ] enum/union 함수 인자/반환값 전달
- [ ] 기존 테스트 전체 통과
- [ ] 최소 20개 unit + 6개 E2E 추가

---

## 6. M5c — Ptr\<T\> (포인터) ✅ 완료 (473 unit + 100 E2E, commit 8cd3a10)

### 6.1 포인터 2종

```kern
Ptr<T>        읽기 전용 포인터 → pure
Ptr<var T>    읽기/쓰기 포인터 → impure(mem)
```

### 6.2 Ptr\<T\> from var = pure 근거 (Pragmatic Trade-off)

`var y = 42; val p: Ptr<i64> = &y; y = 99; *p` → `*p`는 99를 반환.
엄밀히는 비결정적이지만 **pure로 취급**하는 근거:

1. **Ptr\<T\>로는 쓸 수 없다** — 읽기만 가능하므로 *이 포인터를 통한* 부수효과는 없음
2. **단일 스레드 커널 섹션** 기준 — 같은 스레드 내에서 var 원본을 변경하는 것은 프로그래머가 제어 가능
3. **3종 포인터(Ptr/Ref/MutPtr) 대비 실용성** — 타입 2개로 단순한 멘탈 모델 유지
4. **쓰기 부수효과 없음 = 컴파일러 최적화 보장** — 메모이제이션/제거/재배치에 안전 (읽기 시점의 값은 변할 수 있지만 쓰기로 인한 상태 변경 없음)

향후 소유권 시스템 (Ownership/Borrowing) 도입 시 이 trade-off를 재검토.

### 6.3 기능 요구사항

**FR-1: 주소 획득**
```kern
val x: i64 = 42
var y: i64 = 42

val p: Ptr<i64> = &x            // val에서 Ptr<T> — OK
val q: Ptr<i64> = &y            // var에서 Ptr<T> — OK (읽기 전용 뷰)
val m: Ptr<var i64> = &var y    // var에서 Ptr<var T> — OK
// val n: Ptr<var i64> = &var x  // val에서 Ptr<var T> — 컴파일 에러!
```

| 소스 | `&x` → `Ptr<T>` | `&var x` → `Ptr<var T>` |
|------|:---:|:---:|
| val | OK | **에러** |
| var | OK (읽기 전용 뷰) | OK |

**FR-2: 역참조**
```kern
// Ptr<T> — 읽기만, pure
val p: Ptr<i64> = &x
val value: i64 = *p              // pure

// Ptr<var T> — 읽기/쓰기, impure(mem)
val m: Ptr<var i64> = &var y
val v: i64 = *m                  // impure(mem)
*m = 99                          // impure(mem) — 전체 값 대입
```

**FR-2a: `Ptr<var T>` 대입 의미론 (Whole Assignment)**
```kern
// 원시 타입 — 전체 값 대입
val m: Ptr<var i64> = &var y
*m = 99                          // y가 99로 변경

// 구조체 — 전체 struct 덮어쓰기
val mp: Ptr<var Point> = &var pt
*mp = Point { x: 10, y: 20 }    // pt 전체가 새 값으로 대체

// 구조체 개별 필드는 FR-3에서 (*mp).field = x 로 처리
```
`*ptr = expr` 은 항상 **전체 값 대입** (whole assignment). 포인터가 가리키는 메모리 전체를 새 값으로 덮어쓴다. 부분 대입은 `(*ptr).field = expr` 문법을 사용.

**FR-3: 구조체 포인터**
```kern
val sp: Ptr<Point> = &pt
val x: i64 = (*sp).x            // 명시적 역참조 필수
// sp.x                          // 컴파일 에러! 자동 역참조 없음

// Ptr<var T>로 var 필드 변경
var pcb: PCB = PCB { pid: 1, state: 0, ... }
val mp: Ptr<var PCB> = &var pcb
(*mp).state = 1                  // OK — Ptr<var T> + var 필드
// (*mp).pid = 999               // 컴파일 에러! val 필드
```

**FR-4: 함수 매개변수 변경은 Ptr\<var T\>로만**
```kern
// 읽기만 — pure
fn read_state(pcb: Ptr<PCB>) -> u8 {
    (*pcb).state
}

// 외부 변경 — impure(mem), 호출자가 의도 인지
fn update_state(pcb: Ptr<var PCB>, new_state: u8) -> Unit {
    (*pcb).state = new_state
}

// 호출 — &var로 변경 의도 명시
var my_pcb: PCB = PCB { pid: 1, state: 0, ... }
read_state(&my_pcb)              // 읽기 전용 뷰
update_state(&var my_pcb, 1)     // &var → 변경 가능
```

**FR-5: Purity 연동**

| 포인터 | 동작 | Purity |
|--------|------|--------|
| `Ptr<T>` | `*p` 읽기 | **pure** |
| `Ptr<T>` | `(*p).field` 읽기 | **pure** |
| `Ptr<T>` | `(*p).field = x` | **컴파일 에러** |
| `Ptr<var T>` | `*p` 읽기 | **impure(mem)** |
| `Ptr<var T>` | `(*p).var_field = x` | **impure(mem)** |
| `Ptr<var T>` | `(*p).val_field = x` | **컴파일 에러** |

### 6.4 IR 변경

| 새 Opcode | 의미 |
|-----------|------|
| `AddrOf %var` | 변수의 스택 주소 → 레지스터 (`lea`) |
| `Load %ptr` | 포인터가 가리키는 값 읽기 (`mov reg, [ptr]`) |
| `Store %ptr, %val` | 포인터 위치에 값 쓰기 (`mov [ptr], val`) |

### 6.5 수용 기준

- [ ] `Ptr<T>` — `&x`로 획득, `*p`로 역참조 (pure)
- [ ] `Ptr<var T>` — `&var x`로 획득, 읽기/쓰기 (impure(mem))
- [ ] val에서 `&var` 시도 → 컴파일 에러
- [ ] `Ptr<T>`로 쓰기 시도 → 컴파일 에러
- [ ] `Ptr<var T>`로 val 필드 쓰기 → 컴파일 에러
- [ ] 구조체 포인터 `(*sp).field` 동작
- [ ] 자동 역참조 시도 → 컴파일 에러
- [ ] 함수 매개변수 변경은 `Ptr<var T>`로만
- [ ] Purity 추론 정확성
- [ ] 기존 테스트 전체 통과
- [ ] 최소 15개 unit + 5개 E2E 추가

---

## 7. M5d — String (문자열 리터럴) ✅ 완료 (commit 04cec37)

### 7.1 기능 요구사항

**FR-1: 문자열 리터럴**
```kern
val greeting: String = "Hello, Kern!"
val empty: String = ""
```
- 이중 따옴표 `"..."` 문법
- 이스케이프 시퀀스: `\n`, `\t`, `\\`, `\"`

**FR-2: String 타입 = Fat Pointer**
```
내부 구조:
    data: Ptr<u8>     .rodata 섹션 포인터
    len: u64           바이트 길이 (null 종료 아님)
```
- 16바이트 → Kern ABI: 레지스터 2개 (rdi=ptr, rsi=len)
- `.rodata` 섹션에 리터럴 데이터 배치
- 문자열 리터럴은 항상 불변 (`Ptr<u8>`, not `Ptr<var u8>`) → pure

**FR-3: 범위**
- M5d에서는 리터럴 생성 + 함수 전달만
- 문자열 연산 (concat, slice 등)은 향후 표준 라이브러리에서

### 7.2 수용 기준

- [ ] 문자열 리터럴이 `.rodata`에 배치되고 런타임에 접근 가능
- [ ] String 타입이 (ptr, len) 구조로 동작
- [ ] 이스케이프 시퀀스 처리
- [ ] 빈 문자열 `""` 처리
- [ ] 기존 테스트 전체 통과
- [ ] 최소 10개 unit + 4개 E2E 추가

---

## 8. Purity 체계 (M5 통합)

### 8.1 4단계 Purity

```
pure           부수효과 없음, 결정적
               → 메모이제이션, 병렬화, 제거, 재배치 가능
impure(mut)    로컬 변이 (var 바인딩 + var 필드 변경)
               → 호출자에게 전파 안 됨
impure(mem)    메모리 변경 (Ptr<var T> 통한 읽기/쓰기)
               → 호출자에게 전파
impure(io)     하드웨어 상호작용 (= intrinsic)
               → 호출자에게 전파
```

### 8.2 Purity 판단 원천

| 코드 | Purity | 근거 |
|------|--------|------|
| `val` 바인딩 필드 읽기 | pure | 불변 데이터 |
| `var` 바인딩의 `var` 필드 변경 | impure(mut) | 로컬 변이 |
| 값 타입 함수 인자 접근 (`p.x`) | pure | 값 복사 |
| 큰 struct ABI 내부 포인터 | pure | 소스 수준은 값 접근 |
| `Ptr<T>` 역참조 | pure | 쓰기 불가, 부수효과 없음 |
| `Ptr<var T>` 역참조/쓰기 | impure(mem) | 메모리 변경 가능 |
| `= intrinsic` 호출 | impure(io) | 하드웨어 |

### 8.3 전파 규칙

```
함수 A가 함수 B를 호출하면:
  B가 impure(io)   → A도 impure(io)
  B가 impure(mem)  → A도 impure(mem)
  B가 impure(mut)  → A에 전파 안 됨 (B의 로컬 문제)
  B가 pure         → A에 영향 없음
```

---

## 9. Open Questions (미결 사항)

| # | 질문 | 결정 시점 |
|---|------|----------|
| OQ-1 | 재귀 Union (예: Tree) — Ptr 래핑 필요. 자동 boxed? 수동? | M5b 설계 시 |
| OQ-2 | Struct 비교 연산 (`==`, `!=`) — 구조적 동등성? 미지원? | M5a 이후 |
| OQ-3 | 포인터 산술 (ptr + offset) — 커널 필수지만 안전성 문제 | M5c 구현 시 |
| OQ-4 | String 보간 (`"hello ${name}"`) — M5d vs 향후 | M5d 구현 시 |
| OQ-5 | with-copy 패턴 (`val p2 = p with { x: 99 }`) | M5a 이후 |
| OQ-6 | volatile 포인터 (인터럽트 공유 데이터) | M5c 이후 |
| OQ-7 | 레이아웃 어노테이션 (`@packed`, `@optimize(layout)`) | 향후 |
| OQ-8 | SYNTAX.md §8.2에 `Ref<T>` (안전한 참조)가 정의되어 있지만 M5에서는 미구현. 향후 소유권 시스템 도입 시 `Ref<T>`를 어떻게 통합할지 (Ptr<T>와의 관계/대체) 결정 필요 | M6 이후 |
| OQ-9 | Variant 추론 실패 시 에러 메시지에 원래 desugar 의도를 포함해야 함. 예: `Shape::Circle { radius: 3.14 }` 실패 → "Circle variant는 Circle struct를 감싸므로 `Shape::Circle(Circle { radius: 3.14 })` 형태가 예상됩니다" | M5b 구현 시 |

---

## 10. 예상 테스트 규모

| 서브 마일스톤 | 신규 Unit | 신규 E2E | 누적 예상 | 실제 결과 |
|-------------|----------|---------|----------|----------|
| M5a (Struct) | ~20 | ~6 | 380 unit + 84 E2E | **398 unit + 86 E2E** ✅ |
| M5b (Enum + Union) | ~20 | ~6 | 400 unit + 90 E2E | **433 unit + 93 E2E** ✅ |
| M5c (Ptr) | ~15 | ~5 | 415 unit + 95 E2E | **473 unit + 100 E2E** ✅ |
| M5d (String) | ~10 | ~4 | 425 unit + 99 E2E | **494 unit + 105 E2E** ✅ |
| 에러 경로 커버리지 | +23 | +8 | - | **517 unit + 113 E2E** ✅ |

---

## 11. 다음 단계

- [x] ~~M5a 구현~~ → 398 unit + 86 E2E (commit 58d31d4)
- [x] ~~M5b 구현~~ → 433 unit + 93 E2E (commit a0bbe85)
- [x] ~~M5c 구현~~ → 473 unit + 100 E2E (commit 8cd3a10)
- [x] ~~M5d (String) 구현~~ → 494 unit + 105 E2E (commit 04cec37)
- [x] ~~에러 경로 커버리지~~ → **517 unit + 113 E2E** (commit 0d6fca5)
- [x] ~~M5 완료~~ → **v2 아키텍처 마이그레이션 Phase 2 착수 가능** (`.claude/plans/architecture-v2-design.md`)
