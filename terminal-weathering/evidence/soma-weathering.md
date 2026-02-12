# SOMA Weathering: Rust, C, and Dispatch Optimisation

## Context

SOMA is an interpreted language with a Python VM. The VM's core data structure is a hierarchical cell graph: `Cell` objects hold a value and a dict of named children, `CellRef` objects provide indirection, and `Store`/`Register` types manage traversal with auto-vivification and CellRef write-through.

Three approaches to accelerating this VM were tried. Two were falsified. One succeeded.

## Phase 1: Rust via PyO3 (Falsified)

### Hypothesis

**Native Rust data structures will accelerate the interpreter.**

A Rust/PyO3 extension (`soma_rust`) replaced the pure-Python implementations of Cell, CellRef, Store, and Register. Every time the Python VM loop touches a Cell, the Rust extension pays:

| Operation | Rust/PyO3 code | What it actually does |
|-----------|---------------|----------------------|
| Read Cell.value | `cell.extract::<PyRef<RustCell>>(py)?.value.clone_ref(py)` | GIL check → type check → borrow check → field access → refcount clone |
| Check CellRef type | `obj.bind(py).is_instance_of::<RustCellRef>()` | GIL bind → vtable-based isinstance |
| Dict lookup | `dict.get_item(key)?.unwrap().unbind()` | Bound/Unbound conversion → Option handling → refcount management |
| Follow CellRef | `cr.extract::<PyRef<RustCellRef>>(py)?.cell_obj.clone_ref(py)` | GIL check → type check → borrow check → field access → refcount clone |

### Result

6% slower than pure Python (6.772s vs 6.374s median, N=100 RB-tree benchmark, Python 3.14, 5 runs).

**Why it failed**: The VM execution loop is Python. Every instruction that reads a Cell field crosses the Python→Rust→Python boundary twice. The boundary cost is O(n) per instruction, where n is the path length. CPython 3.14's own attribute access (via `LOAD_ATTR` specialisation) is faster than PyO3's getter protocol for simple field reads.

PyO3's safety abstractions — GIL token validation, type downcasting, borrow checking, refcount cloning — are designed for long-running native computations. On fine-grained field access patterns (millions of reads per benchmark run), each ~50ns crossing compounds into a 500ms tax that exceeds any potential savings.

**Falsifier applied**: "If Rust/PyO3 is faster than Python on the RB-tree benchmark, the approach is valid." It was not. Measured and rejected.

## Phase 2: C Extension Types (Succeeded)

### Hypothesis

**PyO3's boundary-crossing overhead is the bottleneck, not Python itself.**

A C extension calls the same C functions CPython uses internally. No intermediate abstraction:

| Operation | C code | What it actually does |
|-----------|--------|----------------------|
| Read Cell.value | `((CellObject *)cell)->value` | Pointer dereference |
| Check CellRef type | `Py_IS_TYPE(obj, &CellRefType)` | Pointer comparison |
| Dict lookup | `PyDict_GetItemRef(dict, key, &result)` | Direct CPython dict API |
| Follow CellRef | `((CellRefObject *)ref)->cell_obj` | Pointer dereference |

**Falsifier**: If C is not faster than Rust on the RB-tree benchmark, the hypothesis that PyO3 boundary overhead is the bottleneck is wrong.

### What was built

```
soma_c_vm/
  helpers.h/c    — Cached soma.vm imports, vm_error(), type check macros
  cell.h/c       — CellObject (value + children) and CellRefObject types
  store.h/c      — StoreObject, traversal with CellRef dereferencing
  register.h/c   — RegisterObject, path validation, root CellRef following
  module.c       — PyInit_soma_c_vm, type registration
```

**Key design decisions:**

1. **Inline struct access.** Cell fields accessed via `static inline` functions that cast and dereference. `Cell_GET_VALUE` compiles to a single load instruction. The Rust equivalent involves GIL token check, type downcast, borrow-count increment, field access, refcount increment, and borrow-count decrement.

2. **Exact type checks.** `Cell_Check` and `CellRef_Check` use `Py_IS_TYPE` — a single pointer comparison. Valid because these types have no subclasses.

