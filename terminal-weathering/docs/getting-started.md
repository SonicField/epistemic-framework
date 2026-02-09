# Getting Started with Terminal Weathering

## Prerequisites

1. **NBS framework installed.** Follow the instructions in the main [getting-started guide](../../docs/getting-started.md). The `/nbs-terminal-weathering` command must be available in Claude Code.

2. **C compiler.** GCC or Clang with C11 support. Verify:
   ```bash
   gcc --version    # or clang --version
   ```

3. **CPython development headers.** Required for building C extensions against the CPython type API. Install and verify:
   ```bash
   # Debian/Ubuntu
   sudo apt install python3-dev

   # Fedora/RHEL
   sudo dnf install python3-devel

   # macOS (included with Xcode command line tools)
   xcode-select --install

   # Verify headers are accessible
   python3 -c "import sysconfig; print(sysconfig.get_path('include'))"
   ls "$(python3 -c "import sysconfig; print(sysconfig.get_path('include'))")/Python.h"
   ```

4. **AddressSanitizer (ASan).** Comes built-in with GCC and Clang. Verify:
   ```bash
   echo 'int main() { return 0; }' | gcc -fsanitize=address -x c - -o /dev/null && echo "ASan supported"
   ```

5. **Valgrind.** For memory leak analysis. Install and verify:
   ```bash
   # Debian/Ubuntu
   sudo apt install valgrind

   # Fedora/RHEL
   sudo dnf install valgrind

   # Verify
   valgrind --version
   ```

6. **setuptools.** For building C extensions as Python packages:
   ```bash
   pip install setuptools
   python3 -c "import setuptools; print(setuptools.__version__)"
   ```

7. **Profiling tools.** At least one of:
   - `py-spy` — sampling profiler for Python (recommended)
   - `cProfile` — built-in Python profiler
   - `tracemalloc` — built-in memory tracker
   - `memray` — detailed memory profiler
   - `perf` — Linux performance counters, essential for call protocol analysis:
     ```bash
     # Debian/Ubuntu
     sudo apt install linux-tools-common linux-tools-$(uname -r)

     # Verify
     perf --version
     ```

---

## Starting a Session

Run the command:

```
/nbs-terminal-weathering
```

The tool detects context automatically and dispatches to the correct phase. On first run with no existing state, it begins with **goal setting**.

### Goal Setting

The tool asks for a terminal goal. This is not "rewrite in C." It is a measurable system improvement:

- "Reduce P99 latency from 45ms to 15ms"
- "Reduce peak memory from 2GB to 500MB"
- "Eliminate call protocol overhead for nn.Module attribute access"

If the goal is not falsifiable — "make it faster" without specifying faster than what, by how much, measured how — the tool pushes back. This is deliberate.

### State Creation

Once the goal is confirmed, the tool creates the state directory:

```
.nbs/terminal-weathering/
├── status.md          # Current phase, terminal goal, worker count
├── candidates.md      # Ranked conversion candidates (type slots / call protocol paths)
├── trust-levels.md    # Trust gradient per conversion type
├── patterns.md        # Compressed learnings (initially empty)
└── conversions/       # One file per attempted conversion
```

All state lives in these files. Not in conversation history, not in memory. The tool reads them on every invocation to determine what to do next.

---

## What Happens Next: Survey

With the goal set, the tool moves to the survey phase. It profiles the system to find what is actually hurting — not just CPU hotspots in function bodies, but high-hit-count dispatch chains where CPython's call protocol overhead dominates.

Key activities during survey:
- **Call protocol analysis:** Use `perf` to trace type slot dispatch. Identify which `tp_*` slots are invoked most frequently and measure per-invocation overhead.
- **Dispatch chain mapping:** For each hot slot, trace the full chain (e.g., `tp_getattro` → `slot_tp_getattr_hook` → `_PyType_Lookup` → `call_attribute` → frame setup → body → teardown). Measure time spent in dispatch vs. body.
- **Hit count ranking:** Rank candidates by `(hit_count × per_call_dispatch_overhead)` — total dispatch overhead eliminated by a successful slot replacement.

The output is a ranked list of type slot candidates with quantified pain. If profiling reveals that dispatch overhead is negligible, the survey says so. There is nothing to weather. This is an honest outcome, not a failure.

---

## A Single Conversion Cycle

Here is what one cycle looks like end to end, assuming the survey has produced candidates.

### 1. Expose

The tool selects the highest-ranked candidate — a type slot that is a leaf in the dispatch graph (no deeper Python dispatch dependencies) and measurably problematic. It records baseline measurements and creates a branch:

```bash
git checkout -b weathering/<type>/<slot>
```

A conversion record is created in `.nbs/terminal-weathering/conversions/` with the hypothesis, falsifier, and baseline numbers.

### 2. Weather

The verification cycle runs against the candidate:

- **Design** — C implementation replacing the target type slot directly. Use CPython's type API (`tp_getattro`, `PyType_Modified`, `PyDescr_NewMethod`, etc.) to install the C function at the slot level, bypassing the Python dispatch chain.
- **Plan** — Identify what could go wrong: reference counting errors, exception propagation, descriptor protocol compliance, MRO invalidation, thread safety.
- **Deconstruct** — Break into testable steps.
- **Test** — Tests exercising the Python API through the C backend, plus benchmarks. **Mandatory:** Run under ASan (`-fsanitize=address`), Valgrind (`--leak-check=full`), and refcount verification (`--with-pydebug`). Zero errors from all three is a hard gate.
- **Code** — Implement C extension with `setup.py`/`setuptools`. The Python layer remains as an overlay until proven redundant. The slot replacement is internal — consumers see no API change.
- **Document** — Record baseline versus post-conversion measurements. Include ASan and Valgrind output as evidence artefacts.

At the initial trust level (Tight), every step is confirmed with the human. As trust is earned, oversight reduces.

### 3. Assess

The evidence gate. Five mandatory checks:

| Check | Gate Type |
|-------|-----------|
| Correctness (all tests pass) | Hard gate |
| ASan clean (zero errors) | Hard gate |
| Leak-free (Valgrind clean) | Hard gate |
| Refcount clean (debug build) | Hard gate |
| Performance improvement | Evidence gate |

Post-conversion benchmarks are compared against baseline under the same conditions. Three performance outcomes:

| Verdict | What Happens |
|---------|-------------|
| **Benefit confirmed** | Merge branch. Mark conversion permanent. Proceed. |
| **Benefit unclear** | More data needed. Do not merge. |
| **Benefit falsified** | Revert. Document what was learned. Choose next candidate. |

A falsified benefit is the methodology working. "This dispatch chain resists C replacement because of X" is valuable information.

### 4. Advance

Back on main. The dispatch graph is updated — proven slot replacements may expose new accessible slots. The candidate list is re-ranked. The next cycle begins.

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
