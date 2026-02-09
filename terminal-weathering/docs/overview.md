# Terminal Weathering

## What It Is

Terminal weathering is a methodology for progressively replacing Python call protocol paths with C extensions written directly against CPython's type API. It is not a rewrite strategy. It is the NBS pillars — goals, falsifiability, verification cycle, zero-code contract — applied to call protocol optimisation.

The name carries a dual meaning:

- **Terminal**: the work uses terminal-based tools — CLI, pty-session, tmux. The target is a terminal goal: a measurable system improvement, not "rewrite in another language."
- **Weathering**: geological weathering transforms rock from the surface inward. Water finds existing cracks, dissolves weaker material, leaves stronger structures behind. The methodology works from the leaf type slots of the dispatch chain inward, progressively replacing Python's call protocol with direct C implementations where evidence shows the overhead justifies it.

---

## The Problem It Solves

"Rewrite for performance" is demolition. It replaces an understood system with an unproven one in a single commitment, without requiring evidence at any stage. The failure mode is predictable:

| Phase | What Happens |
|-------|-------------|
| Announcement | "We are rewriting for performance" |
| Honeymoon | Early modules convert easily; team reports progress |
| Plateau | Complex modules resist; edge cases multiply |
| Sunk cost | Too much invested to stop; too little working to ship |
| Outcome | Two half-working systems instead of one working one |

The root cause is not the target language. It is the absence of evidence gates. No individual conversion was required to prove its value before commitment.

---

## The Journey: Rust to C

Terminal weathering began with Rust/PyO3 as the replacement language. This was a defensible first choice — Rust's borrow checker provides compile-time memory safety, and PyO3 offers clean Python interop.

Four leaf functions were converted to Rust. Correctness passed (52/52). ABBA benchmarks showed no significant performance effect (mean −1.4%, p > 0.05). The speed-bump experiment revealed why: the overhead is in CPython's call protocol — type slot dispatch, MRO walk, descriptor protocol, bound method creation, frame setup — not in function bodies. PyO3 replaces bodies but cannot access type slots.

Replacing the dispatch chain requires writing C directly against CPython's type API. This is a technical necessity, not a language preference. The Rust work validated the methodology, produced reusable correctness tests, and identified the correct target layer.

C lacks Rust's compile-time memory safety. To compensate, AddressSanitizer (ASan), memory leak analysis, and reference count verification are mandatory in every conversion's correctness phase.

Independent validation from the SOMA project confirmed the pattern: a Rust/PyO3 extension was 6% slower than pure Python on fine-grained field access, while a C extension was 2.06x faster than Rust — uniformly across all operations. See [evidence/soma-weathering.md](../evidence/soma-weathering.md) and [evidence/weathering-at-the-right-layer.md](../evidence/weathering-at-the-right-layer.md) for the full data.

See [terminal-weathering.md](../concepts/terminal-weathering.md) for the full analysis.

---

## How It Works

Terminal weathering operates in iterative cycles. Each cycle processes one candidate — a single call protocol path (type slot) — through six phases:

| Phase | Purpose |
|-------|---------|
| **Survey** | Profile the dispatch chain. Find where call protocol overhead is measurable. |
| **Expose** | Select one leaf type slot with measured overhead. |
| **Weather** | Apply the verification cycle: Design, Plan, Deconstruct, Test (with ASan), Code, Document. |
| **Assess** | The evidence gate. Benefit confirmed, unclear, or falsified. Three outcomes, no others. |
| **Advance** | Update the slot dependency map. Select the next candidate. |
| **Fuse** | When contiguous slot coverage exists, consider removing the Python layer entirely. |

Every conversion carries a falsifiable claim: "this slot replacement provides measurable benefit." The Assess phase exists to attempt falsification. A conversion that fails this gate is reverted — and that reversion is a positive outcome, not a failure. It means the methodology is working.

---

## Relation to NBS

Terminal weathering is an **application** of the NBS pillars, not a pillar itself.

| Pillar | Application in Terminal Weathering |
|--------|-----------------------------------|
| Goals | The terminal goal is system improvement. Language replacement is instrumental. |
| Falsifiability | Each conversion carries a falsifiable hypothesis with an explicit falsifier. |
| Rhetoric | "C is faster" is Ethos. "Replacing `tp_getattro` reduces dispatch from 130ns to 50ns under production load" is Logos. Only the second is acceptable. |
| Bullshit Detection | Failed conversions are reported. ASan findings are reported. A 100% success rate is either dishonest or insufficiently ambitious. |
| Verification Cycle | Each conversion is one full cycle. ASan and leak analysis are part of Test. No shortcuts. |
| Zero-Code Contract | The human defines "benefit." The AI implements and reports evidence. Neither trusts the other's assertions. |

---

## The Trust Gradient

Human oversight does not scale to every step of every conversion. But removing oversight without evidence is negligence.

Terminal weathering defines four oversight levels — Tight, Gate, Batch, Review — earned through consecutive successes and reverted on a single failure. Trust is slow to build and fast to lose. The gradient applies per conversion type, not globally.

See [methodology.md](methodology.md) for details.

---

## The Epistemic Garbage Collector

Every three conversion workers, the supervisor spawns a compression worker to distil raw learnings into patterns, then runs `/nbs` for goal alignment and drift detection. This prevents epistemic debt from accumulating unchecked.

See [methodology.md](methodology.md) for the full mechanism.
