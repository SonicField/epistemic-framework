# NBS Framework

A framework for honest collaboration between humans and AI systems.

## Documentation

- [Why NBS](docs/Why-NBS.md) - The philosophy: falsifiability over bullshit
- [Overview](docs/overview.md) - Why this exists and how it works
- [Getting Started](docs/getting-started.md) - Installation and first use
- [NBS Teams](docs/nbs-teams.md) - Supervisor/worker patterns for multi-agent work
- [Testing Strategy](docs/testing-strategy.md) - AI-evaluates-AI testing approach
- [Interactive Testing](docs/interactive-testing.md) - Multi-turn testing with pty-session
- [pty-session Reference](docs/pty-session.md) - Terminal session manager for automation
- [Style Guide](docs/STYLE.md) - Internal reference for AI writing these materials

## Examples

- [CLAUDE.md](examples/CLAUDE.md) - Example project configuration for NBS programming

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

- [/nbs](claude_tools/nbs.md) - Review and dispatch

Run this after any session. It detects context and dispatches:
- In `investigation/*` branch → reviews investigation rigour
- After `/nbs-discovery` → verifies the discovery report is complete
- After `/nbs-recovery` → reviews the recovery work
- Otherwise → general NBS review

### NBS Teams Tools

Supervisor/worker patterns for multi-agent AI work. See [NBS Teams](docs/nbs-teams.md) for the full overview.

Commands for setting up and using NBS teams:

- [/start-nbs-teams](claude_tools/start-nbs-teams.md) - Bootstrap project with `.nbs/` structure (one command setup)
- [/nbs-teams-help](claude_tools/nbs-teams-help.md) - Interactive guidance for NBS teams usage
- [/nbs-help](claude_tools/nbs-help.md) - Interactive guidance for the NBS framework

For AI-as-supervisor or AI-as-worker roles:
- [NBS Teams Supervisor](claude_tools/nbs-teams-supervisor.md) - Role and responsibilities for supervisor
- [NBS Teams Worker](claude_tools/nbs-teams-worker.md) - Role and responsibilities for worker

### Workflow Commands

- [/nbs-discovery](claude_tools/nbs-discovery.md) - Read-only archaeology for messy projects
- [/nbs-recovery](claude_tools/nbs-recovery.md) - Step-wise restructuring with confirmation

### Side Quest Commands

- [/nbs-investigation](claude_tools/nbs-investigation.md) - Hypothesis testing through experiment (isolated side branch)

Run this when you want to test a hypothesis before committing to a direction. Creates an isolated investigation branch, designs falsifiable experiments, and produces a verdict (falsified / failed to falsify / inconclusive).

### Verification Commands

- [/nbs-discovery-verify](claude_tools/nbs-discovery-verify.md) - Verify discovery report completeness (auto-dispatched by /nbs)

## Testing

The framework includes automated tests using a novel AI-evaluates-AI approach.

- [Testing Strategy](docs/testing-strategy.md) - Philosophy, adversarial testing, test isolation
- [Interactive Testing](docs/interactive-testing.md) - Using pty-session for multi-turn tests
- [pty-session Reference](docs/pty-session.md) - Interactive terminal session manager

See [tests/README.md](tests/README.md) for running tests.

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
