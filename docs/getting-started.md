# Getting Started

## Installation

```bash
git clone https://github.com/SonicField/epistemic-framework.git
cd epistemic-framework
./bin/install.sh
```

This creates symlinks in `~/.claude/commands/` for all commands. Restart Claude Code to pick them up.

## First Use

### During a work session

When you want a check on your current work:

```
/epistemic
```

The command reviews the conversation history and any project files, then produces a short report. Read it in under two minutes. Act on it or not.

### When facing a messy project

When you have scattered artefacts, unclear goals, and no documentation:

```
/epistemic-discovery
```

The AI asks you questions. Answer them. You know things the files cannot tell.

At the end, you get a discovery report: what exists, what it's for, what to keep or discard. Review it. Then:

```
/epistemic-recovery
```

The AI creates a plan. Each step is described, justified, reversible. You confirm each step before it executes. Nothing changes without your approval.

---

## The Documents

Read these when you want to understand the principles:

| Document | What It Contains |
|----------|-----------------|
| `concepts/goals.md` | Foundation - terminal vs instrumental goals |
| `concepts/falsifiability.md` | What makes a claim testable |
| `concepts/rhetoric.md` | Ethos, Pathos, Logos applied to technical work |
| `concepts/verification-cycle.md` | Design → Plan → Deconstruct → [Test → Code → Document] |
| `concepts/zero-code-contract.md` | Engineer specifies, Machinist implements |
| `concepts/bullshit-detection.md` | Honest reporting and negative outcome analysis |

The AI reads these when it needs them. You read them when you want to.

---

## Testing

Verify the installation:

```bash
./tests/automated/test_install.sh
```

Run all tests:

```bash
./tests/automated/test_epistemic_command.sh
./tests/automated/test_epistemic_discovery.sh
./tests/automated/test_epistemic_recovery.sh
```

Tests use AI to evaluate AI. The verdict files contain deterministic pass/fail judgements.

---

## What to Expect

The framework does not guarantee good outcomes. It prompts good questions.

You will be asked:
- What is the terminal goal?
- What would falsify this approach?
- Is this result being reported honestly?
- What does this failure reveal?

The discomfort is the point. Comfortable collaboration produces comfortable bullshit.
