#!/bin/bash
# Test: /epistemic produces normal review when NO discovery was run
#
# This is an ADVERSARIAL test - it verifies that the dispatch system
# does NOT incorrectly trigger verification mode in normal contexts.
#
# Falsification: Test fails if output looks like verification checklist
# instead of normal epistemic review

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
SCENARIO_DIR="$SCRIPT_DIR/scenarios/no_plan_project"
EXTRACT_JSON="$PROJECT_ROOT/bin/extract_json.py"

# Output files
EPISTEMIC_OUTPUT=$(mktemp)
EVAL_TEMP=$(mktemp)
VERDICT_FILE="$SCRIPT_DIR/verdicts/dispatch_adversarial_verdict.json"

# shellcheck disable=SC2317  # cleanup is called by trap
cleanup() {
    rm -f "$EPISTEMIC_OUTPUT" "$EVAL_TEMP"
}
trap cleanup EXIT

echo "=== Testing /epistemic Does NOT Dispatch Without Discovery ==="
echo "Scenario: no_plan_project (normal review context)"
echo ""

# Step 1: Run /epistemic in a normal context (no discovery)
echo "Step 1: Running /epistemic in normal context..."

cd "$SCENARIO_DIR" || exit 1

# Run epistemic with NO discovery context - just on a project
EPISTEMIC_RESULT=$(claude -p "/epistemic" --output-format text 2>&1) || true
echo "$EPISTEMIC_RESULT" > "$EPISTEMIC_OUTPUT"

echo "Epistemic command complete. Output saved."
echo ""

# Step 2: Evaluate - should be normal review, NOT verification
echo "Step 2: Evaluating for correct non-dispatch..."

EVAL_PROMPT="You are a test evaluator. Determine whether /epistemic correctly produced a NORMAL review (not verification mode).

## Expected Behaviour

When /epistemic runs WITHOUT prior discovery, it should produce a NORMAL REVIEW:
- Status section with health assessment (2-4 bullets)
- Issues section with problems found
- Recommendations section (Strategic/Tactical)

It should NOT produce verification mode output:
- No section checklists (✓/✗ Terminal Goal, ✓/✗ Artefacts Found, etc.)
- No 'Discovery Verification' heading
- No checks for 'confirmed restatements'

## Actual Output
$EPISTEMIC_RESULT

## Evaluation Criteria

PASS if the output:
- Looks like a normal epistemic review (Status/Issues/Recommendations)
- Assesses the project's epistemic health
- Does NOT have verification checklist format
- Does NOT mention 'verifying discovery report'

FAIL if the output:
- Contains verification checklist format
- Mentions 'Discovery Verification' or verifying sections
- Looks like it's checking a discovery report

Respond with ONLY valid JSON:
{
  \"verdict\": \"PASS\" or \"FAIL\",
  \"is_normal_review\": true/false,
  \"has_status_section\": true/false,
  \"has_issues_section\": true/false,
  \"has_recommendations\": true/false,
  \"incorrectly_dispatched\": true/false,
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
    echo "TEST PASSED: /epistemic correctly produced normal review (no false dispatch)"
    exit 0
else
    echo "TEST FAILED: /epistemic incorrectly dispatched to verification mode"
    echo ""
    echo "Expected: Normal review format"
    echo "Got: See output below"
    echo ""
    head -50 "$EPISTEMIC_OUTPUT"
    exit 1
fi
