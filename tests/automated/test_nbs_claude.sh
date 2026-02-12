#!/bin/bash
# Test nbs-claude: verify wrapper script basics
#
# Tests:
#   1. Script exists and is executable
#   2. Handles missing pty-session gracefully
#   3. Session naming includes PID
#   4. Poll sidecar function exists
#   5. Args are passed through correctly

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
NBS_CLAUDE="$PROJECT_ROOT/bin/nbs-claude"

PASS=0
FAIL=0
TESTS=0

pass() {
    PASS=$((PASS + 1))
    TESTS=$((TESTS + 1))
    echo "   PASS: $1"
}

fail() {
    FAIL=$((FAIL + 1))
    TESTS=$((TESTS + 1))
    echo "   FAIL: $1"
}

echo "=== nbs-claude Tests ==="
echo ""

# 1. Script exists and is executable
echo "1. Script exists and is executable..."
if [[ -x "$NBS_CLAUDE" ]]; then
    pass "Script is executable"
else
    fail "Script not found or not executable"
fi

# 2. Script contains key components
echo "2. Script contains key components..."
if grep -q 'poll_sidecar' "$NBS_CLAUDE"; then
    pass "Has poll_sidecar function"
else
    fail "Missing poll_sidecar function"
fi

if grep -q 'pty-session' "$NBS_CLAUDE"; then
    pass "References pty-session"
else
    fail "Missing pty-session reference"
fi

if grep -q 'NBS_POLL_INTERVAL' "$NBS_CLAUDE"; then
    pass "Has configurable poll interval"
else
    fail "Missing poll interval config"
fi

if grep -q 'NBS_POLL_DISABLE' "$NBS_CLAUDE"; then
    pass "Has disable option"
else
    fail "Missing disable option"
fi

if grep -q 'cleanup' "$NBS_CLAUDE"; then
    pass "Has cleanup trap"
else
    fail "Missing cleanup trap"
fi

if grep -q '/nbs-poll' "$NBS_CLAUDE"; then
    pass "Injects /nbs-poll command"
else
    fail "Missing /nbs-poll injection"
fi

# 3. Session name includes PID for uniqueness
echo "3. Session naming..."
if grep -q 'SESSION_NAME="nbs-claude-\$\$"' "$NBS_CLAUDE"; then
    pass "Session name includes PID"
else
    fail "Session name missing PID"
fi

# 4. Idle detection logic
echo "4. Idle detection..."
if grep -q 'idle_seconds' "$NBS_CLAUDE"; then
    pass "Has idle counter"
else
    fail "Missing idle counter"
fi

if grep -q 'md5sum' "$NBS_CLAUDE"; then
    pass "Uses content hashing for change detection"
else
    fail "Missing content hashing"
fi

# 5. nbs-poll skill doc exists
echo "5. nbs-poll skill doc..."
POLL_DOC="$PROJECT_ROOT/claude_tools/nbs-poll.md"
if [[ -f "$POLL_DOC" ]]; then
    pass "Skill doc exists"
else
    fail "Skill doc not found"
fi

if grep -q 'Check Chats' "$POLL_DOC"; then
    pass "Skill doc has chat check section"
else
    fail "Skill doc missing chat check"
fi

if grep -q 'Check Workers' "$POLL_DOC"; then
    pass "Skill doc has worker check section"
else
    fail "Skill doc missing worker check"
fi

if grep -q 'return silently' "$POLL_DOC"; then
    pass "Skill doc specifies silent return"
else
    fail "Skill doc missing silent return behaviour"
fi

echo ""
echo "=== Result ==="
if [[ $FAIL -eq 0 ]]; then
    echo "PASS: All $TESTS tests passed"
else
    echo "FAIL: $FAIL of $TESTS tests failed"
fi

exit $FAIL
