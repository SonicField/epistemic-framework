# Epistemic Framework

A framework for honest collaboration between humans and AI systems.

## Documentation

- [Overview](docs/overview.md) - Why this exists and how it works
- [Getting Started](docs/getting-started.md) - Installation and first use

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

- [/epistemic](claude_tools/epistemic.md) - Systematic review of reasoning quality and goal alignment
- [/epistemic-discovery](claude_tools/epistemic-discovery.md) - Read-only archaeology for messy projects
- [/epistemic-recovery](claude_tools/epistemic-recovery.md) - Step-wise restructuring with confirmation

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
