#!/bin/bash
# run_coverage.sh — Build with coverage, run tests, generate report
#
# Usage:
#   bash scripts/run_coverage.sh [build-dir]
#
# If build-dir is not given, defaults to build-cov.
# Requires: Clang, llvm-profdata, llvm-cov

set -euo pipefail

BUILD_DIR="${1:-build-cov}"
PROFRAW_DIR="$BUILD_DIR/profraw"
PROFDATA="$BUILD_DIR/coverage.profdata"
REPORT_DIR="$BUILD_DIR/coverage-report"

# Step 1: Configure + Build with coverage
echo "=== Configuring with coverage ==="
cmake -B "$BUILD_DIR" -DKERN_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build "$BUILD_DIR" -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)"

# Step 2: Clean old profraw
rm -rf "$PROFRAW_DIR"
mkdir -p "$PROFRAW_DIR"

# Step 3: Run unit tests
echo "=== Running unit tests ==="
LLVM_PROFILE_FILE="$PROFRAW_DIR/unit_%p.profraw" \
    "$BUILD_DIR/tests/unit/kern_tests" || true

# Step 4: Run integration tests
echo "=== Running integration tests ==="
LLVM_PROFILE_FILE="$PROFRAW_DIR/e2e_%p.profraw" \
    bash tests/integration/run_tests.sh "$BUILD_DIR/tools/kernc/kernc" tests/integration || true

# Step 5: Merge profraw → profdata
echo "=== Merging profile data ==="
xcrun llvm-profdata merge -sparse "$PROFRAW_DIR"/*.profraw -o "$PROFDATA"

# Step 6: Generate text report
echo "=== Coverage Summary ==="
xcrun llvm-cov report \
    "$BUILD_DIR/tests/unit/kern_tests" \
    -instr-profile="$PROFDATA" \
    -ignore-filename-regex='(googletest|_deps|tests/)' \
    --sources lib/ include/

# Step 7: Generate HTML report
echo "=== Generating HTML report ==="
rm -rf "$REPORT_DIR"
xcrun llvm-cov show \
    "$BUILD_DIR/tests/unit/kern_tests" \
    -instr-profile="$PROFDATA" \
    -ignore-filename-regex='(googletest|_deps|tests/)' \
    --sources lib/ include/ \
    -format=html \
    -output-dir="$REPORT_DIR"

echo "=== HTML report: $REPORT_DIR/index.html ==="
