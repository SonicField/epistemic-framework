# Reference Audit: NBS Migration

## Files to Rename

### Command Files (claude_tools/)
| Current | New |
|---------|-----|
| epistemic.md | nbs.md |
| epistemic-discovery.md | nbs-discovery.md |
| epistemic-discovery-verify.md | nbs-discovery-verify.md |
| epistemic-help.md | nbs-help.md |
| epistemic-investigation.md | nbs-investigation.md |
| epistemic-recovery.md | nbs-recovery.md |
| epistemic-tmux.md | nbs-tmux.md |
| ai-teams-help.md | nbs-teams-help.md |
| ai-teams-supervisor.md | nbs-teams-supervisor.md |
| ai-teams-worker.md | nbs-teams-worker.md |
| start-ai-teams.md | start-nbs-teams.md |

### Binaries (bin/)
| Current | New |
|---------|-----|
| ai-teams-init | nbs-teams-init |

### Templates (templates/)
| Current | New |
|---------|-----|
| ai-teams/ | nbs-teams/ |
| ai-teams/supervisor.md | nbs-teams/supervisor.md |

### Directory Structure
| Current | New |
|---------|-----|
| .epistemic/ | .nbs/ |

## Files Requiring Content Updates

### Documentation (docs/)
- getting-started.md
- overview.md
- interactive-testing.md
- testing-strategy.md
- STYLE.md (no changes needed - no references)

### Root Files
- README.md
- CONTRIBUTING.md
- SECURITY.md
- CODE_OF_CONDUCT.md

### Concepts
- falsifiability.md (1 reference)
- Why-NBS.md (already uses nbs)

### Tests
- tests/README.md
- tests/automated/*.sh (all test files)
- tests/automated/scenarios/no_plan_project/*.md
- tests/manual/*.md

### Examples
- examples/CLAUDE.md

### Planning (historical - may skip updates)
- planning/*.md

## Skill Registration Changes

The Skill tool's available skills list shows:
- epistemic-discovery → nbs-discovery
- epistemic → nbs
- epistemic-tmux → nbs-tmux
- epistemic-recovery → nbs-recovery
- epistemic-help → nbs-help
- epistemic-discovery-verify → nbs-discovery-verify
- epistemic-investigation → nbs-investigation
- start-ai-teams → start-nbs-teams
- ai-teams-help → nbs-teams-help
- ai-teams-worker → nbs-teams-worker
- ai-teams-supervisor → nbs-teams-supervisor

## Cross-Reference Patterns

| Pattern | Replacement |
|---------|-------------|
| epistemic-framework | nbs-framework |
| epistemic framework | NBS framework |
| Epistemic Framework | NBS Framework |
| /epistemic | /nbs |
| .epistemic/ | .nbs/ |
| ai-teams | nbs-teams |
| AI teams | NBS teams |
| AI Teams | NBS Teams |

## Decision: Historical Files

Planning files are historical records. Options:
1. Update them (revisionist)
2. Leave them (confusing)
3. Move to archive/ with note (honest)

Recommendation: Move to planning/archive/ with a note that these predate the rename.
