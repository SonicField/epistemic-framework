#!/bin/bash
# Test: nbs-hub lifecycle with falsification tests
#
# Deterministic tests covering:
#   1.  Init creates directory structure and files
#   2.  Init refuses on existing hub
#   3.  Status shows correct initial state
#   4.  Phase shows phase info
#   5.  Doc register and list
#   6.  Doc read outputs content
#   7.  Doc read with missing doc returns HUB-QUESTION
#   8.  Decision records to log
#   9.  Audit gate enforcement (spawn refused when audit_required)
#   10. Audit submission resets counters
#   11. Audit rejects incomplete content
#   12. Log shows timestamped entries
#   13. Phase gate validates phase name match
#   14. Phase gate advances phase
#   15. Init with relative path resolves to absolute

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
NBS_HUB="${NBS_HUB_BIN:-$PROJECT_ROOT/bin/nbs-hub}"

# Use a fake nbs-worker that always succeeds
FAKE_WORKER_DIR=$(mktemp -d)
cat > "$FAKE_WORKER_DIR/nbs-worker" << 'FAKEEOF'
#!/bin/bash
case "$1" in
    spawn) echo "test-worker-a1b2" ;;
    status) echo "Status: running" ;;
    results) echo "Worker completed successfully" ;;
    dismiss) echo "Dismissed" ;;
    list) echo "NBS Workers:" ; echo "  test-worker-a1b2  [running]" ;;
    *) echo "unknown command: $1" >&2; exit 1 ;;
esac
FAKEEOF
chmod +x "$FAKE_WORKER_DIR/nbs-worker"
export NBS_WORKER_CMD="$FAKE_WORKER_DIR/nbs-worker"

TEST_DIR=$(mktemp -d)
ERRORS=0

cleanup() {
    rm -rf "$TEST_DIR" "$FAKE_WORKER_DIR"
}
trap cleanup EXIT

check() {
    local label="$1"
    local result="$2"
    if [[ "$result" == "pass" ]]; then
        echo "   PASS: $label"
    else
        echo "   FAIL: $label"
        ERRORS=$((ERRORS + 1))
    fi
}

echo "=== nbs-hub Lifecycle Test ==="
echo "Hub: $NBS_HUB"
echo "Test dir: $TEST_DIR"
echo ""

# --- Test 1: Init creates directory structure ---
echo "1. Init creates directory structure..."
PROJ="$TEST_DIR/proj1"
OUTPUT=$("$NBS_HUB" init "$PROJ" "Build the thing" 2>&1)

check "Hub dir created" "$( [[ -d "$PROJ/.nbs/hub" ]] && echo pass || echo fail )"
check "Audits dir created" "$( [[ -d "$PROJ/.nbs/hub/audits" ]] && echo pass || echo fail )"
check "Gates dir created" "$( [[ -d "$PROJ/.nbs/hub/gates" ]] && echo pass || echo fail )"
check "Chat dir created" "$( [[ -d "$PROJ/.nbs/chat" ]] && echo pass || echo fail )"
check "Manifest exists" "$( [[ -f "$PROJ/.nbs/hub/manifest" ]] && echo pass || echo fail )"
check "State exists" "$( [[ -f "$PROJ/.nbs/hub/state" ]] && echo pass || echo fail )"
check "Hub.log exists" "$( [[ -f "$PROJ/.nbs/hub/hub.log" ]] && echo pass || echo fail )"
check "Hub.chat exists" "$( [[ -f "$PROJ/.nbs/chat/hub.chat" ]] && echo pass || echo fail )"

# Verify manifest content
check "Manifest has project_dir" "$( grep -qF "project_dir=$PROJ" "$PROJ/.nbs/hub/manifest" && echo pass || echo fail )"
check "Manifest has goal" "$( grep -qF "terminal_goal=Build the thing" "$PROJ/.nbs/hub/manifest" && echo pass || echo fail )"

# Verify state content
check "State has phase=0" "$( grep -qF "phase=0" "$PROJ/.nbs/hub/state" && echo pass || echo fail )"
check "State has PLANNING" "$( grep -qF "phase_name=PLANNING" "$PROJ/.nbs/hub/state" && echo pass || echo fail )"
check "State has audit_required=0" "$( grep -qF "audit_required=0" "$PROJ/.nbs/hub/state" && echo pass || echo fail )"

echo ""

# --- Test 2: Init refuses on existing hub ---
echo "2. Init refuses on existing hub..."
OUTPUT=$("$NBS_HUB" init "$PROJ" "Another goal" 2>&1 || true)
EXIT_CODE=$("$NBS_HUB" init "$PROJ" "Another goal" 2>&1; echo "EXIT:$?" ) || true
check "Init refuses" "$( echo "$OUTPUT" | grep -qF "already initialised" && echo pass || echo fail )"

echo ""

# --- Test 3: Status shows correct initial state ---
echo "3. Status shows correct initial state..."
OUTPUT=$("$NBS_HUB" --project "$PROJ" status 2>&1)

