#!/bin/bash
# Test: Verify /epistemic command via AI evaluation
#
# 1. Runs /epistemic on test scenario
# 2. Evaluator AI judges output against explicit criteria
# 3. Produces deterministic verdict file (state of truth)
# 4. Exit code based on verdict
#
# Falsification: Test fails if evaluator determines output missed known issues

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCENARIO_DIR="$SCRIPT_DIR/scenarios/no_plan_project"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_FILE="$SCENARIO_DIR/test_output_$TIMESTAMP.txt"
VERDICT_FILE="$SCENARIO_DIR/test_verdict_$TIMESTAMP.json"

# Colours
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

echo "=== Epistemic Command Test ==="
echo "Scenario: no_plan_project"
echo "Timestamp: $TIMESTAMP"
echo ""

# Step 1: Run /epistemic in scenario directory
echo "Step 1: Running /epistemic command..."
cd "$SCENARIO_DIR"

claude -p "/epistemic" --output-format text > "$OUTPUT_FILE" 2>&1 || true

echo "Output captured: $OUTPUT_FILE ($(wc -l < "$OUTPUT_FILE") lines)"
echo ""

# Step 2: Read criteria and evaluate
echo "Step 2: Evaluating output against criteria..."

CRITERIA=$(cat "$SCENARIO_DIR/TEST_CRITERIA.md")
OUTPUT=$(cat "$OUTPUT_FILE")

EVAL_PROMPT="You are a test evaluator. Your job is to determine whether an epistemic review tool produced correct output for a known test scenario.

## Test Criteria
$CRITERIA

## Tool Output To Evaluate
---
$OUTPUT
---

## Your Task

Evaluate the output and produce a JSON verdict. Be strict but fair.

Respond with ONLY valid JSON in this exact format:
{
  \"verdict\": \"PASS\" or \"FAIL\",
  \"identified_issues\": {
    \"no_plan\": true/false,
    \"no_progress_log\": true/false,
    \"unclear_goals\": true/false,
    \"has_recommendations\": true/false
  },
  \"issues_found\": <number 0-4>,
  \"is_concise\": true/false,
  \"has_structure\": true/false,
  \"hallucinations\": true/false,
  \"reasoning\": \"<one sentence explaining verdict>\"
}"

EVAL_RESULT=$(echo "$EVAL_PROMPT" | claude -p - --output-format text 2>&1)

# Extract JSON from response (handle markdown code blocks)
JSON_VERDICT=$(echo "$EVAL_RESULT" | grep -Pzo '\{[\s\S]*\}' | tr -d '\0' | head -1)

if [[ -z "$JSON_VERDICT" ]]; then
    echo -e "${RED}ERROR${NC}: Could not extract JSON from evaluator response"
    echo "Raw response:"
    echo "$EVAL_RESULT"
    exit 2
fi

# Write verdict file (state of truth)
echo "$JSON_VERDICT" > "$VERDICT_FILE"
echo "Verdict written: $VERDICT_FILE"
echo ""

# Step 3: Parse verdict and report
echo "Step 3: Verdict"
echo "---"
echo "$JSON_VERDICT" | python3 -m json.tool 2>/dev/null || echo "$JSON_VERDICT"
echo "---"
echo ""

# Extract pass/fail
if echo "$JSON_VERDICT" | grep -q '"verdict".*"PASS"'; then
    echo -e "${GREEN}TEST PASSED${NC}"
    exit 0
else
    echo -e "${RED}TEST FAILED${NC}"
    echo ""
    echo "Output was:"
    head -50 "$OUTPUT_FILE"
    exit 1
fi