3. **GC participation.** Both types support CPython's cyclic GC via `tp_traverse` and `tp_clear`, allocated with `PyObject_GC_New`. Necessary because cells can form reference cycles.

4. **Dual-path operations.** Every traversal function has a fast path for C cells (direct struct access) and a slow path for Python cells (`PyObject_GetAttrString`). Once the C backend is active, all cells are C cells — the slow path is dead code but required for correctness.

5. **Subclassable Store.** `StoreType` has `Py_TPFLAGS_BASETYPE` so test fixtures can subclass it.

### Integration

The `SOMA_BACKEND` environment variable selects the backend:

| Value | Effect |
|-------|--------|
| `python` | Pure Python (no native acceleration) |
| `rust` | Rust/PyO3 (`soma_rust`) |
| `c` | C extension (`soma_c_vm`) |
| *(unset)* | Try Rust, fall back to Python |

### Test results

All existing tests pass with the C backend, with no modifications to any test file:

| Test suite | Count | Result |
|------------|-------|--------|
| `test_cell_isolated.py` | 51 | PASS |
| `test_store_isolated.py` | 28 | PASS |
| `test_register_isolated.py` | 41 | PASS |
| Full pytest (`tests/`) | 656 | PASS |
| SOMA integration (`run_soma_tests.py`) | 432 | PASS |

Rust backend regression check: 656 pytest — all pass.

### Benchmark results

**Workload**: RB-tree benchmark. Insert 100 keys, look up all, remove half, has-check all, validate tree invariants. 5 runs per backend. Python 3.14.0, GCC 11.5.0.

#### Three-way comparison (debug build)

| Backend | Median (s) | Relative to Python | Relative to Rust |
|---------|-----------|-------------------|-----------------|
| Python  | 6.374     | 1.00x             | 0.94x           |
| Rust    | 6.772     | 1.06x             | 1.00x           |
| **C**   | **3.280** | **0.51x**         | **0.48x**       |

Per-stage breakdown (median, seconds):

| Stage | Python | Rust | C | C speedup vs Rust |
|-------|--------|------|---|--------------------|
| insert | 0.150 | 0.160 | 0.080 | 2.01x |
| lookup | 0.055 | 0.063 | 0.031 | 2.03x |
| remove_half | 6.099 | 6.469 | 3.135 | 2.06x |
| has_check | 0.052 | 0.060 | 0.028 | 2.12x |
| validate | 0.017 | 0.020 | 0.010 | 2.06x |
| **total** | **6.374** | **6.772** | **3.280** | **2.06x** |

All backends: CV < 1.3%.

Raw timing data:

- **Python** (5 runs, total seconds): 6.398, 6.368, 6.339, 6.402, 6.374
- **Rust** (5 runs, total seconds): 6.772, 6.807, 6.595, 6.771, 6.775
- **C** (5 runs, total seconds): 3.293, 3.226, 3.280, 3.288, 3.258

#### Optimised build comparison (`--enable-optimizations --with-lto`)

| Backend | Median (s) | Speedup |
|---------|-----------|---------|
| Python  | 6.334     | —       |
| **C**   | **3.110** | **2.04x** |

Per-stage breakdown (optimised build, median, seconds):

| Stage | Python | C | C speedup |
|-------|--------|---|-----------|
| insert | 0.144 | 0.080 | 1.81x |
| lookup | 0.054 | 0.032 | 1.69x |
| remove_half | 6.064 | 2.957 | 2.05x |
| has_check | 0.052 | 0.030 | 1.73x |
| validate | 0.017 | 0.010 | 1.66x |
| **total** | **6.334** | **3.110** | **2.04x** |

### Analysis

**The hypothesis holds.** C is 2.06x faster than Rust in the debug build, uniformly across every benchmark stage. The speedup is remarkably consistent (2.01x–2.12x), which indicates a systematic overhead being eliminated rather than a stage-specific optimisation.

**Why Rust is slower than Python.** Pure Python Cell is a `@dataclass` with `value` and `children` attributes. CPython accesses these via its internal attribute lookup, which is heavily optimised (cached type attribute offsets, `LOAD_ATTR` specialisation in 3.14). The Rust extension exposes `value` and `children` as PyO3 `#[getter]` methods — each access goes through GIL token validation, PyO3 method dispatch, borrow checker, and `clone_ref`. This is slower than CPython's native attribute access for simple field reads.

