# Getting Started with Terminal Weathering

## Prerequisites

1. **NBS framework installed.** Follow the instructions in the main [getting-started guide](../../docs/getting-started.md). The `/nbs-terminal-weathering` command must be available in Claude Code.

2. **Rust toolchain.** Install via [rustup](https://rustup.rs/). You need `cargo`, `rustc`, and the standard library.

3. **PyO3.** The primary bridge between Python and Rust. Install with:
   ```bash
   pip install maturin
   ```
   Maturin handles building PyO3 extensions and packaging them as Python wheels.

4. **Profiling tools.** At least one of:
   - `py-spy` — sampling profiler for Python (recommended)
   - `cProfile` — built-in Python profiler
   - `tracemalloc` — built-in memory tracker
   - `memray` — detailed memory profiler

---

## Starting a Session

Run the command:

```
/nbs-terminal-weathering
```

The tool detects context automatically and dispatches to the correct phase. On first run with no existing state, it begins with **goal setting**.

### Goal Setting

The tool asks for a terminal goal. This is not "rewrite in Rust." It is a measurable system improvement:

- "Reduce P99 latency from 45ms to 15ms"
- "Reduce peak memory from 2GB to 500MB"
- "Eliminate GIL contention under concurrent load"

If the goal is not falsifiable — "make it faster" without specifying faster than what, by how much, measured how — the tool pushes back. This is deliberate.

### State Creation

Once the goal is confirmed, the tool creates the state directory:

```
.nbs/terminal-weathering/
├── status.md          # Current phase, terminal goal, worker count
├── candidates.md      # Ranked conversion candidates
├── trust-levels.md    # Trust gradient per conversion type
├── patterns.md        # Compressed learnings (initially empty)
└── conversions/       # One file per attempted conversion
```

All state lives in these files. Not in conversation history, not in memory. The tool reads them on every invocation to determine what to do next.

---

## What Happens Next: Survey

With the goal set, the tool moves to the survey phase. It profiles the system to find what is actually hurting — CPU hotspots, memory allocation patterns, latency distributions. The output is a ranked list of candidates with quantified pain.

If profiling reveals no measurable pain, the survey says so. There is nothing to weather. This is an honest outcome, not a failure.

---

## A Single Conversion Cycle

Here is what one cycle looks like end to end, assuming the survey has produced candidates.

### 1. Expose

The tool selects the highest-ranked candidate that is a leaf (no deeper Python dependencies) and measurably problematic. It records baseline measurements and creates a branch:

```bash
git checkout -b weathering/<module>/<function>
```

A conversion record is created in `.nbs/terminal-weathering/conversions/` with the hypothesis, falsifier, and baseline numbers.

### 2. Weather

The verification cycle runs against the candidate:

- **Design** — Rust implementation matching the Python API exactly via PyO3
- **Plan** — identify what could go wrong: semantics drift, edge cases, GIL interactions
- **Deconstruct** — break into testable steps
- **Test** — tests exercising the Python API through the Rust backend, plus benchmarks
- **Code** — implement Rust. The Python layer remains as an overlay until proven redundant
- **Document** — record baseline versus post-conversion measurements

At the initial trust level (Tight), every step is confirmed with the human. As trust is earned, oversight reduces.

### 3. Assess

The evidence gate. Post-conversion benchmarks are compared against baseline under the same conditions. Three outcomes:

| Verdict | What Happens |
|---------|-------------|
| **Benefit confirmed** | Merge branch. Mark conversion permanent. Proceed. |
| **Benefit unclear** | More data needed. Do not merge. |
| **Benefit falsified** | Revert. Document what was learned. Choose next candidate. |

A falsified benefit is the methodology working. "This Python pattern resists Rust replacement because of X" is valuable information.

### 4. Advance

Back on main. The call graph is updated — proven leaf conversions may expose new leaves. The candidate list is re-ranked. The next cycle begins.

---

## Resuming a Session

Run `/nbs-terminal-weathering` again. The tool reads `.nbs/terminal-weathering/status.md`, detects which phase you are in, and resumes from there. No reconfiguration needed.

| Signal | What the Tool Does |
|--------|-------------------|
| No state directory | Starts goal setting |
| Candidates empty | Runs survey |
| On `main`/`master`, candidates ranked | Selects next candidate (Expose) |
| On a `weathering/*` branch | Continues the in-progress conversion (Weather) |
| Conversion complete on branch | Runs the evidence gate (Assess) |
| Back on main, goal not met | Advances to next candidate |
| Terminal goal met | Produces final report |
