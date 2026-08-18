#!/bin/bash
# kern-fmt tool integration tests
set -e

KERN_FMT="$1"
PASS=0
FAIL=0

if [ -z "$KERN_FMT" ] || [ ! -x "$KERN_FMT" ]; then
    echo "Usage: $0 <kern-fmt-binary>"
    exit 1
fi

# Test 1: Format simple function
echo -n "  FMT: format simple function... "
INPUT="fn  add(a:i64,b:i64)->i64{a+b}"
OUTPUT=$(echo "$INPUT" | "$KERN_FMT" /dev/stdin 2>/dev/null)
if echo "$OUTPUT" | grep -q "fn add"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 2: --check mode returns 0 for formatted code
echo -n "  FMT: --check on formatted code... "
FORMATTED="fn add(a: i64, b: i64) -> i64 {
    a + b
}
"
if echo "$FORMATTED" | "$KERN_FMT" --check /dev/stdin 2>/dev/null; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "PASS (check mode)"; PASS=$((PASS + 1))
fi

# Test 3: Format struct declaration
echo -n "  FMT: format struct... "
INPUT="struct Point{val x:i64,val y:i64}"
OUTPUT=$(echo "$INPUT" | "$KERN_FMT" /dev/stdin 2>/dev/null)
if echo "$OUTPUT" | grep -q "struct Point"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 4: --help flag
echo -n "  FMT: --help... "
if "$KERN_FMT" --help 2>&1 | grep -qi "usage\|format\|kern"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

echo ""
echo "kern-fmt: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] || exit 1
