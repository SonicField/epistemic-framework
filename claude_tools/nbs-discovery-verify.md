---
description: Verify discovery report completeness before recovery
allowed-tools: Read, Glob, Grep, AskUserQuestion
---

# NBS Discovery Verification

You have just completed `/nbs-discovery`. Before the human proceeds to recovery, verify that the discovery report is complete and captures everything discussed.

**This is a checkpoint, not a redo.** You're checking your own work.

---

## Verification Checks

### 1. Required Sections

The discovery report must contain all of these:

| Section | Check |
|---------|-------|
| Terminal Goal (Reconstructed) | Present and confirmed by human? |
| Artefacts Found | Table with locations, files, status? |
| Triage Summary | Table with verdicts and rationale? |
| Valuable Outcomes Identified | Listed with evidence pointers? |
| Gap Analysis: Instrumental Goals Summary | Table with goals, why needed, dependencies? |
| Gap Analysis: Confirmed Understanding | Full Q&A restatements, not summaries? |
| Open Questions | Listed? |
| Recommended Next Steps | Present? |

**If any section is missing or incomplete, add it now.**

### 2. Confirmed Restatements

During Gap Analysis, you confirmed understanding with the human: "So [restatement]. Is that correct?"

**Check**: Are ALL confirmed restatements in the report in full, not compressed to one-liners?

The restatements contain distilled human knowledge. They are input to recovery. If they're only in conversation history and not in the report, recovery will not have them.

**If restatements are missing, add a "Confirmed Understanding (Full Detail)" section with each Q&A pair.**

### 3. Context Window Leakage

Review the conversation history. Is there anything the human told you that:
- Didn't make it into the report?
- Was summarised too aggressively?
- Contains nuance that was lost?

**If so, update the report to capture it.**

### 4. Artefact Coverage

Cross-check:
- Did the human mention any files/locations that weren't explored?
- Are there artefacts in the triage that have unclear verdicts?
- Did the human express uncertainty that should be in Open Questions?

---

## Output

After verification, report:

```markdown
## Discovery Verification

### Sections
- [✓/✗] Terminal Goal
- [✓/✗] Artefacts Found
- [✓/✗] Triage Summary
- [✓/✗] Valuable Outcomes
- [✓/✗] Gap Analysis: Instrumental Goals
- [✓/✗] Gap Analysis: Confirmed Understanding
- [✓/✗] Open Questions
- [✓/✗] Recommended Next Steps

### Issues Found
[List any gaps identified and fixed]

### Report Status
[Ready for recovery / Needs human input on X]
```

If issues were found, update the discovery report before declaring it ready.

---

## The Contract

The discovery report is the **sole input** to `/nbs-recovery`. Everything recovery needs must be in that report. This verification ensures nothing is lost between discovery and recovery.

_Check your work. The human trusted you with their knowledge. Make sure it's captured._
