# /kern:pipeline-trace — Pipeline Stage Visualization

Compile a .kern file and show intermediate representations at each pipeline stage.

## Arguments

$ARGUMENTS should be the path to a .kern file (e.g., `tests/integration/fib.kern`)

## Steps

1. Show the source code:
```bash
cat $ARGUMENTS
```

2. Dump AST:
```bash
build/tools/kernc/kernc --dump-ast $ARGUMENTS
```

3. Dump IR (current pipeline — will be HIR in v2):
```bash
build/tools/kernc/kernc --dump-ir $ARGUMENTS
```

4. Dump purity info:
```bash
build/tools/kernc/kernc --dump-purity $ARGUMENTS
```

5. Compile to NASM and show assembly:
```bash
build/tools/kernc/kernc $ARGUMENTS -o /tmp/kern_trace.asm
cat /tmp/kern_trace.asm
```

## Output Format

```
=== Source ===
<file contents>

=== AST (--dump-ast) ===
<AST dump>

=== IR (--dump-ir) ===
<IR dump>

=== Purity (--dump-purity) ===
<purity info>

=== NASM (assembly) ===
<generated assembly>
```

## Notes

- Use this for debugging when a .kern file produces unexpected output
- Compare stages to find where the bug is introduced
- In v2, stages will be: AST → HIR → LIR → MachIR → NASM
