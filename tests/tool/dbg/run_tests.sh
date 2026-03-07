#!/bin/bash
# kern-dbg tool smoke tests
set -e

KERN_DBG="$1"
PASS=0
FAIL=0

if [ -z "$KERN_DBG" ] || [ ! -x "$KERN_DBG" ]; then
    echo "Usage: $0 <kern-dbg-binary>"
    exit 1
fi

# Test 1: help command works
echo -n "  DBG: help command... "
OUTPUT=$(echo "help" | "$KERN_DBG" test 2>&1)
if echo "$OUTPUT" | grep -qi "usage\|commands\|help"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 2: quit command exits cleanly
echo -n "  DBG: quit command... "
echo "quit" | "$KERN_DBG" test >/dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 3: invalid command handled gracefully
echo -n "  DBG: invalid command... "
OUTPUT=$(echo -e "nonexistent_command\nquit" | "$KERN_DBG" test 2>&1)
if [ $? -eq 0 ] || echo "$OUTPUT" | grep -qi "unknown\|invalid\|error"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 4: version/banner on start
echo -n "  DBG: shows banner... "
OUTPUT=$(echo "quit" | "$KERN_DBG" test 2>&1)
if echo "$OUTPUT" | grep -q "kern-dbg"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

echo ""
echo "kern-dbg: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] || exit 1
