# Progress Log: Install Templating System

**Plan:** `31-01-2026-install-templating-plan.md`
**Status:** COMPLETE

---

## Session: 31-01-2026

### Completed

1. **Audited hardcoded paths** - Found 18 paths across 3 files (nbs.md, nbs-help.md, nbs-tmux.md)

2. **Implemented templating** - `{{NBS_ROOT}}` placeholder, pure bash expansion in install.sh

3. **Created test_install_paths.sh** - Regex verification of no leaked paths

4. **Created test_install_worker.sh** - AI worker verification (belt to regex braces)

5. **Fixed false positive** - Example paths `~/proj/v2/` in nbs-discovery.md triggered stray path detection. Changed to `[project]/v2/` placeholder style.

6. **Production installation** - All tests pass, commands work via `/nbs`

### Learnings

- **Falsification vs confirmation**: Checking that paths are valid is not the same as checking for leaked paths. The latter is the real falsification.
- **Test isolation**: Tests must override `$HOME` to avoid polluting real `~/.claude/commands/`

---

## Session: 01-02-2026

### Completed

1. **HOME validation** - Added error handling to install.sh for empty/invalid HOME with clear error messages

2. **test_home_validation.sh** - 12 adversarial tests for HOME validation

3. **Fixed test isolation** - Updated test_install_paths.sh to override HOME

4. **Updated test_install.sh** - Works with new templated structure (symlinks point to ~/.nbs/commands/, not claude_tools/)

5. **Replaced flaky pty-session test** - Original test_pty_session_basic.sh had timing-based assertions that failed intermittently. Created test_pty_session_lifecycle.sh with evidence-based verification.

6. **Created tests/run_all.sh** - Single entry point for test suite with --quick mode

7. **Removed dead code** - Deleted test_pty_session_basic.sh and overcomplicated test_pty_session_worker.sh

8. **Updated nbs.md** - Added pillar drift prevention:
   - Foundation Check now asks about all pillars, not just goals.md
   - Minimum read set when memory unclear
   - "Signal you need to read" column in Deepen Where Needed table
   - "When in doubt, read. The cost of re-reading is lower than the cost of drift."

### Learnings

- **Flaky tests**: Flakiness comes from forcing deterministic assertions on non-deterministic timing. Fix the assertions, not the timing.
- **AI worker overkill**: The pty-session worker test was overcomplicated - the evidence was deterministic once captured properly. Workers add value for ambiguous evidence, not for timing issues.
- **Pillar drift**: After context compaction, conceptual knowledge of pillars persists but practical application erodes. Re-reading the actual text restores discipline.
- **False confidence**: "I know about falsifiability" is not the same as "I have read falsifiability.md recently." The former leads to claims without falsifiers.

### Outstanding

- nbs.md changes untested in fresh post-compaction session

### Decision Log

- **Minimum read set expanded to all 6 pillars**: Originally had 4, excluded verification-cycle.md and zero-code-contract.md without principled reason. All pillars are ~300 lines total - cost of reading is low, cost of drift is high.
