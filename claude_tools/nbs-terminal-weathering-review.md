---
description: Correctness review for terminal weathering conversions
allowed-tools: Read, Glob, Grep, AskUserQuestion, Bash(git log:*), Bash(git status:*), Bash(git diff:*), Bash(git branch:*)
---

# Terminal Weathering Review

You are reviewing a terminal weathering session. This review is dispatched by `/nbs` when it detects a `weathering/*` branch or `.nbs/terminal-weathering/` directory.

**Apply this review IN ADDITION TO the normal NBS review, not instead of it.**

Read these documents if you have not already this session:

1. `{{NBS_ROOT}}/terminal-weathering/concepts/terminal-weathering.md` — the philosophy
2. `{{NBS_ROOT}}/terminal-weathering/concepts/c-extension-performance.md` — the cost model for C extensions

---

## Correctness Checks

| Check | What to look for |
|-------|-----------------|
| **Shared types** | Are types crossing the conversion boundary identified? Are they verified compatible across both implementations? |
| **Reference semantics** | Has reference/pointer indirection been analysed? Do aliasing and mutation visibility behave identically? |
| **Type identity** | Have `isinstance`, `type()`, and class identity checks been verified against C-backed type slot modifications? Has `PyType_Modified` been called after slot changes to propagate through the MRO? |
| **Overlay mechanism** | Is there a clear mechanism for installing and removing the C type slot alongside the Python implementation? Is the installation/removal path documented and reversible? |
| **Existing test suite** | Has the full existing test suite been run against both implementations? Not just new tests. |
| **Correctness vs performance** | Is the Assess phase checking correctness before performance? Correctness gate must pass before performance is even considered. |
| **Failed conversion analysis** | Are failed/reverted conversions documented with what they taught? Are negative results being reported? |
| **ASan cleanliness** | Does all C code pass tests when compiled with `-fsanitize=address -fsanitize=undefined`? ASan is the C equivalent of Rust's borrow checker — without it, memory safety bugs are invisible. This check is non-negotiable. |
| **Leak analysis** | Has `valgrind --leak-check=full` (or equivalent) confirmed zero leaks? Memory leaks in C extensions are silent, cumulative, and invisible to correctness tests. |
| **Refcount discipline** | Is `Py_INCREF`/`Py_DECREF` balance documented and verified for every `PyObject*`? Every parameter, return value, and local variable holding a `PyObject*` must have documented ownership semantics (borrowed vs owned). |
| **Calling convention discipline** | Is `METH_FASTCALL` used for all functions taking positional arguments? Is `PyArg_ParseTuple` absent? Is `Py_BuildValue` absent for single return values? Is `PyBool_FromLong` absent? See `c-extension-performance.md` for the full cost model. |

---

## Leaf Discipline

AIs revert to traditional development thinking under context pressure. This is the most common drift pattern in terminal weathering. Watch for these specific failure modes:

