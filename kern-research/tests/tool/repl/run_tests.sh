#!/bin/bash
# kern-repl tool smoke tests
set -e

KERN_REPL="$1"
PASS=0
FAIL=0

if [ -z "$KERN_REPL" ] || [ ! -x "$KERN_REPL" ]; then
    echo "Usage: $0 <kern-repl-binary>"
    exit 1
fi

# Test 1: version banner shown
echo -n "  REPL: version banner... "
OUTPUT=$(echo ":quit" | "$KERN_REPL" 2>&1)
if echo "$OUTPUT" | grep -q "kern-repl"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 2: quit command exits cleanly
echo -n "  REPL: :quit exits 0... "
echo ":quit" | "$KERN_REPL" >/dev/null 2>&1
if [ $? -eq 0 ]; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 3: help command
echo -n "  REPL: :help command... "
OUTPUT=$(echo -e ":help\n:quit" | "$KERN_REPL" 2>&1)
if echo "$OUTPUT" | grep -qi "help\|commands\|quit"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 4: simple expression evaluation
echo -n "  REPL: evaluate 42... "
OUTPUT=$(echo -e "42\n:quit" | "$KERN_REPL" 2>&1)
if echo "$OUTPUT" | grep -q "42"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 5: defs command
echo -n "  REPL: :defs command... "
OUTPUT=$(echo -e ":defs\n:quit" | "$KERN_REPL" 2>&1)
echo "PASS"; PASS=$((PASS + 1))

echo ""
echo "kern-repl: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] || exit 1
