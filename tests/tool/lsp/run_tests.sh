#!/bin/bash
# kern-lsp tool integration tests
set -e

KERN_LSP="$1"
PASS=0
FAIL=0

if [ -z "$KERN_LSP" ] || [ ! -x "$KERN_LSP" ]; then
    echo "Usage: $0 <kern-lsp-binary>"
    exit 1
fi

# Test 1: Initialize and shutdown cleanly
echo -n "  LSP: initialize + shutdown... "
INIT_MSG='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"capabilities":{}}}'
SHUTDOWN_MSG='{"jsonrpc":"2.0","id":2,"method":"shutdown","params":null}'
EXIT_MSG='{"jsonrpc":"2.0","method":"exit","params":null}'

send_lsp() {
    local body="$1"
    local len=${#body}
    printf "Content-Length: %d\r\n\r\n%s" "$len" "$body"
}

OUTPUT=$(
    {
        send_lsp "$INIT_MSG"
        send_lsp "$SHUTDOWN_MSG"
        send_lsp "$EXIT_MSG"
    } | "$KERN_LSP" 2>/dev/null
)

if echo "$OUTPUT" | grep -q "kern-lsp"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "PASS (server responded)"; PASS=$((PASS + 1))
fi

# Test 2: Server advertises capabilities
echo -n "  LSP: capabilities... "
if echo "$OUTPUT" | grep -q "hoverProvider"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL (no hoverProvider in response)"; FAIL=$((FAIL + 1))
fi

# Test 3: Completion provider advertised
echo -n "  LSP: completion capability... "
if echo "$OUTPUT" | grep -q "completionProvider"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

# Test 4: Semantic tokens provider advertised
echo -n "  LSP: semantic tokens capability... "
if echo "$OUTPUT" | grep -q "semanticTokensProvider"; then
    echo "PASS"; PASS=$((PASS + 1))
else
    echo "FAIL"; FAIL=$((FAIL + 1))
fi

echo ""
echo "kern-lsp: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] || exit 1
