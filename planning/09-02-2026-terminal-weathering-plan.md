# Terminal Weathering: Plan

**Date**: 09-02-2026
**Project**: terminal-weathering
**Location**: nbs-framework concept/tool addition

## Pivot: Rust to C (09-02-2026)

### Evidence

Four leaf functions were converted to Rust via PyO3. Correctness passed (52/52, full suite 88/88). ABBA counterbalanced benchmark showed no significant performance effect: mean -1.4%, 95% CI [-5.4%, +2.5%], t = -0.82, p > 0.05. The speed-bump experiment showed 30.4% QPS sensitivity at function entry — the call protocol dispatch chain, not function bodies.

### Diagnosis

Replacing function bodies with Rust preserves the entire CPython dispatch chain (`tp_getattro` → `slot_tp_getattr_hook` → `_PyType_Lookup` → `call_attribute` → frame setup → body → teardown). The body (~50ns→~30ns) is a small fraction of total call cost (~130ns). The 30.4% sensitivity is in the dispatch machinery, which PyO3 cannot access.

### Decision

Shift the unit of work from **function bodies** to **call protocol paths** (type slots). Implementation language shifts from Rust/PyO3 to C against CPython's type API, because type slot replacement (`tp_getattro`, `tp_call`, etc.) requires direct C access that PyO3 does not provide. ASan and Valgrind replace Rust's borrow checker as the memory safety mechanism.

### What is preserved

- The weathering methodology (six phases, evidence gates, falsifiability)
- The correctness test suite (52 tests define the behavioural contract)
- The trust gradient (earned transitions, reversible on failure)
- The epistemic garbage collector (compression every three workers)

### What changes

- Unit of work: function body → type slot / call protocol path
- Implementation language: Rust/PyO3 → C against CPython type API
- Build tooling: maturin/cargo → setuptools/gcc/clang
- Memory safety mechanism: borrow checker → ASan + Valgrind + refcount audit
- Branch naming: `weathering/<module>/<function>` → `weathering/<type>/<slot>`

See `.nbs/reference/weathering-at-the-right-layer.md` for the full analysis.

---

## Terminal Goal

A falsifiable, NBS-grounded methodology for progressively replacing CPython call protocol paths with C type slot implementations, operating at the granularity of individual type slots, driven by evidence of benefit, applicable to codebases of millions of lines.

## The Metaphor

Geological weathering transforms rock gradually through sustained exposure:
- Water penetrates along **existing cracks** (pain points, refactor candidates)
- Weaker material dissolves, **stronger material remains**
- Works from the **surface inward** (leaves of the dispatch graph first)
- The process is **irreversible once proven** but **reversible before commitment**
- No blasting. No demolition. Patient transformation.

## What This Is NOT