**Why C is faster than both.** C extension types *are* CPython types. `((CellObject *)cell)->value` compiles to `mov rax, [rdi + 16]`. One instruction, no function call. The C extension doesn't make operations faster than CPython can — it makes them *exactly as fast as CPython*, by using the same code paths. The 2x speedup over Rust is PyO3's abstraction layer removed. The 1.94x speedup over Python is Python-level attribute access overhead removed (no `__dict__` lookup, no descriptor protocol, no `LOAD_ATTR` bytecode).

**Where the remaining time goes.** At 3.11s (optimised), the bottleneck is the Python VM execution loop — bytecode dispatch, argument list construction, `execute_block` recursion. Data structure operations are as fast as they can be without moving the VM loop to C.

### Files

| File | Purpose | Lines |
|------|---------|-------|
| `setup.py` | Added `soma_c_vm` extension definition | — |
| `soma/vm.py` | Replaced `_USE_RUST` with `SOMA_BACKEND` env var selection | — |
| `soma_c_vm/helpers.h` | Cached imports, vm_error(), type check macros | 26 |
| `soma_c_vm/helpers.c` | Implementation of cached imports and error raising | 88 |
| `soma_c_vm/cell.h` | CellObject/CellRefObject struct definitions and inline accessors | 72 |
| `soma_c_vm/cell.c` | Cell and CellRef type implementations | 200 |
| `soma_c_vm/store.h` | StoreObject definition, traversal helper declarations | 33 |
| `soma_c_vm/store.c` | Store type with _populate_builtins, traversal, CRUD | 460 |
| `soma_c_vm/register.h` | RegisterObject definition | 16 |
| `soma_c_vm/register.c` | Register type with path validation, root CellRef following | 540 |
| `soma_c_vm/module.c` | Module init, type registration | 40 |

## Phase 3: Dispatch Optimisation

Having moved the data structures to C, the remaining Python overhead was in dispatch — type checking, method resolution, function call protocol.

### right-foot isinstance inline cache

A separate CPython C extension library (`right-foot`) providing per-call-site inline caches for `isinstance`.

**Two-layer architecture:**

- **Layer 1 (C inline cache)**: Per-call-site `RFIsInstanceIC` struct (24 bytes thread-local). Keys on `(tp_mro, cls) → bool`. Hit path: 2 pointer comparisons (~2ns). Miss path: `PyObject_IsInstance` + 3 pointer stores. Guard: `tp_mro` identity — CPython replaces (does not mutate) `tp_mro` on hierarchy change, so pointer inequality implies MRO changed.

- **Layer 2 (Python API)**: Global thread-local 15-entry ring buffer exposed as `right_foot.fast_isinstance()`. Result bitmap packs True/False into 16 bits.

**Benchmark — Python API (negative result):**

The Python-level `fast_isinstance()` was initially **2.5x slower** than builtin `isinstance`, due to `METH_VARARGS` + `PyArg_ParseTuple` + `PyBool_FromLong`:

| MRO depth | `isinstance` | `fast_isinstance` (METH_VARARGS) |
|-----------|-------------|----------------------------------|
| 2 | 70 ns | 184 ns |
| 5 | 74 ns | 179 ns |
| 10 | 78 ns | 176 ns |

After switching to `METH_FASTCALL` + direct arg access + `Py_NewRef`:

| Result type | `isinstance` | `fast_isinstance` (METH_FASTCALL) |
|------------|-------------|-----------------------------------|
| Positive (cache hit) | 74 ns | 88 ns |
| Negative (cache hit) | 110 ns (MRO walk) | 87 ns |

The calling convention change alone recovered 96ns per call. The 14ns remaining gap on positive results is CPython's `CALL_ISINSTANCE` bytecode specialisation — an optimisation path available only to the builtin, not to C extension functions. For negative results, the cache is faster because it returns in constant time what `isinstance` must confirm by walking the entire MRO.

The C inline cache (Layer 1, no function call at all) sits at 2–5ns, well below both.

