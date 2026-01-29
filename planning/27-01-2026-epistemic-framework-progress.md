# Epistemic Framework - Progress Log

## 27-01-2026 - Initial Development Session

### Terminal Goal
Develop an epistemic framework that improves human-AI collaboration quality at scale, externalising Alex's epistemic standards into transmissible, reusable form.

### What Was Built

**Foundation + Pillars**:
- `concepts/goals.md` - Foundation document (goals are the why; everything else serves them)
- `concepts/falsifiability.md` - Core principle
- `concepts/rhetoric.md` - Ethos/Pathos/Logos + information sourcing
- `concepts/verification-cycle.md` - Design → Plan → Deconstruct → [Test → Code → Document]
- `concepts/zero-code-contract.md` - Engineer specifies, Machinist implements
- `concepts/bullshit-detection.md` - Honest reporting, negative outcome analysis

**Tooling**:
- `/epistemic` command with tiered depth (foundation check first, read pillars when ambiguous)
- `bin/install.sh` - Symlinks tools to ~/.claude/commands/
- `bin/extract_json.py` - JSON extraction utility for tests

**Testing**:
- `tests/automated/test_install.sh` - Verifies symlinks (passes)
- `tests/automated/test_epistemic_command.sh` - AI evaluator checks output against explicit criteria (passes)
- `tests/manual/qa_epistemic.md` - Human QA script

### Key Decisions

1. **Goals as foundation, not pillar** - Everything else exists in service of goals
2. **Hybrid depth for /epistemic** - Check if goals.md read; deepen with pillars only when ambiguous
3. **AI evaluates AI** - Non-deterministic output evaluated by second Claude instance; verdict file is deterministic state of truth
4. **Python in .py files** - Refactored heredoc to separate script (Ethos expectations)

### What Was Learned

- Claude slash commands require restart to pick up new files
- The `/epistemic` command correctly identifies missing documentation and unclear goals
- Voice matching worked well - Falsifiability pillar confirmed as sounding like Alex
- Testing AI output requires semantic evaluation, not string matching

### Pathos Clarified

Alex's motivation:
- Resource maximisation (gigawatts saved via better AI collaboration)
- Gap-filling (AI research ignores millennia of epistemic work)
- Transmissibility (standards that persist and can be shared)

### Outstanding

- GitHub sync (blocked on gh CLI installation)
- `/epistemic-recovery` command for recovering messy projects

### Late Session Update

**Discovery/Recovery Split Implemented**:
- Recognised that recovery has two distinct phases with different purposes
- `/epistemic-discovery` - read-only archaeology phase, produces report
- `/epistemic-recovery` - action phase, step-wise with confirmation at each step
- Intentional pause between phases: "go think about it"

**GitHub Sync Completed**:
- gh CLI installed
- Repo created at github.com/SonicField/epistemic-framework
- User pushed manually (proxy/permissions resolved on their end)

### Continued Session

**Testing Expanded**:
- `test_epistemic_discovery.sh` - evaluates discovery reports against ground truth
- `test_epistemic_recovery.sh` - evaluates recovery plan generation
- `test_evaluator_catches_bad.sh` - meta-test verifying evaluator catches errors
- `scenarios/messy_project/` - synthetic scenario with known artefacts
- `scenarios/bad_discovery/` - deliberately bad report for should-fail test
- Shellcheck fixes applied to all scripts

**Documentation Added**:
- LICENSE (MIT)
- CONTRIBUTING.md, CODE_OF_CONDUCT.md, SECURITY.md
- docs/overview.md - why this exists
- docs/getting-started.md - installation and usage
- STYLE.md moved to docs/

**Examples**:
- examples/CLAUDE.md - distilled epistemic programming config, environment-agnostic

**Project Structure Reorganised**:
- planning/ directory for plan and progress files
- README updated with all sections

### What Was Learned

- Shellcheck catches real issues (missing `|| exit` on cd)
- SC2317 false positive for trap cleanup functions - needs disable directive
- Should-fail tests are essential - they validate the test infrastructure itself
- The evaluator correctly catches: missing artefacts, wrong verdicts, missing questions

