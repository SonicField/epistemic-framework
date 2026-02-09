# Terminal Weathering

Rock does not shatter because someone decided it should be gravel. It transforms through sustained exposure — water finding existing cracks, dissolving weaker material, leaving stronger structures behind. The process is gradual, irreversible once proven, and indifferent to impatience.

Code transforms the same way, or it does not transform at all.

## The Anti-Pattern

"Rewrite in Rust" is demolition. It assumes the replacement is better before evidence exists. It replaces an understood system with an unproven one in a single commitment. It is Ethos — trusting the authority of a language's reputation over measured outcomes.

The failure mode is predictable:

| Phase | What happens |
|-------|-------------|
| Announcement | "We are rewriting in Rust for performance" |
| Honeymoon | Early modules convert easily; team reports progress |
| Plateau | Complex modules resist; edge cases multiply; Python semantics prove non-trivial |
| Sunk cost | Too much invested to stop; too little working to ship |
| Outcome | Two half-working systems instead of one working one |

The root cause is not Rust. It is the absence of evidence gates. No individual conversion was required to prove its value. The decision was made once, at the top, and everything downstream was committed before falsification could occur.

## The Metaphor

Geological weathering operates on three principles that map directly:

1. **Existing cracks first.** Water does not attack solid rock. It finds joints, faults, grain boundaries — places where the material is already weak. In code: performance hotspots, memory-bound loops, functions already marked for refactor.

2. **Surface inward.** Weathering works from exposed surfaces toward the interior. In code: leaf functions first — those with no deeper Python dependencies — then progressively inward as leaves convert.

3. **Differential erosion.** Weaker material dissolves; stronger material remains. In code: some Python patterns resist Rust replacement because they derive genuine value from Python's dynamism. This is information, not failure.

## The Phases

Terminal weathering is iterative, not linear. Each cycle processes one candidate through six phases.

### Survey

Identify existing cracks. Not "what could be Rust" but "what is already hurting."

- Profile performance: CPU hotspots, latency distributions
- Profile memory: allocation patterns, peak usage, leaks
- Map the call graph; identify leaf functions and classes
- Identify code already marked problematic or scheduled for refactor

**Exit criterion**: Ranked list of candidates with quantified pain.

**Falsifier**: If no measurable pain exists, stop. There is nothing to weather.

### Expose

Select a single candidate from the ranked list. It must be:

- A leaf or near-leaf in the call graph
- Measurably problematic — not "might be slow" but "is slow, here is the profile"
- Small enough to convert in one verification cycle

**Exit criterion**: Single function or class selected with baseline measurements recorded.

**Falsifier**: If the candidate cannot be isolated as a leaf, it is not ready. Decompose further or choose another.

### Weather

Apply the verification cycle to the selected candidate:

1. **Design**: Rust implementation matching the Python API exactly (PyO3/cffi)
2. **Plan**: Identify what could go wrong — semantics drift, edge cases, GIL interactions
3. **Deconstruct**: Break into testable steps
4. **Test**: Write tests exercising the Python API through the Rust backend; write benchmarks
5. **Code**: Implement Rust, overlay on Python — the Python layer remains until proven redundant
6. **Document**: Record baseline versus post-conversion measurements

**Exit criterion**: Tests pass, benchmarks collected, Python API unchanged.

**Falsifier**: "This conversion provides measurable benefit" — attempt to falsify by benchmarking under realistic load, testing edge cases the Python handled implicitly, and measuring total system impact rather than isolated function performance.

### Assess

The evidence gate. Three outcomes, no others:

1. **Benefit confirmed**: Measurements show improvement beyond noise. Mark conversion as permanent. Proceed.
2. **Benefit unclear**: Measurements are ambiguous. More data needed. Do not proceed until resolved.
3. **Benefit falsified**: Measurements show no improvement, or regression. Revert. Document what was learned.

Outcome 3 is not failure. It is the methodology working. A reverted conversion that taught us "this Python pattern resists Rust replacement because of X" is more valuable than a committed conversion nobody measured.

**Falsifier**: If we cannot distinguish outcomes 1–3 with evidence, our measurement methodology is wrong. Fix that before proceeding with any conversion.

### Advance

With proven leaf conversions, the next layer becomes accessible:

