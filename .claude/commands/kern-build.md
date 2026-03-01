# /kern:build — Full Build + Test

Run the complete build and test pipeline. This is the standard verification after any code change.

## Steps

1. Configure and build:
```bash
cmake -B build && cmake --build build
```

2. Run unit tests:
```bash
build/tests/unit/kern_tests
```

3. Run E2E integration tests:
```bash
bash tests/integration/run_tests.sh build/tools/kernc/kernc tests/integration
```

4. Report results: Show pass/fail counts for both unit and E2E tests.

## On Failure

- If build fails: Show the first compilation error and fix it
- If unit tests fail: Show which test(s) failed and investigate
- If E2E tests fail: Show which .kern file(s) failed and investigate

## Notes

- Always run this after any code change before committing
- All three steps must pass for the change to be considered complete
