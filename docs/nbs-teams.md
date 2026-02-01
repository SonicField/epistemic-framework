# NBS Teams

NBS Teams is a supervisor/worker pattern for multi-agent AI work.

## The Problem

A single Claude session accumulates context. Context accumulation causes drift. You start solving problem A, get distracted by sub-problem B, and emerge hours later having solved neither.

Workers solve this. Each worker gets a fresh context, a specific task, and success criteria. They execute and report. The supervisor maintains the terminal goal across workers.

## The Pattern

```
Supervisor (you or Claude)
    │
    ├── Worker 1: "Implement the parser. Pass all 84 tests."
    │       └── Fresh context, executes, reports findings
    │
    ├── Worker 2: "Update documentation to match new API."
    │       └── Fresh context, executes, reports findings
    │
    └── Worker 3: "Run adversarial tests on edge cases."
            └── Fresh context, executes, reports findings
```

The supervisor:
- Maintains terminal goal clarity
- Decomposes work into worker tasks
- Spawns workers (via pty-session)
- Captures learnings after each worker (3Ws)
- Runs self-check every 3 workers

Workers:
- Read their task file
- Execute with fresh context
- Report findings with evidence
- Escalate blockers (no workarounds)

## Task Scope

**Wrong** (micromanagement):
```
Worker 1: Implement parse_int()
Worker 2: Implement parse_string()
Worker 3: Implement parse_block()
```

**Right** (delegation):
```
Worker: Implement the parser. Pass all 84 tests.
```

If you're writing implementation steps, scope is too narrow. Set the goal, let workers choose the path.

## Directory Structure

```
.nbs/
├── supervisor.md       # Terminal goal, progress, 3Ws log
├── decisions.log       # Append-only decision record
└── workers/
    ├── worker-001.md   # Task files
    └── ...
```

## Commands

| Command | Purpose |
|---------|---------|
| `/start-nbs-teams` | Bootstrap `.nbs/` structure |
| `/nbs-teams-help` | Interactive guidance (for Claude) |
| `/nbs-teams-supervisor` | Supervisor role reference |
| `/nbs-teams-worker` | Worker role reference |

## Quick Start

1. Run `/start-nbs-teams`
2. Answer the terminal goal question
3. Decompose into worker tasks
4. Spawn workers with pty-session
5. Capture 3Ws after each completes

## 3Ws

After every worker:

- **What went well** - Keep doing this
- **What didn't work** - Stop doing this
- **What we can do better** - Change this

After every 3 workers, run self-check:
- Am I still pursuing terminal goal?
- Am I delegating vs doing tactical work myself?
- Have I captured learnings?
- Should I escalate to human?

## When to Use

Use NBS teams when:
- Task requires multiple distinct phases
- Context accumulation is causing drift
- You want fresh perspectives on sub-problems
- Work can be parallelised

Don't use when:
- Single coherent task
- Deep context is the asset (investigation, debugging)
- Task is trivial

## See Also

- [pty-session](pty-session.md) - How to spawn workers
- [Why NBS](../concepts/Why-NBS.md) - The philosophy behind the framework