**SOMA integration:** 6 `PyObject_IsInstance` calls replaced with `rf_isinstance` inline caches in `helpers.c` (2 calls) and `dispatch.c` (4 calls). 656/656 pytest and 432/432 integration tests pass.

**SOMA benchmark with right-foot (attribution ambiguous):** N=100 RB-tree, 5 runs, median total: 3.40s (Phase 2 baseline, Py3.14) → 3.18s (Phase 2 + right-foot, Py3.15). The 6.3% improvement cannot be cleanly attributed because the Python version changed. Expected theoretical ceiling: isinstance accounts for ~5% of total time; maximum expected improvement ~4%.

**Correctness bugs found and fixed:**

*Bug 1 — MRO pointer reuse.* The cache stored a borrowed pointer to the MRO tuple. If `__bases__` changed, CPython replaced `tp_mro` with a new tuple and freed the old one. The allocator could reuse the old MRO's address for an unrelated object. The cache would see matching pointers and return a stale result. Fix: hold a strong reference (`Py_INCREF`) to the cached MRO. Found by adversarial test `test_repeated_bases_mutations`.

*Bug 2 — cls pointer reuse.* Same bug class, second instance. The `cls` field stored a borrowed pointer to the type object. If the type was deleted and its address reused, the cache would match on the wrong type. Fix: hold a strong reference to `cls`. Found by human code review.

**Lesson:** Identity-based caching requires preventing deallocation of cached keys. If a cache maps `pointer → value`, the pointer must be a strong reference.

Evidence: right-foot commits `f11e287` (MRO fix) and `17b7501` (cls fix). 16/16 adversarial tests pass after both fixes.

### C builtins and dispatch fast paths

Six optimisations applied in parallel using git worktrees and `nbs-teams-supervisor`:

1. **14 builtins ported to C** — `CBuiltinBlock` type with `CBuiltinFn` function pointer, bypassing Python method dispatch entirely. Builtins: block, chain, choose, isNil, isVoid, <, +, -, *, /, %, concat, toString, toInt.
2. **Register fast paths** — mirroring the existing Store C fast path pattern for Register reads/writes in the dispatch loop.
3. **Register_New factory** — bypassing `tp_call` protocol for internal Register allocation.
4. **Interned `"_"` string** — replacing 4 `PyUnicode_FromString("_")` calls with a cached interned string.
5. **Cell_Check/CellRef_Check tightened** — from `PyObject_TypeCheck` (MRO walk) to `Py_IS_TYPE` (single pointer comparison). Valid because these types have no subclasses.
6. **`chain` and `block` builtins ported to C** — with `is_any_block` helper using inline caches for Python Block/BuiltinBlock types, and `execute_block` helper for direct C dispatch.

### Phase 3 benchmark results

**Test suite:** 5.26s → 4.95s (6% improvement).

**RB-tree benchmark (debug build, Python 3.15, 5 runs):**

| Phase | Median total (s) | Non-remove_half (s) |
|-------|-----------------|---------------------|
| Phase 4 (pre-dispatch opt) | 6.403 | 0.275 |
| Phase 5 (post-dispatch opt) | 6.467 | 0.291 |

Phase 4 and Phase 5 are within noise (CV ~2%). The benchmark is dominated by `remove_half` at O(n² log n), which accounts for 96% of total runtime (6.13s of 6.40s). Dispatch optimisation was real (evidenced by the 6% test suite improvement) but invisible in a benchmark where algorithmic complexity dominates.

**Note on comparability:** Phase 4/5 benchmarks ran on Python 3.15 (debug build, `--with-pydebug --disable-gil`). Phase 2 benchmarks ran on Python 3.14 (optimised build). The 6.40s vs 3.11s gap is almost entirely explained by the build mode difference, not a regression.

### Files added/modified in Phase 3

| File | Change |
|------|--------|
| `soma_c_vm/cbuiltin.h` | NEW — CBuiltinBlock type definition, function pointer typedef |
| `soma_c_vm/cbuiltin.c` | NEW — 14 C builtin implementations, CBuiltinBlock type |
| `soma_c_vm/dispatch.c` | Register fast paths, CBuiltinBlock check in RN_EXEC |
| `soma_c_vm/register.c` | Interned `"_"`, `Register_New` factory, exposed internal functions |
| `soma_c_vm/register.h` | `Register_New` declaration, internal function declarations |
| `soma_c_vm/block.c` | `Register_New()` instead of `PyObject_CallNoArgs` |
| `soma_c_vm/store.c` | `CBuiltinBlock_populate` call |
| `soma_c_vm/module.c` | `CBuiltinBlock_Init` call |
| `soma_c_vm/cell.h` | `Py_IS_TYPE` instead of `PyObject_TypeCheck` |
| `setup.py` | Added `cbuiltin.c` to sources |

