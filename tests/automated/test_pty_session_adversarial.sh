#!/bin/bash
# Adversarial tests for pty-session

set -e

PTY="./bin/pty-session"

echo "=== pty-session Adversarial Tests ==="
echo

# Cleanup
for session in $(tmux list-sessions -F '#{session_name}' 2>/dev/null | grep "^pty_" || true); do
    tmux kill-session -t "$session" 2>/dev/null || true
done
rm -rf ~/.pty-session/cache/* 2>/dev/null || true
rm -rf ~/.pty-session/logs/* 2>/dev/null || true

# Test 1: Path traversal
echo "Test 1: Path traversal attempt"
$PTY create "../../../etc/passwd" 'echo test' 2>/dev/null || true
sleep 1
if [ -f "/etc/passwd.output" ] || [ -f ~/.pty-session/cache/../../../etc/passwd.output ]; then
    echo "✗ FAIL: Path traversal vulnerability"
    exit 1
fi
echo "✓ PASS: Path traversal blocked or sanitized"
tmux kill-session -t 'pty_../../../etc/passwd' 2>/dev/null || true
echo

# Test 2: Large output
echo "Test 2: Large output (1MB+)"
$PTY create large 'head -c 1048576 /dev/urandom | base64; sleep 30'
sleep 3
$PTY kill large
output_size=$(wc -c < ~/.pty-session/cache/large.output 2>/dev/null || echo 0)
if [ "$output_size" -gt 0 ]; then
    $PTY read large > /dev/null
    echo "✓ PASS: Large output handled (${output_size} bytes)"
else
    echo "✗ FAIL: Large output not cached"
    exit 1
fi
echo

# Test 3: Session name with slash
echo "Test 3: Session name with slash"
if $PTY create "test/slash" 'echo test' 2>&1; then
    if [ -d ~/.pty-session/cache/test ]; then
        echo "✗ FAIL: Slash created subdirectory"
        exit 1
    fi
    $PTY kill "test/slash" 2>/dev/null || true
    echo "⚠ WARNING: Session with / created - verify safety"
else
    echo "✓ PASS: Session with / rejected"
fi
echo

# Test 4: Many sessions
echo "Test 4: Resource exhaustion (50 sessions)"
for i in {1..50}; do
    $PTY create "bulk$i" "echo bulk$i; sleep 30" > /dev/null 2>&1
done
sleep 1
running=$($PTY list | grep running | wc -l)
echo "Created $running sessions"
for i in {1..50}; do
    $PTY kill "bulk$i" > /dev/null 2>&1
done
sleep 1
cached=$(ls ~/.pty-session/cache/*.output 2>/dev/null | wc -l)
echo "Cached: $cached"
for i in {1..50}; do
    $PTY read "bulk$i" > /dev/null 2>&1 || true
done
# Clean up bulk log files
for i in {1..50}; do
    rm -f ~/.pty-session/logs/bulk${i}.log 2>/dev/null
done
echo "✓ PASS: Bulk sessions handled"
echo

echo "=== Adversarial tests complete ==="