### Remaining

- vLLM discovery session (real-world validation)

## 28-01-2026 - Framework Iteration from Real-World Test

### What Was Tested

- Fresh Claude Code instance ran `/epistemic-discovery` on vLLM free-threaded Python project
- Produced discovery report and process log
- ~45 minutes, 8 human questions, 6 core deliverables identified

### What Worked

- Phase 1 (establish context) prevented premature searching
- "Do not proceed until you have answers" forced useful human interaction
- Triage table format enabled efficient review
- "Ask constantly" rule caught stale documentation (INDEX.md said BLOCKED but deadlock was fixed)

### What Was Missing

**Gap analysis**: The report identified artefacts but not the path to terminal goal:
- Nothing about porting patches to latest vLLM
- Nothing about test environment structure
- No instrumental goals beyond artefact handling

### Framework Update

Added Phase 4: Gap Analysis to `/epistemic-discovery`:
- Step 1: AI creates gap analysis plan (3-6 questions)
- Step 2: Work through questions ONE AT A TIME (cognitive load consideration)
- Step 3: Synthesise into instrumental goals with dependencies

Key design decision: **Do not batch questions** - humans have limited working memory. One question, one answer, one confirmation, then next.

### Suggested Improvements from Process Log

- Prompt for "has documentation been updated recently?" check
- Staleness detector heuristic for docs with dates
- More structured triage categories (core/supporting/separate/superseded)

### Remaining

- Complete gap analysis on vLLM project (patch into existing session)
- Run /epistemic-recovery when discovery complete

### Dispatch System Added

**Problem identified**: Complex multi-phase instructions in one command cause AI to lose track. User experience confirmed step-by-step Q&A worked well for gap analysis - "flushed out embedded knowledge" that batched questions would have missed.

**Solution**: `/epistemic` becomes a dispatch system:
- Detects context (was discovery or recovery run this session?)
- Dispatches to appropriate verification command
- Falls back to normal review otherwise

**New command**: `/epistemic-discovery-verify`
- Checks all required sections present in discovery report
- Verifies confirmed restatements captured in full (not one-liners)
- Catches context window leakage (discussed but not in report)
- Produces verification checklist

**Architecture**:
```
/epistemic
    │
    ├─► Discovery context → /epistemic-discovery-verify
    ├─► Recovery context → normal review (future: may specialize)
    └─► Neither → normal review
```

**Pathos validation**: Step-by-step Q&A with confirmation worked. The format "So [restatement]. Is that correct?" served the human - surfaced knowledge that a wall of questions would have missed.

### Style Check

All documentation reviewed against STYLE.md:
- British English throughout
- Short sentences, tables where they clarify
- No padding or hedging
- No "axioms" or Americanised Latin

### Dispatch System Tests Added

**Problem identified**: No tests existed for the dispatch system.

**Tests created**:
- `test_epistemic_dispatch.sh` - Verifies `/epistemic` dispatches to verification mode when run after discovery
- `test_dispatch_adversarial.sh` - Verifies `/epistemic` produces normal review when NO discovery context (no false dispatch)

**Test scenario**: `scenarios/post_discovery/` - Contains a valid discovery report for dispatch testing

**Falsification criteria**:
- Dispatch test fails if output is normal review instead of verification checklist
- Adversarial test fails if output incorrectly triggers verification mode

### Verdict Files Reorganised

**Problem**: Verdict JSON files were scattered in `tests/automated/`, mixing generated outputs with source.

**Solution**: Created `tests/automated/verdicts/` subdirectory:
- All test scripts updated to write verdicts there
- `.gitignore` updated to ignore `verdicts/*.json` and `verdicts/*.txt`
- `verdicts/README.md` explains why files are written here and ignored
- `tests/README.md` updated to document the structure

### Tests README Added