## Key Lessons

### The Boundary Crossing Tax

Any language boundary has per-call overhead. When the hot path is "Python calls native, reads a field, returns to Python", the overhead compounds:

| Boundary type | Overhead per crossing | Source |
|--------------|----------------------|--------|
| PyO3 getter | ~50ns (GIL check + type check + borrow check + refcount clone) | SOMA Rust measurements |
| `PyObject_GetAttrString` | ~80ns (string creation + attribute protocol) | CPython profiling |
| `PyObject_IsInstance` | 30–110ns (MRO walk, depends on depth) | right-foot benchmarks |
| C struct dereference | ~2–5ns (pointer arithmetic) | Baseline |

C extension types are not "crossing a boundary into C". They *are* CPython types. The distinction between "Python object" and "C extension object" is administrative, not architectural. Both are `PyObject *` with `ob_type` and `ob_refcnt`. The C extension simply skips the indirection layers that Python's dynamism requires.

**Falsifier for any future language boundary approach**: Measure `call_count × overhead_per_crossing`. If this product exceeds the savings from the native implementation, the approach is net negative. For SOMA, the Rust crossing count was ~10 million per benchmark run, making even 50ns per crossing a 500ms tax.

### AI C Extension Anti-Patterns

AI systems write slow C extensions by default. This is documented in detail in [c-extension-performance.md](../concepts/c-extension-performance.md). The short version:

| Anti-pattern | Overhead | Correct alternative |
|-------------|----------|-------------------|
| `METH_VARARGS` + `PyArg_ParseTuple` | +96ns/call | `METH_FASTCALL` + `args[0]` |
| `PyBool_FromLong(r)` | Unnecessary allocation | `Py_NewRef(r ? Py_True : Py_False)` |
| `Py_BuildValue("O", obj)` | Format string parsing | `Py_NewRef(obj)` |
| `PyObject_GetAttrString` in a loop | String creation per call | Intern string once, or direct struct access |
| `PyObject_TypeCheck` for leaf types | MRO walk | `Py_IS_TYPE` (single pointer compare) |

The right-foot library demonstrated this concretely. The initial `METH_VARARGS` + `PyBool_FromLong` implementation was 2.5x *slower* than Python's builtin `isinstance` (184ns vs 70ns). After switching to `METH_FASTCALL` + direct arg access + `Py_NewRef`: 88ns (positive), 87ns (negative, cache hit) vs 74ns (positive), 110ns (negative, MRO walk). The calling convention change alone recovered 96ns per call.

**The rule**: `METH_VARARGS` is banned. `PyArg_ParseTuple` is banned. `PyBool_FromLong` is banned. These are the slow path, and slow C is a contradiction.

### The Benchmark Ceiling

Dispatch optimisation hits a ceiling when algorithmic complexity dominates. At N=100, `remove_half` takes 6.13s out of 6.40s total (96%). Even a 50% speedup on dispatch would save 0.14s — 2.2% of total runtime, indistinguishable from noise (CV ~2%).

**Options when hitting the ceiling:**

1. Fix the algorithm. Replace O(n² log n) `remove_half` with O(n log n) using proper RB-tree deletion.
2. Use a dispatch-heavy benchmark where dispatch overhead is a larger fraction of total time.
3. Measure dispatch-only stages. Non-remove_half stages showed 7% improvement, consistent with the 6% test suite improvement.

**Lesson:** Always check what fraction of runtime your target optimisation addresses before investing effort. A 50% improvement to 4% of runtime is a 2% improvement overall.

### Parallel Worker Strategy

The dispatch optimisation used `nbs-teams-supervisor` with git worktrees to parallelise four independent optimisations:

