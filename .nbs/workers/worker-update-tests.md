# Worker: Update All Tests

## Task

Update all test files to use NBS naming consistently.

## Instructions

1. Read `.nbs/audit.md` to understand the patterns to replace
2. Update all files in `tests/`:
   - `tests/README.md`
   - `tests/automated/*.sh`
   - `tests/automated/scenarios/**/*.md`
   - `tests/manual/*.md`
3. Apply these replacements:
   - `/epistemic` → `/nbs`
   - `epistemic-` commands → `nbs-` commands
   - `.epistemic/` → `.nbs/`
   - `ai-teams` → `nbs-teams`
4. Rename test files that have `epistemic` in the name to use `nbs`

## Success Criteria

Answer these questions with evidence:

1. Do all test scripts invoke `/nbs` not `/epistemic`?
2. Are test file names updated (e.g., `test_nbs_command.sh`)?
3. Do scenario files reference `.nbs/` not `.epistemic/`?
4. Run `grep -r "epistemic" tests/` - are there any unexpected matches?

## Status

State: completed
Started: 2026-01-31T15:05
Completed: 2026-01-31T15:12

## Log

### Files Updated (Content)

**tests/README.md**: Updated all references:
- "Epistemic Framework" → "NBS Framework"
- `/epistemic` → `/nbs`
- `/epistemic-discovery` → `/nbs-discovery`
- Test script names in documentation

**tests/automated/*.sh** (15 files updated):
- `test_nbs_command.sh` (renamed from test_epistemic_command.sh)
- `test_nbs_discovery.sh` (renamed from test_epistemic_discovery.sh)
- `test_nbs_dispatch.sh` (renamed from test_epistemic_dispatch.sh)
- `test_nbs_recovery.sh` (renamed from test_epistemic_recovery.sh)
- `test_dispatch_adversarial.sh`
- `test_investigation_adversarial.sh`
- `test_investigation_adv_no_normal.sh`
- `test_investigation_adv_no_silent.sh`
- `test_investigation_ask.sh`
- `test_investigation_branch.sh`
- `test_investigation_dispatch.sh`
- `test_investigation_file.sh`

**tests/manual/*.md**:
- `qa_nbs.md` (renamed from qa_epistemic.md)
- `qa_discovery.md` (content updated)

**tests/automated/scenarios/no_plan_project/*.md**:
- `README.md` and `TEST_CRITERIA.md` updated

### Files Renamed

| Old Name | New Name |
|----------|----------|
| test_epistemic_command.sh | test_nbs_command.sh |
| test_epistemic_discovery.sh | test_nbs_discovery.sh |
| test_epistemic_dispatch.sh | test_nbs_dispatch.sh |
| test_epistemic_recovery.sh | test_nbs_recovery.sh |
| qa_epistemic.md | qa_nbs.md |
| epistemic_output_*.txt | nbs_output_*.txt |
| epistemic_verdict_*.json | nbs_verdict_*.json |

### Success Criteria Verification

1. **Do all test scripts invoke `/nbs` not `/epistemic`?**
   - YES: `grep -r "epistemic" tests/ --include="*.sh"` returns no matches

2. **Are test file names updated?**
   - YES: Files renamed to `test_nbs_command.sh`, `test_nbs_discovery.sh`, etc.

3. **Do scenario files reference `.nbs/` not `.epistemic/`?**
   - YES: No `.epistemic/` references found in scenario files

4. **Run `grep -r "epistemic" tests/` - are there any unexpected matches?**
   - Only historical test output files (`.txt`, `.json` verdict files) contain "epistemic"
   - These are records of actual AI output from previous test runs
   - NOT updated (would be revisionist to change historical records)
