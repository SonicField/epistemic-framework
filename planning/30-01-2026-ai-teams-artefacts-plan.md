# AI Teams Artefacts Plan

## Terminal Goal

Create the artefacts that encode our learnings and make AI teams usable.

## Artefacts to Create

### Priority 1: Core Prompts

1. **claude_tools/ai-teams-supervisor.md**
   - Role definition
   - How to create worker tasks
   - How to spawn workers (pty-session)
   - 3Ws + Self-Check template (bundled)
   - When to escalate to human
   - temp.sh pattern for permissions

2. **claude_tools/ai-teams-worker.md**
   - Role definition
   - How to read task files
   - How to update status
   - Evidence-based reporting
   - When to show initiative vs stay in scope

### Priority 2: Templates

3. **templates/ai-teams/supervisor.md**
   - Terminal Goal section
   - Current State section
   - Decisions Log pointer
   - 3Ws + Self-Check section (with counter)

4. **templates/ai-teams/worker-task.md**
   - Task section
   - Instructions section
   - Success Criteria section
   - Status section (State/Started/Completed)
   - Log section
   - Supervisor Actions section (triggers 3Ws)

5. **templates/ai-teams/decisions.log**
   - Header with format instructions
   - Example entry format

### Priority 3: Tooling

6. **bin/ai-teams-init**
   - Creates .epistemic/ structure
   - Copies templates
   - Prints setup instructions (including permissions)

## Key Design Decisions

1. **3Ws + Self-Check bundled** - Can't do 3Ws without also doing self-check (if counter >= 3)
2. **Counter in supervisor.md** - `workers_since_check: N` field tracks when to self-check
3. **Supervisor Actions in worker template** - Trigger for 3Ws is built into what supervisor already reads
4. **temp.sh pattern documented** - Until proper install, this is the workaround

## Execution Order

1. Extract templates from soma experiment (we have working examples)
2. Write supervisor prompt (encodes learnings)
3. Write worker prompt
4. Write init script
5. Test on a fresh project

## Verification

- Fresh Claude can use ai-teams-init, read supervisor prompt, and manage workers
- 3Ws get captured after each worker
- Self-check triggers after 3 workers