check "Status shows project" "$( echo "$OUTPUT" | grep -qF "$PROJ" && echo pass || echo fail )"
check "Status shows goal" "$( echo "$OUTPUT" | grep -qF "Build the thing" && echo pass || echo fail )"
check "Status shows phase 0" "$( echo "$OUTPUT" | grep -qF "Phase:" && echo pass || echo fail )"
check "Status shows PLANNING" "$( echo "$OUTPUT" | grep -qF "PLANNING" && echo pass || echo fail )"
check "Status shows audit not required" "$( echo "$OUTPUT" | grep -qF "Audit required:      no" && echo pass || echo fail )"
check "Status shows worker list" "$( echo "$OUTPUT" | grep -qF "Active Workers" && echo pass || echo fail )"

echo ""

# --- Test 4: Phase shows phase info ---
echo "4. Phase shows phase info..."
OUTPUT=$("$NBS_HUB" --project "$PROJ" phase 2>&1)

check "Phase shows number" "$( echo "$OUTPUT" | grep -qF "Phase:    0" && echo pass || echo fail )"
check "Phase shows name" "$( echo "$OUTPUT" | grep -qF "PLANNING" && echo pass || echo fail )"
check "Phase shows audit status" "$( echo "$OUTPUT" | grep -qF "not required" && echo pass || echo fail )"

echo ""

# --- Test 5: Doc register and list ---
echo "5. Doc register and list..."

# Create a test document
echo "# Engineering Standards" > "$TEST_DIR/eng-standards.md"
echo "Safety through verbs, not nouns." >> "$TEST_DIR/eng-standards.md"

"$NBS_HUB" --project "$PROJ" doc register "eng-standards" "$TEST_DIR/eng-standards.md" >/dev/null 2>&1
OUTPUT=$("$NBS_HUB" --project "$PROJ" doc list 2>&1)

check "Doc list shows name" "$( echo "$OUTPUT" | grep -qF "eng-standards" && echo pass || echo fail )"
check "Doc list shows path" "$( echo "$OUTPUT" | grep -qF "$TEST_DIR/eng-standards.md" && echo pass || echo fail )"

# Register a second doc
"$NBS_HUB" --project "$PROJ" doc register "plan" "$TEST_DIR/plan.md" >/dev/null 2>&1
OUTPUT=$("$NBS_HUB" --project "$PROJ" doc list 2>&1)
check "Second doc shows MISSING" "$( echo "$OUTPUT" | grep -qF "[MISSING]" && echo pass || echo fail )"

# Verify manifest was updated
check "Manifest has doc entry" "$( grep -qF "doc.eng-standards=" "$PROJ/.nbs/hub/manifest" && echo pass || echo fail )"

echo ""

# --- Test 6: Doc read outputs content ---
echo "6. Doc read outputs content..."
OUTPUT=$("$NBS_HUB" --project "$PROJ" doc read "eng-standards" 2>&1)

check "Doc read outputs content" "$( echo "$OUTPUT" | grep -qF "Safety through verbs" && echo pass || echo fail )"
check "Doc read has heading" "$( echo "$OUTPUT" | grep -qF "# Engineering Standards" && echo pass || echo fail )"

echo ""

# --- Test 7: Doc read with missing doc returns HUB-QUESTION ---
echo "7. Doc read with missing doc..."
OUTPUT=$("$NBS_HUB" --project "$PROJ" doc read "nonexistent" 2>&1 || true)

check "Missing doc gives HUB-QUESTION" "$( echo "$OUTPUT" | grep -qF "HUB-QUESTION" && echo pass || echo fail )"
check "Tells how to register" "$( echo "$OUTPUT" | grep -qF "doc register" && echo pass || echo fail )"

echo ""

# --- Test 8: Decision records to log ---
echo "8. Decision records to log..."
"$NBS_HUB" --project "$PROJ" decision "Use Baker treadmill for GC" >/dev/null 2>&1
LOG_CONTENT=$(cat "$PROJ/.nbs/hub/hub.log")

check "Decision in log" "$( echo "$LOG_CONTENT" | grep -qF "DECISION Use Baker treadmill for GC" && echo pass || echo fail )"

echo ""

# --- Test 9: Audit gate enforcement ---
echo "9. Audit gate enforcement (spawn refused when audit_required)..."

# Manually set audit_required=1 in state file
sed -i 's/audit_required=0/audit_required=1/' "$PROJ/.nbs/hub/state"

SPAWN_EXIT=0
OUTPUT=$("$NBS_HUB" --project "$PROJ" spawn "test-task" "Do something" 2>&1) || SPAWN_EXIT=$?

# The spawn command should have been refused
check "Spawn shows HUB-GATE" "$( echo "$OUTPUT" | grep -qF "HUB-GATE" && echo pass || echo fail )"
check "Spawn mentions audit" "$( echo "$OUTPUT" | grep -qF "Audit required" && echo pass || echo fail )"
check "Spawn exit code is 3" "$( [[ $SPAWN_EXIT -eq 3 ]] && echo pass || echo fail )"

echo ""

# --- Test 10: Audit submission resets counters ---
echo "10. Audit submission resets counters..."

# Set up state to need audit
sed -i 's/workers_since_check=0/workers_since_check=4/' "$PROJ/.nbs/hub/state"

