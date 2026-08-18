#!/bin/bash
# coverage_gate.sh — Check that line coverage meets the 98% threshold
#
# Usage:
#   bash scripts/coverage_gate.sh [build-dir] [threshold]
#
# Defaults: build-dir=build-cov, threshold=98

set -euo pipefail

BUILD_DIR="${1:-build-cov}"
THRESHOLD="${2:-98}"
PROFDATA="$BUILD_DIR/coverage.profdata"

if [ ! -f "$PROFDATA" ]; then
    echo "ERROR: $PROFDATA not found. Run scripts/run_coverage.sh first."
    exit 1
fi

# Extract line coverage percentage from llvm-cov report
COVERAGE=$(xcrun llvm-cov report \
    "$BUILD_DIR/tests/unit/kern_tests" \
    -instr-profile="$PROFDATA" \
    -ignore-filename-regex='(googletest|_deps|tests/)' \
    --sources lib/ include/ \
    | tail -1 \
    | awk '{
        # Last line format: "TOTAL ... XX.XX%"
        for (i=1; i<=NF; i++) {
            if ($i ~ /%$/) {
                gsub(/%/, "", $i);
                print $i;
                exit;
            }
        }
    }')

if [ -z "$COVERAGE" ]; then
    echo "ERROR: Could not parse coverage percentage."
    exit 1
fi

# Compare (integer arithmetic — truncate to floor)
COV_INT=$(echo "$COVERAGE" | awk '{printf "%d", $1}')

echo "Line coverage: ${COVERAGE}% (threshold: ${THRESHOLD}%)"

if [ "$COV_INT" -ge "$THRESHOLD" ]; then
    echo "PASS: Coverage meets threshold."
    exit 0
else
    echo "FAIL: Coverage ${COVERAGE}% is below ${THRESHOLD}% threshold."
    exit 1
fi
