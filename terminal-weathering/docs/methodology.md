# Methodology

## The Six Phases

Terminal weathering is iterative, not linear. Each cycle processes one candidate through six phases. The cycle repeats until the terminal goal is met or determined unachievable.

### Survey

Identify existing cracks. Not "what could be Rust" but "what is already hurting."

**Activities:**
- Profile performance: CPU hotspots, latency distributions under realistic load
- Profile memory: allocation patterns, peak usage, leaks
- Map the call graph. Identify leaf functions — those with no deeper Python dependencies
- Search for code already marked problematic: TODOs, FIXMEs, HACKs, performance comments, open issues

**Exit criterion:** Ranked list of candidates with quantified pain. Each candidate has measured numbers, not guesses.

**Falsifier:** If no measurable pain exists, stop. There is nothing to weather. Report this honestly — it is not failure, it is the survey doing its job.

### Expose

Select a single candidate from the ranked list.

The candidate must be:
- A leaf or near-leaf in the call graph
- Measurably problematic — not "might be slow" but "is slow, here is the profile"
- Small enough to convert in one verification cycle

Baseline measurements are recorded: execution time (distribution, not single run), memory usage, and any domain-specific metrics. These are the numbers the conversion must beat.

Work happens on an isolated branch:
```
weathering/<module>/<function>
```

A conversion record is created in `.nbs/terminal-weathering/conversions/` containing the hypothesis ("converting this function to Rust via PyO3 will [specific measurable improvement]") and its falsifier.

**Exit criterion:** Single function or class selected with baseline measurements recorded.

**Falsifier:** If the candidate cannot be isolated as a leaf, it is not ready. Decompose further or choose another.

### Weather

Apply the verification cycle to the selected candidate.

The six steps of the verification cycle apply directly:

1. **Design** — Rust implementation matching the Python API exactly. The binding layer (PyO3 or cffi) is chosen based on the interface requirements.
2. **Plan** — Identify what could go wrong. Semantics drift between Python and Rust (floating-point behaviour, integer overflow, string encoding). Edge cases the Python handles implicitly. GIL interactions if the function is called concurrently.
3. **Deconstruct** — Break the conversion into testable steps. Each step is small enough to verify independently.
4. **Test** — Write tests exercising the Python API through the Rust backend. Write benchmarks. The tests must cover the same edge cases the Python implementation handles.
5. **Code** — Implement Rust. The Python layer remains — the Rust implementation overlays, it does not replace. Consumers continue to import from Python. The switch is internal.
6. **Document** — Record measurements in the conversion record. Baseline versus post-conversion, under the same conditions.

The level of human interaction during Weather depends on the trust level (see below).

**Exit criterion:** Tests pass, benchmarks collected, Python API unchanged.

**Falsifier:** "This conversion provides measurable benefit" — attempt to falsify by benchmarking under realistic load, testing edge cases the Python handled implicitly, and measuring total system impact rather than isolated function performance.

### Assess

The evidence gate. Every conversion passes through this phase. No exceptions.

**Three outcomes, no others:**

| Verdict | Criterion | Action |
|---------|-----------|--------|
| **Benefit confirmed** | Measurements show improvement beyond noise | Mark permanent. Merge branch. |
| **Benefit unclear** | Measurements are ambiguous | More data needed. Do not merge. |
| **Benefit falsified** | No improvement, or regression | Revert. Document learnings. |

**"Benefit falsified" is not failure.** It is the methodology working. A reverted conversion that taught us "this Python pattern resists Rust replacement because of X" is more valuable than a committed conversion nobody measured.

Evidence quality matters. Single-run comparisons are insufficient. Use statistical methods — distributions, confidence intervals, multiple runs under varying load. If you cannot distinguish the three verdicts with evidence, your measurement methodology is wrong. Fix that before proceeding.

After the verdict:
- Update the conversion record with full evidence
- Update `trust-levels.md` (success increments consecutive count; failure resets to Tight)
- Return to main branch

### Advance

Update the landscape and select the next candidate.

With proven leaf conversions, the call graph changes:
- Former near-leaves may now be leaves, their dependencies already in Rust
- Patterns emerge: which types of conversion yield benefit, which do not
- The candidate list is re-ranked based on new leaves, accumulated evidence, and remaining distance to the terminal goal

No blanket rules. "String processing converts well" is a hypothesis to test per candidate, not a policy to apply wholesale.

**Exit criterion:** Next candidate selected based on updated call graph and accumulated evidence.

### Fuse

When sufficient contiguous Rust coverage exists within a module, consider removing the Python layer entirely. This is a separate verification cycle with its own evidence gate — not an automatic consequence of successful conversions.

**Risks specific to fusion:**
- Python-side consumers that import from the module directly
- Dynamic dispatch that routes through the Python layer
- Monkey-patching in test fixtures
- Implicit interface contracts that the Python layer satisfies but the Rust layer does not

**Falsifier:** "Removing the Python layer does not break any consumer" — test exhaustively. If any consumer breaks, the Python layer remains.

---

## The Trust Gradient

Human oversight is expensive. Applying full oversight to every step of every conversion does not scale. But removing oversight without evidence is negligence.

### The Four Levels

| Level | Oversight | When |
|-------|-----------|------|
| **Tight** | Human reviews every step of every conversion | Initial conversions; no evidence base yet |
| **Gate** | Human reviews evidence at Assess phase only | Pattern of successful conversions established |
| **Batch** | Human reviews evidence for batches of conversions | Strong evidence base; consistent patterns |
| **Review** | Human spot-checks; AI flags anomalies | Extensive track record; mature measurement infrastructure |

