# Terminal Weathering

## What It Is

Terminal weathering is a methodology for progressively replacing Python with Rust native code. It is not a rewrite strategy. It is the NBS pillars — goals, falsifiability, verification cycle, zero-code contract — applied to language migration.

The name carries a dual meaning:

- **Terminal**: the work uses terminal-based tools — CLI, pty-session, tmux. The target is a terminal goal: a measurable system improvement, not "rewrite in Rust."
- **Weathering**: geological weathering transforms rock from the surface inward. Water finds existing cracks, dissolves weaker material, leaves stronger structures behind. Weathering of iron produces *rust*. The methodology works from the leaves of the call graph inward, progressively producing Rust where Python was.

---

## The Problem It Solves

"Rewrite in Rust" is demolition. It replaces an understood system with an unproven one in a single commitment, without requiring evidence at any stage. The failure mode is predictable:

| Phase | What Happens |
|-------|-------------|
| Announcement | "We are rewriting in Rust for performance" |
| Honeymoon | Early modules convert easily; team reports progress |
| Plateau | Complex modules resist; edge cases multiply |
| Sunk cost | Too much invested to stop; too little working to ship |
| Outcome | Two half-working systems instead of one working one |

The root cause is not Rust. It is the absence of evidence gates. No individual conversion was required to prove its value before commitment.

---

## How It Works

Terminal weathering operates in iterative cycles. Each cycle processes one candidate — a single function or class — through six phases:

| Phase | Purpose |
|-------|---------|
| **Survey** | Profile the system. Find what is actually hurting. |
| **Expose** | Select one leaf candidate with measured pain. |
| **Weather** | Apply the verification cycle: Design, Plan, Deconstruct, Test, Code, Document. |
| **Assess** | The evidence gate. Benefit confirmed, unclear, or falsified. Three outcomes, no others. |
| **Advance** | Update the call graph. Select the next candidate. |
| **Fuse** | When contiguous coverage exists, consider removing the Python layer entirely. |

Every conversion carries a falsifiable claim: "this provides measurable benefit." The Assess phase exists to attempt falsification. A conversion that fails this gate is reverted — and that reversion is a positive outcome, not a failure. It means the methodology is working.

---

## Relation to NBS

Terminal weathering is an **application** of the NBS pillars, not a pillar itself.

| Pillar | Application in Terminal Weathering |
|--------|-----------------------------------|
| Goals | The terminal goal is system improvement. Language replacement is instrumental. |
| Falsifiability | Each conversion carries a falsifiable hypothesis with an explicit falsifier. |
| Rhetoric | "Rust is faster" is Ethos. "This function runs in 3ms instead of 12ms under production load" is Logos. Only the second is acceptable. |
| Bullshit Detection | Failed conversions are reported. A 100% success rate is either dishonest or insufficiently ambitious. |
| Verification Cycle | Each conversion is one full cycle. No shortcuts. |
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
