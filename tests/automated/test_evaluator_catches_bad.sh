#!/bin/bash
# Test: Evaluator correctly fails on a bad discovery report
#
# This is a META-TEST: it tests the test infrastructure itself.
# A bad discovery report (missing artefacts, wrong verdicts) is evaluated.
# The evaluator should return FAIL.
# This test passes if the evaluator correctly identifies the bad report.
#
# Falsification: If this test passes when fed a bad report, our evaluator
# is not catching errors and all other tests are suspect.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
SCENARIO_DIR="$SCRIPT_DIR/scenarios/bad_discovery"
BAD_REPORT="$SCENARIO_DIR/mock_bad_discovery.md"
EXTRACT_JSON="$PROJECT_ROOT/bin/extract_json.py"

# Output files
EVAL_TEMP=$(mktemp)
VERDICT_FILE="$SCRIPT_DIR/bad_discovery_test_verdict.json"

# shellcheck disable=SC2317  # cleanup is called by trap
cleanup() {
    rm -f "$EVAL_TEMP"
}
trap cleanup EXIT

echo "=== Testing Evaluator Catches Bad Discovery ==="
echo "Bad report: $BAD_REPORT"
echo ""

# Read the bad discovery report
BAD_REPORT_CONTENT=$(cat "$BAD_REPORT")

# Use the same ground truth as messy_project
GROUND_TRUTH_CONTENT="The project contains:
- loader_v1.py: First attempt, has race condition, BROKEN - should be Discard
- loader_v2.py: Fixed version with locks, WORKS - should be Keep
- loader_v3_experimental.py: Lock-free attempt, INCOMPLETE - should be Evaluate
- benchmark_results.csv: Performance data showing batch_size=32 optimal - should be Keep
- notes.txt: Design decisions about lock vs lock-free approach"

echo "Step 1: Running evaluator on bad report..."

EVAL_PROMPT="You are a test evaluator. Compare a discovery report against known ground truth.

## Ground Truth (what should be found)
$GROUND_TRUTH_CONTENT

## Discovery Report (what was actually produced)
$BAD_REPORT_CONTENT

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

# Extract the verdict
VERDICT=$(echo "$JSON_VERDICT" | python3 -c "import sys,json; print(json.load(sys.stdin).get('verdict','UNKNOWN'))")

echo "=== RESULT ==="
echo "Evaluator verdict: $VERDICT"
echo "Details: $VERDICT_FILE"
echo ""

# This test PASSES if the evaluator returned FAIL (caught the bad report)
if [[ "$VERDICT" == "FAIL" ]]; then
    echo "TEST PASSED: Evaluator correctly identified the bad discovery report"
    echo ""
    echo "Criteria assessment (all should show issues):"
    echo "$JSON_VERDICT" | python3 -c "
import sys, json
data = json.load(sys.stdin)
for k, v in data.get('criteria_met', {}).items():
    status = '✓' if v else '✗'
    print(f'  {status} {k}')
"
    exit 0
else
    echo "TEST FAILED: Evaluator did not catch the bad discovery report"
    echo ""
    echo "This means our test infrastructure is broken - it's not catching errors."
    echo "All other test results are suspect."
    exit 1
fi
