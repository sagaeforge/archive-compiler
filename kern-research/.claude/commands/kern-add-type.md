# /kern:add-type — Add Type to Pipeline

Add a new type kind to the TypeSystem and propagate through the pipeline.

## Arguments

$ARGUMENTS should be the type name (e.g., `Result`, `Maybe`, `Array`)

## Cascade Checklist

1. **`include/kern/support/TypeSystem.h`** — Add `TypeKind::<Name>`, add data struct, add to TypeInfo union
2. **`lib/Support/TypeSystem.cpp`** — Implement sizeOf/alignOf/name for the new kind
3. **`tests/unit/support/TypeSystemTest.cpp`** — Add unit tests for the new type
4. **`include/kern/parser/AST.h`** — Add AST node if needed (e.g., `ArrayLitExpr`)
5. **`lib/Parser/Parser.cpp`** — Add parsing for type annotations
6. **`lib/Sema/TypeChecker.cpp`** (or HIR equivalent) — Add type checking rules
7. **`lib/IR/IRBuilder.cpp`** (or LIR equivalent) — Add IR generation
8. **`lib/CodeGen/CodeGen.cpp`** (or Backend equivalent) — Add code generation

## Post-Generation Steps

1. Run `/kern:build`
2. Add E2E test via `/kern:e2e-add <name>_basic`

## Notes

- New types must have well-defined sizeOf and alignOf
- Consider ABI implications (register vs stack passing)
- Add the type to TypeTable convenience constructors if commonly used
