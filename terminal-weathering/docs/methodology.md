# Methodology

## The Six Phases

Terminal weathering is iterative, not linear. Each cycle processes one candidate through six phases. The cycle repeats until the terminal goal is met or determined unachievable.

The unit of work is a **call protocol path** — a CPython dispatch chain from type slot to function body. Replacing a type slot with a C function eliminates the entire dispatch overhead for that path. This is distinct from replacing function bodies, which leaves the dispatch chain intact.

### Survey

Identify existing cracks. Not "what could be C" but "what is already hurting" — and specifically, *where* in the dispatch chain the cost lies.

**Activities:**
- Profile performance: CPU hotspots, latency distributions under realistic load
- Profile memory: allocation patterns, peak usage, leaks
- **Analyse call protocols:** Identify high-hit-count dispatch chains. Use `perf` to trace CPython slot dispatch (`tp_getattro`, `tp_call`, `tp_richcompare`, etc.) and measure per-invocation overhead. The question is not "which function body is slow" but "which dispatch path is invoked millions of times"
- Map the call graph. Identify leaf type slots — those with no deeper Python dispatch dependencies
- Search for code already marked problematic: TODOs, FIXMEs, HACKs, performance comments, open issues

**Exit criterion:** Ranked list of candidates with quantified pain. Each candidate identifies both the type slot and the dispatch chain it replaces, with measured hit counts and per-call overhead.

**Falsifier:** If no measurable pain exists, stop. There is nothing to weather. Report this honestly — it is not failure, it is the survey doing its job. If pain exists in function bodies but not in dispatch chains, terminal weathering is the wrong tool — consider PyO3 or Cython for body-level replacement instead.

### Expose

Select a single candidate from the ranked list.

The candidate must be:
- A **type slot or call protocol path** — not a function body in isolation
- A leaf or near-leaf in the dispatch graph: no deeper Python dispatch dependencies that must be replaced first
- Measurably problematic — not "might have dispatch overhead" but "dispatch chain accounts for X% of call cost, measured with perf"
- Small enough to convert in one verification cycle

Baseline measurements are recorded: per-call cost (distribution, not single run), dispatch chain depth, hit count under realistic load, and any domain-specific metrics. These are the numbers the conversion must beat.

Work happens on an isolated branch:
```
weathering/<type>/<slot>
```

A conversion record is created in `.nbs/terminal-weathering/conversions/` containing the hypothesis ("replacing `tp_getattro` on `Module` with a C implementation will [specific measurable improvement]") and its falsifier.

**Exit criterion:** Single type slot or call protocol path selected with baseline measurements recorded.

**Falsifier:** If the candidate cannot be isolated as a leaf in the dispatch graph, it is not ready. Decompose further or choose another.

### Weather

Apply the verification cycle to the selected candidate.

The six steps of the verification cycle apply directly:

1. **Design** — C implementation replacing the target type slot directly against CPython's type API. The implementation installs a C function at the slot level (e.g., `tp_getattro`), bypassing the entire Python dispatch chain. Use `PyType_Spec` or direct slot assignment via `PyType_Modified` as appropriate.
2. **Plan** — Identify what could go wrong. Semantic drift between Python and C (reference counting, exception propagation, descriptor protocol compliance). Edge cases the Python dispatch handles implicitly. Thread safety under free-threaded CPython. MRO invalidation when slots change.
3. **Deconstruct** — Break the conversion into testable steps. Each step is small enough to verify independently.
4. **Test** — Write tests exercising the Python API through the C backend. Write benchmarks. The tests must cover the same edge cases the Python dispatch chain handles. **Mandatory sub-steps:**
   - **ASan verification:** Build the C extension with AddressSanitizer enabled (`-fsanitize=address`). Run the full test suite under ASan. Zero ASan errors is a hard gate — no exceptions, no suppressions without documented justification.
   - **Leak analysis:** Run the test suite under Valgrind (`valgrind --leak-check=full`). All memory allocated by the C extension must be accounted for. CPython's own allocations are excluded via suppression file, but extension-allocated memory must be clean.
   - **Reference count verification:** Instrument with `Py_INCREF`/`Py_DECREF` audit or use CPython debug builds (`--with-pydebug`) to detect refcount errors. Leaked references are memory leaks by another name.
