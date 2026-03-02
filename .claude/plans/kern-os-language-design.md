# Kern OS Language System Design

> **Version**: 0.1 (Draft)
> **Date**: 2026-03-02
> **Status**: Design Phase — Pre-Implementation

---

## 1. Vision

Kern is a pure functional language for OS kernel development that occupies a
position no existing language holds:

| Property          | C  | Rust | Haskell | Zig | **Kern** |
|-------------------|----|------|---------|-----|----------|
| Readable          | X  | △    | O       | O   | **O**    |
| Zero-cost abstraction | O | O  | X (GC)  | O   | **O**    |
| Pure functional   | X  | X    | O       | X   | **O**    |
| OS-capable        | O  | O    | X       | O   | **O**    |

Kern fills the empty intersection: readable, zero-cost, pure functional, and
capable of writing production OS kernels.

---

## 2. Three Pillars

### 2.1 Readability — Code reveals intent

The code a programmer writes should communicate **what** they intend, not
**how** they work around the language. Kern achieves this through:

- `val`/`var` instead of `const`/`let mut`
- `and`/`or`/`not` instead of `&&`/`||`/`!`
- `|>` pipe for left-to-right data flow
- Pattern matching as a first-class control structure
- Effects declared with `with`, not wrapped in monadic types
- Ownership expressed with three words: default (borrow), `var`, `own`

### 2.2 Zero-Cost Abstraction — The compiler sees more than the CPU runs

Every abstraction the programmer uses must compile away completely:

- **Effect erasure**: `with io` exists only at compile time. Zero runtime cost.
- **Adaptive dispatch**: Compiler chooses static vs dynamic dispatch automatically.
- **Closure elimination**: Closures are inlined and their structs removed.
- **Monomorphization**: Generics produce specialized code per type.
- **Loop fusion**: HOF chains (`map |> filter |> fold`) compile to single loops.

### 2.3 Mistake Prevention — Wrong code cannot compile

The language should make bugs **structurally impossible**, not catch them
at runtime. This is the compiler's primary job.

---

## 3. Core Philosophy: Monads Are Concepts, Not Values

Haskell treats `IO a` as a value — you construct it, pass it around, compose
it with `>>=`. This has runtime cost (thunks, heap allocation) and obscures
intent behind monadic plumbing.

Kern treats effects as **annotations on functions** — they exist only in the
type system and are completely erased at compile time.

```
// Haskell: monad as value (Kern rejects this)
readPort :: Word16 -> IO Word8
readPort p = do
    x <- volatileRead (castPtr p)
    pure x

// Kern: effect as concept
fn read_port(port: u16) -> u8 with io {
    volatile_read(port as Ptr<u8>)
}
// "with io" is a compile-time annotation. The compiled output is just:
//   mov al, [rdi]
// No IO wrapper, no thunk, no heap allocation.
```

**Consequences of this philosophy:**
- No `IO<T>` wrapper type
- No `bind`/`flatMap`/`>>=` operators for effects
- No monad transformers
- No effect handlers (effects cannot be intercepted or reified)
- Effects compose by simple set union: `with io, atomic`

---

## 4. Effect System

### 4.1 Effect Declarations

```kern
effect io          // Hardware I/O, volatile memory, inline assembly
effect atomic      // Atomic operations (lock cmpxchg, lock xadd, fences)
effect mut         // Mutable state (var bindings, field mutation)
effect mem         // Heap/pointer memory access
```

Effects are a fixed set defined by the language (not user-extensible in v1).
They correspond to the existing `Purity` enum values:

| Current Purity     | New Effect   |
|--------------------|-------------|
| `Pure`             | (no effects) |
| `ImpureMut`        | `mut`       |
| `ImpureMem`        | `mem`       |
| `ImpureIo`         | `io`        |
| (new)              | `atomic`    |

### 4.2 Effect Annotation Syntax

```kern
// Explicit annotation
fn read_port(port: u16) -> u8 with io {
    volatile_read(port as Ptr<u8>)
}

// Multiple effects
fn cas_loop(ptr: Ptr<var u64>, old: u64, new: u64) -> bool with atomic, mem {
    atomic_cas(ptr, old, new)
}

// Pure function (default — no annotation needed)
fn add(a: i64, b: i64) -> i64 {
    a + b
}

// Explicitly marked pure (optional, for documentation)
fn add(a: i64, b: i64) -> i64 with pure {
    a + b
}
```

