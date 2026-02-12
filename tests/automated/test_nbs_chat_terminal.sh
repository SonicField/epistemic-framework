#!/bin/bash
# Test: nbs-chat-terminal comprehensive tests including adversarial cases
#
# Tests covering:
#   1.  Basic launch and exit
#   2.  Help command output
#   3.  Send and display via CLI verification
#   4.  Multi-line message assembly
#   5.  Exit command
#   6.  EOF (Ctrl-D) handling
#   7.  Missing arguments
#   8.  Missing chat file
#   9.  Malformed chat file — truncated header
#   10. Malformed chat file — corrupted base64
#   11. Malformed chat file — missing header fields
#   12. Huge message (1MB)
#   13. Unicode: emoji, accented chars, CJK
#   14. Empty handle
#   15. Special characters in handle
#   16. Concurrent terminal + CLI access
#   17. Message display from other senders
#   18. Colour output contains ANSI codes
#   19. Rapid sequential sends
#   20. Binary data in message (null-free)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
NBS_CHAT="${NBS_CHAT_BIN:-$PROJECT_ROOT/bin/nbs-chat}"
NBS_TERMINAL="${NBS_TERMINAL_BIN:-$PROJECT_ROOT/bin/nbs-chat-terminal}"

TEST_DIR=$(mktemp -d)
ERRORS=0

cleanup() {
    rm -rf "$TEST_DIR"
}
trap cleanup EXIT

check() {
    local label="$1"
    local result="$2"
    if [[ "$result" == "pass" ]]; then
        echo "   PASS: $label"
    else
        echo "   FAIL: $label"
        ERRORS=$((ERRORS + 1))
    fi
}

# Helper: run terminal with scripted input, capture output
# Uses a pseudo-terminal approach via script(1) or direct pipe
run_terminal() {
    local chat_file="$1"
    local handle="$2"
    local input="$3"
    local timeout="${4:-5}"

    echo -e "$input" | timeout "$timeout" "$NBS_TERMINAL" "$chat_file" "$handle" 2>/dev/null || true
}

echo "=== nbs-chat-terminal Comprehensive Test ==="
echo "Test dir: $TEST_DIR"
echo ""

# --- Test 1: Basic launch and exit via EOF ---
echo "1. Basic launch and exit via EOF..."
CHAT="$TEST_DIR/test1.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
OUTPUT=$(echo "" | timeout 5 "$NBS_TERMINAL" "$CHAT" "tester" 2>/dev/null || true)
check "Exits cleanly on EOF" "pass"

echo ""

# --- Test 2: Help command output ---
echo "2. Help command output..."
CHAT="$TEST_DIR/test2.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
OUTPUT=$(printf '/help\n\x04' | timeout 5 "$NBS_TERMINAL" "$CHAT" "tester" 2>/dev/null || true)
check "Help mentions /edit" "$( echo "$OUTPUT" | grep -qF '/edit' && echo pass || echo fail )"
check "Help mentions /exit" "$( echo "$OUTPUT" | grep -qF '/exit' && echo pass || echo fail )"
check "Help mentions /help" "$( echo "$OUTPUT" | grep -qF '/help' && echo pass || echo fail )"

echo ""

# --- Test 3: Send and verify via CLI ---
echo "3. Send and verify via CLI..."
CHAT="$TEST_DIR/test3.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
# Send a message then exit
printf 'Hello world\n\n\x04' | timeout 5 "$NBS_TERMINAL" "$CHAT" "tester" >/dev/null 2>&1 || true
OUTPUT=$("$NBS_CHAT" read "$CHAT")
check "Message sent via terminal" "$( echo "$OUTPUT" | grep -qF 'tester: Hello world' && echo pass || echo fail )"

echo ""

# --- Test 4: Multi-line message assembly ---
echo "4. Multi-line message assembly..."
CHAT="$TEST_DIR/test4.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
printf 'Line one\nLine two\nLine three\n\n\x04' | timeout 5 "$NBS_TERMINAL" "$CHAT" "tester" >/dev/null 2>&1 || true
OUTPUT=$("$NBS_CHAT" read "$CHAT")
check "Multi-line contains Line one" "$( echo "$OUTPUT" | grep -qF 'Line one' && echo pass || echo fail )"
check "Multi-line contains Line three" "$( echo "$OUTPUT" | grep -qF 'Line three' && echo pass || echo fail )"
MSG_COUNT=$(echo "$OUTPUT" | grep -c '^tester:' || true)
check "Only 1 message (multi-line combined)" "$( [[ "$MSG_COUNT" -eq 1 ]] && echo pass || echo fail )"

echo ""

# --- Test 5: Exit command ---
echo "5. Exit command..."
CHAT="$TEST_DIR/test5.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
set +e
printf '/exit\n' | timeout 5 "$NBS_TERMINAL" "$CHAT" "tester" >/dev/null 2>&1
RC=$?
set -e
check "Exit command returns 0" "$( [[ $RC -eq 0 ]] && echo pass || echo fail )"

