# Plan: Supervisor pty-session Awareness Fix

## Terminal Goal

An AI loading `nbs-teams-supervisor.md` knows pty-session is available and uses it without hedging.

## Problem Statement

The supervisor document references pty-session for spawning workers but didn't clarify:
1. That `/nbs-tmux` skill exists for reference
2. That pty-session is guaranteed installed (framework installation includes it)

This caused AIs to sometimes wrongly assume pty-session might not be available.

## Change Already Made

Added "Available Tools" section to `claude_tools/nbs-teams-supervisor.md`:
- References `/nbs-tmux` skill
- States pty-session is guaranteed available
- Cross-references in responsibilities list

## Acceptance Criteria

| Criterion | Pass | Fail |
|-----------|------|------|
| AI uses pty-session commands directly | Proceeds with commands | Hedges about availability |
| AI does NOT check if pty-session exists | No verification step | Suggests checking installation |
| AI knows `/nbs-tmux` is available | References it or uses confidently | Claims no reference available |

## Test Design

**Scenario**: Minimal `.nbs/` structure with supervisor role loaded

**Prompt**: "Spawn a worker to read the README and summarise it"

**Evaluator criteria**:
1. Response contains pty-session commands (create, send, read, kill)
2. Response does NOT contain hedging phrases: "if installed", "check if", "may not be available", "ensure pty-session"
3. Response proceeds confidently without caveats about availability

**Falsification**: Test FAILS if evaluator detects hedging. Test PASSES if AI proceeds confidently.

## Tasks

1. [x] Edit `nbs-teams-supervisor.md` to add Available Tools section
2. [x] Create test scenario: `tests/automated/scenarios/supervisor_spawn/`
3. [x] Create test script: `tests/automated/test_supervisor_pty_awareness.sh`
4. [x] Run test and verify PASS
5. [x] Update `tests/README.md` with new test

## Notes

- AI behaviour is probabilistic; single pass doesn't prove fix works
- Consider multiple runs or adversarial prompting in future
- Current test proves non-hedging in at least one case
