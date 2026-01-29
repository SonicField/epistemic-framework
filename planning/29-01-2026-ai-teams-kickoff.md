# AI Teams - Kickoff Document

## Terminal Goal

Maximise human-specified outcomes per unit of human attention.

The human moves up the abstraction ladder - overseeing a supervisor's approach rather than individual tool calls. Productivity scales with coordinated workers, not human attention bandwidth.

## Architecture

```
Human → Supervisor Claude → Worker Claudes (autonomous, pty-session managed)
                         → Long-running jobs
                         → Fresh contexts, no compaction
```

### Current Model (limitation)
```
Human → Claude → Sub-agents (limited, shared context, compaction issues)
```

### AI Teams Model
```
Human
  ↓ directs
Supervisor (maintains state, delegates, runs /epistemic on workers)
  ↓ manages via pty-session
Workers (fully autonomous Claude instances, fresh contexts)
  ↓ manage
Long-running jobs (builds, tests, monitoring)
```

## Constraints

- **Honest reporting** - No bullshitting the supervisor or human
- **Goal stability** - Drift detected and corrected via /epistemic
- **Transparency on request** - Human can audit any level
- **Fail-safe to human** - Escalate uncertainty, don't guess

## State Management

All state in human-readable markdown. AI is the query engine.

### Structure

```
.epistemic/
├── supervisor.md           # Goals, current focus
├── decisions.log           # Append-only record
└── workers/
    ├── worker-001.md
    ├── worker-002.md
    └── ...
```

### Queries

No database. No JSON. Workers are the query layer.

| Need | Method |
|------|--------|
| "Which workers are blocked?" | Spin up worker to read workers/*.md and report |
| "What decisions since Tuesday?" | Spin up worker to analyse decisions.log |
| "Summarise progress on X" | Spin up worker to synthesise from logs |

Natural language is the query language. Claude is the database engine.

## Instrumental Goals

| Level | Goal | Serves |
|-------|------|--------|
| Supervisor | Decompose human intent into worker tasks | Terminal |
| Supervisor | Maintain coherent state across workers | Terminal |
| Supervisor | Run /epistemic on workers periodically | Goal stability |
| Worker | Complete assigned task with evidence | Supervisor |
| Worker | Report honestly including failures | Honest reporting |
| Worker | Escalate when uncertain | Fail-safe |

## Technology Stack

| Layer | Implementation |
|-------|----------------|
| State | Markdown files |
| Queries | Workers reading markdown |
| Coordination | Supervisor + pty-session |
| Stability | Epistemic framework + /epistemic |
| Long-running jobs | pty-session managed processes |
| Human interface | Supervisor reports up, human directs down |

## Key Enablers

### pty-session

Enables supervisor to:
- Start worker Claude instances
- Send commands and prompts
- Read worker output
- Manage long-running processes

Workers run in fresh contexts - no context window compaction.

### Epistemic Framework

Provides stability mechanism:
- `goals.md` at each level defines that agent's terminal goal
- `/epistemic` is the refresh/verification mechanism
- Falsifiability requirement prevents drift into unfalsifiable territory
- Bullshit detection baked into worker prompts

## Security Model

The security model is trust-based, not technically enforced.

Once the human grants script execution permission:
- Workers have full capability (inherited via pty-session)
- Supervisor controls what human sees
- Audit trail exists but is AI-mediated

The model assumes alignment. The epistemic framework is the alignment mechanism - not cryptographic enforcement, but goal-seeking stability through explicit articulation and periodic verification.

### Permissions

Workers inherit the user's Claude Code permissions from `~/.claude/settings.json`.

**Operational reality**: The permission system is additive. Permissions are granted as workers request them. Over time, the effective permission set converges to "allow all" for active projects.

**Implication**: The permission model provides friction and visibility, not security boundaries. Once AI teams are operational, assume workers have full capability within the user's account permissions.

**Recommendation**: Focus verification effort on output quality (/epistemic), not input restriction (permissions).

### Documentation Required

Before public release, a `security.md` document must be written covering:
- The trust model and its limitations
- What permissions mean in practice
- The role of alignment vs technical enforcement
- User responsibilities and assumptions

This requires careful framing to be honest without being prescriptive about circumvention.

## Next Steps

1. Design supervisor prompt structure
2. Define worker spawn protocol
3. Define worker reporting format
4. Build prototype with 1 supervisor + 2 workers on a test task
5. Iterate on coordination mechanisms

---

*Emerged from discussion, 29-01-2026*
