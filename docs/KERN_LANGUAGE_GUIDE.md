# The Kern Programming Language

> A pure functional language designed for operating system kernel development.

---

## Why Kern?

Most kernel code is written in C or C++ — languages that give you full control but leave safety entirely to the programmer. Functional languages like Haskell offer strong safety guarantees but are impractical for systems programming. Kern bridges this gap.

**Kern's core idea is simple:** write only logic. The compiler figures out everything else.

- **No annotations.** Purity, side effects, recursion style — the compiler infers all of it automatically by analyzing your code.
- **Strict types, zero implicit conversions.** Every type mismatch is a compile error, not a subtle runtime bug.
- **Functional by default, imperative when needed.** Pure functions are the norm. Mutable state is allowed but flagged with a warning, nudging you toward safer patterns.
- **Zero-cost abstractions.** Pipes, pattern matching, and closures compile down to the same machine code you'd write by hand.

---

## Language at a Glance

```kern
fn fib(n: i64) -> i64 {
    if n <= 1 { n }
    else { fib(n - 1) + fib(n - 2) }
}

fn main() -> i64 {
    fib(10)
}
```

This program computes the 10th Fibonacci number (55). Notice:
- No `return` keyword needed — the last expression in a block is its value.
- `if` is an expression, not a statement — it produces a value.
- The compiler automatically infers that `fib` is **pure** and **recursive**.

---

## Core Concepts

### Bindings: `val` and `var`

Kern has two kinds of variable bindings:

```kern
val x: i64 = 42          // Immutable — cannot be reassigned
var y: i64 = 0            // Mutable — can be reassigned
y = y + 1                 // OK
```

`val` bindings are immutable. Once set, they never change. This is the default and preferred way to bind values.

`var` bindings are mutable. They allow reassignment, but using `var` makes the containing function **impure** and triggers a compiler warning suggesting you refactor to use `val` with recursion instead.

### Functions

Functions are declared with `fn`, with explicit parameter types and return type:

```kern
fn add(a: i64, b: i64) -> i64 {
    a + b
}
```

The last expression in the function body is the return value. You can also use `return` explicitly, but it's rarely needed.

**The compiler automatically infers function properties:**

| What the compiler detects | How |
|--------------------------|-----|
| Pure or impure | Checks for `var` usage and impure function calls |
| Recursive | Detects self-calls in the function body |
| Tail-recursive | Verifies all recursive calls are in tail position |
| Distributable | Pure + no captured state = safe to run on any node |

You never write annotations like `@pure` or `@tailrec`. The compiler does the analysis for you.

### Type System

Kern has 11 primitive types with strict, explicit typing:

| Type | Description |
|------|-------------|
| `i8`, `i16`, `i32`, `i64` | Signed integers (8 to 64 bits) |
| `u8`, `u16`, `u32`, `u64` | Unsigned integers (8 to 64 bits) |
| `bool` | Boolean (`true` / `false`) |
| `Unit` | No meaningful value (like `void`) |

**Strict means strict.** There are no implicit type conversions. Adding an `i32` to an `i64` is a compile error — you must be explicit about your intent. This prevents an entire class of subtle bugs that plague kernel code.

```kern
val a: i32 = 100
val b: i64 = 200
// a + b   ← compile error: type mismatch (i32 vs i64)
```

Integer literals (like `42`) are always `i64`. Context-based type inference (letting `42` adapt to the expected type) is planned for a future release.

### Control Flow

**`if` is an expression.** It always produces a value:

```kern
val max: i64 = if a > b { a } else { b }
```

**Recursion replaces loops.** In a pure functional language, iteration is expressed through recursion. The compiler automatically optimizes tail-recursive functions into efficient loops:

```kern
fn sum(n: i64, acc: i64) -> i64 {
    if n <= 0 { acc }
    else { sum(n - 1, acc + n) }    // Tail position → optimized
}
```

**Logical operators use keywords**, not symbols:

```kern
if a > 0 and b > 0 { ... }
if x or y { ... }
if not done { ... }
```

`and`, `or`, and `not` replace `&&`, `||`, and `!`. This improves readability, especially in complex kernel logic where bitwise operations (which would use symbols) must be clearly distinguishable from logical operations.

---

## Purity Inference

This is Kern's most distinctive feature. The compiler analyzes every function and classifies it automatically:

| Classification | Meaning | Example trigger |
|---------------|---------|-----------------|
| **Pure** | No side effects, no mutable state | `fn add(a: i64, b: i64) -> i64 { a + b }` |
| **Impure (mutation)** | Uses `var` for local mutable state | `var x: i64 = 0; x = x + 1` |
| **Impure (I/O)** | Interacts with hardware or external systems | Calls an intrinsic like `outb()` |
| **Impure (memory)** | Performs pointer or memory operations | Pointer dereference (planned) |

