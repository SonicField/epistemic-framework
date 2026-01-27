# Epistemic Framework - Progress Log

## 27-01-2026 - Initial Development Session

### Terminal Goal
Develop an epistemic framework that improves human-AI collaboration quality at scale, externalising Alex's epistemic standards into transmissible, reusable form.

### What Was Built

**Foundation + Pillars**:
- `concepts/goals.md` - Foundation document (goals are the why; everything else serves them)
- `concepts/falsifiability.md` - Core principle
- `concepts/rhetoric.md` - Ethos/Pathos/Logos + information sourcing
- `concepts/verification-cycle.md` - Design → Plan → Deconstruct → [Test → Code → Document]
- `concepts/zero-code-contract.md` - Engineer specifies, Machinist implements
- `concepts/bullshit-detection.md` - Honest reporting, negative outcome analysis

**Tooling**:
- `/epistemic` command with tiered depth (foundation check first, read pillars when ambiguous)
- `bin/install.sh` - Symlinks tools to ~/.claude/commands/
- `bin/extract_json.py` - JSON extraction utility for tests

**Testing**:
- `tests/automated/test_install.sh` - Verifies symlinks (passes)
- `tests/automated/test_epistemic_command.sh` - AI evaluator checks output against explicit criteria (passes)
- `tests/manual/qa_epistemic.md` - Human QA script

### Key Decisions

1. **Goals as foundation, not pillar** - Everything else exists in service of goals
2. **Hybrid depth for /epistemic** - Check if goals.md read; deepen with pillars only when ambiguous
3. **AI evaluates AI** - Non-deterministic output evaluated by second Claude instance; verdict file is deterministic state of truth
4. **Python in .py files** - Refactored heredoc to separate script (Ethos expectations)

### What Was Learned

- Claude slash commands require restart to pick up new files
- The `/epistemic` command correctly identifies missing documentation and unclear goals
- Voice matching worked well - Falsifiability pillar confirmed as sounding like Alex
- Testing AI output requires semantic evaluation, not string matching

### Pathos Clarified

Alex's motivation:
- Resource maximisation (gigawatts saved via better AI collaboration)
- Gap-filling (AI research ignores millennia of epistemic work)
- Transmissibility (standards that persist and can be shared)

### Outstanding

- GitHub sync (blocked on gh CLI installation)
- `/epistemic-recovery` command for recovering messy projects

### Late Session Update

**Discovery/Recovery Split Implemented**:
- Recognised that recovery has two distinct phases with different purposes
- `/epistemic-discovery` - read-only archaeology phase, produces report
- `/epistemic-recovery` - action phase, step-wise with confirmation at each step
- Intentional pause between phases: "go think about it"

**GitHub Sync Completed**:
- gh CLI installed
- Repo created at github.com/SonicField/epistemic-framework
- User pushed manually (proxy/permissions resolved on their end)
