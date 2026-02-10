# Test Scenario: Supervisor nbs-worker Adoption

## Description

Tests that an AI loading the updated supervisor role document uses `nbs-worker` commands for worker management, not the old `pty-session` spawn pattern.

## Setup

The AI is given the supervisor role document and asked to spawn a worker for a specific task.

## Pass Criteria

For PASS, the output must:
1. Include `nbs-worker spawn` (or close equivalent showing awareness of the tool)
2. NOT contain the old temp.sh + pty-session spawn pattern
3. NOT contain hedging phrases about nbs-worker availability
4. Demonstrate awareness that nbs-worker handles naming, task files, and logging

## Fail Criteria

For FAIL, the output contains ANY of:

### Old pattern markers (should NOT appear):
- "temp.sh" (the old workaround script)
- "pty-session create" followed by "pty-session send" (raw spawn sequence)
- Creating task files manually before spawning (nbs-worker does this automatically)

### Hedging markers (should NOT appear):
- "if nbs-worker is installed"
- "check if nbs-worker"
- "ensure nbs-worker is available"
- "may not be available"
- "might not be installed"
- "verify nbs-worker exists"

## Evaluation

The evaluator should:
1. Check for presence of `nbs-worker` commands (REQUIRED for PASS)
2. Check for absence of old pty-session spawn pattern (REQUIRED for PASS)
3. Check for absence of hedging phrases (REQUIRED for PASS)
4. Return FAIL if old pattern or hedging detected, PASS otherwise
