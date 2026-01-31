#!/bin/bash
# Test: /nbs correctly dispatches to verification after discovery
#
# This test simulates a post-discovery context and verifies that
# /nbs produces verification output (not normal review output).
#
# Falsification: Test fails if output is normal review instead of verification

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
SCENARIO_DIR="$SCRIPT_DIR/scenarios/post_discovery"
DISCOVERY_REPORT="$SCENARIO_DIR/discovery_report.md"
EXTRACT_JSON="$PROJECT_ROOT/bin/extract_json.py"

# Output files
NBS_OUTPUT=$(mktemp)
EVAL_TEMP=$(mktemp)
VERDICT_FILE="$SCRIPT_DIR/verdicts/dispatch_test_verdict.json"

# shellcheck disable=SC2317  # cleanup is called by trap
cleanup() {
    rm -f "$NBS_OUTPUT" "$EVAL_TEMP"
}
trap cleanup EXIT

echo "=== Testing /nbs Dispatch to Verification ==="
echo "Scenario: post_discovery"
echo ""

# Step 1: Run /nbs with simulated post-discovery context
echo "Step 1: Running /nbs with discovery context..."

DISCOVERY_CONTENT=$(cat "$DISCOVERY_REPORT")

# Simulate that discovery was just run by providing context
NBS_PROMPT="Context: I just ran /nbs-discovery on a project and produced this discovery report:

$DISCOVERY_CONTENT

Now run /nbs to review/verify this work."

NBS_RESULT=$(echo "$NBS_PROMPT" | claude -p - --output-format text 2>&1) || true
echo "$NBS_RESULT" > "$NBS_OUTPUT"

echo "NBS command complete. Output saved."
echo ""

# Step 2: Evaluate with AI - check for verification markers
echo "Step 2: Evaluating for dispatch behaviour..."

EVAL_PROMPT="You are a test evaluator. Determine whether the /nbs command correctly dispatched to discovery verification mode.

## Expected Behaviour

When /nbs is run after /nbs-discovery, it should:
1. Recognise discovery was just run
2. Dispatch to verification mode
3. Produce output with verification checklist format (✓/✗ for sections)
4. Check: Terminal Goal, Artefacts Found, Triage, Gap Analysis, etc.

When /nbs runs a NORMAL review, it produces:
- Status section with health assessment
- Issues section with problems found
- Recommendations section (Strategic/Tactical)

## Actual Output
$NBS_RESULT

## Evaluation Criteria

PASS if the output:
- Contains verification checklist format (checking sections, ✓/✗ markers)
- Mentions verifying discovery report completeness
- Checks for confirmed restatements or gap analysis content
- Does NOT look like a normal nbs review

FAIL if the output:
- Looks like a normal review (Status/Issues/Recommendations format)
- Doesn't verify the discovery report sections
- Ignores the discovery context

Respond with ONLY valid JSON:
{
  \"verdict\": \"PASS\" or \"FAIL\",
  \"dispatch_detected\": true/false,
  \"verification_format\": true/false,
  \"checks_sections\": true/false,
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
    echo "TEST PASSED: /nbs correctly dispatched to verification"
    exit 0
else
    echo "TEST FAILED: /nbs did not dispatch correctly"
    echo ""
    echo "Expected: Verification checklist format"
    echo "Got: See output below"
    echo ""
    head -50 "$NBS_OUTPUT"
    exit 1
fi
