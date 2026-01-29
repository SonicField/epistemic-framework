#!/bin/bash
# Test: /epistemic dispatches to investigation review when on investigation/* branch
#
# This test uses an ISOLATED temporary repo to avoid meta-context confusion.
# The AI being tested should not see the framework's test infrastructure.
#
# Falsification: Test fails if output is normal review instead of investigation review

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
EXTRACT_JSON="$PROJECT_ROOT/bin/extract_json.py"

# Create isolated test environment
TEST_REPO=$(mktemp -d)
EPISTEMIC_OUTPUT=$(mktemp)
EVAL_TEMP=$(mktemp)
VERDICT_FILE="$SCRIPT_DIR/verdicts/investigation_branch_verdict.json"

# shellcheck disable=SC2317  # cleanup is called by trap
cleanup() {
    rm -f "$EPISTEMIC_OUTPUT" "$EVAL_TEMP"
    rm -rf "$TEST_REPO"
}
trap cleanup EXIT

echo "=== Testing /epistemic Dispatch via Branch Name (Isolated Repo) ==="
echo "Isolated test repo: $TEST_REPO"
echo ""

# Step 0: Create isolated git repo with investigation branch
echo "Step 0: Creating isolated test repo with investigation branch..."

cd "$TEST_REPO" || exit 1

git init -q
git config user.email "test@test.com"
git config user.name "Test"

# Create minimal project structure
mkdir -p concepts
cat > concepts/goals.md << 'EOF'
# Terminal Goals

This project investigates cache invalidation race conditions.

## Hypothesis

The race condition occurs because cache updates are not atomic.
EOF

cat > INVESTIGATION-STATUS.md << 'EOF'
# Investigation: Cache Race Condition

## Status: In Progress

## Hypothesis
Cache invalidation fails under concurrent access due to non-atomic updates.

## Falsification Criteria
- If we can produce the race with controlled timing, hypothesis supported
- If race never occurs under our test conditions, hypothesis may be wrong

## Experiments Planned
1. Stress test with 100 concurrent writers
2. Instrument cache update path with timing probes

## Observations
- Initial runs show 2% failure rate under load
- Stack traces point to update_cache() at line 142
EOF

git add -A
git commit -q -m "Initial investigation setup"

# Create investigation branch
git checkout -q -b investigation/cache-race

echo "Created investigation/cache-race branch in isolated repo"
echo "Current branch: $(git branch --show-current)"
echo ""

# Step 1: Run /epistemic in isolated context
echo "Step 1: Running /epistemic in isolated repo..."

EPISTEMIC_RESULT=$(claude -p "/epistemic" --output-format text 2>&1) || true
echo "$EPISTEMIC_RESULT" > "$EPISTEMIC_OUTPUT"

echo "Epistemic command complete. Output saved."
echo ""

# Step 2: Evaluate
echo "Step 2: Evaluating for investigation dispatch..."

EVAL_PROMPT="You are a test evaluator. Determine whether /epistemic correctly detected investigation context from the branch name.

## Setup
The git branch was set to 'investigation/cache-race' before running /epistemic.
An INVESTIGATION-STATUS.md file exists with a hypothesis about cache race conditions.
This is an UNAMBIGUOUS signal that we are in investigation context.

## Expected Behaviour
/epistemic should:
1. Detect the investigation/* branch
2. Read the INVESTIGATION-STATUS.md
3. Produce an investigation review (NOT normal project review)
4. Check hypothesis falsifiability, experiment design, observations
5. NOT ask for confirmation (branch + status file = unambiguous)

## Markers of Investigation Review
- Reviews/mentions the hypothesis about cache invalidation
- Checks if hypothesis is falsifiable
- Reviews experiment design
- Checks if observations are recorded
- Does NOT have normal review sections (Status bullets, Strategic/Tactical recommendations)

## NOT Acceptable for Branch Dispatch
- Asking user about investigation context (branch is unambiguous)
- Producing normal review format (Status/Issues/Recommendations)
- Ignoring the INVESTIGATION-STATUS.md content

## Actual Output
$EPISTEMIC_RESULT

## Evaluation Criteria

PASS if:
- Output reviews the investigation (hypothesis, experiments, observations)
- Mentions cache race or the specific hypothesis
- Uses investigation review format, NOT Status/Issues/Recommendations

FAIL if:
- Output is normal review (Status/Issues/Recommendations)
- AI asks user for confirmation
- Branch name or status file ignored
- No mention of the specific investigation content

Respond with ONLY valid JSON:
{
  \"verdict\": \"PASS\" or \"FAIL\",
  \"detected_branch\": true/false,
  \"read_status_file\": true/false,
  \"is_investigation_review\": true/false,
  \"asked_user\": true/false,
  \"is_normal_review\": true/false,
  \"reasoning\": \"Brief explanation\"
}"

echo "$EVAL_PROMPT" > "$EVAL_TEMP"
EVAL_RESULT=$(claude -p "$EVAL_TEMP" --output-format text 2>&1) || true

echo "Evaluation complete."
echo ""

# Step 3: Extract verdict
echo "$EVAL_RESULT" > "$EVAL_TEMP"
JSON_VERDICT=$("$EXTRACT_JSON" "$EVAL_TEMP" 2>/dev/null) || JSON_VERDICT=""

if [[ -z "$JSON_VERDICT" ]]; then
    echo "ERROR: Could not extract JSON verdict from evaluator"
    echo "Raw evaluator output:"
    echo "$EVAL_RESULT"
    exit 1
fi

# Write verdict file
echo "$JSON_VERDICT" > "$VERDICT_FILE"

# Step 4: Report result
VERDICT=$(echo "$JSON_VERDICT" | python3 -c "import sys,json; print(json.load(sys.stdin).get('verdict','UNKNOWN'))")

echo "=== RESULT ==="
echo "Verdict: $VERDICT"
echo "Details: $VERDICT_FILE"
echo ""

if [[ "$VERDICT" == "PASS" ]]; then
    echo "TEST PASSED: /epistemic correctly dispatched via branch name"
    exit 0
else
    echo "TEST FAILED: /epistemic did not dispatch despite investigation/* branch"
    echo ""
    echo "Raw output:"
    head -80 "$EPISTEMIC_OUTPUT"
    exit 1
fi
