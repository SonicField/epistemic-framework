#!/bin/bash
# Test: Verify supervisor role correctly understands pty-session availability
#
# 1. Loads supervisor role document and asks AI to spawn a worker
# 2. Evaluator AI checks for hedging phrases about pty-session
# 3. Produces deterministic verdict file
# 4. Exit code based on verdict
#
# Falsification: Test fails if AI hedges about pty-session availability

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
SCENARIO_DIR="$SCRIPT_DIR/scenarios/supervisor_spawn"
SUPERVISOR_DOC="$PROJECT_ROOT/claude_tools/nbs-teams-supervisor.md"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
OUTPUT_FILE="$SCRIPT_DIR/verdicts/supervisor_spawn_output_$TIMESTAMP.txt"
VERDICT_FILE="$SCRIPT_DIR/verdicts/supervisor_spawn_verdict_$TIMESTAMP.json"
EXTRACT_JSON="$PROJECT_ROOT/bin/extract_json.py"

# Colours
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

echo "=== Supervisor pty-session Awareness Test ==="
echo "Timestamp: $TIMESTAMP"
echo ""

# Step 1: Build prompt with supervisor role
echo "Step 1: Running supervisor spawn prompt..."
cd "$SCENARIO_DIR" || exit 1

SUPERVISOR_CONTENT=$(cat "$SUPERVISOR_DOC")

PROMPT="You are acting as an NBS Teams supervisor. Here is your role document:

---
$SUPERVISOR_CONTENT
---

You are in a project directory with a .nbs/ structure already set up.

Your task: Spawn a worker to read the README.md file and summarise it.

Respond with the commands and steps you would take to spawn this worker. Be specific about the pty-session commands you would use."

echo "$PROMPT" | claude -p - --output-format text > "$OUTPUT_FILE" 2>&1 || true

echo "Output captured: $OUTPUT_FILE ($(wc -l < "$OUTPUT_FILE") lines)"
echo ""

# Step 2: Read criteria and evaluate
echo "Step 2: Evaluating output for hedging..."

CRITERIA=$(cat "$SCENARIO_DIR/TEST_CRITERIA.md")
OUTPUT=$(cat "$OUTPUT_FILE")

EVAL_PROMPT="You are a test evaluator. Your job is to determine whether an AI supervisor correctly understood that pty-session is available.

## Test Criteria
$CRITERIA

## AI Output To Evaluate
---
$OUTPUT
---

## Your Task

Check the output for:
1. Does it contain pty-session commands (create, send, read, kill)? This is REQUIRED for PASS.
2. Does it contain ANY hedging about pty-session availability? This is REQUIRED to be FALSE for PASS.

Hedging patterns to look for (case insensitive):
- \"if pty-session is installed\"
- \"check if pty-session\"
- \"ensure pty-session\"
- \"may not be available\"
- \"might not be installed\"
- \"verify pty-session exists\"
- \"confirm pty-session is installed\"
- Any suggestion to check installation before use

Respond with ONLY valid JSON in this exact format:
{
  \"verdict\": \"PASS\" or \"FAIL\",
  \"has_pty_commands\": true/false,
  \"hedging_detected\": true/false,
  \"hedging_phrases_found\": [\"list\", \"of\", \"phrases\"] or [],
  \"reasoning\": \"<one sentence explaining verdict>\"
}"

EVAL_RESULT=$(echo "$EVAL_PROMPT" | claude -p - --output-format text 2>&1)

# Save eval result to temp file for extraction
EVAL_TEMP=$(mktemp)
echo "$EVAL_RESULT" > "$EVAL_TEMP"

# Extract JSON from response
JSON_VERDICT=$("$EXTRACT_JSON" "$EVAL_TEMP")
EXTRACT_STATUS=$?

rm -f "$EVAL_TEMP"

if [[ $EXTRACT_STATUS -ne 0 ]] || [[ -z "$JSON_VERDICT" ]]; then
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
