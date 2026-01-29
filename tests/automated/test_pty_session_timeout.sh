#!/bin/bash
# Test: pty-session wait times out correctly when pattern absent
#
# Falsification: Test fails if wait doesn't timeout when pattern never appears

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
PTY_SESSION="$PROJECT_ROOT/bin/pty-session"
VERDICT_FILE="$SCRIPT_DIR/verdicts/pty_session_timeout_verdict.json"

SESSION_NAME="test_timeout_$$"

cleanup() {
    "$PTY_SESSION" kill "$SESSION_NAME" 2>/dev/null || true
}
trap cleanup EXIT

echo "=== Testing pty-session Wait Timeout ==="
echo ""

ERRORS=0

# Create session
echo "Setup: Creating session..."
"$PTY_SESSION" create "$SESSION_NAME" 'bash'
sleep 1

# Test 1: Wait for pattern that will never appear, should timeout
echo "Test 1: Wait for non-existent pattern (should timeout)..."
START_TIME=$(date +%s)

if "$PTY_SESSION" wait "$SESSION_NAME" 'THIS_PATTERN_WILL_NEVER_APPEAR_12345' --timeout=3; then
    echo "  FAIL: Wait returned success for non-existent pattern"
    ERRORS=$((ERRORS + 1))
else
    EXIT_CODE=$?
    END_TIME=$(date +%s)
    ELAPSED=$((END_TIME - START_TIME))

    if [[ $EXIT_CODE -eq 3 ]]; then
        echo "  PASS: Correct exit code (3 = timeout)"
    else
        echo "  FAIL: Wrong exit code: $EXIT_CODE (expected 3)"
        ERRORS=$((ERRORS + 1))
    fi

    if [[ $ELAPSED -ge 2 ]] && [[ $ELAPSED -le 5 ]]; then
        echo "  PASS: Timeout took expected time (~3s, actual: ${ELAPSED}s)"
    else
        echo "  FAIL: Timeout took wrong time: ${ELAPSED}s (expected ~3s)"
        ERRORS=$((ERRORS + 1))
    fi
fi

echo ""
echo "=== RESULT ==="

if [[ $ERRORS -eq 0 ]]; then
    echo "Verdict: PASS"
    echo '{"verdict": "PASS", "errors": 0, "tests_run": 2}' > "$VERDICT_FILE"
    exit 0
else
    echo "Verdict: FAIL"
    echo "{\"verdict\": \"FAIL\", \"errors\": $ERRORS, \"tests_run\": 2}" > "$VERDICT_FILE"
    exit 1
fi
