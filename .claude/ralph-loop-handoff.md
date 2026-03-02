# Ralph Loop Handoff — Quality Improvement Iteration

## Current State
- **603 unit tests + 252 E2E tests**, 0 failures
- All v2 architecture phases (0-7) complete
- All OS roadmap phases (A-D) complete
- Branch: `develop`

## Bugs Fixed (12 total)

### 1. Closure Match Dispatch (ba72b1f)
- **Problem**: Closures from different match arms used direct call optimization that hardcoded a single lambda name. When variable could hold different closure types at runtime, both calls went to same lambda.
- **Fix**: Removed direct call optimization. Always use indirect call through `__fn` field.
- **Files**: `lib/HIR/HIRBuilder.cpp` (typesMatchClosure, closure call site)

### 2. Match Guard Evaluation (0416998)
- **Problem**: Match guards were parsed into HIR but completely ignored during LIR lowering. Guards were never evaluated at runtime.
- **Fix**: Added guard evaluation in `lowerMatch` for all pattern types. Guard failure branches to next arm.
- **Files**: `lib/LIR/LIRBuilder.cpp`

### 3. >16B Struct Parameter ABI (0416998)
- **Problem**: >16B struct args were packed into 2 GPR registers (truncating to first 16 bytes). Should be passed by pointer.
- **Fix**: >16B struct args passed by pointer via single GPR. Callee copies to local stack.
- **Files**: `lib/Backend/InstructionSelector.cpp` (selectCall, selectCallIndirect, selectBlockArg)

### 4. Register Allocator Loop Liveness (d0b8f1a)
- **Problem**: Linear scan computed live intervals as simple [min, max] without CFG awareness. Vregs defined before a loop and used inside it got their registers clobbered on subsequent iterations.
- **Fix**: Detect back-edges and extend live intervals of vregs spanning loop headers to cover entire loop body.
- **Files**: `lib/Backend/RegisterAllocator.cpp` (computeIntervals)

### 5. Generic Functions with Fn Types (4356b20)
- **Problem**: `fn apply<T>(f: fn(T) -> T, x: T) -> T` failed with "expected Fn, got Fn" because `containsTypeVar` and `deepTypeMatch` didn't recurse into Fn types.
- **Fix**: Added Fn handling to both functions + `substituteTypeVars` for recursive return type substitution.
- **Files**: `lib/HIR/HIRBuilder.cpp`

### 6. LIR Dead Code Elimination (8507ac6)
- **Problem**: After break/continue in if-then blocks, an unreachable branch to the merge block was emitted.
- **Fix**: `blockTerminated()` helper checks for all terminators (Ret, Branch, CondBranch), not just Ret.
- **Files**: `lib/LIR/LIRBuilder.cpp`

### 7. Atomic CAS Register Pinning (5c9d0bb)
- **Problem**: `lock cmpxchg` implicitly uses RAX for expected value and writes old value to RAX. The ISel was not pinning expected→RAX or reading result from RAX, so failed CAS returned the expected value instead of the actual old memory value.
- **Fix**: Pre-color expected→RAX, use precolored RAX in LockCmpxchg operand, move RAX→result after instruction. Added `recordPhysUse(RAX)` for LockCmpxchg in register allocator.
- **Files**: `lib/Backend/InstructionSelector.cpp` (selectAtomicCas), `lib/Backend/RegisterAllocator.cpp`

### 8. Atomic FetchAdd Result Not Written (5c9d0bb)
- **Problem**: `lock xadd [ptr], reg` modifies `reg` in-place to hold the old value. But the result vreg was different from the value vreg, so the old value was lost and result was uninitialized.
- **Fix**: Copy value→result first, then use result vreg as the xadd operand so it receives the old value.
- **Files**: `lib/Backend/InstructionSelector.cpp` (selectAtomicFetchAdd)

### 9. Float Pointer Load/Store (cf5cb4b)
- **Problem**: Float pointer dereference (`*ptr` where ptr is `Ptr<f64>`) generated invalid x86: `mov qword [rax], xmm0` and `movsd xmm0, rax` — register forms instead of memory addressing. NASM rejected these with "invalid combination of opcode and operands".
- **Fix**: Added `FloatLoad`/`FloatStore` x86 opcodes that emit proper `movsd xmm, [gpr]` / `movsd [gpr], xmm`. Store detection uses `float_vregs_` map since Store instruction type is Unit. RegisterAllocator updated with mixed GPR+XMM register handling.
- **Files**: `include/kern/backend/MachIR.h`, `lib/Backend/MachIR.cpp`, `lib/Backend/InstructionSelector.cpp`, `lib/Backend/Emitter.cpp`, `lib/Backend/RegisterAllocator.cpp`