Created `tests/README.md` documenting:
- Testing philosophy (AI evaluates AI, verdict as state of truth)
- All automated tests with falsification criteria
- Test scenarios and their purpose
- Manual QA procedures
- How to add new tests

### /epistemic-investigation Command Added

**Origin**: External feedback that read-only discovery can't validate hypotheses about runtime behaviour.

**Design**: Falsification-focused hypothesis testing as a "side quest":
- Creates isolated `investigation/<topic>` branch
- Creates `INVESTIGATION-STATUS.md` as breadcrumb for dispatch
- Step-wise Q&A to clarify hypothesis and design experiments
- Records observations, produces verdict (falsified/failed to falsify/inconclusive)
- Hands back to main context when complete

**Dispatch integration**: `/epistemic` now checks for investigation context first:
- Detects `investigation/*` branch pattern or `INVESTIGATION-STATUS.md` file
- Reviews investigation rigour instead of normal project review

### Security Issue Found and Resolved

**Problem**: vLLM project files (`planning/28-01-2026-vllm-*.md`) were committed to the framework repo and pushed to GitHub. These contained Meta-internal references.

**Solution**:
- Backed up working tree
- Used `git filter-branch` to remove files from entire history
- Force-pushed to GitHub
- Files preserved in backup for vLLM project use

**Lesson**: Project-specific discovery reports should not be committed to the framework repo.

## 28-01-2026 (Continued) - Investigation Dispatch Testing

### Problem Identified

Investigation dispatch tests failing because:
1. AI reasons about context rather than following detection instructions mechanically
2. `allowed-tools` didn't include commands needed for detection (`find`, `test`)
3. Test scenarios didn't provide sufficient context for AI to recognise investigation
4. AI correctly identifies `INVESTIGATION-STATUS.md` but judges it as "test fixture" and proceeds with normal review

### Attempted Fixes

1. **Added explicit detection commands** - Updated `epistemic.md` with `find` and `test -f` commands
   - Result: Failed - commands not in `allowed-tools`

2. **Switched to Glob** - Use Glob tool instead of bash find
   - Result: AI still not following detection steps reliably

3. **Made detection mandatory** - Added "MANDATORY FIRST ACTION - DO NOT SKIP" header
   - Result: AI still reasons about context rather than following mechanically

4. **Added context simulation** - Test provides fake progress log showing active investigation
   - Result: Pending testing

### Key Insight

The AI's behaviour (reason about context, ask when uncertain) may be better than mechanical detection. The test expectation may be wrong, not the AI behaviour.

### Plan Created

See `28-01-2026-investigation-testing-plan.md` for systematic approach:
- Three outcomes: Clearly Investigating, Maybe Investigating, Not Investigating
- Confirmational and adversarial tests for each
- Priority-ordered implementation

### Uncommitted Changes

- `claude_tools/epistemic.md` - Multiple detection logic improvements
- `tests/automated/test_investigation_dispatch.sh` - Added context simulation
- `planning/28-01-2026-investigation-testing-plan.md` - New plan file

### Next Steps

1. Commit current state
2. Implement P0 tests (branch-based dispatch, no markers → normal)
3. Run tests and iterate
4. Push to remote

## 28-01-2026 (Continued) - P0 Tests Passing

### Problem Solved

The key issue was **meta-context pollution**: when running tests in the framework repo itself, the AI being tested could see test infrastructure files, plans about testing dispatch, and other context that caused it to reason about the meta-situation rather than follow detection logic.

### Solution: Isolated Test Repos

Rewrote `test_investigation_branch.sh` to create an isolated temporary git repo:
- Fresh git repo in `/tmp/`
- Minimal project structure with `concepts/goals.md`
- `INVESTIGATION-STATUS.md` with realistic investigation content
- `investigation/*` branch created before test

This removes the meta-context that was confusing the AI.

### P0 Tests Complete

| Test | Result | Notes |
|------|--------|-------|
| `test_investigation_branch.sh` | PASS | Detects branch, produces investigation review |
| `test_investigation_adversarial.sh` | PASS | No markers → normal review |

### Command Updates

