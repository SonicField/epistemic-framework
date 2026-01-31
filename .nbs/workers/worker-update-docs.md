# Worker: Update All Documentation

## Task

Update all documentation files in `docs/` and root to use NBS naming consistently.

## Instructions

1. Read `.nbs/audit.md` to understand the patterns to replace
2. Update these files:
   - `docs/getting-started.md`
   - `docs/overview.md`
   - `docs/interactive-testing.md`
   - `docs/testing-strategy.md`
   - `README.md`
   - `CONTRIBUTING.md`
   - `SECURITY.md`
   - `CODE_OF_CONDUCT.md`
3. Apply these replacements:
   - `epistemic-framework` → `nbs-framework`
   - `epistemic framework` → `NBS framework`
   - `/epistemic` → `/nbs`
   - `.epistemic/` → `.nbs/`
   - `ai-teams` → `nbs-teams`
   - `AI teams` → `NBS teams`
4. Verify no stale references remain

## Success Criteria

Answer these questions with evidence:

1. Do all docs use `/nbs` not `/epistemic` for commands?
2. Do all docs reference `.nbs/` not `.epistemic/`?
3. Are there any remaining `ai-teams` references?
4. Does README.md accurately describe the framework as NBS?

## Status

State: completed
Started: 2026-01-31
Completed: 2026-01-31

## Log

### Files Updated

1. **docs/getting-started.md** - Updated all command references (`/nbs`, `/nbs-teams-help`, `/nbs-help`, `/nbs-discovery`, `/nbs-recovery`), directory references (`.nbs/`), git clone URL, test script names
2. **docs/overview.md** - Updated all command table entries and dispatch descriptions
3. **docs/interactive-testing.md** - Updated example `/nbs` command references
4. **docs/testing-strategy.md** - Updated framework name in intro and example command
5. **README.md** - Updated title to "NBS Framework", all command links and names, team tool references, file path references
6. **CONTRIBUTING.md** - Updated title and framework name, git clone URL, test script names
7. **SECURITY.md** - Updated framework name and command references
8. **CODE_OF_CONDUCT.md** - Updated framework name reference

### Verification Results

1. **Do all docs use `/nbs` not `/epistemic` for commands?**
   - YES. Verified via grep - no `/epistemic` command references remain in docs/ or root files.

2. **Do all docs reference `.nbs/` not `.epistemic/`?**
   - YES. All directory references updated to `.nbs/`.

3. **Are there any remaining `ai-teams` references?**
   - NO. All replaced with `nbs-teams`.

4. **Does README.md accurately describe the framework as NBS?**
   - YES. Title is "# NBS Framework", all internal references use NBS naming.

### Note on "epistemic" word usage

The word "epistemic" (lowercase, standalone) remains in some documents where it refers to the philosophical concept (relating to knowledge/epistemology), not the framework name. Examples:
- "epistemic discipline" (overview.md)
- "epistemic problems" (overview.md)
- "epistemic standards" (CONTRIBUTING.md)

These are correct English usage and not stale references.
