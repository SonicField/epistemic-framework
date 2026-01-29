# AI Teams - Implementation Plan

## Terminal Goal

Enable sceptical programmers who've hit Claude's "falls apart on big projects" problem to use AI teams to tackle work too large for a single Claude instance.

## The Problem (in user's words)

- "Works for small stuff but falls apart on big projects"
- "Gets lost"
- "Does stuff I don't want it to do"
- "Can't trust it"

## Why It Happens

Context window fills with tactical noise. Instrumental goals and moment-to-moment problems overwhelm sight of terminal goals. Even with plans, entropy happens.

## The Solution

Hierarchy with abstraction layers:

| Layer | Role | Context contains |
|-------|------|------------------|
| Supervisor | Maintain goal clarity, orchestrate | High-level state, progress, decisions |
| Workers | Execute tactics | One task, ephemeral, fresh context |

Each layer reduces detail ~10:1. With tree span of 4:
- Supervisor: 100x effective context vs 16 workers
- 100k tokens → 10 million effective tokens through vertical layering

## Technology

Simple and elegant, not complex:
- **State**: Markdown files (human-readable, AI-queryable)
- **Coordination**: pty-session (pseudo-terminal wrapper around tmux)
- **Roles**: Claude tools (markdown prompts defining supervisor/worker behaviour)

No framework to learn. No infrastructure. Just terminal sessions and text files.

## Audience

Sceptical programmer or dev manager who:
- Has used Claude Code
- Hit the entropy problem on larger projects
- Is suspicious of clever-sounding solutions
- Wants proof, not theory

## Hooks

1. **Simplicity**: "It's just markdown files and terminal sessions"
2. **Proof**: Working example (vllm installer building multi-hour Python environment)

---

## Artefacts Required

### Priority 1: Core (makes it work)

| Artefact | Purpose |
|----------|---------|
| `claude_tools/ai-teams-supervisor.md` | Skill/prompt defining supervisor role |
| `claude_tools/ai-teams-worker.md` | Skill/prompt defining worker role |

### Priority 2: Onboarding (makes it usable)

| Artefact | Purpose |
|----------|---------|
| `bin/ai-teams-init` | Creates `.epistemic/` structure in a project |
| `templates/supervisor.md` | Generic supervisor state template |
| `templates/worker.md` | Generic worker task template |

### Priority 3: Documentation (makes it understandable)

| Artefact | Purpose |
|----------|---------|
| `docs/ai-teams-why.md` | Problem, solution, hooks (for sceptical audience) |
| `docs/ai-teams.md` | Getting started guide |

### Priority 4: Proof (makes it credible)

| Artefact | Purpose |
|----------|---------|
| vllm installer complete | Working multi-phase build managed by AI teams |

---

## Implementation Order

1. **Supervisor prompt** - Core behaviour: maintain goals, spawn workers, read status, log decisions
2. **Worker prompt** - Core behaviour: read task, execute, update status, escalate if stuck
3. **Test on parallel-depickle** - Validate prompts work with real build phases
4. **Init script** - Automate `.epistemic/` structure creation
5. **Templates** - Generalise from parallel-depickle specifics
6. **ai-teams-why.md** - Write once we have proof working
7. **ai-teams.md** - Getting started, referencing working example

---

## Verification

### Prompts work
- *Falsification*: Supervisor with prompt can spawn worker, worker completes task, supervisor reads result
- Test: Run BUILD_PYTHON phase via AI teams on parallel-depickle

### Usable by newcomer
- *Falsification*: Someone unfamiliar with the prototype can run `ai-teams-init`, read docs, and start a supervisor
- Test: Fresh Claude instance with no prior context

### Sceptic-friendly docs
- *Falsification*: Read aloud - does it sound practical or academic?
- Test: No jargon (epistemics, terminal goals, falsifiability) in user-facing docs

---

## Constraints

- Plain language in user-facing docs
- Honest about limitations (not push-button, still needs human oversight)
- security.md must be written before public release
- vllm installer is proof, not just test case

---

## Open Questions

1. How deep should role prompts go? Minimal behaviour vs comprehensive guidance?
2. Should `/ai-teams` be a command that starts a supervisor session?
3. How to handle permissions pre-granting for workers?

---

*Plan created 29-01-2026*