### 4.3 Effect Enforcement Rules

1. **Default is pure.** A function with no `with` clause has no effects.
2. **Calling requires effects.** If `f` has effect `E`, any caller of `f` must
   also have effect `E` (or a superset).
3. **Violation is a compile error**, not a warning.

```kern
fn pure_fn() -> i64 {
    read_port(0x60)
    // ERROR: 'read_port' requires effect 'io',
    //        but 'pure_fn' has no effects.
    //        Add 'with io' to 'pure_fn' or remove the call.
}
```

4. **Effects propagate upward** through the call graph. This is identical to
   the current `PurityAnalysisPass` propagation, but enforced.
5. **`mut` is auto-inferred** from `var` bindings and `var` parameters. The
   programmer does not need to write `with mut` explicitly (though they can).

### 4.4 Effect Polymorphism (Implicit)

Higher-order functions automatically inherit the effects of their callbacks:

```kern
fn map<T, U>(list: [T], f: fn(T) -> U) -> [U] {
    // f's effects are unknown here — they are an effect variable
    // map's effects = body effects ∪ f's effects
    ...
}

// Usage — effects are resolved at call site:
val doubled = [1, 2, 3] |> map(fn(x) { x * 2 })
// map is pure here (f is pure)

val data = ports |> map(fn(p) with io { read_port(p) })
// map has 'io' here (f has 'io')
```

**Implementation**: When a function parameter has `Fn` type without explicit
effects, the compiler assigns an **effect variable** `?E`. At each call site,
`?E` is unified with the actual argument's effects. This is analogous to type
variable unification in generics.

For constraining callbacks:

```kern
// Only accept pure callbacks
fn safe_map<T, U>(list: [T], f: fn(T) -> U with pure) -> [U] { ... }

safe_map([1,2,3], fn(x) with io { read_port(x) })
// ERROR: 'f' requires pure, but argument has effect 'io'
```

### 4.5 Trait Effect Contravariance

Trait methods may declare effects. Implementations may have **fewer** effects
than the trait declares (contravariance):

```kern
trait BlockDevice {
    fn read_block(block: u64, buf: var [u8]) with io
}

// Production: uses io (matches trait)
impl BlockDevice for VirtioBlock {
    fn read_block(block: u64, buf: var [u8]) with io {
        volatile_read(...)
    }
}

// Test mock: pure (fewer effects — allowed)
impl BlockDevice for MockBlock {
    fn read_block(block: u64, buf: var [u8]) {
        buf[0] = self.test_data[block]
    }
}
```

This enables testability without effect handlers: production code uses real
I/O, test code substitutes pure mocks through the same trait interface.

### 4.6 Effect Erasure

Effects are **completely removed** at LIR lowering. The LIR has no effect
information. The generated assembly is identical whether effects are annotated
or not. This is zero-cost by construction, not by optimization.

### 4.7 Integration with Current Codebase

The current `PurityAnalysisPass` is the evolutionary starting point:

| Current                        | Target                                |
|-------------------------------|---------------------------------------|
| `Purity` enum (5 values)     | `EffectSet` bitmask (4 bits)          |
| `exprUsesVar()` etc.         | Same functions → effect inference     |
| `fn->purity = ImpureIo`      | `fn->effects = Effect::IO`            |
| Warning on impurity           | **Error** on effect violation          |
| `FnData{params, ret}`        | `FnData{params, ret, effects}`        |
| No call-site checking         | Call-site effect validation            |

---

## 5. Ownership Lite

### 5.1 Design Principles

Rust's ownership system is powerful but harms readability with lifetime
annotations (`&'a mut Vec<Box<dyn Trait + 'b>>`). Kern uses a simpler model:

- **No lifetime annotations.** References cannot escape their scope.
- **Three passing modes.** Default (immutable borrow), `var` (mutable borrow),
  `own` (ownership transfer).
- **Ptr<T> for raw access.** Requires `mem` effect. Used in kernel internals.