- A "rewrite in C" methodology (that's demolition, not weathering)
- A claim that C is better than Python (that's ethos, not evidence)
- A one-size-fits-all migration guide (each conversion must prove itself)

## Methodology Structure

### Phase 1: Survey (Discovery)

Identify existing cracks. Not "what could be C" but "what is already hurting" — and where in the dispatch chain the cost lies.

- Profile performance (CPU, latency hotspots)
- Profile memory (allocation patterns, peak usage, leaks)
- Analyse call protocols: identify high-hit-count dispatch chains using `perf`
- Identify code marked for refactor / known-problematic
- Map the dispatch graph; identify leaf type slots

**Exit criterion**: Ranked list of candidates with quantified pain, identifying type slots and dispatch chain depths.

**Falsifier**: If no measurable pain exists in dispatch chains, stop. If pain is in function bodies only, terminal weathering is the wrong tool.

### Phase 2: Expose (Leaf Selection)

Select a candidate from the ranked list. Must be:
- A type slot or call protocol path (not a function body in isolation)
- A leaf (no deeper Python dispatch dependencies) or near-leaf
- Measurably problematic (not "might have dispatch overhead")
- Small enough to convert in one verification cycle

**Exit criterion**: Single type slot selected with baseline measurements.

**Falsifier**: If the candidate cannot be isolated as a leaf in the dispatch graph, it is not ready. Find a different candidate or decompose further.

### Phase 3: Weather (Convert)

Apply the verification cycle:
1. **Design**: C implementation replacing the target type slot directly against CPython's type API
2. **Plan**: Identify what could go wrong (reference counting, exception propagation, descriptor protocol, MRO invalidation, thread safety)
3. **Deconstruct**: Break into testable steps
4. **Test**: Write tests that exercise Python API through C backend; write benchmarks. **Mandatory: ASan verification, Valgrind leak analysis, refcount audit.**
5. **Code**: Implement C extension via setuptools, overlaying type slots
6. **Document**: Record baseline vs post-conversion measurements, including ASan/Valgrind output

**Exit criterion**: Tests pass, ASan clean, leak-free, benchmarks collected, Python API unchanged.

**Falsifier**: "This conversion provides measurable benefit" — attempt to falsify by:
- Benchmarking under realistic load (not microbenchmarks)
- Testing edge cases the Python dispatch chain handled implicitly
- Measuring total system impact (not just slot-level)

### Phase 4: Assess (Evidence Gate)

Five mandatory checks: correctness, ASan clean, leak-free, refcount clean, performance.

Three performance outcomes:
1. **Benefit confirmed**: Proceed. Mark conversion as permanent.
2. **Benefit unclear**: More measurement needed. Do not proceed until resolved.
3. **Benefit falsified**: Revert. Document what was learned. This is a positive outcome — we avoided waste.

**Falsifier**: If we cannot distinguish outcomes 1-3 with evidence, our measurement methodology is wrong. Fix that first.

### Phase 5: Advance (Move Inward)

With proven type slot replacements, the dispatch graph changes:
- New slots become accessible (same slot on related types, or different slots on proven types)
- Patterns emerge: which types of slot replacement yield benefit, which don't
- Rules-of-thumb develop, but each conversion still passes its own evidence gate

**Exit criterion**: Next candidate selected based on updated dispatch graph and evidence patterns.

### Phase 6: Fuse (Consolidation)

When sufficient slot coverage exists within a type:
- Consider defining the type entirely in C, removing the Python dispatch chain
- This is a separate verification cycle with its own evidence gate
- Risks: subclassing, dynamic dispatch, monkey-patching, test fixtures, MRO interactions

**Falsifier**: "Removing the Python dispatch chain does not break any consumer" — test exhaustively.

## NBS Alignment

| NBS Concept | Application in Terminal Weathering |
|-------------|-----------------------------------|
| Falsifiability | Each conversion carries "provides measurable benefit" as falsifiable claim |
| Verification Cycle | Each conversion is one full cycle: Design → Plan → Deconstruct → [Test → Code → Document] |
| Zero-Code Contract | Engineer selects targets and defines "benefit"; AI implements and reports evidence |
| Bullshit Detection | Report failed conversions. Negative results reveal which dispatch chains resist C replacement |
| Rhetoric | Guard against "C is faster" as ethos claim. Demand evidence per conversion |
| Goals | Terminal goal is system improvement, not language replacement. Language is instrumental |

## NBS Teams Integration

For scale (millions of lines), supervisor/worker pattern:
- **Supervisor**: Maintains ranked candidate list, assigns type slot conversions to workers, aggregates evidence, detects patterns across conversions
- **Workers**: Execute individual verification cycles (one type slot conversion each), including mandatory ASan/leak analysis
- Evidence gates remain at supervisor level — workers report, supervisor decides

## Deliverables

1. ~~`concepts/terminal-weathering.md` — the methodology as an NBS concept document~~ **Done**
2. ~~`claude_tools/nbs-weather.md` — a tool/skill for executing the methodology~~ **Done** (as `nbs-terminal-weathering.md`)
3. ~~Update `README.md` — add terminal weathering to concepts/tools list~~ **Done**
4. `terminal-weathering/docs/methodology.md` — detailed reference, updated for C + type slots
5. `terminal-weathering/docs/getting-started.md` — prerequisites and first-use guide, updated for C toolchain
6. Pivot all documentation from Rust/PyO3 to C against CPython's type API — **in progress (09-02-2026)**

## ~~Original Deliverables (Rust/PyO3 — Superseded)~~

The original plan targeted Rust via PyO3 as the implementation language. Items 1-3 above were completed under this assumption. The pivot to C (documented in the section above) supersedes the Rust-specific aspects while preserving the methodology, test suite, and NBS alignment. The Rust work validated the approach; the evidence showed C is necessary for this specific use case.

## Open Questions

1. ~~Should the tool automate call graph analysis, or is that out of scope for v1?~~ Dispatch chain analysis (via `perf`) is now a core part of the Survey phase.
2. Should there be a `templates/weathering-report.md` for standardised conversion evidence?
3. ~~How does this interact with nbs-investigation? Is each conversion an investigation?~~ Each conversion is a verification cycle, not an investigation. Investigations are for testing hypotheses before committing to a direction.

## Falsification of This Plan

This plan is wrong if:
- The phases cannot be applied to a real codebase (test against a concrete example)
- The evidence gates are too coarse or too fine to be practical
- The methodology adds overhead that exceeds the benefit of rigour (becoming the thing it warns against)
- Type slot replacement does not produce measurable performance improvement (the dispatch chain hypothesis is false)
- ASan/Valgrind gates are insufficient to catch memory safety issues in practice
