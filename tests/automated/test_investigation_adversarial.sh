#!/bin/bash
# Test: /nbs does NOT dispatch to investigation review without markers
#
# This is an ADVERSARIAL test - verifies the dispatch system does NOT
# incorrectly trigger investigation mode in normal contexts.
#
# Falsification: Test fails if output looks like investigation review
# instead of normal nbs review

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
SCENARIO_DIR="$SCRIPT_DIR/scenarios/no_plan_project"
EXTRACT_JSON="$PROJECT_ROOT/bin/extract_json.py"

# Output files
NBS_OUTPUT=$(mktemp)
EVAL_TEMP=$(mktemp)
VERDICT_FILE="$SCRIPT_DIR/verdicts/investigation_adversarial_verdict.json"

# shellcheck disable=SC2317  # cleanup is called by trap
cleanup() {
    rm -f "$NBS_OUTPUT" "$EVAL_TEMP"
}
trap cleanup EXIT

echo "=== Testing /nbs Does NOT Dispatch to Investigation Mode ==="
echo "Scenario: no_plan_project (no investigation markers)"
echo ""

# Step 1: Run /nbs in a normal context (no investigation markers)
echo "Step 1: Running /nbs in normal context..."

cd "$SCENARIO_DIR" || exit 1

# Run nbs - should produce normal review, NOT investigation review
NBS_RESULT=$(claude -p "/nbs" --output-format text 2>&1) || true
echo "$NBS_RESULT" > "$NBS_OUTPUT"

echo "NBS command complete. Output saved."
echo ""

# Step 2: Evaluate - should be normal review, NOT investigation review
echo "Step 2: Evaluating for correct non-dispatch..."

EVAL_PROMPT="You are a test evaluator. Determine whether /nbs correctly produced a NORMAL review (not investigation mode).

## Expected Behaviour

When /nbs runs WITHOUT investigation markers (no investigation/* branch, no INVESTIGATION-STATUS.md), it should produce a NORMAL REVIEW:
- Status section with health assessment
- Issues section with problems found
- Recommendations section (Strategic/Tactical)

It should NOT produce investigation review output:
- No checks for hypothesis falsifiability
- No review of experiment design
- No mentions of 'investigation rigour'

## Actual Output
$NBS_RESULT

## Evaluation Criteria

PASS if the output:
- Looks like a normal nbs review (Status/Issues/Recommendations)
- Assesses the project's NBS health
- Does NOT review hypothesis or experiments
- Does NOT mention investigation context

FAIL if the output:
- Reviews hypothesis quality or falsifiability
- Checks experiment design
- Mentions investigation rigour
- References INVESTIGATION-STATUS.md (which doesn't exist here)

Respond with ONLY valid JSON:
{
  \"verdict\": \"PASS\" or \"FAIL\",
  \"is_normal_review\": true/false,
  \"has_status_section\": true/false,
  \"has_recommendations\": true/false,
  \"incorrectly_reviews_investigation\": true/false,
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
    echo "TEST PASSED: /nbs correctly produced normal review (no false investigation dispatch)"
    exit 0
else
    echo "TEST FAILED: /nbs incorrectly dispatched to investigation mode"
    echo ""
    echo "Expected: Normal review format"
    echo "Got: See output below"
    echo ""
    head -50 "$NBS_OUTPUT"
    exit 1
fi
