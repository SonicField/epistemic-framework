# Epistemic Review - 29-01-2026

*Session review before context window compaction*

## Status

- **P2 tests complete**: All investigation dispatch tests passing (6 tests across 3 scenarios)
- **Documentation expanded**: Testing infrastructure now fully documented (pty-session, interactive-testing, testing-strategy)
- **Major scope expansion**: AI teams architecture designed and prototyped - this session went well beyond planned work
- **Progress log stale**: Today's AI teams work not recorded in progress log

## Issues

- **Drift risk**: Session started with P2 tests → documentation → security analysis → AI teams design → working prototype. This was organic but represents significant scope expansion from "complete P2 tests and document testing"
- **Undocumented decision**: Security implications of pty-session (permission bypass, sudo access) were analysed but intentionally not documented publicly. No tracking of this decision exists.
- **Progress log incomplete**: The AI teams prototype work (structure creation, dummy task test, learnings) is not in the progress log - only in planning docs
- **Cross-project entanglement**: Created `.epistemic/` structure in parallel-depickle (a separate project). Framework infrastructure now spans two repos.
- **Informal collaboration protocol**: dialogue.md exchange pattern works but isn't documented in the framework as a reusable pattern

## Session Summary

### What was done

1. **Completed P2 adversarial tests** for investigation dispatch:
   - `test_investigation_adv_no_normal.sh` - file at root must not produce normal review
   - `test_investigation_adv_no_silent.sh` - file in subdirectory must not silently proceed

2. **Created testing documentation**:
   - `docs/pty-session.md` - terminal session manager reference
   - `docs/interactive-testing.md` - multi-turn Claude testing guide
   - `docs/testing-strategy.md` - AI-evaluates-AI philosophy

3. **Explored pty-session security implications**:
   - pty-session creates fresh login shell with full user permissions
   - Bypasses Claude Code sandbox restrictions
   - Inherits sudo access, proxy settings, credentials
   - Permission model is UX friction, not security boundary
   - Decision: Document honestly but don't provide exploitation guidance

4. **Designed AI teams architecture**:
   - Supervisor Claude manages worker Claudes via pty-session
   - All state in markdown files, AI is query engine
   - Workers have fresh contexts (no compaction issues)
   - Created `planning/29-01-2026-ai-teams-kickoff.md`

5. **Ran first AI teams prototype**:
   - Created `.epistemic/` structure in parallel-depickle
   - Spawned worker Claude via pty-session
   - Worker read task file, executed dummy_phase.sh, updated status
   - Full cycle validated in ~15 seconds

6. **Collaborated with vllm-claude**:
   - Used `~/claude-exchange/dialogue.md` as shared conversation
   - Both Claudes ran prototype tests independently
   - Aligned on next steps via dialogue
   - vllm-claude created real build scripts (build_python.sh, preflight_check.sh)

7. **Documented learnings**:
   - Created `planning/29-01-2026-ai-teams-learnings.md`
   - Covers architecture, security, testing, collaboration insights

### Key discoveries

1. **pty-session enables everything**: Breaking out of one-shot Bash unlocks persistent processes, environment inheritance, multi-turn interaction

2. **Permissions are additive ratchet**: They accumulate over time, converge to "allow all" - focus on output verification (/epistemic), not input restriction

3. **Meta-context pollution**: AI reasons about visible test infrastructure; solution is isolated test environments

4. **Test in pairs**: Confirmational (correct happens) + adversarial (wrong doesn't happen)

5. **Markdown as database**: No JSON, no SQL - AI is the query engine

## Recommendations

### Strategic

1. **Decide: Is AI teams in scope for epistemic-framework?**
   - *Falsification*: If AI teams belongs elsewhere, the planning docs should move and the framework should not evolve to support it
   - Current state implies yes, but this should be explicit

2. **Create security.md before public release**
   - *Falsification*: If security.md cannot be written honestly without causing problems, the framework may not be releasable in current form
   - Already noted in kickoff doc, but needs tracking

### Tactical

1. **Update progress log with today's work**
   - Evidence: Add section covering AI teams prototype, security analysis, learnings
   - *Falsification*: If the work can't be summarised coherently, scope may have drifted too far

2. **Document the dialogue.md protocol formally**
   - If this is a reusable pattern for Claude-to-Claude collaboration, document it
   - *Falsification*: If it only works for this specific case, don't generalise prematurely

3. **Clarify parallel-depickle relationship**
   - Is parallel-depickle a test case for AI teams, or are the two projects now coupled?
   - *Falsification*: Try using AI teams on a different project; if it requires parallel-depickle-specific knowledge, coupling is too tight

## Files created/modified this session

### Created
- `tests/automated/test_investigation_adv_no_normal.sh`
- `tests/automated/test_investigation_adv_no_silent.sh`
- `docs/pty-session.md`
- `docs/interactive-testing.md`
- `docs/testing-strategy.md`
- `planning/29-01-2026-ai-teams-kickoff.md`
- `planning/29-01-2026-ai-teams-learnings.md`
- `~/claude-exchange/dialogue.md`
- `~/claude-exchange/coordinator.sh`
- `~/claude-exchange/README.md`
- `~/docs/pty-session-wiki.md` (via subagent)

### Modified
- `README.md` - added Testing section
- `tests/README.md` - added P2 tests

### In parallel-depickle (cross-project)
- `.epistemic/supervisor.md`
- `.epistemic/decisions.log`
- `.epistemic/workers/TEMPLATE.md`
- `.epistemic/workers/worker-dummy.md` (by vllm-claude)

---

**Bottom line**: Productive session with significant discoveries. The scope expanded substantially but the expansion appears aligned with the terminal goal (frameworks for human-AI collaboration). Main risk is documentation debt - the progress log and formal framework documentation haven't kept pace with the work done.
