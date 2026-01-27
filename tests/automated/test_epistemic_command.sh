#!/bin/bash
# Test: Verify /epistemic command produces expected output on known scenario
#
# Runs /epistemic on a test project with known issues, checks output contains
# expected elements.
#
# Falsification: Exits 0 if output mentions expected issues, non-zero otherwise

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SCENARIO_DIR="$SCRIPT_DIR/scenarios/no_plan_project"
OUTPUT_FILE=$(mktemp)
EVAL_FILE=$(mktemp)

# Colours
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m'

cleanup() {
    rm -f "$OUTPUT_FILE" "$EVAL_FILE"
}
trap cleanup EXIT

echo "Testing /epistemic command on scenario: no_plan_project"
echo "Scenario dir: $SCENARIO_DIR"
echo "Output file: $OUTPUT_FILE"
echo ""

# Run epistemic command in the scenario directory
echo "Running /epistemic command..."
cd "$SCENARIO_DIR"

# Use claude -p to run the command non-interactively
# The --dangerously-skip-permissions flag may be needed for CI
claude -p "/epistemic" --output-format text > "$OUTPUT_FILE" 2>&1

if [[ $? -ne 0 ]]; then
    echo -e "${YELLOW}WARN${NC}: claude command returned non-zero (may be normal)"
fi

echo "Command completed. Output length: $(wc -c < "$OUTPUT_FILE") bytes"
echo ""

# Check for expected patterns in output
FAILED=0

check_pattern() {
    local pattern="$1"
    local description="$2"

    if grep -qi "$pattern" "$OUTPUT_FILE"; then
        echo -e "${GREEN}PASS${NC}: Output mentions $description"
    else
        echo -e "${RED}FAIL${NC}: Output missing $description"
        FAILED=1
    fi
}

echo "Checking for expected issues in output..."
check_pattern "plan" "missing plan"
check_pattern "goal" "goals (terminal or unclear)"
check_pattern "Status\|Issues\|Recommend" "required output sections"

echo ""

# Optional: Have another Claude instance evaluate the output
echo "Running reasonableness check..."

EVAL_PROMPT="You are evaluating the output of an epistemic review tool.

The tool was run on a test project that has these known issues:
1. No plan file
2. No progress log
3. Unclear terminal goal
4. No version control

Here is the tool's output:
---
$(cat "$OUTPUT_FILE")
---

Answer these questions with YES or NO only:
1. Does the output identify the lack of a plan?
2. Does the output mention goals or ask about them?
3. Is the output concise (under 50 lines)?
4. Does it include recommendations?

Then give an overall PASS or FAIL."

echo "$EVAL_PROMPT" | claude -p - --output-format text > "$EVAL_FILE" 2>&1

echo "Evaluation result:"
cat "$EVAL_FILE"
echo ""

# Check if evaluation contains PASS
if grep -qi "overall.*pass\|PASS" "$EVAL_FILE"; then
    echo -e "${GREEN}Reasonableness check: PASS${NC}"
else
    echo -e "${RED}Reasonableness check: FAIL or unclear${NC}"
    FAILED=1
fi

echo ""
if [[ $FAILED -eq 0 ]]; then
    echo -e "${GREEN}All checks passed${NC}"
    exit 0
else
    echo -e "${RED}Some checks failed${NC}"
    echo ""
    echo "Full output was:"
    echo "---"
    cat "$OUTPUT_FILE"
    echo "---"
    exit 1
fi