5. **Code** — Implement C. Build via `setuptools` with a `setup.py` or `pyproject.toml` defining the C extension module. The Python layer remains — the C implementation overlays the type slots, it does not remove the Python module. Consumers continue to import from Python. The slot replacement is internal.
6. **Document** — Record measurements in the conversion record. Baseline versus post-conversion, under the same conditions. Include ASan and Valgrind output as evidence artefacts.

The level of human interaction during Weather depends on the trust level (see below).

**Exit criterion:** Tests pass, ASan clean, leak-free, benchmarks collected, Python API unchanged.

**Falsifier:** "This conversion provides measurable benefit" — attempt to falsify by benchmarking under realistic load, testing edge cases the Python dispatch chain handled implicitly, and measuring total system impact rather than isolated slot performance.

### Assess

The evidence gate. Every conversion passes through this phase. No exceptions.

**Five mandatory checks:**

| Check | Criterion | Gate |
|-------|-----------|------|
| **Correctness** | All tests pass through the C backend | Hard gate |
| **ASan clean** | Zero AddressSanitizer errors | Hard gate |
| **Leak-free** | Valgrind reports no extension-allocated leaks | Hard gate |
| **Refcount clean** | No reference count errors under debug build | Hard gate |
| **Performance** | Measurements show improvement beyond noise | Evidence gate |

**Three outcomes for performance, no others:**

| Verdict | Criterion | Action |
|---------|-----------|--------|
| **Benefit confirmed** | Measurements show improvement beyond noise | Mark permanent. Merge branch. |
| **Benefit unclear** | Measurements are ambiguous | More data needed. Do not merge. |
| **Benefit falsified** | No improvement, or regression | Revert. Document learnings. |

A conversion that passes all four hard gates but fails the performance evidence gate is still reverted — correct C code that provides no benefit is complexity without value. However, the correctness evidence is preserved: it proves the type slot *can* be replaced, which informs future work.

**"Benefit falsified" is not failure.** It is the methodology working. A reverted conversion that taught us "this dispatch chain resists C replacement because of X" is more valuable than a committed conversion nobody measured.

Evidence quality matters. Single-run comparisons are insufficient. Use statistical methods — distributions, confidence intervals, multiple runs under varying load. If you cannot distinguish the three verdicts with evidence, your measurement methodology is wrong. Fix that before proceeding.

After the verdict:
- Update the conversion record with full evidence (including ASan/Valgrind output)
- Update `trust-levels.md` (success increments consecutive count; failure resets to Tight)
- Return to main branch

### Advance

Update the landscape and select the next candidate.

With proven type slot replacements, the dispatch graph changes:
- New type slots become accessible: once `tp_getattro` is proven on `Module`, `tp_setattro` can be attempted, or the same slot on related types
- Patterns emerge: which types of slot replacement yield benefit, which do not
- The candidate list is re-ranked based on new accessible slots, accumulated evidence, and remaining distance to the terminal goal

No blanket rules. "`tp_getattro` replacements always work" is a hypothesis to test per type, not a policy to apply wholesale.

**Exit criterion:** Next candidate selected based on updated dispatch graph and accumulated evidence.

### Fuse

When sufficient contiguous C slot coverage exists within a type, consider removing the Python dispatch chain entirely for that type. This is a separate verification cycle with its own evidence gate — not an automatic consequence of successful conversions.

**What fusion means in practice:** When all significant type slots on a type (`tp_getattro`, `tp_setattro`, `tp_call`, etc.) have been replaced with C implementations, the type can be defined entirely in C, eliminating the Python class definition and its associated `slot_*` wrappers. The Python module remains for API compatibility, but the type object is created in C.

