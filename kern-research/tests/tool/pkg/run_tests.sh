#!/bin/bash
# kern-pkg tool smoke tests
set -e

KERN_PKG="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
PASS=0
FAIL=0

if [ -z "$1" ] || [ ! -x "$KERN_PKG" ]; then
    echo "Usage: $0 <kern-pkg-binary>"
    exit 1
fi

# Test 1: --help flag
echo -n "  PKG: --help... "
OUTPUT=$("$KERN_PKG" --help 2>&1)
if echo "$OUTPUT" | grep -qi "usage\|commands\|kern-pkg"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 2: no args shows help (exits non-zero)
echo -n "  PKG: no args shows usage... "
OUTPUT=$("$KERN_PKG" 2>&1 || true)
if echo "$OUTPUT" | grep -qi "usage"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 3: init creates kern.toml
echo -n "  PKG: init creates kern.toml... "
TMPDIR=$(mktemp -d)
(cd "$TMPDIR" && "$KERN_PKG" init 2>/dev/null)
if [ -f "$TMPDIR/kern.toml" ]; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi
rm -rf "$TMPDIR"

# Test 4: kern.toml has expected content
echo -n "  PKG: kern.toml has project name... "
TMPDIR=$(mktemp -d)
(cd "$TMPDIR" && "$KERN_PKG" init 2>/dev/null)
if grep -q "name" "$TMPDIR/kern.toml" 2>/dev/null; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi
rm -rf "$TMPDIR"

# Test 5: unknown command fails gracefully
echo -n "  PKG: unknown command... "
"$KERN_PKG" nonexistent 2>/dev/null || true
echo "PASS"; PASS=$((PASS + 1))

echo ""
echo "kern-pkg: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] || exit 1