### 5.2 Passing Modes

```kern
// Default: immutable borrow — callee reads, caller keeps
fn read_header(buf: Buffer) -> u32 {
    buf.header   // read-only access
}

// var: mutable borrow — callee can modify, caller sees changes
fn clear(buf: var Buffer) -> Unit {
    buf.len = 0  // mutation allowed
}

// own: ownership transfer — callee takes, caller loses access
fn destroy(buf: own Buffer) -> Unit {
    free(buf.data)
}
```

### 5.3 Rules

1. **Borrowed values cannot escape.** Cannot return a borrowed parameter or
   store it in a struct field.

```kern
fn bad(buf: Buffer) -> Buffer {
    buf   // ERROR: cannot return borrowed value 'buf'
}

struct Holder { ref: Buffer }
// ERROR: struct fields cannot hold borrowed references.
// Use 'own Buffer' for ownership or 'Ptr<Buffer>' for raw pointer.
```

2. **Use-after-move is a compile error.**

```kern
val b = Buffer.new(1024)
destroy(own b)
read_header(b)    // ERROR: 'b' was moved at line N
```

3. **`var` implies `mut` effect.** Functions with `var` parameters automatically
   have the `mut` effect (inferred, not annotated).

4. **No aliasing of `var` borrows.** Two `var` borrows of the same value
   cannot exist simultaneously.

```kern
var b = Buffer.new(10)
process(var b, var b)
// ERROR: cannot borrow 'b' as mutable more than once
```

### 5.4 Ptr<T> and Kernel Code

`Ptr<T>` (immutable pointer) and `Ptr<var T>` (mutable pointer) remain for
kernel internals. They require `mem` effect and are not exposed in the driver
SDK layer.

| Mechanism       | Safety   | Escapable | Arithmetic | Kernel Use        |
|----------------|----------|-----------|------------|-------------------|
| Default borrow | Safe     | No        | No         | General code      |
| `var` borrow   | Safe     | No        | No         | Mutation          |
| `own` transfer | Safe     | N/A       | No         | Resource mgmt     |
| `Ptr<T>`       | `mem` required | Yes  | Yes        | MMIO, page tables |

### 5.5 Parser Changes

Current parameter syntax: `name: Type`
New parameter syntax: `name: Type` | `name: var Type` | `name: own Type`

```cpp
// New: PassingMode enum
enum class PassingMode : uint8_t { Borrow, MutBorrow, Own };

// Updated Param
struct Param {
    std::string_view name;
    TypeRef type;
    PassingMode mode = PassingMode::Borrow;  // new field
    SourceLocation loc;
};
```

---

## 6. No `unchecked` / No `unsafe`

### 6.1 Philosophy

`unchecked` blocks mean the language failed to express the programmer's intent.
Instead of providing escape hatches, Kern makes the type system expressive
enough that escape is unnecessary.

### 6.2 Array Bounds: Flow-Sensitive Typing

Instead of disabling bounds checks, **prove they are unnecessary**:

```kern
// Static: constant index checked at compile time
val arr: [i64; 4] = [1, 2, 3, 4]
val x = arr[3]    // OK: 3 < 4 (compile-time verified)
val y = arr[4]    // ERROR: index 4 out of bounds for [i64; 4]

// Dynamic: condition narrows the range
fn get(arr: [i64; 512], i: u64) -> Option<i64> {
    if i < 512 {
        Some(arr[i])   // Safe: compiler knows i < 512 in this branch
    } else {
        None
    }
}

// Loop: range is bounded
for i in 0..arr.len {
    arr[i]   // Safe: i < arr.len by construction
}
```

The compiler tracks value ranges through conditionals and loop bounds. When
the range is provably within bounds, no runtime check is inserted. When it
cannot be proven, `Option<T>` return is required.

### 6.3 Integer Overflow: Intent-Expressing Operators

Instead of disabling overflow checks, use **different operators** for different
intents:

| Operator | Semantics           | Use Case                    |
|----------|--------------------|-----------------------------|
| `+`      | Trapping add        | Default — overflow is a bug |
| `+%`     | Wrapping add        | Hash functions, crypto      |
| `+\|`    | Saturating add      | Sensor values, clamping     |

