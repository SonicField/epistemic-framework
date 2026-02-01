#!/bin/bash
# Test: Install Path Verification
#
# Verifies that template expansion produces correct paths and no leaked paths.
# This is the REGEX check (deterministic, fast).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"

echo "=== Install Path Verification Test ==="
echo ""

# Create unique temp directory for test installation
TEST_DIR=$(mktemp -d)
echo "Test directory: $TEST_DIR"

# Cleanup on exit
cleanup() {
    rm -rf "$TEST_DIR"
    echo "Cleaned up: $TEST_DIR"
}
trap cleanup EXIT

# Run installation to test directory
echo ""
echo "Step 1: Installing to test directory..."
"$PROJECT_ROOT/bin/install.sh" --prefix="$TEST_DIR"

# Verification functions
ERRORS=0

check_no_pattern() {
    local pattern="$1"
    local description="$2"

    if grep -rq "$pattern" "$TEST_DIR/commands/" 2>/dev/null; then
        echo "FAIL: Found '$pattern' in expanded files ($description)"
        grep -rn "$pattern" "$TEST_DIR/commands/" || true
        ERRORS=$((ERRORS + 1))
    else
        echo "PASS: No '$pattern' found ($description)"
    fi
}

check_pattern_exists() {
    local pattern="$1"
    local description="$2"

    if grep -rq "$pattern" "$TEST_DIR/commands/" 2>/dev/null; then
        echo "PASS: Found '$pattern' ($description)"
    else
        echo "FAIL: Expected '$pattern' not found ($description)"
        ERRORS=$((ERRORS + 1))
    fi
}

echo ""
echo "Step 2: Checking for leaked paths (should find NONE)..."

# Check for old hardcoded dev paths
check_no_pattern "claude_docs" "old dev path"

# Check for absolute user paths
check_no_pattern "/home/alexturner" "absolute user path"

# Check for unexpanded templates
check_no_pattern "{{NBS_ROOT}}" "unexpanded template"

# Check for any ~/ that isn't ~/.claude or ~/.nbs
# This regex finds ~/X where X is not a dot
if grep -rE '~/[^.]' "$TEST_DIR/commands/" 2>/dev/null | grep -v '~/.claude' | grep -v '~/.nbs'; then
    echo "FAIL: Found stray ~/ path (not ~/.claude or ~/.nbs)"
    ERRORS=$((ERRORS + 1))
else
    echo "PASS: No stray ~/ paths"
fi

echo ""
echo "Step 3: Checking for correct paths (should find these)..."

# Check that TEST_DIR paths exist in output
check_pattern_exists "$TEST_DIR/concepts" "test prefix in concepts paths"
check_pattern_exists "$TEST_DIR/bin" "test prefix in bin paths"

echo ""
echo "Step 4: Verifying directory structure..."

# Check directories exist
for dir in commands concepts docs templates bin; do
    if [[ -d "$TEST_DIR/$dir" ]] || [[ -L "$TEST_DIR/$dir" ]]; then
        echo "PASS: $TEST_DIR/$dir exists"
    else
        echo "FAIL: $TEST_DIR/$dir missing"
        ERRORS=$((ERRORS + 1))
    fi
done

# Check symlinks point to valid targets
for dir in concepts docs templates bin; do
    if [[ -L "$TEST_DIR/$dir" ]] && [[ -e "$TEST_DIR/$dir" ]]; then
        echo "PASS: $TEST_DIR/$dir symlink valid"
    else
        echo "FAIL: $TEST_DIR/$dir symlink broken"
        ERRORS=$((ERRORS + 1))
    fi
done

# Check command files exist
CMD_COUNT=$(find "$TEST_DIR/commands" -name "*.md" | wc -l)
if [[ "$CMD_COUNT" -ge 10 ]]; then
    echo "PASS: Found $CMD_COUNT command files"
else
    echo "FAIL: Only $CMD_COUNT command files (expected >= 10)"
    ERRORS=$((ERRORS + 1))
fi

echo ""
echo "Step 5: Adversarial check - plant bad path and verify detection..."

# Create a file with a bad path
echo "Read \`~/claude_docs/bad/path.md\`" > "$TEST_DIR/commands/ADVERSARIAL_TEST.md"

# Verify our check catches it
if grep -rq "claude_docs" "$TEST_DIR/commands/" 2>/dev/null; then
    echo "PASS: Adversarial path detected (as expected)"
    rm "$TEST_DIR/commands/ADVERSARIAL_TEST.md"
else
    echo "FAIL: Adversarial path NOT detected - check is broken!"
    ERRORS=$((ERRORS + 1))
fi

echo ""
echo "=== Results ==="
if [[ "$ERRORS" -eq 0 ]]; then
    echo "ALL TESTS PASSED"
    exit 0
else
    echo "FAILED: $ERRORS errors"
    exit 1
fi
