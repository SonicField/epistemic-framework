# Manual Discovery Test Template

**Purpose**: Run `/nbs-discovery` on a real project while recording the process for framework evaluation.

## Prerequisites

- A project that was developed without NBS discipline, or has drifted into disorder
- The project owner available to answer questions
- Time for a thorough discovery session (typically 30-60 minutes)

## Instructions for Claude Code

You have two parallel tasks:

### Task 1: Run Discovery

Execute `/nbs-discovery` on the target project.

Locations to investigate:
- Primary work directory: [ASK PROJECT OWNER]
- Additional artefacts in: [ASK PROJECT OWNER]

When the command asks questions, engage with the project owner to get context. They have the knowledge the files cannot provide.

### Task 2: Maintain Process Log

As you work, maintain a **process log** in a separate section of your output. Record:

1. **What you searched for** - commands, patterns, locations
2. **What you found vs expected** - surprises, gaps, dead ends
3. **Where human input was essential** - questions that couldn't be answered from files
4. **Where the command's prompts helped** - did the structure guide useful work?
5. **Where the command's prompts hindered** - awkward, missing, or misleading guidance
6. **Confusion points** - anything unclear about the project or the process

Format the process log as:

```markdown
## Process Log

### Search Actions
- [sequence] Searched for X, found Y

### Surprises
- Expected A, found B because C

### Human Input Required
- Asked about X because files showed Y but meaning unclear

### Command Guidance
- Helpful: [what worked]
- Unhelpful: [what didn't]

### Confusion
- Unclear about X
```

## Outputs Expected

Write both outputs to files:

1. **Discovery Report**: `<date>-<project>-discovery-report.md`
   - Follow the format defined in `/nbs-discovery` command
   - Must include: Terminal Goal, Artefacts Found, Triage Summary, Gap Analysis, Valuable Outcomes, Open Questions, Recommended Next Steps

2. **Process Log**: `<date>-<project>-process-log.md`
   - Use the format above
   - Write entries as you go, not retrospectively

## Evaluation Criteria

After the session, compare:

| Criterion | Check |
|-----------|-------|
| Coverage | Did the report find the artefacts that matter? |
| Accuracy | Are the verdicts (keep/discard/evaluate) correct? |
| Gap analysis | Were instrumental goals identified? |
| Human interaction | Did the Q&A surface knowledge the files couldn't provide? |
| Confirmed restatements | Are they in the report in full, not compressed? |
| Command guidance | Did the prompts guide useful work? |
| Time efficiency | Was the session productive, not wasteful? |

## Recording Results

| Metric | Value | Notes |
|--------|-------|-------|
| Session duration | | |
| Human questions asked | | |
| Artefacts found | | |
| Key decisions surfaced | | |
| Coverage accuracy | Pass/Fail | |
| Framework suggestions | | |

**Tester**:
**Date**:
**Project**:
