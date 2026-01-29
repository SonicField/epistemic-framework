#!/bin/bash
# Test: pty-session sessions don't collide with user tmux sessions
#
# Falsification: Test fails if pty-session affects non-pty_ prefixed sessions

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
PTY_SESSION="$PROJECT_ROOT/bin/pty-session"
VERDICT_FILE="$SCRIPT_DIR/verdicts/pty_session_no_collision_verdict.json"

USER_SESSION="user_session_test_$$"
PTY_SESSION_NAME="collision_test_$$"

cleanup() {
    "$PTY_SESSION" kill "$PTY_SESSION_NAME" 2>/dev/null || true
    tmux kill-session -t "$USER_SESSION" 2>/dev/null || true
}
trap cleanup EXIT

echo "=== Testing pty-session No Collision with User Sessions ==="
echo ""

ERRORS=0

# Create a user tmux session (not via pty-session)
echo "Setup: Creating user tmux session..."
tmux new-session -d -s "$USER_SESSION" 'bash'
sleep 1

# Create a pty-session
echo "Setup: Creating pty-session..."
"$PTY_SESSION" create "$PTY_SESSION_NAME" 'bash'
sleep 1

# Test 1: User session not visible in pty-session list
echo "Test 1: User session not in pty-session list..."
if ! "$PTY_SESSION" list | grep -q "$USER_SESSION"; then
    echo "  PASS: User session not exposed in pty-session list"
else
    echo "  FAIL: User session appears in pty-session list"
    ERRORS=$((ERRORS + 1))
fi

# Test 2: pty-session kill doesn't affect user session
echo "Test 2: Killing pty-session doesn't affect user session..."
"$PTY_SESSION" kill "$PTY_SESSION_NAME"

if tmux has-session -t "$USER_SESSION" 2>/dev/null; then
    echo "  PASS: User session still exists after pty-session kill"
else
    echo "  FAIL: User session was killed by pty-session kill"
    ERRORS=$((ERRORS + 1))
fi

# Test 3: Cannot operate on user session via pty-session
echo "Test 3: Cannot read user session via pty-session..."
if ! "$PTY_SESSION" read "$USER_SESSION" 2>/dev/null; then
    echo "  PASS: Cannot read user session (session not found)"
else
    echo "  FAIL: Was able to read user session via pty-session"
    ERRORS=$((ERRORS + 1))
fi

echo ""
echo "=== RESULT ==="

if [[ $ERRORS -eq 0 ]]; then
    echo "Verdict: PASS"
    echo '{"verdict": "PASS", "errors": 0, "tests_run": 3}' > "$VERDICT_FILE"
    exit 0
else
    echo "Verdict: FAIL"
    echo "{\"verdict\": \"FAIL\", \"errors\": $ERRORS, \"tests_run\": 3}" > "$VERDICT_FILE"
    exit 1
fi
