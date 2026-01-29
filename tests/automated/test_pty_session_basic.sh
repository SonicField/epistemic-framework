#!/bin/bash
# Test: pty-session basic create, send, read, kill cycle
#
# Falsification: Test fails if any operation errors or read doesn't show sent text

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
PTY_SESSION="$PROJECT_ROOT/bin/pty-session"
VERDICT_FILE="$SCRIPT_DIR/verdicts/pty_session_basic_verdict.json"

SESSION_NAME="test_basic_$$"

cleanup() {
    "$PTY_SESSION" kill "$SESSION_NAME" 2>/dev/null || true
}
trap cleanup EXIT

echo "=== Testing pty-session Basic Cycle ==="
echo ""

ERRORS=0

# Test 1: Create session
echo "Test 1: Create session..."
if "$PTY_SESSION" create "$SESSION_NAME" 'bash'; then
    echo "  PASS: Session created"
else
    echo "  FAIL: Could not create session"
    ERRORS=$((ERRORS + 1))
fi

# Test 2: Session appears in list
echo "Test 2: Session appears in list..."
if "$PTY_SESSION" list | grep -q "$SESSION_NAME"; then
    echo "  PASS: Session visible in list"
else
    echo "  FAIL: Session not in list"
    ERRORS=$((ERRORS + 1))
fi

# Test 3: Send text
echo "Test 3: Send text..."
sleep 1  # Wait for bash to start
if "$PTY_SESSION" send "$SESSION_NAME" 'echo MARKER_12345'; then
    echo "  PASS: Text sent"
else
    echo "  FAIL: Could not send text"
    ERRORS=$((ERRORS + 1))
fi

# Test 4: Read shows sent text
echo "Test 4: Read shows sent text..."
sleep 1  # Wait for command to execute
OUTPUT=$("$PTY_SESSION" read "$SESSION_NAME")
if echo "$OUTPUT" | grep -q "MARKER_12345"; then
    echo "  PASS: Marker found in output"
else
    echo "  FAIL: Marker not found in output"
    echo "  Output was:"
    echo "$OUTPUT" | head -20
    ERRORS=$((ERRORS + 1))
fi

# Test 5: Kill session
echo "Test 5: Kill session..."
if "$PTY_SESSION" kill "$SESSION_NAME"; then
    echo "  PASS: Session killed"
else
    echo "  FAIL: Could not kill session"
    ERRORS=$((ERRORS + 1))
fi

# Test 6: Session no longer in list
echo "Test 6: Session no longer in list..."
if ! "$PTY_SESSION" list | grep -q "$SESSION_NAME"; then
    echo "  PASS: Session removed from list"
else
    echo "  FAIL: Session still in list after kill"
    ERRORS=$((ERRORS + 1))
fi

echo ""
echo "=== RESULT ==="

if [[ $ERRORS -eq 0 ]]; then
    echo "Verdict: PASS"
    echo '{"verdict": "PASS", "errors": 0, "tests_run": 6}' > "$VERDICT_FILE"
    echo "All 6 tests passed"
    exit 0
else
    echo "Verdict: FAIL"
    echo "{\"verdict\": \"FAIL\", \"errors\": $ERRORS, \"tests_run\": 6}" > "$VERDICT_FILE"
    echo "$ERRORS of 6 tests failed"
    exit 1
fi