### 10. XMM Parameter Counter (ffaf98a)
- **Problem**: `selectBlockArg` used `block_arg.index` (parameter ordinal) as XMM register index. For mixed GPR+float functions like `fn f(a: i64, b: f64, c: f64)`, the second f64 would use XMM2 instead of XMM1.
- **Fix**: Added `xmm_arg_slot_` member variable for independent XMM register tracking, matching how `selectCall` already used separate counters.
- **Files**: `include/kern/backend/InstructionSelector.h`, `lib/Backend/InstructionSelector.cpp`

### 11. Width-Correct Sign Extension for Division (ffaf98a)
- **Problem**: Emitter always emitted `cqo` for signed division regardless of operand width. For i32 division, should use `cdq`; for i16, should use `cwd`.
- **Fix**: Emit `cwd`/`cdq`/`cqo` based on instruction width in Emitter.
- **Files**: `lib/Backend/Emitter.cpp`

### 12. Type Widening Casts: movsx/movzx Instead of shl/sar (478cc77)
- **Problem**: Signed widening casts (e.g., i32→i64) used 5-instruction shl+sar sequence. Unsigned widening used mov+and mask. Both are correct but suboptimal.
- **Fix**: Replaced with single `movsx`/`movsxd`/`movzx` instructions. Special handling: 32→64 signed uses `movsxd` (NASM requirement), 32→64 unsigned uses `mov eax, eax` (implicit zero-extend). Added MovSX/MovZX spill fixup in register allocator.
- **Files**: `lib/Backend/InstructionSelector.cpp`, `lib/Backend/Emitter.cpp`, `lib/Backend/RegisterAllocator.cpp`

## Known Limitations (Structural — Not Simple Bugs)

### 1. Generic Body Type Checking with TypeVars
- `fn map<T>(f: fn(T)->T, a: T, b: T) -> T { f(a) + f(b) }` fails because `+` on TypeVars is rejected
- **Root cause**: Generic bodies are type-checked once with TypeVars, then monomorphized. Arithmetic on TypeVars has no meaning.
- **Solution**: Needs trait bounds (`T: Add`) or deferred body checking after monomorphization

### 2. Closure Coercion
- Closures (struct with `__fn` field) can't be passed as `fn` typed parameters
- Closures work when called directly or through `val f: fn() -> T = { => ... }; f()`
- **Solution**: Auto-wrap closure structs when passed to fn-typed parameters

### 3. Parallel Move Cycle-Breaking (TODO in Emitter)
- `emitParallelMoves` in Emitter.cpp doesn't handle cycles (A→B, B→A)
- Current code uses sequential moves which can break on swap patterns

### 4. >16B Struct Copy Over-Read
- Copy loops use 8-byte chunks unconditionally; if struct size % 8 != 0, reads past struct boundary
- Currently benign since all Kern types are 8-byte aligned, but needs fix for i32/i16 struct fields

## Suggested Next Steps (Priority Order)

### High Priority
1. **Trait bounds for generics** — Would unblock `fn<T: Add>(a: T, b: T) -> T { a + b }`
2. **Module system** — `import`/`export` for multi-file compilation
3. **Optimizer passes** — Constant folding, dead code elimination, inlining

### Medium Priority
4. **Improved error messages** — Source location spans instead of single points
5. **Pattern matching completeness** — Nested struct destructuring in patterns
6. **Closure coercion** — Auto-wrap closure structs as fn-typed parameters

### Low Priority
7. **Parallel move cycle-breaking** in emitter
8. **>16B struct copy alignment** — handle non-8-byte-aligned final chunk
9. **Debug info** — DWARF emission for gdb/lldb support

## Test Coverage Summary
- **Error paths**: ~65 of 82 `diag.error()` calls covered by E2E tests (~79%)
- **Remaining uncovered**: ~17 error paths (mostly generic type errors blocked by feature, plus a few edge cases)
- **Feature combos tested**: closure capture, nested match, struct params, recursive structs, mixed GPR+XMM, generic+fn types