```kern
val x: u8 = 255
val a = x + 1      // Trap (debug) or defined error (release)
val b = x +% 1     // 0 — wrapping is intentional
val c = x +| 1     // 255 — saturation is intentional
```

These are not "unchecked" operations. They are **different operations** that
express different programmer intents. A reader can see the intent in the code.

Same pattern for subtraction (`-`, `-%`, `-|`) and multiplication (`*`, `*%`).

### 6.4 Lossy Casts: Intent-Expressing Methods

Instead of silent truncation, require explicit intent:

```kern
val big: i64 = 1000

// Widening: always safe, implicit or .widen()
val wide: i64 = small_i32.widen()

// Narrowing: must choose behavior explicitly
val a = big.truncate<u8>()     // Truncate to 232 — "I accept data loss"
val b = big.try_narrow<u8>()   // Option<u8> — None if out of range
val c = big.clamp<u8>()        // 255 — clamp to max
val d = big as u8              // ERROR: potentially lossy cast.
                               //        Use .truncate(), .try_narrow(), or .clamp()
```

### 6.5 Typed Hardware Abstractions (Instead of Raw Pointers)

Instead of raw pointer arithmetic for hardware access, use typed abstractions:

```kern
// ── MMIO Registers ──
struct MmioReg<const ADDR: u64, T> {}

fn read_reg<const ADDR: u64, T>(reg: MmioReg<ADDR, T>) -> T with io {
    volatile_read(ADDR as Ptr<T>)
}

// ── I/O Ports ──
struct Port<T> { number: u16 }

fn inb(port: Port<u8>) -> u8 with io {
    asm { "in al, dx" }
}

// ── Address Space Types ──
newtype PhysAddr = u64
newtype VirtAddr = u64

// PhysAddr + u64 = PhysAddr   (OK)
// PhysAddr + VirtAddr          (ERROR: meaningless)
// PhysAddr cannot be dereferenced — must be mapped first
```

### 6.6 Inline Assembly: The Only "Escape"

`asm { }` blocks are not "unchecked" — they are a bridge to a different
language (assembly). They require `io` effect.

```kern
@naked fn switch_context(old_sp: Ptr<var u64>, new_sp: u64) with io {
    asm {
        "mov [rdi], rsp"
        "mov rsp, rsi"
        "ret"
    }
}
```

This is acceptable because:
- It expresses intent clearly: "this is CPU-level code"
- It requires `io` effect: callers must acknowledge the effect
- It is **visible** and **auditable** — grep for `asm {` to find all sites

### 6.7 Effect List (No `unsafe`)

```cpp
enum class Effect : uint8_t {
    Mut    = 1 << 0,   // Mutable state
    Mem    = 1 << 1,   // Pointer/heap memory
    IO     = 1 << 2,   // Hardware I/O, volatile, asm
    Atomic = 1 << 3,   // Atomic operations
};
using EffectSet = uint8_t;
// Pure = 0 (no bits set)
```

There is no `Unsafe` effect. The language does not have the concept.

---

## 7. Integrated Example: Kernel Code

All design decisions applied together:

