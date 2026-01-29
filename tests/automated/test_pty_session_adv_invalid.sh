#!/bin/bash
# Test: pty-session handles invalid session names gracefully
#
# Falsification: Test fails if commands crash instead of returning proper errors

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
PTY_SESSION="$PROJECT_ROOT/bin/pty-session"
VERDICT_FILE="$SCRIPT_DIR/verdicts/pty_session_invalid_session_verdict.json"

echo "=== Testing pty-session Invalid Session Handling ==="
echo ""

ERRORS=0

# Test 1: Read non-existent session
echo "Test 1: Read non-existent session..."
if "$PTY_SESSION" read "nonexistent_session_12345" 2>/dev/null; then
    echo "  FAIL: Read succeeded on non-existent session"
    ERRORS=$((ERRORS + 1))
else
    EXIT_CODE=$?
    if [[ $EXIT_CODE -eq 2 ]]; then
        echo "  PASS: Correct exit code (2 = session not found)"
    else
        echo "  FAIL: Wrong exit code: $EXIT_CODE (expected 2)"
        ERRORS=$((ERRORS + 1))
    fi
fi

# Test 2: Send to non-existent session
echo "Test 2: Send to non-existent session..."
if "$PTY_SESSION" send "nonexistent_session_12345" "hello" 2>/dev/null; then
    echo "  FAIL: Send succeeded on non-existent session"
    ERRORS=$((ERRORS + 1))
else
    EXIT_CODE=$?
    if [[ $EXIT_CODE -eq 2 ]]; then
        echo "  PASS: Correct exit code (2 = session not found)"
    else
        echo "  FAIL: Wrong exit code: $EXIT_CODE (expected 2)"
        ERRORS=$((ERRORS + 1))
    fi
fi

# Test 3: Kill non-existent session
echo "Test 3: Kill non-existent session..."
if "$PTY_SESSION" kill "nonexistent_session_12345" 2>/dev/null; then
    echo "  FAIL: Kill succeeded on non-existent session"
    ERRORS=$((ERRORS + 1))
else
    EXIT_CODE=$?
    if [[ $EXIT_CODE -eq 2 ]]; then
        echo "  PASS: Correct exit code (2 = session not found)"
    else
        echo "  FAIL: Wrong exit code: $EXIT_CODE (expected 2)"
        ERRORS=$((ERRORS + 1))
    fi
fi

# Test 4: Wait on non-existent session
echo "Test 4: Wait on non-existent session..."
if "$PTY_SESSION" wait "nonexistent_session_12345" "pattern" --timeout=1 2>/dev/null; then
    echo "  FAIL: Wait succeeded on non-existent session"
    ERRORS=$((ERRORS + 1))
else
    EXIT_CODE=$?
    if [[ $EXIT_CODE -eq 2 ]]; then
        echo "  PASS: Correct exit code (2 = session not found)"
    else
        echo "  FAIL: Wrong exit code: $EXIT_CODE (expected 2)"
        ERRORS=$((ERRORS + 1))
    fi
fi

# Test 5: Create with missing arguments
echo "Test 5: Create with missing arguments..."
if "$PTY_SESSION" create 2>/dev/null; then
    echo "  FAIL: Create succeeded without arguments"
    ERRORS=$((ERRORS + 1))
else
    EXIT_CODE=$?
    if [[ $EXIT_CODE -eq 4 ]]; then
        echo "  PASS: Correct exit code (4 = invalid arguments)"
    else
        echo "  FAIL: Wrong exit code: $EXIT_CODE (expected 4)"
        ERRORS=$((ERRORS + 1))
    fi
fi

# Test 6: Unknown command
echo "Test 6: Unknown command..."
if "$PTY_SESSION" unknowncommand 2>/dev/null; then
    echo "  FAIL: Unknown command succeeded"
    ERRORS=$((ERRORS + 1))
else
    EXIT_CODE=$?
    if [[ $EXIT_CODE -eq 4 ]]; then
        echo "  PASS: Correct exit code (4 = invalid arguments)"
    else
        echo "  FAIL: Wrong exit code: $EXIT_CODE (expected 4)"
        ERRORS=$((ERRORS + 1))
    fi
fi

echo ""
echo "=== RESULT ==="

if [[ $ERRORS -eq 0 ]]; then
    echo "Verdict: PASS"
    echo '{"verdict": "PASS", "errors": 0, "tests_run": 6}' > "$VERDICT_FILE"
    exit 0
else
    echo "Verdict: FAIL"
    echo "{\"verdict\": \"FAIL\", \"errors\": $ERRORS, \"tests_run\": 6}" > "$VERDICT_FILE"
    exit 1
fi