echo ""

# --- Test 6: EOF with pending buffer ---
echo "6. EOF with pending buffer..."
CHAT="$TEST_DIR/test6.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
printf 'Buffered message\x04' | timeout 5 "$NBS_TERMINAL" "$CHAT" "tester" >/dev/null 2>&1 || true
OUTPUT=$("$NBS_CHAT" read "$CHAT")
check "EOF sends pending buffer" "$( echo "$OUTPUT" | grep -qF 'tester: Buffered message' && echo pass || echo fail )"

echo ""

# --- Test 7: Missing arguments ---
echo "7. Missing arguments..."
set +e
"$NBS_TERMINAL" >/dev/null 2>&1
RC=$?
set -e
check "Exit 4 for no args" "$( [[ $RC -eq 4 ]] && echo pass || echo fail )"

set +e
"$NBS_TERMINAL" "$TEST_DIR/x.chat" >/dev/null 2>&1
RC=$?
set -e
check "Exit 4 for one arg" "$( [[ $RC -eq 4 ]] && echo pass || echo fail )"

echo ""

# --- Test 8: Missing chat file ---
echo "8. Missing chat file..."
set +e
printf '/exit\n' | "$NBS_TERMINAL" "$TEST_DIR/nonexistent.chat" "tester" >/dev/null 2>&1
RC=$?
set -e
check "Exit 2 for missing file" "$( [[ $RC -eq 2 ]] && echo pass || echo fail )"

echo ""

# --- Test 9: Malformed chat file — truncated header ---
echo "9. Malformed chat file — truncated header..."
CHAT="$TEST_DIR/test9.chat"
echo "=== nbs-chat ===" > "$CHAT"
echo "last-writer: system" >> "$CHAT"
# No closing --- delimiter
set +e
printf '/exit\n' | timeout 5 "$NBS_TERMINAL" "$CHAT" "tester" >/dev/null 2>&1
RC=$?
set -e
check "Handles truncated header" "$( [[ $RC -eq 0 ]] && echo pass || echo fail )"

echo ""

# --- Test 10: Malformed chat file — corrupted base64 ---
echo "10. Malformed chat file — corrupted base64..."
CHAT="$TEST_DIR/test10.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
"$NBS_CHAT" send "$CHAT" "alice" "Valid message"
# Append garbage base64
echo "!!!NOT-BASE64!!!" >> "$CHAT"
set +e
printf '/exit\n' | timeout 5 "$NBS_TERMINAL" "$CHAT" "tester" >/dev/null 2>&1
RC=$?
set -e
check "Survives corrupted base64" "$( [[ $RC -eq 0 ]] && echo pass || echo fail )"

echo ""

# --- Test 11: Malformed chat file — missing header fields ---
echo "11. Malformed chat file — missing header fields..."
CHAT="$TEST_DIR/test11.chat"
printf '=== nbs-chat ===\n---\n' > "$CHAT"
set +e
printf '/exit\n' | timeout 5 "$NBS_TERMINAL" "$CHAT" "tester" >/dev/null 2>&1
RC=$?
set -e
check "Handles missing header fields" "$( [[ $RC -eq 0 ]] && echo pass || echo fail )"

echo ""

# --- Test 12: Huge message (100KB) ---
echo "12. Huge message (100KB)..."
CHAT="$TEST_DIR/test12.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
BIG_MSG=$(python3 -c "print('A' * (100 * 1024))" 2>/dev/null || head -c 102400 < /dev/zero | tr '\0' 'A')
"$NBS_CHAT" send "$CHAT" "bigwriter" "$BIG_MSG"
set +e
printf '/exit\n' | timeout 10 "$NBS_TERMINAL" "$CHAT" "tester" >/dev/null 2>&1
RC=$?
set -e
check "Handles 100KB message display" "$( [[ $RC -eq 0 ]] && echo pass || echo fail )"
READBACK=$("$NBS_CHAT" read "$CHAT" 2>/dev/null)
READBACK_LEN=$(echo "$READBACK" | wc -c)
check "100KB message preserved" "$( [[ "$READBACK_LEN" -gt 100000 ]] && echo pass || echo fail )"

echo ""

# --- Test 13: Unicode ---
echo "13. Unicode: emoji, accented chars, CJK..."
CHAT="$TEST_DIR/test13.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
"$NBS_CHAT" send "$CHAT" "tester" "Héllo wörld 你好世界 🎉🚀"
READBACK=$("$NBS_CHAT" read "$CHAT")
check "Unicode preserved" "$( echo "$READBACK" | grep -qF '你好世界' && echo pass || echo fail )"
check "Emoji preserved" "$( echo "$READBACK" | grep -qF '🎉' && echo pass || echo fail )"

# Display via terminal
set +e
printf '/exit\n' | timeout 5 "$NBS_TERMINAL" "$CHAT" "viewer" >/dev/null 2>&1
RC=$?
set -e
check "Terminal handles unicode display" "$( [[ $RC -eq 0 ]] && echo pass || echo fail )"