| Drift | Correction |
|-------|-----------|
| **Converting coupled slots instead of leaf slots** | A leaf is a single type slot, not "the minimum viable coupled subsystem." If the candidate slot depends on other unconverted Python-backed slots, mock the boundary. Do not convert the dependency too. |
| **Avoiding mocks to "do it properly"** | Mocks at the conversion boundary are not a shortcut — they are the methodology. Each leaf slot must be proven correct in isolation before fusing. |
| **Treating boundary overhead as a problem** | Python↔C boundary crossings via type slots have different characteristics than full-stack Python call protocol dispatch. During the correctness phase, boundary overhead is irrelevant. If the AI is optimising for performance before all leaf slots are correct, it has lost the plot. |
| **Fusing before correctness is proven** | Correctness → Fuse → Performance is a phase sequence, not a balance. Fusing is only permitted after every leaf slot passes its correctness gate independently. |
| **Conflating correctness and performance** | "It works but it's slower" is a **success** during the correctness phase. If the AI treats this as failure, it has confused the terminal goal of the current phase. |
| **Skipping ASan because "it compiles fine"** | Compiling without errors is necessary but not sufficient. C code that compiles cleanly can contain use-after-free, buffer overflows, and undefined behaviour that only ASan catches. Skipping ASan because tests pass is the C equivalent of disabling the borrow checker. This is the critical drift pattern for C extensions. |
| **Undocumented refcount ownership** | Every `PyObject*` must have documented ownership (borrowed reference vs new/strong reference). If the AI is writing C that manipulates Python objects without documenting who owns each reference, it is accumulating silent refcount bugs. These may not manifest until long after the buggy code runs. |
| **Tutorial calling conventions** | `METH_VARARGS`, `PyArg_ParseTuple`, `Py_BuildValue` for single values, and `PyBool_FromLong` are the patterns most represented in training data. They are also the slow patterns. If the AI is using any of these, it has defaulted to the tutorial path. The C extension exists because Python is too slow — if the extension itself uses slow calling conventions, it has no reason to exist. See `c-extension-performance.md`. |
| **Reflexive INCREF/DECREF in traversal loops** | AIs default to `Py_INCREF`/`Py_DECREF` on every pointer copy because that is what the CPython documentation recommends. In tight traversal loops over C-struct-backed types where the GIL guarantees object lifetime, this is 0.6 ns/node of unnecessary overhead — and unlike GC tracking overhead, it does not vanish on hardware with larger caches. If the loop body is cheap (pointer dereferences, comparisons, arithmetic), look for borrowed references and raw pointer traversal instead of per-node refcounting. This is not a ban — sometimes INCREF is necessary — but the reviewer should verify the AI has considered the lifetime guarantee rather than reflexively refcounting. See `c-extension-performance.md`. |

---

## The Phase Separation

Terminal weathering has two distinct phases that must not be conflated:

**Correctness phase**: Replace each leaf type slot individually with a C implementation. Mock dependencies at the boundary. Prove semantic identity with the Python implementation. Accept any performance penalty — it does not matter yet. Verify ASan cleanliness, zero leaks, and refcount balance for every conversion.

**Fuse phase**: Once all leaf slots in a region are individually proven correct, remove the mocks and connect the C implementations directly. Install multiple slots on the same type. This is where performance improvement appears — the dispatch chain overhead is removed.

If the AI is discussing performance during the correctness phase, it has drifted. Pull it back.

If the AI is skipping ASan or leak analysis during either phase, it has drifted critically. This must be corrected immediately.

---

## Output

Include a **Terminal Weathering Correctness** section in the review output, after the normal NBS review dimensions:

```markdown
## Terminal Weathering Correctness

### Leaf Discipline
[Are conversions targeting actual leaf type slots? Are mocks in place? Is the AI conflating correctness with performance?]

### Boundary Safety
[Are shared types, reference semantics, and type identity being checked? Has PyType_Modified been called after slot changes?]

### Memory Safety
[Is ASan being run on all C code? Is valgrind confirming zero leaks? Is refcount ownership documented for every PyObject*?]

### Calling Convention Discipline
[Is METH_FASTCALL used everywhere? Are tutorial patterns (METH_VARARGS, PyArg_ParseTuple, Py_BuildValue, PyBool_FromLong) absent? Are attribute accesses using interned strings or direct struct access? In tight traversal loops, is the code using borrowed references where lifetime is guaranteed, or paying for INCREF/DECREF on every iteration?]

### Phase Clarity
[Is the current phase (correctness vs fuse) clear? Is the AI working within it?]
```

---

## The Contract

Correctness first. Performance is a consequence of correct fusing, not a goal of individual slot replacement. An AI that produces a semantically identical but slower C type slot implementation has succeeded. An AI that produces a faster but subtly different implementation has failed. An AI that produces a C implementation without ASan verification and refcount documentation has not completed the correctness phase, regardless of test results.
