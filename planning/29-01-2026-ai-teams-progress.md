# AI Teams - Progress Log

## 29-01-2026 - Plan Created

### What Was Done

1. Q&A session to understand the "why" of AI teams
2. Identified audience: sceptical programmers who've hit entropy on big projects
3. Identified hooks: simplicity of mechanism + working proof
4. Created implementation plan with prioritised artefacts

### Key Insights from Q&A

**The problem in user words**:
- "Works for small stuff but falls apart on big projects"
- "Gets lost"
- "Does stuff I don't want it to do"
- "Can't trust it"

**Why hierarchy solves it**:
- Supervisor concentrates on goals, delegates tactics to workers
- Detail reduction ~10:1 per layer
- 100k tokens → 10 million effective tokens through vertical layering
- Workers are ephemeral, rarely need compaction

**Audience characteristics**:
- Sceptical of clever-sounding solutions
- Suspicious of complexity
- Want proof, not theory
- Plain language, not academic jargon

**Hooks that might work**:
1. "It's just markdown files and terminal sessions" (simple, elegant)
2. Working vllm installer as proof

### Artefacts Identified

| Priority | Artefact | Status |
|----------|----------|--------|
| P1 | claude_tools/ai-teams-supervisor.md | Not started |
| P1 | claude_tools/ai-teams-worker.md | Not started |
| P2 | bin/ai-teams-init | Not started |
| P2 | templates/supervisor.md | Not started |
| P2 | templates/worker.md | Not started |
| P3 | docs/ai-teams-why.md | Not started |
| P3 | docs/ai-teams.md | Not started |
| P4 | vllm installer complete | In progress (parallel-depickle) |

### Next Steps

1. Draft supervisor prompt (ai-teams-supervisor.md)
2. Draft worker prompt (ai-teams-worker.md)
3. Test on parallel-depickle BUILD_PYTHON phase

---