Updated `claude_tools/epistemic.md`:
- Added dispatch table at top for clarity
- Made `investigation/*` branch explicitly UNAMBIGUOUS (no confirmation needed)
- Separated branch detection from file detection logic
- Clarified expected output format for investigation review

### Remaining

- P1 tests: File at repo root, File in subdirectory → ask
- P2 tests: Full adversarial matrix
- Commit and push

## 29-01-2026 - PTY Session Tool (Side Quest)

### Origin

While implementing the "ask user" test for investigation dispatch, discovered a fundamental limitation: Claude's Bash tool is one-shot and cannot interact with running processes across multiple tool calls.

### Problem

Testing "ask user and wait for response" behaviour requires:
1. Starting claude in a test scenario
2. Sending a command that triggers AskUserQuestion
3. Detecting the question in output
4. Responding to it
5. Verifying the flow completed correctly

This is impossible with one-shot bash commands.

### Solution: pty-session

Created `bin/pty-session` - a tmux-based session manager that enables persistent interactive sessions:

```bash
pty-session create <name> <command>   # Create session
pty-session send <name> <text>        # Send keystrokes
pty-session read <name>               # Capture screen
pty-session wait <name> <pattern>     # Poll for pattern
pty-session kill <name>               # Terminate
pty-session list                      # Show active
```

### Key Design Decisions

1. **Session isolation**: All sessions prefixed with `pty_` to avoid collision with user's tmux sessions
2. **Exit codes**: Distinct codes for different failures (2=not found, 3=timeout, 4=invalid args)
3. **Wait with timeout**: Polling-based pattern detection with configurable timeout
4. **Self-documenting**: Built-in help extracted from script comments

### Tests Created

| Test | Purpose | Result |
|------|---------|--------|
| `test_pty_session_basic.sh` | Create, send, read, kill cycle | PASS |
| `test_pty_session_wait.sh` | Wait detects patterns | PASS |
| `test_pty_session_timeout.sh` | Wait times out correctly | PASS |
| `test_pty_session_adv_no_collision.sh` | No interference with user sessions | PASS |
| `test_pty_session_adv_invalid.sh` | Graceful error handling | PASS |

### Skill Document

Created `claude_tools/epistemic-tmux.md` documenting:
- When to use interactive sessions
- Command reference
- Usage patterns for REPLs, testing, monitoring
- Important notes on timing and nested tmux

### Discovery: Meta-Context Pollution

During investigation dispatch testing, discovered that the AI reasons about test context when visible. Running tests in the framework repo caused the AI to identify the test scenario as a test rather than following detection logic.

**Solution**: Use isolated temporary repositories for tests to remove meta-context that confuses the AI.

Wrote this up as `~/docs/ai-meta-reasoning-in-testing.md`.

### Exit Condition

Side quest complete. Can now return to investigation dispatch testing with the ability to properly test interactive "ask user" behaviour.

## 29-01-2026 (Continued) - Investigation Dispatch Tests Complete

### P1 Tests Implemented

Used `pty-session` to properly test the interactive "ask user" behaviour.

| Test | Method | Result |
|------|--------|--------|
| `test_investigation_file.sh` | `claude -p` with isolated repo | PASS |
| `test_investigation_ask.sh` | `pty-session` interactive | PASS |

### Key Discovery: AskUserQuestion Rendering

AskUserQuestion in Claude Code renders as a selection UI:
```
☐ Investigation?

I found an INVESTIGATION-STATUS.md file at...

❯ 1. Active investigation
  2. Test fixture / old file
```

Pattern matching needed to detect this format, not just text patterns.

### Test Matrix Complete

| Scenario | Test | Result |
|----------|------|--------|
| Branch `investigation/*` | test_investigation_branch.sh | PASS |
| File at repo root | test_investigation_file.sh | PASS |
| File in subdirectory | test_investigation_ask.sh | PASS |
| No markers | test_investigation_adversarial.sh | PASS |

All P0 and P1 investigation dispatch tests pass.