### Behavioural Differences

| Level | During Weather | During Assess | Human Interaction |
|-------|---------------|---------------|-------------------|
| **Tight** | Confirm every step | Present all evidence | Continuous |
| **Gate** | Run autonomously | Present evidence at gate | At Assess only |
| **Batch** | Run autonomously, multiple | Present batch evidence | At batch Assess |
| **Review** | Run continuously | Flag anomalies only | On exception |

### Transitions

**Transitions are earned, not assumed.**

| Transition | Requirement |
|-----------|------------|
| Tight → Gate | N consecutive successes where human review found no issues the evidence gate missed. N is project-dependent — ask the human. |
| Gate → Batch | Further consecutive successes at Gate level. Supervisor may batch-assign. |
| Batch → Review | Extensive track record. Mature measurement infrastructure. Human explicitly approves. |
| Any → Tight | Single failure where oversight level was insufficient — the human discovers a problem the evidence gate missed. |

**Transitions are reversible.** A single conversion where the oversight level was insufficient — where the human discovers a problem the evidence gate missed — reverts the level. Trust is slow to build and fast to lose.

**The gradient applies per conversion type, not globally.** String processing conversions may earn Gate level while numerical code remains at Tight. Each domain of conversion builds its own trust independently.

---

## The Epistemic Garbage Collector

Raw conversion records accumulate. Patterns hide in the noise. Without periodic compression, the evidence base becomes unwieldy and drift goes undetected.

### The Mechanism

Every three conversion workers, the supervisor must:

1. **Spawn a compression worker.** This is a pure role — it reads all conversion records in `.nbs/terminal-weathering/conversions/`, extracts patterns (which conversion types succeed, which fail, common pitfalls, useful techniques), and writes compressed patterns to `patterns.md`. It does not make decisions. It summarises.

2. **Run `/nbs`.** The standard NBS audit checks goal alignment, falsifiability discipline, and drift detection. This is separate from the compression worker — compression handles patterns, `/nbs` handles epistemics.

3. **Reset the counter.** `workers_since_check` in `status.md` returns to zero.

### Why Every Three

Frequent enough to catch drift before it compounds. Infrequent enough not to dominate the work. This matches the NBS teams self-check cadence.

### What Good Compression Looks Like

- "Four out of five string processing conversions showed >3x speedup. The exception involved heavy Unicode normalisation — Python's `unicodedata` module has no direct Rust equivalent at comparable coverage."
- "All memory-focused conversions reduced peak allocation by 40-60%. The pattern: Python creates intermediate lists that Rust replaces with iterators."

### What Bad Compression Looks Like

- "Conversions are going well." (No falsifiable content.)
- "Rust is faster than Python." (Ethos, not evidence.)

---

## NBS Teams Integration

For codebases at scale — millions of lines — terminal weathering maps onto the supervisor/worker pattern.

### Supervisor

The supervisor holds the terminal goal, the ranked candidate list, and the evidence gates.

**Responsibilities:**
- Maintain `status.md`, `candidates.md`, `trust-levels.md`
- Select candidates and assign individual conversions to workers
- Adjudicate at the Assess phase — workers report evidence, the supervisor decides
- Track trust levels and adjust oversight accordingly
- Run the epistemic garbage collector every three workers
- Escalate to the human when uncertain

**The supervisor does not convert code.** It delegates, monitors, and decides.

### Conversion Workers

Each worker executes one conversion on an isolated `weathering/<module>/<function>` branch.

**Responsibilities:**
- Execute the full Weather phase
- Record observations in the conversion record
- Return evidence to the supervisor at Assess
- Operate within the trust level assigned by the supervisor

**Workers do not adjudicate.** They report evidence. The supervisor (and ultimately the human) decides.

### Compression Worker

A periodic, pure role that distils raw learnings into compressed patterns. Spawned by the supervisor every three conversion workers as part of the epistemic garbage collector.

**Responsibilities:**
- Read all conversion records
- Extract patterns: which conversion types succeed, which fail, common pitfalls, useful techniques
- Write compressed patterns to `patterns.md`

**The compression worker does not make decisions.** It summarises.

---

## Evidence Gates

### What Good Evidence Looks Like

- Benchmarks run under realistic load, not synthetic microbenchmarks alone
- Statistical distributions, not single-run numbers
- Memory measurements under sustained operation, not just peak
- Edge cases explicitly tested — the ones the Python handled implicitly
- Total system impact measured, not just isolated function performance
- Comparison conditions identical to baseline (same hardware, same data, same load)

### What Bad Evidence Looks Like

- "It feels faster" (not measured)
- A single benchmark run showing 2x improvement (not statistically significant)
- Microbenchmark in isolation without system-level measurement (does not capture integration costs)
- Missing edge case coverage (the Python might handle cases the Rust does not)
- Different conditions from baseline (invalidates comparison)

### Failed Conversions as Positive Outcomes

A conversion that fails the evidence gate teaches something. Document it:

- **What was the hypothesis?** "Converting `parse_header` to Rust will reduce P99 latency by 30%."
- **What did the evidence show?** "Latency reduced by 4%, within noise. The bottleneck is I/O wait, not parsing."
- **What did we learn?** "Header parsing is not a performance-critical path. The survey's ranking was based on CPU profile alone; I/O-bound functions need a different profiling approach."

This is more valuable than a successful conversion that nobody examined critically. A conversion log showing 100% success rate is either dishonest or insufficiently ambitious.
