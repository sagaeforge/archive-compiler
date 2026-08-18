# /kern:coverage — Coverage Build + Report + Gate

Build with coverage instrumentation, run all tests, generate report, and check the 98% threshold.

## Steps

1. Build with coverage:
```bash
cmake -B build-cov -DKERN_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-cov -j$(sysctl -n hw.ncpu)
```

2. Run tests with profiling:
```bash
mkdir -p build-cov/profraw
LLVM_PROFILE_FILE="build-cov/profraw/unit_%p.profraw" build-cov/tests/unit/kern_tests
LLVM_PROFILE_FILE="build-cov/profraw/e2e_%p.profraw" bash tests/integration/run_tests.sh build-cov/tools/kernc/kernc tests/integration
```

3. Generate report:
```bash
bash scripts/run_coverage.sh build-cov
```

4. Check 98% gate:
```bash
bash scripts/coverage_gate.sh build-cov 98
```

## Shortcut

For a full run using the provided script:
```bash
bash scripts/run_coverage.sh build-cov
bash scripts/coverage_gate.sh build-cov 98
```

## Notes

- Use before PR creation and at Phase completion milestones
- Coverage is measured on `lib/` and `include/` only (tests excluded)
- If below 98%, identify uncovered lines in `build-cov/coverage-report/index.html`
