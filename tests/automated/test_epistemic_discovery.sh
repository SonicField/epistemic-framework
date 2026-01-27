#!/bin/bash
# Test: /epistemic-discovery produces a valid discovery report
#
# This test runs /epistemic-discovery on a prepared messy scenario,
# then uses an AI evaluator to verify the report captures known artefacts.
#
# The scenario has a GROUND_TRUTH.md file that the test reads but the
# discovery command should NOT read.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
SCENARIO_DIR="$SCRIPT_DIR/scenarios/messy_project"
GROUND_TRUTH="$SCENARIO_DIR/GROUND_TRUTH.md"
EXTRACT_JSON="$PROJECT_ROOT/bin/extract_json.py"

# Output files
DISCOVERY_OUTPUT=$(mktemp)
EVAL_TEMP=$(mktemp)
VERDICT_FILE="$SCRIPT_DIR/discovery_test_verdict.json"

cleanup() {
    rm -f "$DISCOVERY_OUTPUT" "$EVAL_TEMP"
}
trap cleanup EXIT

echo "=== Testing /epistemic-discovery ==="
echo "Scenario: $SCENARIO_DIR"
echo ""

# Step 1: Run discovery on the scenario
# We simulate human responses by providing them upfront
echo "Step 1: Running discovery command..."

DISCOVERY_PROMPT="Run /epistemic-discovery on this project: $SCENARIO_DIR

When asked about context, respond:
- Terminal goal: Parallelise data loading for a machine learning pipeline
- Timeframe: January 2024
- Locations: src/, old/, tests/
- Valuable outcomes: A working parallel loader
- Dead ends: First version had a race condition

Produce the discovery report."

DISCOVERY_RESULT=$(echo "$DISCOVERY_PROMPT" | claude -p - --output-format text 2>&1) || true
echo "$DISCOVERY_RESULT" > "$DISCOVERY_OUTPUT"

echo "Discovery complete. Output saved."
echo ""

# Step 2: Read ground truth
GROUND_TRUTH_CONTENT=$(cat "$GROUND_TRUTH")

# Step 3: Evaluate with AI
echo "Step 2: Evaluating discovery report..."

EVAL_PROMPT="You are a test evaluator. Compare a discovery report against known ground truth.

## Ground Truth (what should be found)
$GROUND_TRUTH_CONTENT

## Discovery Report (what was actually produced)
$DISCOVERY_RESULT

## Evaluation Criteria

The discovery report PASSES if it:
1. Identifies loader_v2.py as the working version (Keep)
2. Identifies loader_v1.py as failed/broken (Discard)
3. Identifies loader_v3_experimental.py as incomplete (Evaluate)
4. Mentions the benchmark results or performance data
5. Notes that v3 continuation is an open question
6. Produces a structured report (not just prose)

The discovery report FAILS if it:
- Misses any of the three loader versions
- Incorrectly assesses which version works
- Fails to identify the key decision (locks vs lock-free)
- Produces unstructured output

## Your Task

Evaluate the discovery report against the criteria.

Respond with ONLY valid JSON (no markdown, no explanation):
{
  \"verdict\": \"PASS\" or \"FAIL\",
  \"criteria_met\": {
    \"v2_identified_as_working\": true/false,
    \"v1_identified_as_failed\": true/false,
    \"v3_identified_as_incomplete\": true/false,
    \"benchmark_data_mentioned\": true/false,
    \"open_questions_identified\": true/false,
    \"structured_report\": true/false
  },
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
print('Reasoning:', data.get('reasoning', 'none'))
"
    exit 1
fi
