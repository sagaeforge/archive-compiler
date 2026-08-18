# Test Policy

## Coverage Requirements
- Line coverage: 98% minimum for all `lib/` code
- Branch coverage: 90% minimum
- Function coverage: 100% (every public function has at least one test)
- New code must meet coverage thresholds before merging

## Test Structure
- Unit tests: `tests/unit/<module>/` — test individual functions/classes in isolation
- Integration (E2E): `tests/integration/` — .kern + .expected pairs, anchor tests
- Tool tests: `tests/tool/<tool>/` — tool-specific integration tests (LSP, debugger, etc.)

## E2E Anchor Tests
- Anchor tests use exit codes and error messages as contracts
- Anchor .expected files are **immutable** — never modify, only add new tests
- Dump tests (in `tests/integration/dump/`) may be modified when IR format changes

## TDD Workflow
1. Write failing test first
2. Implement until test passes
3. Run `/kern:build` (full build + all tests)
4. Check coverage with `/kern:coverage` before PR

## Error Path Testing
- Every `diag.error()` call must have a test that triggers it
- Every `if (error_condition)` branch must have a negative test
- Boundary values: test min/max for integer types, empty inputs, null pointers
