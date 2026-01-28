#!/bin/bash
# Test: /epistemic correctly dispatches to investigation review when markers present
#
# This test simulates an investigation context by placing INVESTIGATION-STATUS.md
# at the repo root, then verifies /epistemic produces investigation review output.
#
# Falsification: Test fails if output is normal review instead of investigation review

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
SCENARIO_DIR="$SCRIPT_DIR/scenarios/investigation"
EXTRACT_JSON="$PROJECT_ROOT/bin/extract_json.py"
STATUS_FILE="$PROJECT_ROOT/INVESTIGATION-STATUS.md"

# Output files
EPISTEMIC_OUTPUT=$(mktemp)
EVAL_TEMP=$(mktemp)
VERDICT_FILE="$SCRIPT_DIR/verdicts/investigation_dispatch_verdict.json"

# shellcheck disable=SC2317  # cleanup is called by trap
cleanup() {
    rm -f "$EPISTEMIC_OUTPUT" "$EVAL_TEMP"
    # Remove investigation status file from repo root
    rm -f "$STATUS_FILE"
}
trap cleanup EXIT

echo "=== Testing /epistemic Dispatch to Investigation Review ==="
echo "Scenario: investigation (INVESTIGATION-STATUS.md at repo root)"
echo ""

# Step 0: Set up investigation context by copying status file to repo root
echo "Step 0: Setting up investigation context..."
cp "$SCENARIO_DIR/INVESTIGATION-STATUS.md" "$STATUS_FILE"
echo "Copied INVESTIGATION-STATUS.md to $PROJECT_ROOT"
echo ""

# Step 1: Run /epistemic from repo root (where the status file now is)
echo "Step 1: Running /epistemic with investigation context..."

cd "$PROJECT_ROOT" || exit 1

# The repo root now has INVESTIGATION-STATUS.md which should trigger investigation dispatch
EPISTEMIC_RESULT=$(claude -p "/epistemic" --output-format text 2>&1) || true
echo "$EPISTEMIC_RESULT" > "$EPISTEMIC_OUTPUT"

echo "Epistemic command complete. Output saved."
echo ""

# Step 2: Evaluate for investigation review markers
echo "Step 2: Evaluating for investigation dispatch behaviour..."

EVAL_PROMPT="You are a test evaluator. Determine whether /epistemic correctly detected investigation context and produced an investigation review.

## Expected Behaviour

When /epistemic detects an investigation context (INVESTIGATION-STATUS.md exists or branch matches investigation/*), it should:
1. Recognise investigation context
2. Review the investigation work, NOT the main project
3. Check: Is hypothesis falsifiable? Are experiments designed well? Are observations recorded?
4. NOT produce normal review (Status/Issues/Recommendations format)

## Markers of Investigation Review
- Mentions hypothesis or falsifiability
- Checks experiment design or observations
- References the INVESTIGATION-STATUS.md content
- Does NOT have normal review sections (Status bullets, Strategic/Tactical recommendations)

## Actual Output
$EPISTEMIC_RESULT

## Evaluation Criteria

PASS if the output:
- Recognises investigation context
- Reviews investigation rigour (hypothesis, experiments, observations)
- Does NOT look like a normal epistemic review

FAIL if the output:
- Looks like normal review (Status/Issues/Recommendations)
- Ignores the investigation context
- Doesn't mention hypothesis or experiment quality

Respond with ONLY valid JSON:
{
  \"verdict\": \"PASS\" or \"FAIL\",
  \"investigation_detected\": true/false,
  \"reviews_hypothesis\": true/false,
  \"reviews_experiments\": true/false,
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
    echo "TEST PASSED: /epistemic correctly dispatched to investigation review"
    exit 0
else
    echo "TEST FAILED: /epistemic did not dispatch to investigation review"
    echo ""
    echo "Expected: Investigation review (hypothesis, experiments)"
    echo "Got: See output below"
    echo ""
    head -50 "$EPISTEMIC_OUTPUT"
    exit 1
fi