**Risks specific to fusion:**
- Python-side consumers that subclass the type
- Dynamic dispatch that routes through the Python layer
- Monkey-patching in test fixtures
- Implicit interface contracts that the Python dispatch chain satisfies but the C slots do not
- MRO interactions with other Python types in the hierarchy

**Falsifier:** "Removing the Python dispatch chain does not break any consumer" — test exhaustively. If any consumer breaks, the Python layer remains.

---

## The Trust Gradient

Human oversight is expensive. Applying full oversight to every step of every conversion does not scale. But removing oversight without evidence is negligence.

### What Enables Trust in C

Rust's borrow checker provides compile-time memory safety guarantees. C has no such guarantees. In this methodology, the role of the borrow checker is replaced by **ASan and leak analysis as mandatory verification gates**. Trust advancement requires not just correct behaviour but proven memory safety through tooling:

- **ASan** catches buffer overflows, use-after-free, double-free, and other memory errors at runtime
- **Valgrind** catches memory leaks and uninitialised memory access
- **CPython debug builds** catch reference count errors

These tools are not optional aids — they are the mechanism by which C code earns trust. A conversion that passes correctness tests but has not been run under ASan and Valgrind has not been verified.

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
| **Tight** | Confirm every step | Present all evidence (including ASan/Valgrind output) | Continuous |
| **Gate** | Run autonomously | Present evidence at gate (ASan/Valgrind must be clean) | At Assess only |
| **Batch** | Run autonomously, multiple | Present batch evidence (aggregate ASan/Valgrind results) | At batch Assess |
| **Review** | Run continuously | Flag anomalies only (ASan/Valgrind failures always flag) | On exception |

### Transitions

**Transitions are earned, not assumed.**

| Transition | Requirement |
|-----------|------------|
| Tight → Gate | N consecutive successes where human review found no issues the evidence gate missed — including no ASan/Valgrind issues the automated gate missed. N is project-dependent — ask the human. |
| Gate → Batch | Further consecutive successes at Gate level. Supervisor may batch-assign. |
| Batch → Review | Extensive track record. Mature measurement infrastructure. Human explicitly approves. |
| Any → Tight | Single failure where oversight level was insufficient — the human discovers a problem the evidence gate missed. **Any ASan or Valgrind failure that reached Assess without being caught reverts trust to Tight immediately.** |

**Transitions are reversible.** A single conversion where the oversight level was insufficient — where the human discovers a problem the evidence gate missed — reverts the level. Trust is slow to build and fast to lose.

**The gradient applies per conversion type, not globally.** `tp_getattro` replacements may earn Gate level while `tp_call` replacements remain at Tight. Each domain of conversion builds its own trust independently.

---

## The Epistemic Garbage Collector

Raw conversion records accumulate. Patterns hide in the noise. Without periodic compression, the evidence base becomes unwieldy and drift goes undetected.

### The Mechanism

Every three conversion workers, the supervisor must:

1. **Spawn a compression worker.** This is a pure role — it reads all conversion records in `.nbs/terminal-weathering/conversions/`, extracts patterns (which slot replacements succeed, which fail, common pitfalls, useful techniques), and writes compressed patterns to `patterns.md`. It does not make decisions. It summarises.

2. **Run `/nbs`.** The standard NBS audit checks goal alignment, falsifiability discipline, and drift detection. This is separate from the compression worker — compression handles patterns, `/nbs` handles epistemics.

3. **Reset the counter.** `workers_since_check` in `status.md` returns to zero.

### Why Every Three

Frequent enough to catch drift before it compounds. Infrequent enough not to dominate the work. This matches the NBS teams self-check cadence.

### What Good Compression Looks Like

