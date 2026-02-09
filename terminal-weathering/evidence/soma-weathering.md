# SOMA Weathering: C Extension for VM Types

## Context

SOMA is an interpreted language with a Python VM. The VM's core data structure is a hierarchical cell graph: `Cell` objects hold a value and a dict of named children, `CellRef` objects provide indirection, and `Store`/`Register` types manage traversal with auto-vivification and CellRef write-through.

A Rust/PyO3 extension (`soma_rust`) had already replaced the pure-Python implementations of these four types. The hypothesis was that native data structures would accelerate the interpreter. Benchmarks showed the Rust extension was actually ~6% *slower* than pure Python.

This document records the construction and results of a C extension (`soma_c_vm`) that tests whether PyO3's safety abstractions explain the Rust slowdown.

## Hypothesis

**PyO3's boundary-crossing overhead is the bottleneck, not Python itself.**

Every time the Python VM loop touches a Cell, the Rust extension pays:

| Operation | Rust/PyO3 code | What it actually does |
|-----------|---------------|----------------------|
| Read Cell.value | `cell.extract::<PyRef<RustCell>>(py)?.value.clone_ref(py)` | GIL check → type check → borrow check → field access → refcount clone |
| Check CellRef type | `obj.bind(py).is_instance_of::<RustCellRef>()` | GIL bind → vtable-based isinstance |
| Dict lookup | `dict.get_item(key)?.unwrap().unbind()` | Bound/Unbound conversion → Option handling → refcount management |
| Follow CellRef | `cr.extract::<PyRef<RustCellRef>>(py)?.cell_obj.clone_ref(py)` | GIL check → type check → borrow check → field access → refcount clone |

A C extension calls the same C functions CPython uses internally. No intermediate abstraction:

| Operation | C code | What it actually does |
|-----------|--------|----------------------|
| Read Cell.value | `((CellObject *)cell)->value` | Pointer dereference |
| Check CellRef type | `PyObject_TypeCheck(obj, &CellRefType)` | Pointer comparison |
| Dict lookup | `PyDict_GetItemRef(dict, key, &result)` | Direct CPython dict API |
| Follow CellRef | `((CellRefObject *)ref)->cell_obj` | Pointer dereference |

**Falsifier**: If C is not faster than Rust on the RB-tree benchmark, the hypothesis that PyO3 boundary overhead is the bottleneck is wrong.

## What Was Built

### File structure

```
soma_c_vm/
  helpers.h/c    — Cached soma.vm imports, vm_error(), type check macros
  cell.h/c       — CellObject (value + children) and CellRefObject types
  store.h/c      — StoreObject, traversal with CellRef dereferencing
  register.h/c   — RegisterObject, path validation, root CellRef following
  module.c       — PyInit_soma_c_vm, type registration
```

### Key design decisions

**1. Inline struct access.** Cell fields are accessed via `static inline` functions that cast and dereference:

```c
static inline PyObject *Cell_GET_VALUE(PyObject *cell) {
    return ((CellObject *)cell)->value;
}
```

This compiles to a single load instruction. The Rust equivalent (`cell.extract::<PyRef<RustCell>>(py)?.value.clone_ref(py)`) involves at minimum: a GIL token check, a type downcast, a borrow-count increment, a field access, a refcount increment, and a borrow-count decrement.

**2. Macro type checks.** Type checking uses `PyObject_TypeCheck`, which is a pointer comparison against the type object:

```c
#define CellRef_Check(op) PyObject_TypeCheck((op), &CellRefType)
```

**3. GC participation.** Both `CellObject` and `CellRefObject` support CPython's cyclic GC via `tp_traverse` and `tp_clear`, allocated with `PyObject_GC_New`. This is necessary because cells can form reference cycles through their children dict and CellRef indirection.

**4. Dual-path operations.** Every traversal function has a fast path for C-created cells (`Cell_Check` → direct struct access) and a slow path for Python-created cells (`PyObject_GetAttrString`). In practice, once the C backend is active, all cells are C cells, so the slow path is dead code — but it's required for correctness if Python cells leak in from test fixtures or edge cases.

**5. Subclassable Store.** `StoreType` has `Py_TPFLAGS_BASETYPE` so that test fixtures can subclass it (`BareStore(Store)` overrides `_populate_builtins` to create empty stores).

### Integration

The `SOMA_BACKEND` environment variable selects the backend:

| Value | Effect |
|-------|--------|
| `python` | Pure Python (no native acceleration) |
| `rust` | Rust/PyO3 (`soma_rust`) |
| `c` | C extension (`soma_c_vm`) |
| *(unset)* | Try Rust, fall back to Python |

`SOMA_NO_RUST=1` remains as a legacy alias for `SOMA_BACKEND=python`.

## Test Results

All existing tests pass with the C backend, with no modifications to any test file:

| Test suite | Count | Result |
|------------|-------|--------|
| `test_cell_isolated.py` | 51 | PASS |
| `test_store_isolated.py` | 28 | PASS |
| `test_register_isolated.py` | 41 | PASS |
| Full pytest (`tests/`) | 656 | PASS |
| SOMA integration (`run_soma_tests.py`) | 432 | PASS |

Rust backend regression check: 656 pytest — all pass.

## Benchmark Results

**Workload**: RB-tree benchmark. Insert 100 keys, look up all, remove half, has-check all, validate tree invariants. 5 runs per backend. Python 3.14.0, GCC 11.5.0, `-O3`.

