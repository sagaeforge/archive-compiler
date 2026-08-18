# Architecture Invariants (v2)

## Pipeline (4-level IR)
```
Source → Lexer → Parser → AST → HIRBuilder → HIR → LIRBuilder → LIR → Backend → MachIR → NASM → ld
```

## IR Levels
- **AST**: Untyped, syntax-preserving (pipe, match, fn-patterns all preserved)
- **HIR**: Typed, desugared, high-level ops (pipe→call, fn-patterns→match, TypeId on every node)
- **LIR**: SSA with VReg, low-level ops, block arguments (no phi nodes)
- **MachIR**: x86-64 physical registers, ready for NASM emission

## Core Invariants
- **TypeId everywhere**: `uint32_t` TypeId from Support/TypeTable shared by all levels
- **CompilationContext**: Arena + StringPool + TypeTable + DiagnosticEngine — passed to all stages
- **Arena allocation**: All AST/HIR/LIR/MachIR nodes arena-allocated. Never raw new.
- **string_view lifetime**: Tokens/AST hold string_views into source. Source must outlive all consumers.
- **StringPool**: All identifiers interned. String comparison = pointer comparison.

## ABI
- System V AMD64: rdi, rsi, rdx, rcx, r8, r9 → rax return
- Struct ≤8B → 1 GPR, 9-16B → 2 GPR, >16B → stack

## Layer Dependencies (strict — enforced by layer-guard hook)
```
Support → Lexer → Parser → HIR → LIR → Backend
                             ↑               ↑
                            IDE          (no reverse)
                             ↑
                          Debug (Support only)
```

## macOS Linking
- `-platform_version macos 14.0.0 14.0.0 -arch x86_64 -lSystem`
