#!/bin/bash
# Kern integration test runner
# Usage: ./run_tests.sh <kernc-path> [test-dir]
#
# Each .kern file can have a companion .expected file with:
#   exit:<number>  — expected exit code
#   error:<text>   — expected error output substring (for error tests)

set -e

KERNC="$1"
TEST_DIR="${2:-.}"
PASS=0
FAIL=0
SKIP=0
FAILURES=""

if [ -z "$KERNC" ] || [ ! -x "$KERNC" ]; then
    echo "Usage: $0 <kernc-binary> [test-dir]"
    exit 1
fi

for kern_file in $(find "$TEST_DIR" -name '*.kern' | sort); do
    [ -f "$kern_file" ] || continue
    test_name=$(basename "$kern_file" .kern)
    expected_file="${kern_file%.kern}.expected"

    if [ ! -f "$expected_file" ]; then
        echo "  SKIP  $test_name (no .expected file)"
        SKIP=$((SKIP + 1))
        continue
    fi

    # Read expected values
    expected_exit=""
    expected_error=""
    expected_stdout=""
    compiler_args=""
    while IFS= read -r line; do
        case "$line" in
            exit:*) expected_exit="${line#exit:}" ;;
            error:*) expected_error="${line#error:}" ;;
            stdout:*) expected_stdout="${line#stdout:}" ;;
            args:*) compiler_args="${line#args:}" ;;
        esac
    done < "$expected_file"

    # Determine if this is an error test
    if [ -n "$expected_error" ]; then
        # Error test — compilation should fail
        output=$("$KERNC" "$kern_file" -o /tmp/kern_test_bin 2>&1) && {
            echo "  FAIL  $test_name (expected compilation error, but succeeded)"
            FAIL=$((FAIL + 1))
            FAILURES="$FAILURES\n  $test_name: expected error containing '$expected_error'"
            continue
        }
        if echo "$output" | grep -qF "$expected_error"; then
            echo "  PASS  $test_name (error)"
            PASS=$((PASS + 1))
        else
            echo "  FAIL  $test_name (error message mismatch)"
            echo "        expected: $expected_error"
            echo "        got: $output"
            FAIL=$((FAIL + 1))
            FAILURES="$FAILURES\n  $test_name: error mismatch"
        fi
        continue
    fi

    # Stdout test — check compiler output (e.g., --dump-ir)
    if [ -n "$expected_stdout" ]; then
        output=$("$KERNC" "$kern_file" $compiler_args 2>&1)
        if echo "$output" | grep -qF "$expected_stdout"; then
            echo "  PASS  $test_name (stdout)"
            PASS=$((PASS + 1))
        else
            echo "  FAIL  $test_name (stdout mismatch)"
            echo "        expected: $expected_stdout"
            echo "        got: $output"
            FAIL=$((FAIL + 1))
            FAILURES="$FAILURES\n  $test_name: stdout mismatch"
        fi
        continue
    fi

    # Normal test — compile and run
    output=$("$KERNC" "$kern_file" $compiler_args -o /tmp/kern_test_bin 2>&1) || true
    if [ ! -f /tmp/kern_test_bin ]; then
        echo "  FAIL  $test_name (compilation failed)"
        echo "        $output"
        FAIL=$((FAIL + 1))
        FAILURES="$FAILURES\n  $test_name: compilation failed"
        continue
    fi

    set +e
    /tmp/kern_test_bin
    actual_exit=$?
    set -e
    rm -f /tmp/kern_test_bin

    if [ "$actual_exit" = "$expected_exit" ]; then
        echo "  PASS  $test_name (exit=$actual_exit)"
        PASS=$((PASS + 1))
    else
        echo "  FAIL  $test_name (expected exit=$expected_exit, got exit=$actual_exit)"
        FAIL=$((FAIL + 1))
        FAILURES="$FAILURES\n  $test_name: expected=$expected_exit actual=$actual_exit"
    fi
done

echo ""
echo "Results: $PASS passed, $FAIL failed, $SKIP skipped"
if [ $FAIL -gt 0 ]; then
    echo -e "Failures:$FAILURES"
    exit 1
fi
exit 0
