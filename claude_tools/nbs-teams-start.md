---
description: Bootstrap a project for NBS teams in one command
allowed-tools: Write, AskUserQuestion, Bash(mkdir:*)
---

# Start NBS Teams

You are bootstrapping a project for NBS teams. This creates the minimal structure needed to start working with supervisor/worker patterns.

**This is a one-time setup command.** If `.nbs/` already exists, confirm before overwriting.

---

## Process

### Step 1: Check for Existing Structure

Before anything, check if `.nbs/` exists:

```bash
ls -la .nbs/ 2>/dev/null
```

**If it exists:**
- Ask: "An .nbs/ directory already exists. Would you like to reset it? This will overwrite supervisor.md and decisions.log."
- If no, stop. If yes, proceed.

### Step 2: Ask Terminal Goal

Ask the user:

> "What is the terminal goal for this project? (One sentence describing what you're trying to achieve)"

Wait for their answer. This becomes the foundation of the supervisor document.

**Do not proceed without a terminal goal.** If the answer is vague, ask for clarification:
- "Can you be more specific? What would success look like?"
- "What problem are you solving?"

### Step 3: Create Directory Structure

Create the `.nbs/` directory and `workers/` subdirectory:

```bash
mkdir -p .nbs/workers
```

### Step 4: Create supervisor.md

Write `.nbs/supervisor.md` using this template, inserting the user's terminal goal:

```markdown
# Supervisor: [Project Name from terminal goal]

## Terminal Goal

[User's terminal goal - verbatim]

## Current State

Phase: PLANNING
Active workers: none
Workers since last check: 0

## Progress

[Track major milestones and questions answered]

## Decisions Log

See `.nbs/decisions.log`

---

## 3Ws + Self-Check Log

[Append after each worker completes]

<!--
Template for each entry:

### Worker: [name] - [date]

**What went well:**
-

**What didn't work:**
-

**What we can do better:**
-

**Self-check** (if workers_since_check >= 3):
- [ ] Am I still pursuing terminal goal?
- [ ] Am I delegating vs doing tactical work myself?
- [ ] Have I captured learnings that should improve future tasks?
- [ ] Should I escalate anything to human?

[Reset workers_since_check to 0 after self-check]
-->

---
```

### Step 5: Create decisions.log

Write `.nbs/decisions.log`:

```markdown
# Decisions Log

Append all significant decisions using this format:

---
[YYYY-MM-DD HH:MM] [DECISION TITLE]
Context: [why this decision was needed]
Decision: [what was decided]
Implication: [what this means for the work]
---

```

### Step 6: Confirm and Explain

Tell the user what was created:

```
Created:
- .nbs/supervisor.md (your state and learnings)
- .nbs/decisions.log (append-only decision record)
- .nbs/workers/ (worker task files go here)

You are now the supervisor. Your terminal goal is recorded.

Next steps:
1. Read claude_tools/nbs-teams-supervisor.md to understand your role
2. Decompose your goal into worker tasks
3. Spawn workers with nbs-worker (see /nbs-tmux-worker for reference)
4. Capture learnings after each worker completes

Run /nbs-teams-help if you need guidance on any of these.
```

---

## Rules

- **One question, one action, confirmation.** This is not a wizard that asks 10 questions upfront.
- **Terminal goal is mandatory.** Do not create structure without it.
- **Confirm before overwriting.** Existing `.nbs/` directories contain valuable state.
- **Explain what was created.** The user should understand the structure.

---

## What Happens Next

The user is now the supervisor. They should:
1. Read `nbs-teams-supervisor.md` to understand their role
2. For teams work with phased delivery, initialise the hub: `nbs-hub init <project-dir> <goal>`
3. Decompose work into worker tasks
4. Use `nbs-hub spawn` (with hub) or `nbs-worker spawn` (without) to create and start worker Claudes
5. Capture 3Ws after each worker completes

If they need help, they run `/nbs-teams-help`.
