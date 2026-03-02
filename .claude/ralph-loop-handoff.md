# Ralph Loop Handoff — Quality Improvement Iteration

## Current State
- **603 unit tests + 232 E2E tests**, 0 failures
- All v2 architecture phases (0-7) complete
- All OS roadmap phases (A-D) complete
- Branch: `develop`

## Bugs Fixed This Session (9 total)

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

## Known Limitations (Structural — Not Simple Bugs)

### 1. Generic Body Type Checking with TypeVars
- `fn map<T>(f: fn(T)->T, a: T, b: T) -> T { f(a) + f(b) }` fails because `+` on TypeVars is rejected
- **Root cause**: Generic bodies are type-checked once with TypeVars, then monomorphized. Arithmetic on TypeVars has no meaning.
- **Solution**: Needs trait bounds (`T: Add`) or deferred body checking after monomorphization

### 2. XMM Parameter Counter (TODO in ISel)
- `selectBlockArg` uses `block_arg.index` for XMM args instead of a separate counter
- Won't cause issues until a function mixes >1 float param with GPR params out of order

### 3. Parallel Move Cycle-Breaking (TODO in Emitter)
- `emitParallelMoves` in Emitter.cpp doesn't handle cycles (A→B, B→A)
- Current code uses sequential moves which can break on swap patterns

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
8. **XMM parameter tracking** separate counter
9. **Debug info** — DWARF emission for gdb/lldb support

### 9. Float Pointer Load/Store (cf5cb4b)
- **Problem**: Float pointer dereference (`*ptr` where ptr is `Ptr<f64>`) generated invalid x86: `mov qword [rax], xmm0` and `movsd xmm0, rax` — register forms instead of memory addressing. NASM rejected these with "invalid combination of opcode and operands".
- **Fix**: Added `FloatLoad`/`FloatStore` x86 opcodes that emit proper `movsd xmm, [gpr]` / `movsd [gpr], xmm`. Store detection uses `float_vregs_` map since Store instruction type is Unit. RegisterAllocator updated with mixed GPR+XMM register handling.
- **Files**: `include/kern/backend/MachIR.h`, `lib/Backend/MachIR.cpp`, `lib/Backend/InstructionSelector.cpp`, `lib/Backend/Emitter.cpp`, `lib/Backend/RegisterAllocator.cpp`

## Test Coverage Gaps (Remaining)
- Error paths: ~30 `diag.error()` calls still without dedicated E2E tests (82 total, 53 covered)
- Feature combos: closure+try operator inside lambda, recursive closures, array of structs
- Backend issues found but not fixed: cqo for <64-bit div (Issue 6), sign-extend via shl/sar instead of movsx (Issue 7), XMM param counter (Issue 5)
