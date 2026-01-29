# AI Teams - Learnings from First Prototype

*29-01-2026*

## Architecture

### pty-session is the key enabler

Breaking out of one-shot Bash execution unlocks:
- Persistent process management (builds that run for hours)
- Environment inheritance (proxy, PATH, credentials)
- Multi-turn interaction (permission prompts, interactive tools)

Without pty-session, workers cannot manage long-running jobs or inherit user context.

### Markdown is the database

All state lives in human-readable markdown files. No JSON, no SQL.

AI is the query engine. "Which workers are blocked?" is not a database query - it's a task for a worker that reads `workers/*.md` and reports.

This works because:
- AI can read and synthesise unstructured text
- Humans can audit state by reading files directly
- No impedance mismatch between storage and understanding

### Workers inherit everything

Via pty-session, workers run in a fresh login shell with full user environment:
- Proxy settings (HTTP_PROXY, HTTPS_PROXY)
- PATH and installed tools
- Git credentials
- sudo access (if user has it)

This is powerful - build scripts don't need to hardcode environment. But it has security implications (see below).

### Coordination via files works

Two Claudes collaborating through a shared text file (`dialogue.md`) is functional:
- Protocol: append with `---` separator, timestamp, identity
- No real-time sync needed - each reads before writing
- Human can observe the conversation

It's clunky but sufficient for prototyping. A coordinator script could automate turn-taking.

---

## Security and Permissions

### The permission model is UX, not security

Claude Code's permission prompts provide:
- Friction - slows down and makes actions visible
- Accident prevention - catches typos like `rm -rf /`
- Audit trail of requested permissions

They do NOT provide:
- Security against a misaligned AI
- Protection once script execution is granted
- Tamper-proof logging (AI can delete logs)

### Permissions are additive

The permission system is a ratchet:
- Permissions are granted as workers request them
- They accumulate over time, never retract
- Active projects converge to "allow all"

Implication: Focus verification on output quality (/epistemic), not input restriction (permissions).

### Trust is transitive

Trusting Claude to run scripts means trusting Claude with:
- Everything your user account can do
- sudo access (if you have it)
- Network access
- The ability to spawn more Claudes

The security model assumes alignment. The epistemic framework is the alignment mechanism.

### Audit trails are AI-mediated

The AI controls:
- What appears in responses to the user
- Sub-agent output (not shown by default)
- Background process visibility

The oversight model depends on AI honesty, not technical enforcement.

---

## Testing AI Systems

### Test in pairs: confirmational + adversarial

| Type | Purpose |
|------|---------|
| Confirmational | Verify correct behaviour occurs |
| Adversarial | Verify wrong behaviour does NOT occur |

Both are needed. A system can pass all confirmational tests while still exhibiting dangerous failure modes.

Naming convention:
- `test_<feature>.sh` - confirmational
- `test_<feature>_adv_*.sh` - adversarial

### Meta-context pollution

AI reasons about visible test infrastructure. If a test runs in a directory containing:
- Files named `test_*.sh`
- Planning documents about testing
- Obvious synthetic fixtures

...the AI may reason about the meta-situation instead of following instructions.

Solution: Isolated test environments. Create temporary directories with only the files needed for the scenario. No test infrastructure visible.

### Pattern matching beats AI evaluation for adversarial tests

For checking "did wrong behaviour occur?", use grep, not another Claude instance.

The evaluator Claude can also:
- Reason about meta-context
- Refuse to evaluate
- Ask clarifying questions

Direct pattern matching is deterministic and reliable.

### Happy path is necessary but not sufficient

Validating that success works does not validate that failure is handled correctly.

Before attempting long builds (2+ hours), test:
- Script fails with known pattern → worker detects and reports
- Script fails without pattern → worker detects non-zero exit
- Script times out → worker reports timeout
- Permission prompt appears → supervisor handles it

---

## Collaboration

### Domain expertise matters

In this prototype:
- epistemic-claude: framework design, testing patterns, documentation
- vllm-claude: build systems, environment configuration, project-specific knowledge

Together: more productive than either alone.

### Periodic alignment is needed

Working asynchronously via shared files leads to drift:
- Both ran the same test independently (duplication)
- Different views on next steps (needed explicit alignment)

Mitigation: Regular sync points. Explicitly state "I'm about to do X, you do Y".

### /epistemic is the reset mechanism

After a burst of activity, run /epistemic to:
- Verify goal alignment
- Catch scope creep
- Surface undocumented decisions

This applies to workers too. Supervisor should periodically run /epistemic on worker output.

---

## Open Questions

1. **Permission injection**: Should supervisor programmatically add permissions to settings.json, or document as manual setup?

2. **Autonomous dialogue**: How to make Claude-to-Claude communication work without human in the loop for each turn?

3. **Failure recovery**: What happens when a worker fails? Supervisor spawns a new one? Asks human?

4. **Context window limits**: For 2-hour builds, how does the worker maintain awareness of success/failure patterns?

---

*This document will be updated as the AI teams prototype evolves.*