```kern
effect io
effect atomic
effect mut
effect mem

// ── Pure data structures (no effects) ──

struct PageEntry {
    frame: u64
    flags: u16
}

fn make_entry(frame: u64, flags: u16) -> PageEntry {
    PageEntry { frame: frame, flags: flags }
}

// ── Typed hardware abstractions ──

newtype PhysAddr = u64
newtype VirtAddr = u64
struct Port<T> { number: u16 }

val COM1: Port<u8> = Port { number: 0x3F8 }

fn serial_write(port: Port<u8>, byte: u8) -> Unit with io {
    outb(port, byte)
}

// ── Driver interface (trait + effects) ──

trait BlockDevice {
    fn read_block(block: u64, buf: var [u8]) with io
    fn write_block(block: u64, buf: [u8]) with io
    fn capacity() -> u64    // pure
}

// ── Ownership for resource management ──

fn init_driver(config: own DeviceConfig) -> VirtioBlock with io {
    val base = config.mmio_base
    VirtioBlock.new(base)
    // config is consumed — caller cannot reuse
}

// ── Spinlock (atomic effect) ──

fn acquire(lock: Ptr<var u64>) -> Unit with atomic {
    loop {
        if atomic_cas(lock, 0, 1) { break }
    }
}

fn release(lock: Ptr<var u64>) -> Unit with atomic {
    atomic_store(lock, 0)
    mfence()
}

// ── Safe IPC with flow typing (no unchecked) ──

fn ipc_send(dest: u64, msg: [u8], region: SharedRegion) -> Unit with io {
    for i in 0..msg.len {
        // i < msg.len proven by loop bound → no bounds check
        region.write(i, msg[i])
    }
    region.notify()
}

// ── Hash with intentional wrapping ──

fn fnv1a(data: [u8]) -> u64 {
    var hash: u64 = 0xCBF29CE484222325
    for i in 0..data.len {
        hash = hash ^% data[i].widen()   // XOR wrapping
        hash = hash *% 0x100000001B3     // multiply wrapping
    }
    hash
}

// ── Pure function calling effectful → compile error ──

fn bad_calc(x: i64) -> i64 {
    serial_write(COM1, 0x41)
    // ERROR: 'serial_write' requires effect 'io',
    //        but 'bad_calc' has no effects
    x + 1
}

// ── HOF with effect polymorphism ──

fn for_each_page(table: [PageEntry; 512], f: fn(PageEntry) -> Unit) -> Unit {
    for i in 0..512 {
        f(table[i])
    }
}

// Pure callback → for_each_page is pure
for_each_page(pt, fn(e) { log_entry(e) })

// IO callback → for_each_page inherits io
for_each_page(pt, fn(e) with io { flush_tlb(e.frame) })
```

---

## 8. Static Analyzer Architecture

### 8.1 Current Infrastructure

| Component                  | Status              |
|---------------------------|---------------------|
| `HIRPass` + `HIRPassManager` | Exists, operational  |
| `LIRPass` + `LIRPassManager` | Exists, unused       |
| `PurityAnalysisPass`      | Exists (effect system prototype) |
| `TailCallAnalysisPass`    | Exists               |
| `DiagnosticEngine`        | Exists               |
| IDE `DiagnosticProvider`   | Exists (LSP wired)   |
| `LintPass` / `LintEngine` | Not implemented       |
| Dataflow framework         | Not implemented       |

### 8.2 Phase 1: Tree-Walk Analyses (on HIRPassManager)

These require only walking the HIR tree, no dataflow framework:

| Pass                      | What It Catches                  | Priority |
|--------------------------|----------------------------------|----------|
| `EffectEnforcementPass`  | Pure fn calling effectful fn     | P0       |
| `UseAfterMovePass`       | Accessing moved `own` values     | P0       |
| `ConstBoundsCheckPass`   | `arr[4]` on `[T; 4]`            | P0       |
| `ConstOverflowPass`      | `255 + 1` in constant exprs     | P0       |
| `LossyCastPass`          | `big as u8` without explicit method | P0   |
| `UnusedBindingPass`      | `val x = ...` never read         | P1       |
| `ExhaustiveMatchPass`    | Report ALL missing variants      | P1       |
| `UnusedFunctionPass`     | Unreachable functions            | P2       |
| `MutBorrowAliasPass`     | Simultaneous `var` borrows       | P1       |
| `BorrowEscapePass`       | Returning or storing borrows     | P0       |

### 8.3 Phase 2: Flow-Sensitive Analyses

Requires a lightweight dataflow framework on HIR:

| Analysis                  | What It Enables                  |
|--------------------------|----------------------------------|
| Range tracking           | `if i < len` proves bounds safe  |
| Nullability tracking     | Option<T> unwrap after check     |
| Initialization tracking   | Future: partial init detection   |

**Implementation approach**: Abstract interpretation with interval domains
on HIR control flow. Not full SSA — HIR is a tree, so flow sensitivity
is tracked via branch context stacks.

### 8.4 Phase 3: LIR Optimization Passes

Using the existing `LIRPassManager`:

