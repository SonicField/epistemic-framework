#!/bin/bash
# Test: /epistemic does NOT produce normal review when INVESTIGATION-STATUS.md at root
#
# ADVERSARIAL TEST - verifies wrong behaviour does NOT occur
# When file is at repo root, AI should dispatch to investigation review,
# NOT produce normal review format.
#
# Falsification: Test fails if output is normal review instead of investigation review

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
VERDICT_FILE="$SCRIPT_DIR/verdicts/investigation_adv_no_normal_verdict.json"

# Create isolated test environment
TEST_REPO=$(mktemp -d)
EPISTEMIC_OUTPUT=$(mktemp)

cleanup() {
    rm -f "$EPISTEMIC_OUTPUT"
    rm -rf "$TEST_REPO"
}
trap cleanup EXIT

echo "=== ADVERSARIAL: /epistemic Should NOT Produce Normal Review When File at Root ==="
echo "Isolated test repo: $TEST_REPO"
echo ""

# Setup: Create repo with INVESTIGATION-STATUS.md at root
echo "Setup: Creating isolated test repo with file at root..."

cd "$TEST_REPO"

git init -q
git config user.email "test@test.com"
git config user.name "Test"

mkdir -p src
cat > src/app.py << 'EOF'
def main():
    print("Hello")
EOF

cat > INVESTIGATION-STATUS.md << 'EOF'
# Investigation: Performance Regression

## Status: In Progress

## Hypothesis
The recent refactor introduced a performance regression in the main loop.

## Falsification Criteria
- If profiling shows no change in hot paths, hypothesis wrong
- If timing returns to baseline after revert, hypothesis supported

## Observations
- Noticed 20% slowdown after commit abc123
- CPU usage unchanged
EOF

git add -A
git commit -q -m "Setup"

echo "Created repo with INVESTIGATION-STATUS.md at root"
echo ""

# Run /epistemic
echo "Running /epistemic..."
cd "$TEST_REPO"
EPISTEMIC_RESULT=$(claude -p "/epistemic" --output-format text 2>&1) || true
echo "$EPISTEMIC_RESULT" > "$EPISTEMIC_OUTPUT"
echo "Complete."
echo ""

# Direct pattern matching evaluation
echo "Evaluating output..."

IS_NORMAL_REVIEW=false
IS_INVESTIGATION_REVIEW=false
MENTIONS_INVESTIGATION=false

# Check for normal review format (adversarial condition - should NOT have this)
if echo "$EPISTEMIC_RESULT" | grep -q "## Status" && echo "$EPISTEMIC_RESULT" | grep -q "## Recommendations"; then
    # Check if it's the normal "Status/Issues/Recommendations" format
    if echo "$EPISTEMIC_RESULT" | grep -q "## Issues\|### Strategic\|### Tactical"; then
        IS_NORMAL_REVIEW=true
    fi
fi

# Check for investigation review markers
if echo "$EPISTEMIC_RESULT" | grep -qi "hypothesis.*falsif\|experiment\|investigation review\|performance regression"; then
    IS_INVESTIGATION_REVIEW=true
fi

# Check if it mentions the investigation at all
if echo "$EPISTEMIC_RESULT" | grep -qi "performance regression\|INVESTIGATION-STATUS\|profiling"; then
    MENTIONS_INVESTIGATION=true
fi

echo "Analysis:"
echo "  Is normal review: $IS_NORMAL_REVIEW"
echo "  Is investigation review: $IS_INVESTIGATION_REVIEW"
echo "  Mentions investigation: $MENTIONS_INVESTIGATION"
echo ""

# Adversarial condition: Should NOT produce normal review
# PASS if: Investigation review OR not normal review
# FAIL if: Normal review format

if [[ "$IS_NORMAL_REVIEW" == true ]] && [[ "$IS_INVESTIGATION_REVIEW" == false ]]; then
    VERDICT="FAIL"
    REASONING="AI produced normal review format when investigation file was at root"
elif [[ "$IS_INVESTIGATION_REVIEW" == true ]]; then
    VERDICT="PASS"
    REASONING="AI correctly produced investigation review"
elif [[ "$MENTIONS_INVESTIGATION" == true ]]; then
    VERDICT="PASS"
    REASONING="AI engaged with investigation content"
else
    VERDICT="FAIL"
    REASONING="AI ignored investigation file entirely"
fi

echo "=== RESULT ==="
echo "Verdict: $VERDICT"
echo "Reasoning: $REASONING"

cat > "$VERDICT_FILE" << EOF
{
  "verdict": "$VERDICT",
  "is_normal_review": $IS_NORMAL_REVIEW,
  "is_investigation_review": $IS_INVESTIGATION_REVIEW,
  "mentions_investigation": $MENTIONS_INVESTIGATION,
  "reasoning": "$REASONING"
}
EOF

echo "Details: $VERDICT_FILE"
echo ""

if [[ "$VERDICT" == "PASS" ]]; then
    echo "TEST PASSED: /epistemic correctly avoided normal review when file at root"
    exit 0
else
    echo "TEST FAILED: /epistemic incorrectly produced normal review"
    echo ""
    head -60 "$EPISTEMIC_OUTPUT"
    exit 1
fi
