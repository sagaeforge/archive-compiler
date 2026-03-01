# /kern:layer-check — Verify Layer Dependencies

Scan the codebase for reverse-direction #include dependencies that violate the layer boundary rules.

## Steps

1. Check Support layer (must not depend on anything above):
```bash
grep -rn '#include.*kern/\(lexer\|parser\|hir\|lir\|backend\|sema\|ir\|codegen\|ide\|debug\)/' include/kern/support/ lib/Support/ 2>/dev/null
```

2. Check Lexer layer:
```bash
grep -rn '#include.*kern/\(parser\|hir\|lir\|backend\|sema\|ir\|codegen\)/' include/kern/lexer/ lib/Lexer/ 2>/dev/null
```

3. Check Parser layer:
```bash
grep -rn '#include.*kern/\(hir\|lir\|backend\|sema\|ir\|codegen\)/' include/kern/parser/ lib/Parser/ 2>/dev/null
```

4. Check HIR layer (v2):
```bash
grep -rn '#include.*kern/\(lir\|backend\)/' include/kern/hir/ lib/HIR/ 2>/dev/null
```

5. Check LIR layer (v2):
```bash
grep -rn '#include.*kern/backend/' include/kern/lir/ lib/LIR/ 2>/dev/null
```

6. Check IDE layer (v2):
```bash
grep -rn '#include.*kern/\(lir\|backend\)/' include/kern/ide/ lib/IDE/ 2>/dev/null
```

## Expected Output

- If clean: "No reverse dependencies found."
- If violations found: List each file:line with the offending #include

## Notes

- Current (v1) pipeline has Sema → IR → CodeGen which is fine
- The check is forward-looking for v2 layers (HIR, LIR, IDE, etc.)
- Run after any refactoring that moves code between layers