- Former near-leaves may now be leaves, their dependencies already in Rust
- Patterns emerge: which types of conversion yield benefit, which do not
- Rules of thumb develop — but each conversion still passes its own evidence gate

No blanket rules. "String processing converts well" is a hypothesis to test per candidate, not a policy to apply wholesale.

**Exit criterion**: Next candidate selected based on updated call graph and accumulated evidence.

### Fuse

When sufficient contiguous coverage exists within a module, consider removing the Python layer entirely. This is a separate verification cycle with its own evidence gate.

Risks specific to fusion: Python-side consumers, dynamic dispatch, monkey-patching in test fixtures, implicit interface contracts.

**Falsifier**: "Removing the Python layer does not break any consumer" — test exhaustively. If any consumer breaks, the Python layer remains.

## The Trust Gradient

Human oversight is expensive. Applying full oversight to every conversion does not scale. But removing oversight without evidence is negligence.

Terminal weathering defines four oversight levels, ordered from tightest to loosest:

| Level | Oversight | When |
|-------|-----------|------|
| Tight | Human reviews every step of every conversion | Initial conversions; no evidence base yet |
| Gate | Human reviews evidence at Assess phase only | Pattern of successful conversions established |
| Batch | Human reviews evidence for batches of conversions | Strong evidence base; consistent patterns |
| Review | Human spot-checks; AI flags anomalies | Extensive track record; mature measurement infrastructure |

**Transitions are earned, not assumed.** Moving from Tight to Gate requires N consecutive conversions where the human's review found no issues the evidence gates missed. The specific N is project-dependent.

**Transitions are reversible.** A single conversion where oversight level was insufficient — the human discovers a problem the evidence gate missed — reverts the level. Trust is slow to build and fast to lose.

**The gradient applies per conversion type, not globally.** String processing conversions may earn Gate level while numerical code remains at Tight. Each domain of conversion builds its own trust independently.

## NBS Alignment

Terminal weathering is not a new methodology. It is the existing NBS pillars applied to language migration.

| Pillar | Application |
|--------|-------------|
| Goals | The terminal goal is system improvement. Language replacement is instrumental. If the system is not measurably better, the conversion has no purpose |
| Falsifiability | Each conversion carries "this provides measurable benefit" as a falsifiable claim. The Assess phase exists to attempt falsification |
| Rhetoric | "Rust is faster" is Ethos. "This function runs in 3ms instead of 12ms under production load" is Logos. Only the second is acceptable as evidence |
| Bullshit Detection | Report failed conversions. Report ambiguous results. A conversion log showing 100% success rate is either dishonest or insufficiently ambitious |
| Verification Cycle | Each conversion is one full cycle: Design, Plan, Deconstruct, Test, Code, Document. No shortcuts |
| Zero-Code Contract | The Engineer selects targets and defines "benefit". The Machinist implements and reports evidence. Neither trusts the other's assertions |

## NBS Teams Integration

For codebases at scale — millions of lines — terminal weathering maps onto the supervisor/worker pattern:

**Supervisor** maintains the ranked candidate list, assigns individual conversions to workers, aggregates evidence across conversions, detects cross-conversion patterns, and holds the evidence gates. The Supervisor decides whether a conversion passes the Assess phase — workers report, they do not adjudicate.

**Workers** execute individual verification cycles. One conversion per worker. Each worker operates on an isolated branch, runs the full Weather phase, and returns evidence to the Supervisor.

The trust gradient applies at the Supervisor level. As evidence accumulates, the Supervisor may batch-assign conversions at Gate or Batch oversight levels, but retains the ability to revert to Tight for any conversion type where evidence is thin.

## The Practical Questions

1. What is actually hurting? Can I point to a profile, a measurement, a user complaint — not a hunch?
2. Is this candidate a leaf? If not, what must convert first?
3. What would prove this conversion does not help? Have I tried to prove it?
4. Am I converting because of evidence, or because "Rust is faster" and I have not questioned that?
5. What oversight level has this type of conversion earned? What evidence supports that level?
6. What failed conversions have I documented? What did they teach me?

---

## Pillar Check

Have you read all pillars in this session?

- goals.md
- falsifiability.md
- rhetoric.md
- bullshit-detection.md
- verification-cycle.md
- zero-code-contract.md
- pte.md
- terminal-weathering.md *(you are here)*

If you cannot clearly recall reading each one, read them now. Next: `goals.md`
