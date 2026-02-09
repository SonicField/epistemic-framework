# Terminal Weathering: Plan

**Date**: 09-02-2026
**Project**: terminal-weathering
**Location**: nbs-framework concept/tool addition

## Terminal Goal

A falsifiable, NBS-grounded methodology for progressively replacing Python source with Rust native code, operating at the granularity of individual functions/classes, driven by evidence of benefit, applicable to codebases of millions of lines.

## The Metaphor

Geological weathering transforms rock gradually through sustained exposure:
- Water penetrates along **existing cracks** (pain points, refactor candidates)
- Weaker material dissolves, **stronger material remains**
- Works from the **surface inward** (leaves of the call graph first)
- The process is **irreversible once proven** but **reversible before commitment**
- No blasting. No demolition. Patient transformation.

## What This Is NOT

- A "rewrite in Rust" methodology (that's demolition, not weathering)
- A claim that Rust is better than Python (that's ethos, not evidence)
- A one-size-fits-all migration guide (each conversion must prove itself)

## Methodology Structure (Proposed)

### Phase 1: Survey (Discovery)

Identify existing cracks. Not "what could be Rust" but "what is already hurting."

- Profile performance (CPU, latency hotspots)
- Profile memory (allocation patterns, peak usage, leaks)
- Identify code marked for refactor / known-problematic
- Map the call graph; identify leaf functions/classes

**Exit criterion**: Ranked list of candidates with quantified pain.

**Falsifier**: If no measurable pain exists, stop. There is nothing to weather.

### Phase 2: Expose (Leaf Selection)

Select a candidate from the ranked list. Must be:
- A leaf (no deeper Python dependencies) or near-leaf
- Measurably problematic (not "might be slow")
- Small enough to convert in one verification cycle

**Exit criterion**: Single function/class selected with baseline measurements.

**Falsifier**: If the candidate cannot be isolated as a leaf, it is not ready. Find a different candidate or decompose further.

### Phase 3: Weather (Convert)

Apply the verification cycle:
1. **Design**: Rust implementation matching Python API exactly (PyO3/cffi)
2. **Plan**: Identify what could go wrong (semantics drift, edge cases, GIL interactions)
3. **Deconstruct**: Break into testable steps
4. **Test**: Write tests that exercise Python API through Rust backend; write benchmarks
5. **Code**: Implement Rust, overlay on Python
6. **Document**: Record baseline vs post-conversion measurements

**Exit criterion**: Tests pass, benchmarks collected, Python API unchanged.

**Falsifier**: "This conversion provides measurable benefit" — attempt to falsify by:
- Benchmarking under realistic load (not microbenchmarks)
- Testing edge cases the Python handled implicitly
- Measuring total system impact (not just function-level)

### Phase 4: Assess (Evidence Gate)

Three outcomes:
1. **Benefit confirmed**: Proceed. Mark conversion as permanent.
2. **Benefit unclear**: More measurement needed. Do not proceed until resolved.
3. **Benefit falsified**: Revert. Document what was learned. This is a positive outcome — we avoided waste.

**Falsifier**: If we cannot distinguish outcomes 1-3 with evidence, our measurement methodology is wrong. Fix that first.

### Phase 5: Advance (Move Inward)

With proven leaf conversions, the next layer becomes accessible:
- Former near-leaves may now be leaves (their dependencies are now Rust)
- Patterns emerge: which types of conversion yield benefit, which don't
- Rules-of-thumb develop, but each conversion still passes its own evidence gate

**Exit criterion**: Next candidate selected based on updated call graph and evidence patterns.

### Phase 6: Fuse (Consolidation)

When sufficient coverage exists within a module:
- Consider removing the Python layer entirely
- This is a separate verification cycle with its own evidence gate
- Risks: Python-side consumers, dynamic dispatch, monkey-patching, test fixtures

**Falsifier**: "Removing the Python layer does not break any consumer" — test exhaustively.

## NBS Alignment

| NBS Concept | Application in Terminal Weathering |
|-------------|-----------------------------------|
| Falsifiability | Each conversion carries "provides measurable benefit" as falsifiable claim |
| Verification Cycle | Each conversion is one full cycle: Design → Plan → Deconstruct → [Test → Code → Document] |
| Zero-Code Contract | Engineer selects targets and defines "benefit"; AI implements and reports evidence |
| Bullshit Detection | Report failed conversions. Negative results reveal which Python patterns resist Rust replacement |
| Rhetoric | Guard against "Rust is faster" as ethos claim. Demand evidence per conversion |
| Goals | Terminal goal is system improvement, not language replacement. Language is instrumental |

## NBS Teams Integration

For scale (millions of lines), supervisor/worker pattern:
- **Supervisor**: Maintains ranked candidate list, assigns conversions to workers, aggregates evidence, detects patterns across conversions
- **Workers**: Execute individual verification cycles (one conversion each)
- Evidence gates remain at supervisor level — workers report, supervisor decides

## Deliverables

1. `concepts/terminal-weathering.md` — the methodology as an NBS concept document
2. `claude_tools/nbs-weather.md` — a tool/skill for executing the methodology
3. Update `README.md` — add terminal weathering to concepts/tools list

## Open Questions

1. Should the tool automate call graph analysis, or is that out of scope for v1?
2. Should there be a `templates/weathering-report.md` for standardised conversion evidence?
3. How does this interact with nbs-investigation? Is each conversion an investigation?

## Falsification of This Plan

This plan is wrong if:
- The phases cannot be applied to a real codebase (test against a concrete example)
- The evidence gates are too coarse or too fine to be practical
- The methodology adds overhead that exceeds the benefit of rigour (becoming the thing it warns against)
