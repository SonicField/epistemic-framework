# Epistemic Framework

A framework for honest collaboration between humans and AI systems.

## Documentation

- [Overview](docs/overview.md) - Why this exists and how it works
- [Getting Started](docs/getting-started.md) - Installation and first use
- [Style Guide](docs/STYLE.md) - Internal reference for AI writing these materials (not for human contributors)

## Examples

- [CLAUDE.md](examples/CLAUDE.md) - Example project configuration for epistemic programming

## Foundation

- [Goals](concepts/goals.md) - The why. Everything else exists in service of this.

## Pillars

Built on the foundation:

- [Falsifiability](concepts/falsifiability.md) - Claims require potential falsifiers
- [Rhetoric](concepts/rhetoric.md) - Ethos, Pathos, Logos and knowing when to ask
- [Verification Cycle](concepts/verification-cycle.md) - Design → Plan → Deconstruct → [Test → Code → Document]
- [Zero-Code Contract](concepts/zero-code-contract.md) - Engineer specifies, Machinist implements with evidence
- [Bullshit Detection](concepts/bullshit-detection.md) - Honest reporting, negative outcome analysis

## Tools

### Core Command

- [/epistemic](claude_tools/epistemic.md) - Review and dispatch

Run this after any session. It detects context and dispatches:
- In `investigation/*` branch → reviews investigation rigour
- After `/epistemic-discovery` → verifies the discovery report is complete
- After `/epistemic-recovery` → reviews the recovery work
- Otherwise → general epistemic review

### Workflow Commands

- [/epistemic-discovery](claude_tools/epistemic-discovery.md) - Read-only archaeology for messy projects
- [/epistemic-recovery](claude_tools/epistemic-recovery.md) - Step-wise restructuring with confirmation

### Side Quest Commands

- [/epistemic-investigation](claude_tools/epistemic-investigation.md) - Hypothesis testing through experiment (isolated side branch)

Run this when you want to test a hypothesis before committing to a direction. Creates an isolated investigation branch, designs falsifiable experiments, and produces a verdict (falsified / failed to falsify / inconclusive).

### Verification Commands

- [/epistemic-discovery-verify](claude_tools/epistemic-discovery-verify.md) - Verify discovery report completeness (auto-dispatched by /epistemic)

## Planning

Project plans and progress logs live in `planning/`:
- `<date>-<project>-plan.md` - Terminal goal, completed/outstanding items, decisions
- `<date>-<project>-progress.md` - Session-by-session record of what was done

## Installation

```bash
./bin/install.sh
```

This creates symlinks in `~/.claude/commands/` for all Claude Code tools.

## Author

Dr Alex Turner

## Licence

[MIT](LICENSE)
