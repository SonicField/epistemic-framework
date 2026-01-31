# Worker: Fix Command File Content

## Task

Complete the content updates in all claude_tools command files that were missed in the earlier rename.

## Instructions

The files were renamed (`epistemic-*.md` → `nbs-*.md`) but internal references weren't all updated.

Update the following files' CONTENT:
1. `claude_tools/nbs-discovery.md`
2. `claude_tools/nbs-discovery-verify.md`
3. `claude_tools/nbs-recovery.md`
4. `claude_tools/nbs-help.md`
5. `claude_tools/nbs-investigation.md`
6. `claude_tools/nbs-tmux.md`

Apply these replacements:
- `/epistemic-discovery` → `/nbs-discovery`
- `/epistemic-discovery-verify` → `/nbs-discovery-verify`
- `/epistemic-recovery` → `/nbs-recovery`
- `/epistemic-investigation` → `/nbs-investigation`
- `/epistemic-help` → `/nbs-help`
- `/epistemic` (as command) → `/nbs`
- `epistemic-framework` (as path) → `nbs-framework`
- `Epistemic Framework` → `NBS Framework`
- `epistemic framework` → `NBS framework`
- `epistemic discovery` → `NBS discovery`
- `epistemic recovery` → `NBS recovery`

**PRESERVE**: Lowercase "epistemic" when used philosophically (e.g., "epistemically honest", "epistemic claim")

## Success Criteria

1. No `/epistemic*` command references remain in any claude_tools file
2. grep for "epistemic" in claude_tools returns only philosophical uses
3. All command cross-references work (e.g., nbs-discovery.md references /nbs-recovery)

## Status

State: completed
Started: 2026-01-31T10:45:00Z
Completed: 2026-01-31T10:50:00Z

## Log

### Changes Applied

**nbs-discovery.md:**
- Title: `# Epistemic Discovery` → `# NBS Discovery`
- Body: `**epistemic discovery**` → `**NBS discovery**`
- Commands: all `/epistemic-recovery` → `/nbs-recovery` (4 occurrences)
- Preserved: "epistemic structure", "epistemic discipline" (philosophical)

**nbs-discovery-verify.md:**
- Title: `# Epistemic Discovery Verification` → `# NBS Discovery Verification`
- Commands: `/epistemic-discovery` → `/nbs-discovery`
- Commands: `/epistemic-recovery` → `/nbs-recovery`

**nbs-recovery.md:**
- Title: `# Epistemic Recovery` → `# NBS Recovery`
- Body: `**epistemic recovery**` → `**NBS recovery**`
- Commands: all `/epistemic-discovery` → `/nbs-discovery` (2 occurrences)
- Preserved: "epistemic discipline", "epistemic structure" (philosophical)

**nbs-help.md:**
- Title: `# Epistemic Help` → `# NBS Help`
- All "the epistemic framework" → "the NBS framework" (5 occurrences)
- Commands: `/epistemic` → `/nbs` (3 occurrences)
- Commands: `/epistemic-discovery` → `/nbs-discovery` (2 occurrences)
- Commands: `/epistemic-recovery` → `/nbs-recovery`
- Section headers updated

**nbs-investigation.md:**
- Title: `# Epistemic Investigation` → `# NBS Investigation`
- Body: `**epistemic investigation**` → `**NBS investigation**`
- Commands: all `/epistemic` → `/nbs` (2 occurrences)

**nbs-tmux.md:**
- Command in example: `'/epistemic'` → `'/nbs'`
- Path: `epistemic-framework` → `nbs-framework`

### Verification

1. ✓ No `/epistemic*` command references remain (verified via grep)
2. ✓ Remaining "epistemic" uses are philosophical: "epistemic discipline", "epistemic structure", "epistemic review"
3. ✓ Cross-references work: nbs-discovery → /nbs-recovery, nbs-help → /nbs-discovery, etc.