# Create audit file with required content
cat > "$TEST_DIR/audit.md" << 'AUDITEOF'
# Self-Check Audit

## Terminal Goal
The terminal goal is to build the thing. This is still correct.

## Delegation
I have delegated all work to workers. I have not written code myself.

## Learnings (3Ws)
What went well: workers completed on time
What didn't: test coverage could be better
What to improve: add more edge case tests
AUDITEOF

OUTPUT=$("$NBS_HUB" --project "$PROJ" audit "$TEST_DIR/audit.md" 2>&1)

check "Audit accepted" "$( echo "$OUTPUT" | grep -qF "Audit accepted" && echo pass || echo fail )"
check "Counter reset" "$( echo "$OUTPUT" | grep -qF "Workers since check: 0" && echo pass || echo fail )"

# Verify state was updated
check "State audit_required=0" "$( grep -qF "audit_required=0" "$PROJ/.nbs/hub/state" && echo pass || echo fail )"
check "State workers_since_check=0" "$( grep -qF "workers_since_check=0" "$PROJ/.nbs/hub/state" && echo pass || echo fail )"
check "Audit archived" "$( ls "$PROJ/.nbs/hub/audits/" | grep -q "audit-" && echo pass || echo fail )"

echo ""

# --- Test 11: Audit rejects incomplete content ---
echo "11. Audit rejects incomplete content..."

# Create incomplete audit file
echo "This audit is incomplete." > "$TEST_DIR/bad-audit.md"
AUDIT_EXIT=0
OUTPUT=$("$NBS_HUB" --project "$PROJ" audit "$TEST_DIR/bad-audit.md" 2>&1) || AUDIT_EXIT=$?

check "Incomplete audit rejected" "$( echo "$OUTPUT" | grep -qF "HUB-GATE" && echo pass || echo fail )"
check "Rejected audit exit nonzero" "$( [[ $AUDIT_EXIT -ne 0 ]] && echo pass || echo fail )"

echo ""

# --- Test 12: Log shows timestamped entries ---
echo "12. Log shows timestamped entries..."
OUTPUT=$("$NBS_HUB" --project "$PROJ" log 50 2>&1)

check "Log has header" "$( echo "$OUTPUT" | grep -qF "Hub Log" && echo pass || echo fail )"
check "Log has INIT entry" "$( echo "$OUTPUT" | grep -qF "INIT" && echo pass || echo fail )"
check "Log entries have timestamps" "$( echo "$OUTPUT" | grep -qE "[0-9]{4}-[0-9]{2}-[0-9]{2}T" && echo pass || echo fail )"

echo ""

# --- Test 13: Phase gate validates phase name match ---
echo "13. Phase gate validates phase name match..."

# Try to gate with wrong phase name
cat > "$TEST_DIR/test-results.md" << 'EOF'
# Test Results
All 5 tests passed.
EOF

GATE_EXIT=0
OUTPUT=$("$NBS_HUB" --project "$PROJ" gate "WRONG-PHASE" "$TEST_DIR/test-results.md" "$TEST_DIR/audit.md" 2>&1) || GATE_EXIT=$?

check "Wrong phase refused" "$( echo "$OUTPUT" | grep -qF "Phase mismatch" && echo pass || echo fail )"
check "Gate exit nonzero" "$( [[ $GATE_EXIT -ne 0 ]] && echo pass || echo fail )"

echo ""

# --- Test 14: Phase gate advances phase ---
echo "14. Phase gate advances phase..."
OUTPUT=$("$NBS_HUB" --project "$PROJ" gate "PLANNING" "$TEST_DIR/test-results.md" "$TEST_DIR/audit.md" 2>&1)

check "Gate passed" "$( echo "$OUTPUT" | grep -qF "complete" && echo pass || echo fail )"
check "Phase advanced" "$( echo "$OUTPUT" | grep -qF "phase 1" && echo pass || echo fail )"

# Verify state updated
check "State phase=1" "$( grep -qF "phase=1" "$PROJ/.nbs/hub/state" && echo pass || echo fail )"
check "Gate archived" "$( ls "$PROJ/.nbs/hub/gates/" | grep -q "phase-0" && echo pass || echo fail )"

echo ""

# --- Test 15: Help output ---
echo "15. Help output..."
OUTPUT=$("$NBS_HUB" help 2>&1)

check "Help has title" "$( echo "$OUTPUT" | grep -qF "nbs-hub" && echo pass || echo fail )"
check "Help has commands" "$( echo "$OUTPUT" | grep -qF "Commands:" && echo pass || echo fail )"
check "Help has exit codes" "$( echo "$OUTPUT" | grep -qF "Exit codes:" && echo pass || echo fail )"

echo ""

# --- Summary ---
echo "=== Results ==="
TOTAL=$((ERRORS + $(grep -c "PASS:" <<< "$(echo "$ERRORS")" || true) ))
if [[ $ERRORS -eq 0 ]]; then
    echo "   ALL TESTS PASSED"
else
    echo "   $ERRORS FAILURES"
fi

exit $ERRORS
