# /kern:test — Module-Scoped Build + Test

Build and run tests for a specific module only, for faster feedback during development.

## Arguments

$ARGUMENTS should be the module name (e.g., `support`, `hir`, `lir`, `backend`, `ide`)

## Steps

1. Build the project:
```bash
cmake -B build && cmake --build build
```

2. Run only the matching unit tests using GoogleTest filter:
```bash
build/tests/unit/kern_tests --gtest_filter="*$ARGUMENTS*"
```

Note: GoogleTest filter is case-insensitive partial match. Examples:
- `support` → runs ArenaTest, DiagnosticTest, StringPoolTest, TypeSystemTest
- `hir` → runs HIRTest, HIRBuilderTest
- `lir` → runs LIRTest, LIRBuilderTest
- `backend` → runs BackendTest, InstSelTest

3. Report: Show pass/fail counts for the filtered tests.

## Notes

- This is for fast iteration. Always run full `/kern:build` before committing.
- If the module name doesn't match any tests, show available test names.