- "Three out of four `tp_getattro` replacements on `nn.Module` subclasses showed >20% reduction in per-call overhead. The exception was a type with a complex descriptor protocol that required MRO traversal even in C."
- "All slot replacements required explicit reference count management for the return value. Two out of five initial implementations had refcount leaks caught by Valgrind."

### What Bad Compression Looks Like

- "Conversions are going well." (No falsifiable content.)
- "C is faster than Python." (Ethos, not evidence.)

---

## NBS Teams Integration

For codebases at scale — millions of lines — terminal weathering maps onto the supervisor/worker pattern.

### Supervisor

The supervisor holds the terminal goal, the ranked candidate list, and the evidence gates.

**Responsibilities:**
- Maintain `status.md`, `candidates.md`, `trust-levels.md`
- Select candidates and assign individual type slot conversions to workers
- Adjudicate at the Assess phase — workers report evidence (including ASan/Valgrind results), the supervisor decides
- Track trust levels and adjust oversight accordingly
- Run the epistemic garbage collector every three workers
- Escalate to the human when uncertain

**The supervisor does not write C extensions.** It delegates, monitors, and decides.

### Conversion Workers

Each worker executes one type slot conversion on an isolated `weathering/<type>/<slot>` branch.

**Responsibilities:**
- Execute the full Weather phase, including mandatory ASan and leak analysis
- Build the C extension via `setuptools` with appropriate compiler flags (`-fsanitize=address` for ASan builds)
- Record observations in the conversion record
- Return evidence to the supervisor at Assess, including ASan and Valgrind output
- Operate within the trust level assigned by the supervisor

**Workers do not adjudicate.** They report evidence. The supervisor (and ultimately the human) decides.

### Compression Worker

A periodic, pure role that distils raw learnings into compressed patterns. Spawned by the supervisor every three conversion workers as part of the epistemic garbage collector.

**Responsibilities:**
- Read all conversion records
- Extract patterns: which slot replacements succeed, which fail, common refcount pitfalls, ASan findings, useful CPython API techniques
- Write compressed patterns to `patterns.md`

**The compression worker does not make decisions.** It summarises.

---

## Evidence Gates

### What Good Evidence Looks Like

- Benchmarks run under realistic load, not synthetic microbenchmarks alone
- Statistical distributions, not single-run numbers
- Memory measurements under sustained operation, not just peak
- **ASan output showing zero errors** across the full test suite
- **Valgrind output showing zero extension-allocated leaks**
- **Refcount verification** under CPython debug build (`--with-pydebug`)
- Edge cases explicitly tested — the ones the Python dispatch chain handled implicitly
- Total system impact measured, not just isolated slot performance
- Comparison conditions identical to baseline (same hardware, same data, same load)

### What Bad Evidence Looks Like

- "It feels faster" (not measured)
- A single benchmark run showing 2x improvement (not statistically significant)
- Microbenchmark in isolation without system-level measurement (does not capture integration costs)
- Missing edge case coverage (the Python dispatch chain might handle cases the C slot does not)
- Different conditions from baseline (invalidates comparison)
- **ASan not run** ("it compiled cleanly" is not evidence of memory safety)
- **Valgrind not run** ("I checked the refcounts manually" is not evidence of leak freedom)
- **ASan suppressions without justification** (suppressing errors is hiding evidence)

### Failed Conversions as Positive Outcomes

A conversion that fails the evidence gate teaches something. Document it:

- **What was the hypothesis?** "Replacing `tp_getattro` on `Module` with a C function will reduce per-call overhead by 50%."
- **What did the evidence show?** "Per-call overhead reduced by 8%, within noise. The dispatch chain is only three levels deep for this type — most overhead is in the descriptor protocol, which remains in Python."
- **What did we learn?** "Shallow dispatch chains are poor candidates. Target types where the chain depth exceeds five levels."

This is more valuable than a successful conversion that nobody examined critically. A conversion log showing 100% success rate is either dishonest or insufficiently ambitious.
