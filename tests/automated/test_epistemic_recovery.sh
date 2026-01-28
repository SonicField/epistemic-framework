#!/bin/bash
# Test: /epistemic-recovery produces a valid recovery plan
#
# This test provides a mock discovery report and verifies that
# recovery produces a structured, step-wise plan.
#
# NOTE: This tests plan generation only, not execution (which is interactive).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
SCENARIO_DIR="$SCRIPT_DIR/scenarios/messy_project"
MOCK_REPORT="$SCENARIO_DIR/mock_discovery_report.md"
EXTRACT_JSON="$PROJECT_ROOT/bin/extract_json.py"

# Output files
RECOVERY_OUTPUT=$(mktemp)
EVAL_TEMP=$(mktemp)
VERDICT_FILE="$SCRIPT_DIR/verdicts/recovery_test_verdict.json"

# shellcheck disable=SC2317  # cleanup is called by trap
cleanup() {
    rm -f "$RECOVERY_OUTPUT" "$EVAL_TEMP"
}
trap cleanup EXIT

echo "=== Testing /epistemic-recovery (plan generation) ==="
echo "Scenario: $SCENARIO_DIR"
echo "Discovery report: $MOCK_REPORT"
echo ""

# Step 1: Run recovery to generate plan only
echo "Step 1: Running recovery command (plan generation)..."

RECOVERY_PROMPT="Run /epistemic-recovery on this project: $SCENARIO_DIR

The discovery report is at: $MOCK_REPORT

Generate ONLY the recovery plan. Do NOT execute any steps.
Stop after presenting the plan and ask for approval.

When asked questions:
- Discovery report location: $MOCK_REPORT
- Nothing has changed since discovery
- No decisions reconsidered"

RECOVERY_RESULT=$(echo "$RECOVERY_PROMPT" | claude -p - --output-format text 2>&1) || true
echo "$RECOVERY_RESULT" > "$RECOVERY_OUTPUT"

echo "Recovery plan generated. Output saved."
echo ""

# Step 2: Read the mock report for context
MOCK_REPORT_CONTENT=$(cat "$MOCK_REPORT")

# Step 3: Evaluate with AI
echo "Step 2: Evaluating recovery plan..."

EVAL_PROMPT="You are a test evaluator. Assess whether a recovery plan meets quality criteria.

## Discovery Report (input to recovery)
$MOCK_REPORT_CONTENT

## Recovery Plan (output to evaluate)
$RECOVERY_RESULT

## Evaluation Criteria

The recovery plan PASSES if it:
1. Contains numbered/ordered steps
2. Each step describes WHAT will happen
3. Each step explains WHY (linked to discovery findings)
4. Each step explains HOW TO UNDO (reversibility)
5. Addresses the key items from discovery:
   - Archive/discard v1 and scratch.py
   - Keep v2 as main loader
   - Handle v3 (evaluate/decide)
   - Update README
6. Does NOT execute any steps (plan only)

The recovery plan FAILS if it:
- Steps are not atomic (multiple actions per step)
- Missing reversibility information
- Skips major artefacts from discovery
- Actually executes changes instead of planning

## Your Task

Evaluate the recovery plan against the criteria.

Respond with ONLY valid JSON (no markdown, no explanation):
{
  \"verdict\": \"PASS\" or \"FAIL\",
  \"criteria_met\": {
    \"numbered_steps\": true/false,
    \"what_described\": true/false,
    \"why_linked\": true/false,
    \"reversibility_noted\": true/false,
    \"key_items_addressed\": true/false,
    \"plan_only_no_execution\": true/false
  },
  \"step_count\": <number of steps in plan>,
  \"reasoning\": \"Brief explanation of verdict\"
}"

echo "$EVAL_PROMPT" > "$EVAL_TEMP"
EVAL_RESULT=$(claude -p "$EVAL_TEMP" --output-format text 2>&1) || true

echo "Evaluation complete."
echo ""

# Step 4: Extract verdict
# Write result to temp file for extraction
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

# Step 5: Report result
VERDICT=$(echo "$JSON_VERDICT" | python3 -c "import sys,json; print(json.load(sys.stdin).get('verdict','UNKNOWN'))")

echo "=== RESULT ==="
echo "Verdict: $VERDICT"
echo "Details: $VERDICT_FILE"
echo ""

if [[ "$VERDICT" == "PASS" ]]; then
    echo "TEST PASSED"
    exit 0
else
    echo "TEST FAILED"
    echo ""
    echo "Criteria assessment:"
    echo "$JSON_VERDICT" | python3 -c "
import sys, json
data = json.load(sys.stdin)
for k, v in data.get('criteria_met', {}).items():
    status = '✓' if v else '✗'
    print(f'  {status} {k}')
print()
print('Step count:', data.get('step_count', 'unknown'))
print('Reasoning:', data.get('reasoning', 'none'))
"
    exit 1
fi