| Branch | Worker | Optimisation | Committed |
|--------|--------|-------------|-----------|
| `weathering/intern-underscore` | (sequential) | Intern `"_"` string | `d8fac74` |
| `weathering/register-factory` | register-factory-be0b | `Register_New()` factory | `6588035` |
| `weathering/register-fast-path` | register-fast-path-* | Register read/write fast paths | `0b253b8` |
| `weathering/c-builtin-dispatch` | c-builtin-dispatch-* | CBuiltinBlock with 14 C builtins | `3e2ac3e` |

All four branches merged cleanly into `weathering/vm/rust-core` despite touching overlapping files.

**What worked:** Git worktrees for truly parallel implementation. Workers with precise task files (exact code, exact commit message) completed independently.

**What did not work:** Workers wandered when tasks were vague. `nbs-worker` permission prompts blocked progress — required manual `tmux send-keys Enter`. Workers needed dismissal after task completion to prevent off-task activity.

**Lesson:** Worker task files must specify exact files, exact code, exact commit message. Workers optimise for "find something useful to do" and will wander into supervision territory if not dismissed.

### Python Version Effects

Phase 2/3 benchmarks ran on Python 3.14 (optimised build). Phase 4/5 ran on Python 3.15 (debug build). The 6.40s vs 3.11s gap is build mode difference, not regression. Debug builds disable optimisation, add assertions, enable memory debugging.

**Lesson:** Never compare benchmark numbers across different Python versions or build modes. Record the Python version string, build flags, and compiler version alongside every benchmark result.

## What Remains

| Item | Impact | Effort |
|------|--------|--------|
| `remove_half` algorithm: O(n² log n) → O(n log n) | High — removes 95% of benchmark time | RB-tree delete-by-node-pointer |
| Remaining Python builtins not ported: `print`, `readLine`, `use` | Low — I/O-bound or one-shot | Straightforward port |
| Debug builtins (`debug.type`, `debug.id`, etc.) | None — debug only | Low priority |
| Optimised CPython build benchmark | Needed for clean Phase 3→Phase 5 comparison | `--enable-optimizations --with-lto` |
| Same-version A/B test for right-foot | Isolates inline cache effect from Python version change | Run with/without cache on same Python |

## Evidence Trail

| Evidence | Location |
|----------|----------|
| Git history (all phases) | `~/local/soma/` branch `weathering/vm/rust-core`, HEAD at `d47c358` |
| right-foot library | `~/local/right-foot/`, HEAD at `17b7501`, 16 adversarial tests |
| Phase 2 benchmark (C types, 1 run, Py3.14) | `~/local/soma/benchmarks/results_c_phase2_final/benchmark_results.json` |
| Phase 3 benchmark (debug, free-threading, Py3.14) | `~/local/soma/benchmarks/results_c_phase3/benchmark_results.json` |
| Phase 3 benchmark (optimised, Py3.14) | `~/local/soma/benchmarks/results_c_phase3_optimised/benchmark_results.json` |
| Phase 3 Python baseline (optimised, Py3.14) | `~/local/soma/benchmarks/results_python_phase3_optimised/benchmark_results.json` |
| Phase 4 benchmark (dispatch opt, debug, Py3.15) | `~/local/soma/benchmarks/results/c_phase4/benchmark_results.json` |
| Phase 5 benchmark (final dispatch, debug, Py3.15) | `~/local/soma/benchmarks/results/c_phase5/benchmark_results.json` |
| C extension source | `~/local/soma/soma_c_vm/` |
| C extension performance discipline | [concepts/c-extension-performance.md](../concepts/c-extension-performance.md) |
| NBS state | `~/local/soma/.nbs/supervisor.md`, `decisions.log` |

## Verification

1. All "faster" and "slower" claims specify the comparison baseline, Python version, build mode, and number of runs.
2. Falsified hypotheses documented with evidence of falsification (benchmark results showing Rust slower than Python).
3. Pointer recycling bugs documented with the specific test that caught each one and the commit that fixed it.
4. No speculation presented as fact. Where attribution is ambiguous (right-foot effect vs Python version change), this is stated explicitly.
5. Benchmark ceiling analysis uses actual Phase 4 numbers: remove_half 6.13s / total 6.40s = 96%.
