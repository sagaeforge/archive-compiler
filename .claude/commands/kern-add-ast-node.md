# /kern:add-ast-node — Add AST Node (10+ File Pipeline Cascade)

Generate a new AST expression or statement node and propagate through the entire pipeline.

## Arguments

$ARGUMENTS should be: `<NodeName>` (e.g., `LambdaExpr`, `ForStmt`, `ArrayLitExpr`)

## Pipeline Cascade Checklist

### AST Layer
1. **`include/kern/parser/AST.h`** — Add `ExprKind::<Name>` or `StmtKind::<Name>` + struct definition
2. **`lib/Parser/Parser.cpp`** — Add parsing logic (new parse function or extend existing)
3. (If dump exists) **ASTDump** — Add text output

### HIR Layer (v2)
4. **`include/kern/hir/HIR.h`** — Add `HIRKind::<Name>` + struct
5. **`lib/HIR/HIRBuilder.cpp`** — Add AST → HIR lowering (may desugar)
6. **`lib/HIR/HIRDump.cpp`** — Add text output

### LIR Layer (v2)
7. **`lib/LIR/LIRBuilder.cpp`** — Add HIR → LIR lowering (may need new opcodes → use `/kern:add-opcode`)

### Backend Layer (v2)
8. (If new opcodes) **InstructionSelector + Emitter** — via `/kern:add-opcode`

### Sema (current pipeline)
9. **`lib/Sema/TypeChecker.cpp`** — Add type checking for the new node
10. **`lib/IR/IRBuilder.cpp`** — Add IR generation

### Tests
11. **`tests/unit/parser/ParserTest.cpp`** — Parser test
12. **`tests/unit/sema/SemaTest.cpp`** — Type checking test
13. **`tests/unit/ir/IRTest.cpp`** — IR generation test
14. **`tests/integration/<name>.kern`** + `.expected` — E2E test (use `/kern:e2e-add`)

## Verification

```bash
/kern:build
```

## Notes

- This is the most complex cascade. Take it one layer at a time.
- Run `/kern:build` after each layer to catch errors early.
- If the node is syntax sugar, it may be desugared in HIR and not need LIR/Backend changes.
