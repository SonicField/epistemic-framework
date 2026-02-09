---
description: Progressive Python-to-Rust conversion using evidence-gated weathering cycles
allowed-tools: Read, Write, Edit, Glob, Grep, AskUserQuestion, Bash(git:*), Bash(python:*), Bash(pytest:*), Bash(cargo:*), Bash(make:*), Bash(./*), Bash(perf:*), Bash(py-spy:*), Bash(hyperfine:*)
---

# NBS Terminal Weathering

**MANDATORY FIRST ACTION — DO NOT SKIP**

Read `{{NBS_ROOT}}/terminal-weathering/concepts/terminal-weathering.md` before proceeding. That document defines the philosophy. This document defines what you DO.

Then detect context and dispatch.

---

## Context Detection and Dispatch

Run these checks immediately:

```
1. Check for .nbs/terminal-weathering/ directory
2. If exists, read .nbs/terminal-weathering/status.md
3. git branch --show-current
```

Dispatch based on results:

| Signal | Dispatch |
|--------|----------|
| No `.nbs/terminal-weathering/` directory | **New session** → Phase 1: Goal Setting |
| Directory exists, `candidates.md` empty or absent | **Survey needed** → Phase 2: Survey |
| Candidates ranked, on `main`/`master` branch | **Select next** → Phase 3: Expose |
| On a `weathering/*` branch | **In-progress conversion** → Phase 4: Weather (continue) |
| Conversion complete on `weathering/*` branch | **Evidence gate** → Phase 5: Assess |
| Back on main, terminal goal not met | **Advance** → Phase 6: Advance |
| Terminal goal met | **Done** → Phase 7: Final Report |

**Do not ask the human which phase to enter.** The signals are unambiguous. Detect and route.

---

## Phase 1: Goal Setting

A new terminal weathering session. No state exists yet.

**What to do:**

1. **Ask the human** for the terminal goal. Not "rewrite in Rust" — that is instrumental. The terminal goal is a measurable system improvement:
   - "Reduce P99 latency from 45ms to 15ms"
   - "Reduce peak memory from 2GB to 500MB"
   - "Eliminate GIL contention under concurrent load"

2. **Confirm the goal is falsifiable.** If the human says "make it faster" — push back. Faster than what? By how much? Measured how?

3. **Create state directory:**

```
.nbs/terminal-weathering/
├── status.md
├── candidates.md
├── trust-levels.md
├── patterns.md
└── conversions/
```

4. **Write `status.md`:**

```markdown
# Terminal Weathering Status

**Terminal Goal**: [one sentence, confirmed by human]
**Falsifier**: [what would prove the goal is not achievable via Rust conversion]
**Phase**: Survey
**Started**: [date]
**Workers Since Check**: 0

## Active Workers
[none]

## Completed Conversions
[none]
```

5. **Write `candidates.md`:**

```markdown
# Conversion Candidates

**Status**: Awaiting survey

| Rank | Function/Class | Module | Pain (measured) | Leaf? | Baseline | Notes |
|------|---------------|--------|----------------|-------|----------|-------|
```

6. **Write `trust-levels.md`:**

```markdown
# Trust Levels

All conversion types start at **Tight**.

| Conversion Type | Level | Consecutive Successes | Last Failure |
|----------------|-------|----------------------|--------------|
| [none yet] | Tight | 0 | — |

## Level Definitions

- **Tight**: Confirm every step with the human
- **Gate**: Run conversion autonomously, present evidence at Assess
- **Batch**: Assign multiple conversions, present batch evidence
- **Review**: Run continuously, flag anomalies only
```

7. **Write empty `patterns.md`:**

```markdown
# Compressed Patterns

**Status**: No conversions completed yet. Patterns will be distilled after first compression cycle.
```

8. **Proceed to Phase 2.**

---

## Phase 2: Survey

Identify existing cracks. Not "what could be Rust" but "what is already hurting."

**What to do:**

1. **Profile performance.** Run or ask the human to run profiling tools:
   - CPU hotspots: `py-spy`, `cProfile`, `perf`
   - Memory: `tracemalloc`, `memray`, `valgrind`
   - Latency distributions under load

2. **Map the call graph.** Identify leaf functions — those with no deeper Python dependencies. Tools: `pyan`, `import analysis`, manual inspection.

3. **Identify code already marked problematic.** Search for TODOs, FIXME, HACK, performance comments, open issues.

4. **Rank candidates.** For each candidate, record in `candidates.md`:
   - Measured pain (not "probably slow" — actual numbers)
   - Whether it is a leaf or near-leaf
   - Estimated conversion scope
   - Baseline measurements

5. **Present ranked list to human.** Get confirmation before proceeding.

6. **Update `status.md`**: Phase → Expose.

**Falsifier**: If profiling reveals no measurable pain, stop. There is nothing to weather. Report this honestly.

---

## Phase 3: Expose

Select a single candidate for conversion.

**What to do:**

1. **Select the highest-ranked candidate** that is:
   - A leaf or near-leaf in the call graph
   - Measurably problematic (numbers recorded)
   - Small enough to convert in one verification cycle

2. **Record baseline measurements.** These are the numbers the conversion must beat:
   - Execution time (distribution, not single run)
   - Memory usage
   - Any domain-specific metrics

3. **Check trust level** for this conversion type in `trust-levels.md`. This determines behaviour in Phase 4.

4. **Create branch:**
   ```bash
   git checkout -b weathering/<module>/<function>
   ```

5. **Create conversion record** in `.nbs/terminal-weathering/conversions/<module>-<function>.md`:

```markdown
# Conversion: <module>.<function>

**Candidate Rank**: [N]
**Branch**: weathering/<module>/<function>
**Trust Level**: [from trust-levels.md]
**Started**: [date]

## Baseline
- Execution time: [measurement]
- Memory: [measurement]
- [other metrics]

## Hypothesis
"Converting <function> to Rust via PyO3 will [specific measurable improvement]."

## Falsifier
"This conversion does NOT help if [specific condition]."

## Weather Log
[To be filled during Phase 4]

## Evidence
[To be filled during Phase 5]

## Verdict
[To be filled during Phase 5]
```

6. **Proceed to Phase 4.**

**Falsifier**: If the candidate cannot be isolated as a leaf, it is not ready. Choose another or decompose further.

---

## Phase 4: Weather

Execute the verification cycle on the selected candidate. Behaviour depends on trust level.

### Trust Level: Tight

Confirm every step with the human before proceeding.

1. **Design**: Rust implementation matching the Python API exactly (PyO3/cffi). Present design to human.
2. **Plan**: Identify what could go wrong — semantics drift, edge cases, GIL interactions. Present plan to human.
3. **Deconstruct**: Break into testable steps. Present breakdown to human.
4. **Test**: Write tests exercising the Python API through the Rust backend. Write benchmarks. Show tests to human.
5. **Code**: Implement Rust. The Python layer remains until proven redundant. Show code to human.
6. **Document**: Record measurements in the conversion record. Show measurements to human.

### Trust Level: Gate

Run the full verification cycle autonomously. Do not interrupt the human at each step. Present complete evidence at Phase 5 (Assess).

### Trust Level: Batch

This level applies when the supervisor assigns multiple conversions. Execute each conversion's Weather phase autonomously. Present batch evidence at Phase 5.

### Trust Level: Review

Run continuously. Only flag anomalies — unexpected test failures, performance regressions, semantic mismatches. The human spot-checks.

**For all levels:**

- Update the conversion record's Weather Log with observations at each step
- If anything unexpected occurs, stop and consult the human regardless of trust level
- The Python API must remain unchanged — the Rust implementation overlays, it does not replace yet
- Update `status.md` as work progresses

---

## Phase 5: Assess

The evidence gate. This is where conversions live or die.

**What to do:**

1. **Collect evidence:**
   - Post-conversion benchmarks (same conditions as baseline)
   - Test results (all existing tests pass, new tests pass)
   - Memory measurements
   - Edge case coverage

2. **Compare against baseline.** Use statistical methods where appropriate — single-run comparisons are insufficient.

3. **Determine verdict.** Three outcomes, no others:

| Verdict | Criterion | Action |
|---------|-----------|--------|
| **Benefit confirmed** | Measurements show improvement beyond noise | Mark permanent. Merge branch. Proceed. |
| **Benefit unclear** | Measurements are ambiguous | More data needed. Do not merge. |
| **Benefit falsified** | No improvement, or regression | Revert. Document learnings. Choose next candidate. |

4. **Record verdict** in the conversion record with full evidence.

5. **If benefit falsified**: This is not failure. This is the methodology working. Document what was learned — "this Python pattern resists Rust replacement because of X" is valuable.

6. **Present verdict to human** (at all trust levels — the evidence gate always involves the human unless at Review level).

7. **Update `trust-levels.md`:**
   - Success: increment consecutive successes for this conversion type
   - Failure: reset to Tight for this conversion type, reset consecutive successes to 0

8. **Return to main branch:**
   ```bash
   git checkout main  # or master
   ```

9. **Proceed to Phase 6.**

**Falsifier**: If you cannot distinguish the three verdicts with evidence, your measurement methodology is wrong. Fix that before proceeding.

---

## Phase 6: Advance

Update the landscape and select the next candidate.

**What to do:**

1. **Update the call graph.** Proven leaf conversions may have exposed new leaves.

2. **Update `candidates.md`.** Re-rank based on:
   - New leaves now accessible
   - Patterns from completed conversions
   - Remaining distance to terminal goal

3. **Check terminal goal progress.** Is the system measurably closer to the goal? Update `status.md`.

4. **If terminal goal met**: Proceed to Phase 7.

5. **If terminal goal not met**: Return to Phase 3 (Expose) with updated candidate list.

6. **Consider Fuse.** If sufficient contiguous coverage exists within a module, consider removing the Python layer entirely. This is a separate verification cycle with its own evidence gate. Risks: Python-side consumers, dynamic dispatch, monkey-patching in test fixtures, implicit interface contracts.

---

## Phase 7: Final Report

Terminal goal achieved (or determined unachievable).

**What to do:**

1. **Compile final report** summarising:
   - Terminal goal and whether it was met
   - All conversions attempted (successes, failures, reversions)
   - Total measured improvement
   - Patterns learned
   - Trust levels achieved per conversion type
   - Failed conversions and what they taught

2. **Update `status.md`**: Phase → Complete.

3. **Present to human.**

---

## The Three Roles

Terminal weathering at scale uses three roles.

### Supervisor

The supervisor holds the terminal goal, the ranked candidate list, and the evidence gates.

**Responsibilities:**
- Maintain `status.md`, `candidates.md`, `trust-levels.md`
- Select candidates and assign conversions to workers
- Adjudicate at the Assess phase — workers report evidence, the supervisor decides
- Track trust levels and adjust oversight accordingly
- Run the epistemic garbage collector (see below)
- Escalate to the human when uncertain

**The supervisor does not convert code.** It delegates, monitors, and decides.

### Conversion Workers

Each worker executes one conversion on an isolated `weathering/<module>/<function>` branch.

**Responsibilities:**
- Execute the full Weather phase (Phase 4)
- Record observations in the conversion record
- Return evidence to the supervisor at Phase 5
- Operate within the trust level assigned by the supervisor

**Workers do not adjudicate.** They report evidence. The supervisor (and ultimately the human) decides.

### Compression Worker

A periodic, pure role that distils raw learnings into compressed patterns.

**Responsibilities:**
- Read all conversion records in `.nbs/terminal-weathering/conversions/`
- Extract patterns: which conversion types succeed, which fail, common pitfalls, useful techniques
- Write compressed patterns to `patterns.md`
- This is a pure function: raw learnings in, compressed patterns out

**The compression worker does not make decisions.** It summarises.

---

## Epistemic Garbage Collector

Every 3 conversion workers, the supervisor must:

1. **Spawn a compression worker** to distil `conversions/` → `patterns.md`
2. **Run `/nbs`** for goal alignment and drift detection
3. **Reset the counter** (`workers_since_check` in `status.md` → 0)

This is mandatory, not optional. The counter is tracked in `status.md`. The compression worker is pure — it handles pattern extraction. `/nbs` handles the epistemic audit separately.

**Why every 3?** Frequent enough to catch drift before it compounds. Infrequent enough not to dominate the work. This matches the nbs-teams self-check cadence.

---

## Trust Gradient Runtime Behaviour

The trust gradient is tracked in `trust-levels.md` and adjusts tool behaviour per level.

### Level Transitions

| Transition | Requirement |
|-----------|------------|
| Tight → Gate | N consecutive successes where human review found no issues the evidence gate missed. N is project-dependent — ask the human. |
| Gate → Batch | Further consecutive successes at Gate level. Supervisor may batch-assign. |
| Batch → Review | Extensive track record. Mature measurement infrastructure. Human explicitly approves. |
| Any → Tight | Single failure where oversight level was insufficient — the human discovers a problem the evidence gate missed. |

**Transitions are earned, not assumed.** The human can say "get on with it" to signal readiness for transition, but only if evidence supports it.

**The gradient applies per conversion type, not globally.** String processing conversions may earn Gate level while numerical code remains at Tight. Each domain builds its own trust independently.

### Behavioural Adjustments

| Level | Phase 4 (Weather) | Phase 5 (Assess) | Human Interaction |
|-------|-------------------|-------------------|-------------------|
| **Tight** | Confirm every step | Present all evidence | Continuous |
| **Gate** | Run autonomously | Present evidence at gate | At Assess only |
| **Batch** | Run autonomously, multiple | Present batch evidence | At batch Assess |
| **Review** | Run continuously | Flag anomalies only | On exception |

---

## Branch Pattern

All conversion work happens on branches following this pattern:

```
weathering/<module>/<function>
```

Examples:
- `weathering/parser/tokenize`
- `weathering/data/serialize_batch`
- `weathering/core/matrix_multiply`

This enables parallel workers on different leaves without conflicts. Each worker operates on its own branch. Merges to main happen only after the Assess phase confirms benefit.

---

## Rules

- **Evidence over authority.** "Rust is faster" is Ethos. "This function runs in 3ms instead of 12ms under production load" is Logos. Only the second is acceptable.
- **Leaf-first, always.** Never convert a function with unconverted Python dependencies. Decompose or wait.
- **The Python layer remains until proven redundant.** Overlay, do not replace, until evidence confirms the conversion.
- **Failed conversions are not failures.** They are the methodology working. Document and learn.
- **No blanket rules.** "String processing converts well" is a hypothesis to test per candidate, not a policy.
- **Report all outcomes.** A conversion log showing 100% success rate is either dishonest or insufficiently ambitious.
- **State lives in `.nbs/terminal-weathering/`.** Not in conversation history, not in your memory. Read the files.
- **The evidence gate is non-negotiable.** Every conversion passes through Assess. No exceptions.
- **The epistemic garbage collector is mandatory.** Every 3 workers, compress and audit. No skipping.
- **Trust is slow to build and fast to lose.** One failure reverts the trust level for that conversion type.
- **When in doubt, escalate.** Ask the human rather than guess.

---

## The Contract

The human defines "benefit." The AI implements and reports evidence. Neither trusts the other's assertions — both trust evidence.

The terminal goal is system improvement. Language replacement is instrumental. If the system is not measurably better, the conversion has no purpose.

_Seek to falsify each conversion. Record what you observe. Let the evidence speak._
