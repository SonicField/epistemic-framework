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

