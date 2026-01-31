#!/bin/bash
# Test: /nbs dispatches to investigation review when INVESTIGATION-STATUS.md at repo root
#
# This tests file-based detection (no investigation branch).
# Uses isolated repo to avoid meta-context pollution.
#
# Falsification: Test fails if output is normal review instead of investigation review

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
EXTRACT_JSON="$PROJECT_ROOT/bin/extract_json.py"

# Create isolated test environment
TEST_REPO=$(mktemp -d)
NBS_OUTPUT=$(mktemp)
EVAL_TEMP=$(mktemp)
VERDICT_FILE="$SCRIPT_DIR/verdicts/investigation_file_verdict.json"

# shellcheck disable=SC2317  # cleanup is called by trap
cleanup() {
    rm -f "$NBS_OUTPUT" "$EVAL_TEMP"
    rm -rf "$TEST_REPO"
}
trap cleanup EXIT

echo "=== Testing /nbs Dispatch via INVESTIGATION-STATUS.md at Root ==="
echo "Isolated test repo: $TEST_REPO"
echo ""

# Step 0: Create isolated git repo with status file at root (no investigation branch)
echo "Step 0: Creating isolated test repo with INVESTIGATION-STATUS.md at root..."

cd "$TEST_REPO" || exit 1

git init -q
git config user.email "test@test.com"
git config user.name "Test"

# Create minimal project structure
mkdir -p src
cat > src/main.py << 'EOF'
# Main application - investigating memory leak
def process_data(data):
    cache = {}
    for item in data:
        cache[item.id] = item  # Suspected leak here
    return cache
EOF

cat > INVESTIGATION-STATUS.md << 'EOF'
# Investigation: Memory Leak in Data Processor

## Status: In Progress

## Hypothesis
The cache dictionary in process_data() grows unboundedly because items are never evicted.

## Falsification Criteria
- If memory usage stabilises after processing N items, hypothesis is wrong
- If memory grows linearly with input size indefinitely, hypothesis is supported

## Experiments Planned
1. Profile memory usage with 10k, 100k, 1M items
2. Add cache size logging at 1-second intervals
3. Compare with LRU cache implementation

## Observations
- Memory usage at 10k items: 50MB
- Memory usage at 100k items: 450MB
- Linear growth confirmed, no plateau observed
EOF

git add -A
git commit -q -m "Initial setup with investigation"

# Stay on main/master branch - NO investigation/* branch
echo "Created repo with INVESTIGATION-STATUS.md at root"
echo "Current branch: $(git branch --show-current)"
echo "Branch is NOT investigation/* - testing file-based detection"
echo ""

# Step 1: Run /nbs
echo "Step 1: Running /nbs..."

NBS_RESULT=$(claude -p "/nbs" --output-format text 2>&1) || true
echo "$NBS_RESULT" > "$NBS_OUTPUT"

echo "NBS command complete. Output saved."
echo ""

# Step 2: Evaluate
echo "Step 2: Evaluating for investigation dispatch..."

EVAL_PROMPT="You are a test evaluator. Determine whether /nbs correctly detected investigation context from INVESTIGATION-STATUS.md at repo root.

## Setup
- INVESTIGATION-STATUS.md exists at the repository root
- The file contains a hypothesis about memory leaks with observations
- The git branch is main/master (NOT investigation/*)
- This tests file-based detection, not branch-based detection

## Expected Behaviour
/nbs should:
1. Detect INVESTIGATION-STATUS.md at repo root
2. Read its contents
3. Produce an investigation review (NOT normal project review)
4. Check hypothesis falsifiability, experiment design, observations

## Markers of Investigation Review
- Reviews/mentions the hypothesis about memory leak
- Checks if hypothesis is falsifiable
- Reviews experiment design
- Checks if observations are recorded
- Does NOT use normal review sections (Status bullets, Strategic/Tactical recommendations)

## NOT Acceptable
- Producing normal review format (Status/Issues/Recommendations)
- Ignoring the INVESTIGATION-STATUS.md content

## Actual Output
$NBS_RESULT

## Evaluation Criteria

PASS if:
- Output reviews the investigation (hypothesis, experiments, observations)
- Mentions memory leak or the specific hypothesis
- Uses investigation review format

FAIL if:
- Output is normal review (Status/Issues/Recommendations)
- INVESTIGATION-STATUS.md ignored
- No mention of the specific investigation content

Respond with ONLY valid JSON:
{
  \"verdict\": \"PASS\" or \"FAIL\",
  \"detected_file\": true/false,
  \"read_status_file\": true/false,
  \"is_investigation_review\": true/false,
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
    echo "TEST PASSED: /nbs correctly dispatched via INVESTIGATION-STATUS.md at root"
    exit 0
else
    echo "TEST FAILED: /nbs did not dispatch despite INVESTIGATION-STATUS.md at root"
    echo ""
    echo "Raw output:"
    head -80 "$NBS_OUTPUT"
    exit 1
fi