| Pass                      | Zero-Cost Benefit                |
|--------------------------|----------------------------------|
| `InliningPass`           | Eliminate function call overhead |
| `ConstPropPass`          | Fold constants, eliminate dead paths |
| `CSEPass`                | Eliminate redundant computations |
| `LoopInvariantPass`      | Hoist invariants out of loops    |
| `ClosureEliminationPass` | Remove closure struct allocations |
| `DevirtualizationPass`   | Convert vtable calls to direct calls |

---

## 9. Implementation Roadmap

### Stage 1: Effect System Foundation

**Goal**: Evolve current purity system into enforced effect system.

```
1.1  Define EffectSet bitmask type in Support
1.2  Add EffectSet to FnData in TypeSystem (makeFn change)
1.3  Add KwWith token to Lexer
1.4  Parse "with effect, effect" in function declarations (Parser)
1.5  Add effects field to HIRFnDecl, HIRParam (HIR)
1.6  Evolve PurityAnalysisPass → EffectInferencePass
     - Infer effects from function bodies (same logic as current)
     - Validate explicit annotations match inferred effects
1.7  Add call-site effect checking in HIRBuilder::buildCall
     - Caller must have callee's effects (superset check)
     - Effect violation → diag.error (not warning)
1.8  Effect polymorphism for Fn type parameters
     - Effect variable unification at call sites
1.9  Trait effect contravariance checking
1.10 Verify effect erasure: LIR/MachIR must have no effect info
```

**Tests**: Unit tests for each sub-step. E2E error tests for violations.

### Stage 2: Ownership Lite

**Goal**: Add move semantics and borrow checking without lifetimes.

```
2.1  Add PassingMode to AST Param (Borrow/MutBorrow/Own)
2.2  Parse "var Type" and "own Type" in parameter lists
2.3  HIRBuilder: track moved_vars_ set
2.4  Use-after-move detection → diag.error
2.5  Borrow escape detection (return/struct store) → diag.error
2.6  MutBorrow alias detection → diag.error
2.7  var parameter → automatic mut effect inference
```

### Stage 3: Compile-Time Verification

**Goal**: Eliminate need for unchecked blocks.

```
3.1  Constant index bounds checking in buildIndexAccess
3.2  Constant overflow detection in constEvalInt
3.3  Wrapping operators: +%, -%, *% (Lexer + Parser + HIR + LIR + Backend)
3.4  Saturating operators: +|, -| (same pipeline)
3.5  Lossy cast error + truncate/try_narrow/clamp methods
3.6  Runtime bounds check insertion for dynamic indices
3.7  Flow-sensitive range narrowing after if-conditions
3.8  Loop bound range inference (for i in 0..N)
```

### Stage 4: Typed Hardware Abstractions

**Goal**: Make raw pointers unnecessary for driver code.

```
4.1  newtype declarations (zero-cost wrapper types)
4.2  Port<T>, MmioReg<ADDR, T> standard library types
4.3  PhysAddr/VirtAddr type separation
4.4  Restrict Ptr<T> creation to functions with mem effect
4.5  Kernel-internal vs driver-SDK boundary enforcement
```

### Stage 5: OS Infrastructure

**Goal**: Language features required for kernel development.

```
5.1  Global variables (static val/var → .data/.bss sections)
5.2  Adaptive dispatch (auto static/dynamic, vtable generation)
5.3  Enhanced module system (cross-module type checking)
5.4  Module signature files (.kerni) for separate compilation
```

### Stage 6: Optimization Pipeline

**Goal**: Zero-cost abstractions realized through compiler optimization.

```
6.1  Function inlining (cost-model based, LIR pass)
6.2  Constant propagation (SSA-based lattice analysis)
6.3  Common subexpression elimination
6.4  Closure struct elimination (inline + remove)
6.5  Devirtualization (vtable → direct call when type is known)
6.6  Loop invariant code motion
6.7  Loop fusion for HOF chains
```

### Stage 7: Multi-Architecture

**Goal**: Same Kern source compiles to x86-64 and ARM64.

```
7.1  Backend abstraction interface (TargetBackend trait)
7.2  AArch64 InstructionSelector
7.3  AArch64 RegisterAllocator
7.4  AArch64 NASMEmitter (or GAS emitter)
7.5  Conditional compilation (@cfg annotations)
7.6  Cross-compilation (--target flag)
```

