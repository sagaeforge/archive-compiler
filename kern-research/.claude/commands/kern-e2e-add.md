# /kern:e2e-add — Add E2E Test Pair

Create a new .kern source file and matching .expected file for an E2E integration test.

## Arguments

$ARGUMENTS should be the test name (e.g., `struct_nested`, `match_wildcard`)

## Steps

1. Create `tests/integration/$ARGUMENTS.kern` with the test source code
2. Create `tests/integration/$ARGUMENTS.expected` with the expected output format:

```
stdout:
<expected stdout lines, if any>
exit:N
```

Where N is the expected exit code (0-255).

3. Verify the test runs correctly:
```bash
bash tests/integration/run_tests.sh build/tools/kernc/kernc tests/integration
```

## Conventions

- Test name should describe what's being tested (e.g., `float_add`, `enum_match`)
- Prefer testing via exit code (fn main returns a value) over stdout
- Each test should exercise one specific feature or edge case
- The .kern file should be minimal — only what's needed to test the feature