**Key insight:** local mutation (`var`) does not propagate to callers. If function `A` uses `var` internally but produces the same output for the same input, callers of `A` are still considered pure. Only I/O and memory impurity propagate through the call chain.

```kern
// Impure (local mutation) — warning issued
fn counter() -> i64 {
    var x: i64 = 0
    x = x + 1
    x
}

// Pure — calling counter() doesn't make this impure
fn double_count() -> i64 {
    counter() * 2
}
```

This design gives kernel developers a clear picture of which functions touch hardware (and must run on specific cores) versus which functions are safe to distribute across processors.

---

## Planned Features

Kern is under active development. Here's what's coming:

### Floating Point & Type Inference (M3 — next)

```kern
val pi: f64 = 3.141592653589793
val half: f32 = 0.5
```

Plus context-based type inference, so `val x: i32 = 42` will work without requiring a cast.

### Pipe Operator (M4)

Data flows left-to-right, with zero runtime cost:

```kern
val result: i64 = 10
    |> add(3)          // add(10, 3) → 13
    |> multiply(2)     // multiply(13, 2) → 26
```

The pipe `a |> f(b)` is syntactic sugar for `f(a, b)`. No closures, no overhead.

### Pattern Matching (M4)

Both as expressions and at the function definition level:

```kern
// Expression-level
val label = match status_code {
    200 => "OK"
    404 => "Not Found"
    _   => "Unknown"
}

// Function-level (multiple clauses compiled into a single decision tree)
fn fib(0) -> i64 { 0 }
fn fib(1) -> i64 { 1 }
fn fib(n: i64) -> i64 { fib(n - 1) + fib(n - 2) }
```

### Structs, Enums & Pointers (M5)

User-defined types with explicit pointer semantics for kernel work:

```kern
val ptr: Ptr<u8> = ...
val value: u8 = *ptr             // Explicit dereference
val name = (*ptr).name           // No auto-deref — always explicit
```

### Generics, Lambdas & Modules (M6)

Higher-order functions, monomorphized generics, and a module system:

```kern
fn identity<T>(x: T) -> T { x }

val double = { x: i64 => x * 2 }

module Kernel.Serial
```

---

## The Vision: Kernel Code in Kern

Here's what kernel-level serial port initialization looks like in Kern's future:

```kern
module Kernel.Serial

fn outb(port: u16, val: u8) -> Unit = intrinsic
fn inb(port: u16) -> u8 = intrinsic

fn baudToDivisor(baud: u32) -> u16 {
    (115200 / baud) |> toU16
}
// Compiler: pure, distributable

fn initSerial(port: u16, baud: u32) -> Unit {
    val divisor: u16 = baudToDivisor(baud)
    outb(port + 1, 0x00)
    outb(port + 3, 0x80)
    outb(port + 0, divisor.low())
    outb(port + 1, divisor.high())
    outb(port + 3, 0x03)
    outb(port + 2, 0xC7)
}
// Compiler: impure(io), pinned to current core
```

The compiler knows `baudToDivisor` is pure and can run anywhere, while `initSerial` touches hardware and must stay on the current core. No annotations needed.

---

## How Kern Compares

| Feature | Kern | C | Rust | Haskell |
|---------|------|---|------|---------|
| Purity inference | Automatic | N/A | N/A | Manual (`IO` monad) |
| Type safety | Strict, no implicit conversions | Weak | Strict | Strict |
| Kernel-ready | Yes (native x86-64) | Yes | Yes | No |
| Annotations needed | None | N/A | Extensive (`#[derive]`, lifetimes) | Extensive (type classes) |
| Mutable state | Allowed with warning | Default | Opt-in (`mut`) | Discouraged (`IORef`) |
| Pattern matching | Planned (M4) | No | Yes | Yes |
| Zero-cost abstractions | Yes | Manual | Yes | Partial |
| Learning curve | Low (Kotlin-like syntax) | Low | High | High |

---

## Current Status

| Milestone | Status | What it delivers |
|-----------|--------|-----------------|
| **M1** | Done | Basic compiler: functions, arithmetic, recursion, native binary output |
| **M2** | Done | Strong type system (11 types), purity inference, typed IR |
| **M3** | Next | Floating-point types (f32/f64), context-based type inference |
| **M4** | Planned | Pattern matching, pipe operator, intrinsics, tail-call optimization |
| **M5** | Planned | Structs, enums, pointers, string literals |
| **M6** | Planned | Lambdas, generics, monads, module system |

The compiler currently produces native x86-64 macOS binaries. All 255 unit tests and 44 end-to-end integration tests pass.

---

## Try It

```bash
# Build the compiler
cmake -B build && cmake --build build

# Compile and run a Kern program
echo 'fn main() -> i64 { 42 }' > hello.kern
build/tools/kernc/kernc hello.kern -o hello
./hello
echo $?    # prints: 42
```

---

*Kern is open source and under active development.*