### Total time (median, seconds)

| Backend | Median (s) | Relative to Python | Relative to Rust |
|---------|-----------|-------------------|-----------------|
| Python  | 6.374     | 1.00x             | 0.94x           |
| Rust    | 6.772     | 1.06x             | 1.00x           |
| **C**   | **3.280** | **0.51x**         | **0.48x**       |

### Per-stage breakdown (median, seconds)

| Stage | Python | Rust | C | C speedup vs Rust |
|-------|--------|------|---|--------------------|
| insert | 0.150 | 0.160 | 0.080 | 2.01x |
| lookup | 0.055 | 0.063 | 0.031 | 2.03x |
| remove_half | 6.099 | 6.469 | 3.135 | 2.06x |
| has_check | 0.052 | 0.060 | 0.028 | 2.12x |
| validate | 0.017 | 0.020 | 0.010 | 2.06x |
| **total** | **6.374** | **6.772** | **3.280** | **2.06x** |

### Repeatability

All backends: CV < 1.3%. Highly repeatable.

### Raw timing data

**Python** (5 runs, total seconds): 6.398, 6.368, 6.339, 6.402, 6.374

**Rust** (5 runs, total seconds): 6.772, 6.807, 6.595, 6.771, 6.775

**C** (5 runs, total seconds): 3.293, 3.226, 3.280, 3.288, 3.258

## Analysis

### The hypothesis holds

C is 2.06x faster than Rust, uniformly across every benchmark stage. The speedup is remarkably consistent (2.01x–2.12x), which indicates a systematic overhead being eliminated rather than a stage-specific optimisation.

### Why Rust is slower than Python

This is the more surprising finding. The Rust extension adds ~6% overhead compared to pure Python. The mechanism:

1. Pure Python Cell is a `@dataclass` with `value` and `children` attributes. CPython accesses these via its internal attribute lookup, which is heavily optimised (cached type attribute offsets, `LOAD_ATTR` specialisation in 3.14).

2. The Rust extension exposes `value` and `children` as PyO3 `#[getter]` methods. Each access goes through: GIL token validation → PyO3 method dispatch → borrow checker → `clone_ref` for refcounting. This is slower than CPython's native attribute access for simple field reads.

3. The VM execution loop is Python. Every instruction that touches a Cell crosses the Python→Rust→Python boundary twice (once to read, once to write back). The boundary cost is paid O(n) times per instruction, where n is the path length.

### Why C is faster than both

C calls exactly the same C functions that CPython calls when accessing Python objects. There is no boundary to cross — a C extension function is a C function called by C code (the CPython interpreter). Specifically:

- **Cell.value access**: `((CellObject *)cell)->value` compiles to `mov rax, [rdi + 16]`. One instruction, no function call.
- **Dict lookup**: `PyDict_GetItemRef` is the function CPython itself uses when executing `dict[key]`.
- **Type check**: `PyObject_TypeCheck` compares `ob_type` pointer. Same check CPython does for `isinstance`.

The C extension doesn't make these operations faster than CPython can — it makes them *exactly as fast as CPython*, by using the same code paths. The 2x speedup over Rust is the removal of PyO3's abstraction layer. The 1.94x speedup over Python comes from eliminating Python-level attribute access overhead (no `__dict__` lookup, no descriptor protocol, no `LOAD_ATTR` bytecode).

### Where the remaining time goes

At 3.28 seconds for N=100, the bottleneck is now the Python VM execution loop itself — bytecode dispatch, argument list construction, the `execute_block` recursion. The data structure operations (cell access, dict lookup) are as fast as they can be without moving the VM loop to C.

## What This Means for SOMA Performance

The architecture is: **Python VM loop → native data structures**. This work has established:

1. **PyO3 is the wrong tool for this job.** When the hot path is "Python calls native, reads a field, returns to Python", PyO3's safety mechanisms (designed for long-running native computations) add more overhead than they save.

2. **The C extension has hit the data-structure ceiling.** Further acceleration requires moving the VM execution loop itself — `execute_block`, `evaluate_node`, argument passing — to native code. The data structures are no longer the bottleneck.

3. **The 2x speedup is real and uniform.** It applies across all operation types (insert, lookup, remove, traversal), confirming it's a systematic boundary-cost elimination rather than a workload-specific optimisation.

## Files Modified

| File | Change |
|------|--------|
| `setup.py` | Added `soma_c_vm` extension definition |
| `soma/vm.py` | Replaced `_USE_RUST` with `SOMA_BACKEND` env var selection |

## Files Created

| File | Purpose | Lines |
|------|---------|-------|
| `soma_c_vm/helpers.h` | Cached imports, vm_error(), type check macros | 26 |
| `soma_c_vm/helpers.c` | Implementation of cached imports and error raising | 88 |
| `soma_c_vm/cell.h` | CellObject/CellRefObject struct definitions and inline accessors | 72 |
| `soma_c_vm/cell.c` | Cell and CellRef type implementations | 200 |
| `soma_c_vm/store.h` | StoreObject definition, traversal helper declarations | 33 |
| `soma_c_vm/store.c` | Store type with _populate_builtins, traversal, CRUD | 460 |
| `soma_c_vm/register.h` | RegisterObject definition | 16 |
| `soma_c_vm/register.c` | Register type with path validation, root CellRef following | 540 |
| `soma_c_vm/module.c` | Module init, type registration | 40 |
