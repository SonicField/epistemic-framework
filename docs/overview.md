# Why This Exists

AI systems generate plausible text. Plausibility is not truth.

The gap between "sounds right" and "is right" costs energy - human attention, compute cycles, wasted iterations. When a collaborator (human or AI) bullshits, the debt compounds. Every unchallenged assumption propagates. Every vague goal invites scope creep. Every untested claim becomes technical debt.

This framework externalises the epistemic discipline required to close that gap.

---

## The Problem

Most AI collaboration fails not from lack of capability but from lack of structure:

| Failure Mode | What Happens |
|--------------|--------------|
| **Goal drift** | We optimise for the wrong thing |
| **Confirmation bias** | We accept what sounds right without verification |
| **Authority worship** | We assume existing code/docs are correct |
| **Cherry-picking** | We report successes, bury failures |
| **Vague confidence** | "This should work" without defining what would falsify it |

These are not AI problems. They are epistemic problems. Humans have wrestled with them for millennia. AI research largely ignores this.

---

## The Solution

Three components:

### 1. Foundation and Pillars

Documented principles that can be referenced, not just invoked:

- **Goals** (foundation) - Terminal vs instrumental; what we actually want
- **Falsifiability** - Claims require potential falsifiers
- **Rhetoric** - Ethos, Pathos, Logos applied to technical work
- **Verification Cycle** - Design → Plan → Deconstruct → [Test → Code → Document]
- **Zero-Code Contract** - Clear separation of specification and implementation
- **Bullshit Detection** - Honest reporting; negative outcomes are data

### 2. Commands

Claude Code slash commands that apply these principles:

| Command | Purpose |
|---------|---------|
| `/epistemic` | Review current session for drift, bullshit, blind spots |
| `/epistemic-discovery` | Read-only archaeology of a messy project |
| `/epistemic-recovery` | Step-wise restructuring with confirmation |

### 3. Testing

Tests that evaluate AI output using AI, with human-verifiable verdicts. Non-deterministic output, deterministic judgement.

---

## How It Works

The framework does not enforce rules. It prompts questions.

When you run `/epistemic`, the AI reads the foundation document. If ambiguity arises, it reads the relevant pillar. It then produces a short report surfacing:

- Terminal goal status (clear? drifted? abandoned?)
- Instrumental goal coherence (sequenced or scattered?)
- Documentation state (current or stale?)
- Falsifiability discipline (evidence or assertion?)
- Bullshit detection (all outcomes reported?)

The human reads the report and decides what to do. The AI proposes; the human disposes.

For messy projects, `/epistemic-discovery` maps what exists before any changes. The human provides context the files cannot. The output is a discovery report - a triage table of artefacts with verdicts.

Then `/epistemic-recovery` creates a step-wise plan. Each step is atomic, reversible, described. The human confirms each step before execution. Nothing is deleted without explicit approval.

---

## The Underlying Claim

Better epistemics produce better outcomes.

This is falsifiable:
- If projects using this framework show no improvement in goal alignment, it fails
- If the framework is unused, it fails
- If the pillars don't match the author's actual standards, they're wrong

The framework is a hypothesis. Use it, measure it, revise or discard it.

---

## Who This Is For

Anyone collaborating with AI on technical work who wants:
- Explicit goals instead of assumed understanding
- Verifiable claims instead of plausible assertions
- Honest reporting instead of optimistic summaries
- Structured recovery instead of chaotic archaeology

If you trust vibes, this is not for you. If you want evidence, read on.
