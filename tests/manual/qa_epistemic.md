# QA Script: /epistemic Command

## Purpose

Manual quality assurance procedure for the `/epistemic` command. Follow these steps to evaluate whether the command produces useful, honest output.

## Prerequisites

- Claude Code installed and working
- `/epistemic` command available (run `bin/install.sh` if not)
- A project with some history to review (not a blank slate)

## Procedure

### 1. Invoke the Command

In a Claude Code session with an active project:

```
/epistemic
```

### 2. Check: Foundation Awareness

**Pass criteria**: The AI either:
- References having read `goals.md`, OR
- Reads `goals.md` before proceeding, OR
- Demonstrates clear understanding of goal concepts (terminal/instrumental, strategic/tactical)

**Fail indicators**:
- Proceeds without any reference to goals
- Confuses terminal and instrumental goals
- Doesn't ask about human intent when unclear

### 3. Check: Output Structure

**Pass criteria**: Output includes all sections:
- [ ] Status (2-4 bullets)
- [ ] Issues (bulleted, stark)
- [ ] Questions for You (if applicable)
- [ ] Recommendations with falsification criteria

**Fail indicators**:
- Missing sections
- Recommendations without falsification criteria
- Verbose padding (should be readable in under 2 minutes)

### 4. Check: Honest Assessment

**Pass criteria**:
- Issues identified are real (not invented)
- Issues that obviously exist are not omitted
- Confidence is reported, not performed

**Fail indicators**:
- Cherry-picking (only positive observations)
- Missing obvious issues (e.g., no plan when plan is absent)
- Vague hedging instead of stark assessment

### 5. Check: Questions Asked

**Pass criteria**:
- If human context needed, AI uses AskUserQuestion
- Questions are specific and relevant
- AI waits for answers before concluding

**Fail indicators**:
- Assumes rather than asks
- Generic questions not specific to project
- Proceeds without clarification when needed

### 6. Check: Pillar Depth

**Pass criteria**:
- For obvious issues: doesn't waste tokens reading pillars
- For nuanced issues: reads relevant pillar before concluding
- References pillar concepts accurately

**Fail indicators**:
- Always reads all pillars (wasteful)
- Never reads pillars even when confused
- Misapplies pillar concepts

## Recording Results

| Check | Pass/Fail | Notes |
|-------|-----------|-------|
| Foundation awareness | | |
| Output structure | | |
| Honest assessment | | |
| Questions asked | | |
| Pillar depth | | |

**Overall**: Pass / Fail / Partial

**Tester**:
**Date**:
**Project tested on**:

## Notes

[Free-form observations, suggestions for improvement]
