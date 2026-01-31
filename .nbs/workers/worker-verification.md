# Worker: Final Verification

## Task

Verify the NBS migration is complete and consistent.

## Instructions

1. Search entire repo for remaining `epistemic` references:
   ```bash
   grep -r "epistemic" --include="*.md" --include="*.sh" . | grep -v ".nbs/" | grep -v "archive/"
   ```
2. Search for remaining `ai-teams` references:
   ```bash
   grep -r "ai-teams" --include="*.md" --include="*.sh" .
   ```
3. Verify command file names in `claude_tools/`:
   - All should be `nbs-*.md` or `start-nbs-teams.md`
   - No `epistemic-*.md` or `ai-teams-*.md` files
4. Verify `.nbs/` structure is correct
5. Check `bin/install.sh` handles new file names

## Success Criteria

Answer these questions with evidence:

1. Are there any unexpected `epistemic` references outside archive?
2. Are there any `ai-teams` references anywhere?
3. Do all command files have correct NBS names?
4. Does `bin/install.sh` need updates for new names?

## Status

State: completed
Started: 2026-01-31T15:15:00Z
Completed: 2026-01-31T15:18:00Z

## Log

### Verification Results

#### 1. Are there any unexpected `epistemic` references outside archive?

**YES - CRITICAL ISSUE FOUND**

The claude_tools files were renamed (`epistemic-*.md` → `nbs-*.md`) but their **content** still references old command names:

| File | Problem |
|------|---------|
| `claude_tools/nbs-tmux.md` | References `/epistemic` command (line 89), `epistemic-framework` path (line 163) |
| `claude_tools/nbs-discovery.md` | References `/epistemic-recovery` throughout, "epistemic discovery" terminology |
| `claude_tools/nbs-discovery-verify.md` | References `/epistemic-discovery`, `/epistemic-recovery` |
| `claude_tools/nbs-recovery.md` | References `/epistemic-discovery` |
| `claude_tools/nbs-help.md` | References `/epistemic`, `/epistemic-discovery`, `/epistemic-recovery` throughout |
| `claude_tools/nbs-investigation.md` | References `/epistemic` command |

**Expected references (philosophical use of "epistemic"):**
- `CONTRIBUTING.md` - "epistemic standards", "epistemic principle" (philosophy)
- `examples/CLAUDE.md` - "epistemic discipline" (philosophy)
- `docs/overview.md` - "epistemic discipline", "epistemic problems" (philosophy)
- `concepts/falsifiability.md` - "epistemically honest" (philosophy)
- `concepts/Why-NBS.md` - "epistemic claim", "epistemic method" (philosophy)

**Other:**
- `temp.sh` - Contains old path references (should be deleted or updated)

#### 2. Are there any `ai-teams` references anywhere?

**NO unexpected references.**

All `ai-teams` references are in:
- `.nbs/` - Worker task descriptions and audit trail (expected)
- `planning/archive/` - Historical planning documents (expected)

#### 3. Do all command files have correct NBS names?

**YES.** All files in `claude_tools/`:
- `nbs.md`
- `nbs-discovery.md`
- `nbs-discovery-verify.md`
- `nbs-help.md`
- `nbs-investigation.md`
- `nbs-recovery.md`
- `nbs-tmux.md`
- `nbs-teams-help.md`
- `nbs-teams-supervisor.md`
- `nbs-teams-worker.md`
- `start-nbs-teams.md`

No `epistemic-*.md` or `ai-teams-*.md` files present.

#### 4. Does `bin/install.sh` need updates for new names?

**NO.** The script uses a generic loop:
```bash
for tool in "$PROJECT_ROOT/claude_tools"/*.md; do
```
This automatically picks up all renamed files. The comment "Epistemic Framework" on line 2 is cosmetic only.

### Summary

| Check | Result |
|-------|--------|
| Unexpected `epistemic` outside archive | **FAIL** - claude_tools content not updated |
| `ai-teams` references | PASS |
| Command file names correct | PASS |
| `bin/install.sh` works | PASS |

### Required Action

A new worker task is needed to update the content of all `claude_tools/nbs-*.md` files:
- `/epistemic` → `/nbs`
- `/epistemic-discovery` → `/nbs-discovery`
- `/epistemic-discovery-verify` → `/nbs-discovery-verify`
- `/epistemic-recovery` → `/nbs-recovery`
- `/epistemic-investigation` → `/nbs-investigation`
- `/epistemic-help` → `/nbs-help`
- "epistemic framework" (as framework name) → "NBS framework"

Note: Keep philosophical uses of "epistemic" (lowercase, referring to knowledge/epistemology).
