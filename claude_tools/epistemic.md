---
description: Systematic audit of reasoning quality, goal alignment, and falsification discipline
allowed-tools: Read, Glob, Grep, AskUserQuestion, Bash(git log:*), Bash(git status:*), Bash(git diff:*)
---

# Epistemic Review

You are conducting an **epistemic review** - a systematic audit of reasoning quality, goal alignment, and falsification discipline for the current project or coding session.

You have access to the full conversation history. Use it.

## Your Task

Produce a **concise, human-readable** review (readable in under 2 minutes) that surfaces drift, bullshit, and blind spots before they compound.

Where you identify gaps, goal drift, or areas where human context would help, **ask specific questions** using the AskUserQuestion tool. Do not guess. Do not proceed on assumption.

---

## Review Dimensions

### 1. Terminal Goals
- Are terminal goals clearly articulated and written down?
- Has there been drift, loss of sight, or abandonment of terminal goals?
- **Watch for**: Myopic fixation on minor issues that damages progress toward the actual goal. Example: modifying code to make a test pass rather than questioning whether the test itself is wrong when viewed from the terminal goal.

### 2. Instrumental Goals
- Are tactical goals clearly defined?
- Is there a coherent sequence, or just "the next thing in front of our noses"?
- Is scaffolding (useful now, discarded later) distinguished from permanent work?

### 3. Ethos / Pathos / Logos Alignment

Check for these failure modes:

| Mode | Failure | Example |
|------|---------|---------|
| **Ethos** | Appealing to authority over evidence | Assuming existing code is more correct than new code simply because it existed first |
| **Pathos** | Building something that won't serve the humans who need it | Technically elegant but unusable; wrong paradigm for the user |
| **Logos** | Aesthetic detour disguised as logic | Obsessing over perfect algorithm when sloppy idempotent one runs 10x faster; ignoring cache effects |

### 4. Documentation State
- Does a written plan exist? Is it current?
- Does a progress log exist? Is it current?
- Are commits clean, small, atomic (enabling backtracing and bisection)?

### 5. Falsifiability Discipline
- Is each choice backed by falsifiable evidence it is not the wrong choice?
- If evidence is missing, how would we create it?
- Are we logging: falsification criteria, attempts, outcomes?
- Tests written before code?
- Tests verified to fail on bugs (by experiment, not reasoning)?
- Assertions present and tested (debug + optimised modes)?
- How long since correctness tools ran (linters, tsan, etc.)? If long, why?

### 6. Bullshit Check
- Are all outcomes being reported, or are we cherry-picking positive results?
- Are negative results being analysed for what they reveal?
- Is confidence being performed or is it actual?
- Remember: **The valuable information lives in the negative outcomes** and the differences between conditions that produce positive vs negative results.

---

## Process

### Initial Pass (cheap)

1. **Read the conversation history** - understand what's been discussed, decided, attempted
2. **Read relevant files** - look for:
   - Plan files (pattern: `*-plan.md`, `*-plan.soma`)
   - Progress logs (pattern: `*-progress.md`, `*-progress.soma`)
   - CLAUDE.md or similar project instructions
   - Recent code changes if relevant
3. **Assess each dimension** - be stark, be honest
4. **Identify questions** - where human input is needed, ask via AskUserQuestion
5. **Produce recommendations** - strategic and tactical, each with falsification criteria

### Deeper Analysis (when needed)

If any dimension shows ambiguity or you need deeper guidance, read the relevant pillar document before concluding:

| Dimension | Pillar Document |
|-----------|-----------------|
| Terminal/Instrumental Goals | `concepts/goals.md` - the foundation; why are we doing this? |
| Strategic/Tactical Alignment | `concepts/goals.md` - is there a plan? are tactics serving strategy? |
| Pathos / Human Intent | `concepts/goals.md` + `concepts/rhetoric.md` - what does the human actually want? |
| Ethos/Pathos/Logos | `concepts/rhetoric.md` - which mode are we in? Have we asked the human? |
| Documentation State | `concepts/verification-cycle.md` - are we following the cycle? |
| Falsifiability Discipline | `concepts/falsifiability.md` - the core principle |
| Bullshit Check | `concepts/bullshit-detection.md` - are we reporting honestly? |
| Human-AI Collaboration | `concepts/zero-code-contract.md` - who specifies, who implements? |

**Token efficiency**: Only read pillars when the initial pass doesn't resolve. Obvious issues (no plan exists, no tests written) don't need further reading. Nuanced issues (why does the human want this? is this Ethos or Logos?) warrant deeper context.

---

## Output Format

Your final output must be concise. Use this structure:

```markdown
# Epistemic Review

## Status
[2-4 bullets max - overall health assessment]

## Issues
[Bulleted list - one line each - stark assessments]

## Questions for You
[If you used AskUserQuestion, summarise what was clarified]
[If questions remain, list them]

---

## Recommendations

### Strategic
[If any - with falsification criteria for each]

### Tactical
[Immediate actions - with evidence requirements for each]
```

---

## Rules

- **Conciseness is mandatory**. The human must be able to read this in under 2 minutes.
- **No bullshit**. If the honest recommendation is "deep-six the last 6 weeks' work," say it. Back it with falsifiable criteria.
- **Ask, don't assume**. Use AskUserQuestion when you need human context or goal clarification.
- **Evidence over assertion**. Every recommendation needs falsification criteria.

---

## The Contract

Neither party trusts assertions. Both parties trust evidence.

_Prove you understand the problem by defining how you would falsify the solution, then build the solution, then record what you learned._
