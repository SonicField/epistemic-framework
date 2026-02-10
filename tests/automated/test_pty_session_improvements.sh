#!/bin/bash
# Automated tests for pty-session improvements
#
# Tests three new features:
# 1. Status display in list (running/killed)
# 2. Blocking read with --wait flag
# 3. Dead session cache

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PTY="${SCRIPT_DIR}/../../bin/pty-session"

# Cleanup function
cleanup() {
    $PTY kill test1 2>/dev/null || true
    $PTY kill test2 2>/dev/null || true
    $PTY kill test3 2>/dev/null || true
    rm -rf ~/.pty-session/cache/test* 2>/dev/null || true
    rm -rf ~/.pty-session/logs/test* 2>/dev/null || true
}

# Run cleanup before starting
cleanup
trap cleanup EXIT

echo "=== pty-session Improvements Tests ==="
echo

# Test 1: Status display - running
echo "Test 1: List shows 'running' status"
$PTY create test1 'echo "Test 1"; sleep 30'
sleep 1
if $PTY list | grep -q "test1.*running"; then
    echo "✓ PASS: Shows 'running' status"
else
    echo "✗ FAIL: Should show 'running' status"
    exit 1
fi
echo

# Test 2: Status display - killed
echo "Test 2: List shows 'killed' status"
$PTY kill test1
sleep 1
if $PTY list | grep -q "test1.*killed"; then
    echo "✓ PASS: Shows 'killed' status"
else
    echo "✗ FAIL: Should show 'killed' status"
    exit 1
fi
echo

# Test 3: Read from cache
echo "Test 3: Read cached output"
output=$($PTY read test1)
if echo "$output" | grep -q "Test 1"; then
    echo "✓ PASS: Successfully read from cache"
else
    echo "✗ FAIL: Failed to read from cache"
    exit 1
fi
echo

# Test 4: Cache consumed after read (log fallback still works)
echo "Test 4: Cache consumed after first read"
if [[ ! -f "${HOME}/.pty-session/cache/test1.output" ]]; then
    echo "✓ PASS: Cache file properly consumed"
else
    echo "✗ FAIL: Cache file should be consumed"
    exit 1
fi
# Second read should still work via persistent log
if $PTY read test1 2>&1 | grep -q "Test 1"; then
    echo "✓ PASS: Persistent log fallback works after cache consumed"
else
    echo "✗ FAIL: Persistent log fallback should return content"
    exit 1
fi
echo

# Test 5: Blocking read with --wait
echo "Test 5: Blocking read with --wait"
$PTY create test2 'echo "Blocking test"; sleep 30'
sleep 1
# Kill in background after 3 seconds
(sleep 3 && $PTY kill test2 > /dev/null 2>&1) &
start=$(date +%s)
output=$($PTY read test2 --wait)
end=$(date +%s)
elapsed=$((end - start))
if [[ $elapsed -ge 2 ]] && [[ $elapsed -le 5 ]]; then
    if echo "$output" | grep -q "Blocking test"; then
        echo "✓ PASS: Blocked for ${elapsed}s and got output"
    else
        echo "✗ FAIL: Wrong output from blocking read"
        exit 1
    fi
else
    echo "✗ FAIL: Wrong timing: ${elapsed}s (expected ~3s)"
    exit 1
fi
echo

# Test 6: --wait with already-killed session
echo "Test 6: --wait with already-killed session"
$PTY create test3 'echo "Quick test"; sleep 30'
sleep 1
$PTY kill test3
sleep 1
output=$($PTY read test3 --wait)
if echo "$output" | grep -q "Quick test"; then
    echo "✓ PASS: --wait works with already-dead session"
else
    echo "✗ FAIL: Failed to read already-dead session"
    exit 1
fi
echo

echo "=== All tests passed ==="
