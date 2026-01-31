# Worker: Update Templates and Directory Structure

## Task

Update the templates directory to use NBS naming, renaming `ai-teams/` to `nbs-teams/` and updating all internal references.

## Instructions

1. Rename `templates/ai-teams/` to `templates/nbs-teams/`
2. Read `templates/nbs-teams/supervisor.md` and update all references:
   - `.epistemic/` → `.nbs/`
   - `ai-teams` → `nbs-teams`
   - Any other epistemic → nbs references
3. Check for any other template files that need updating

## Success Criteria

Answer these questions with evidence:

1. Is `templates/nbs-teams/` the only teams template directory?
2. Does `supervisor.md` template use `.nbs/` consistently?
3. Are there any remaining `epistemic` or `ai-teams` references in templates?

## Status

State: completed
Started: 2026-01-31 15:05
Completed: 2026-01-31 15:06

## Log

### Actions Taken

1. **Renamed directory**: `templates/ai-teams/` → `templates/nbs-teams/`
2. **Updated `supervisor.md`**: Changed `.epistemic/decisions.log` → `.nbs/decisions.log`
3. **Updated `decisions.log`**: Changed `AI Teams learning:` → `NBS Teams learning:`
4. **Verified `worker-task.md`**: No changes needed (no epistemic/ai-teams references)

### Success Criteria Evidence

1. **Is `templates/nbs-teams/` the only teams template directory?**
   - YES. Directory listing shows only `nbs-teams/` exists:
   ```
   drwxr-xr-x nbs-teams
   ```

2. **Does `supervisor.md` template use `.nbs/` consistently?**
   - YES. Updated line 19 from `.epistemic/decisions.log` to `.nbs/decisions.log`

3. **Are there any remaining `epistemic` or `ai-teams` references in templates?**
   - NO. Grep search for `epistemic|ai-teams|AI Teams` returned no matches.