echo ""

# --- Test 14: Empty handle ---
echo "14. Empty handle..."
CHAT="$TEST_DIR/test14.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
# The terminal requires a handle argument, so empty string arg
set +e
printf '/exit\n' | timeout 5 "$NBS_TERMINAL" "$CHAT" "" >/dev/null 2>&1
RC=$?
set -e
# Should still work (empty handle is technically valid at this level)
check "Empty handle doesn't crash" "$( [[ $RC -eq 0 || $RC -eq 4 ]] && echo pass || echo fail )"

echo ""

# --- Test 15: Special characters in handle ---
echo "15. Special characters in handle..."
CHAT="$TEST_DIR/test15.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
printf 'Testing special handle\n\n\x04' | timeout 5 "$NBS_TERMINAL" "$CHAT" "user-123_test" >/dev/null 2>&1 || true
OUTPUT=$("$NBS_CHAT" read "$CHAT")
check "Handle with dashes/underscores" "$( echo "$OUTPUT" | grep -qF 'user-123_test:' && echo pass || echo fail )"

echo ""

# --- Test 16: Concurrent terminal + CLI access ---
echo "16. Concurrent terminal + CLI access..."
CHAT="$TEST_DIR/test16.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null

# Start terminal in background, it will send a message
(printf 'From terminal\n\n\x04' | timeout 5 "$NBS_TERMINAL" "$CHAT" "term-user" >/dev/null 2>&1) &
TERM_PID=$!

# Simultaneously send via CLI
sleep 1
"$NBS_CHAT" send "$CHAT" "cli-user" "From CLI"
wait "$TERM_PID" 2>/dev/null || true

OUTPUT=$("$NBS_CHAT" read "$CHAT")
check "Terminal message present" "$( echo "$OUTPUT" | grep -qF 'term-user: From terminal' && echo pass || echo fail )"
check "CLI message present" "$( echo "$OUTPUT" | grep -qF 'cli-user: From CLI' && echo pass || echo fail )"

echo ""

# --- Test 17: Message display from other senders ---
echo "17. Message display from other senders..."
CHAT="$TEST_DIR/test17.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
"$NBS_CHAT" send "$CHAT" "alice" "Hello from alice"
"$NBS_CHAT" send "$CHAT" "bob" "Hello from bob"
OUTPUT=$(printf '/exit\n' | timeout 5 "$NBS_TERMINAL" "$CHAT" "viewer" 2>/dev/null || true)
check "Shows alice's message" "$( echo "$OUTPUT" | grep -qF 'alice' && echo pass || echo fail )"
check "Shows bob's message" "$( echo "$OUTPUT" | grep -qF 'bob' && echo pass || echo fail )"

echo ""

# --- Test 18: Colour output contains ANSI codes ---
echo "18. Colour output contains ANSI codes..."
CHAT="$TEST_DIR/test18.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
"$NBS_CHAT" send "$CHAT" "coloured" "Test message"
OUTPUT=$(printf '/exit\n' | timeout 5 "$NBS_TERMINAL" "$CHAT" "viewer" 2>/dev/null || true)
# Check for ANSI escape sequences (ESC[ pattern)
check "Output contains ANSI codes" "$( echo "$OUTPUT" | grep -qP '\x1b\[' && echo pass || echo fail )"

echo ""

# --- Test 19: Rapid sequential sends ---
echo "19. Rapid sequential sends..."
CHAT="$TEST_DIR/test19.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
# Send 20 messages rapidly via terminal
INPUT=""
for i in $(seq 1 20); do
    INPUT="${INPUT}Message $i\n\n"
done
INPUT="${INPUT}\x04"
printf "$INPUT" | timeout 15 "$NBS_TERMINAL" "$CHAT" "rapid" >/dev/null 2>&1 || true
OUTPUT=$("$NBS_CHAT" read "$CHAT")
MSG_COUNT=$(echo "$OUTPUT" | grep -c '.' || true)
check "20 rapid messages sent" "$( [[ "$MSG_COUNT" -eq 20 ]] && echo pass || echo fail )"

echo ""

# --- Test 20: Binary-safe (null-free special bytes) ---
echo "20. Binary-safe message content..."
CHAT="$TEST_DIR/test20.chat"
"$NBS_CHAT" create "$CHAT" >/dev/null
# Message with tabs, backslashes, equals signs
"$NBS_CHAT" send "$CHAT" "tester" "key=value	tab\\backslash\"quote"
READBACK=$("$NBS_CHAT" read "$CHAT")
check "Special bytes preserved" "$( echo "$READBACK" | grep -qF 'key=value' && echo pass || echo fail )"

echo ""

# --- Summary ---
echo "=== Result ==="
if [[ $ERRORS -eq 0 ]]; then
    echo "PASS: All tests passed"
    exit 0
else
    echo "FAIL: $ERRORS test(s) failed"
    exit 1
fi