---

## 10. Open Design Questions

### 10.1 Effect Extensibility

Should user-defined effects be allowed in later versions?

```kern
// Hypothetical: user-defined effect
effect log

fn trace(msg: [u8]) -> Unit with log {
    serial_write(COM1, msg)
}
```

**Current decision**: No. Effects are a fixed set in v1. User-defined effects
add significant complexity (effect algebras, effect subtyping) for unclear
kernel-development benefit. Revisit after v1 ships.

### 10.2 Linear Types vs Ownership Lite

The current design uses simple move semantics (use-after-move detection)
without full linear types. Linear types would guarantee that every `own`
value is consumed exactly once (no leaks, no double-free).

**Current decision**: Start with move semantics. Add linear type enforcement
as a future lint pass if resource leak detection proves important.

### 10.3 Borrow Checker Depth

Without lifetime annotations, the borrow checker cannot track references
across function boundaries. This means:

- Cannot return references from functions
- Cannot store references in structs
- Must use `Ptr<T>` (with `mem` effect) for cross-scope references

**Current decision**: Accept this limitation. Kernel code primarily uses
owned data and raw pointers. The simplicity gain outweighs the expressiveness
loss.

### 10.4 Runtime Bounds Check Overhead

For dynamic array indices where flow typing cannot prove safety, runtime
bounds checks are inserted. In hot kernel paths (IPC, interrupt handlers),
this overhead may be unacceptable.

**Current decision**: The programmer should restructure code to use provable
patterns (`for i in 0..N`, `if i < len`). If the compiler cannot prove
safety, the code should use `Option<T>` returns. No backdoor to disable
checks.

### 10.5 Algebraic Effects (Future)

Koka-style algebraic effects are a potential evolution path that aligns with
"monads are concepts." They would allow:

- Effect abstraction and composition
- Controlled effect handling for testing
- Stack-switching for coroutines

**Current decision**: Defer to post-v1. The current fixed effect set covers
kernel needs. Algebraic effects are a research direction, not a requirement.

---

## 11. Relationship to Existing Code

### Files That Change

| File | Change |
|------|--------|
| `include/kern/support/TypeSystem.h` | Add `EffectSet` to `FnData` |
| `lib/Support/TypeSystem.cpp` | Update `makeFn` for effect-aware dedup |
| `include/kern/lexer/Token.h` | Add `KwWith`, `KwOwn`, `KwPure` tokens |
| `lib/Lexer/Lexer.cpp` | Recognize new keywords |
| `include/kern/parser/AST.h` | Add `PassingMode` to `Param`, effects to `FnDecl` |
| `lib/Parser/Parser.cpp` | Parse `with` clause, `var`/`own` params |
| `include/kern/hir/HIR.h` | Add `EffectSet` to `HIRFnDecl` |
| `include/kern/hir/HIRPasses.h` | `EffectInferencePass` replaces `PurityAnalysisPass` |
| `lib/HIR/HIRBuilder.cpp` | Call-site effect checking, moved_vars_ tracking |
| `lib/HIR/HIRPasses.cpp` | Effect inference + enforcement logic |

### Files That Do NOT Change

| File | Reason |
|------|--------|
| `include/kern/lir/LIR.h` | Effects are erased before LIR |
| `lib/LIR/LIRBuilder.cpp` | No effect info in LIR |
| `lib/Backend/*` | Backend is effect-unaware |
| `tests/integration/*.expected` | Anchor tests are immutable |

---

## 12. Success Criteria

The language system design is successful when:

1. **Effect enforcement**: A pure function cannot call an effectful function.
   This is a compile error, never a warning.
2. **Zero cost verified**: Generated assembly for effectful code is identical
   to the same code without effect annotations.
3. **Use-after-move caught**: Moving a value and then using it is a compile
   error.
4. **No unchecked needed**: All kernel test programs compile without any
   `unchecked` or `unsafe` blocks. Only `asm {}` is used for CPU primitives.
5. **Readability**: A new developer can read kernel code and understand intent
   from the code alone, without documentation.
6. **Performance parity**: Benchmarks show Kern kernel code performs within
   5% of equivalent hand-written C/assembly.
