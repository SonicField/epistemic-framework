# Weathering at the Right Layer: Function Bodies vs Call Protocol

## The Problem We Found

Terminal weathering — progressive replacement of Python hot paths with compiled
alternatives — was applied to four leaf functions in PyTorch's `nn.Module` and
`torch.utils._pytree`. All four were compiled to Rust via PyO3, correctness
tests pass (52/52, full suite 88/88), and the first fusion was achieved
(`_get_node_type` calling `is_namedtuple_class` directly in Rust). ABBA
counterbalanced QPS measurement shows no significant effect: mean -1.4%,
95% CI [-5.4%, +2.5%], t = -0.82, p > 0.05.

The speed-bump experiments measured 30.4% QPS sensitivity to adding 1µs at
every `torch_python` function entry. Replacing function bodies with faster
compiled code should recover some of that. It didn't. Why?

## What "Replacing Function Bodies" Actually Means

When Python executes `m.weight` on an `nn.Module`, CPython runs this chain:

```
m.weight
  → tp_getattro(m, "weight")                    # C: type slot dispatch
    → slot_tp_getattr_hook(m, "weight")          # C: looks up __getattr__ in type dict
      → _PyType_Lookup(Module, "__getattr__")    # C: MRO walk
      → call_attribute(m, getattr_func, "weight")# C: descriptor protocol
        → func_descr_get(func, m, Module)        # C: creates bound method
        → PyObject_CallOneArg(bound, "weight")   # C: call machinery
          → _PyEval_EvalFrame(...)               # C: frame setup
            ┌─────────────────────────────┐
            │  self._parameters["weight"] │      ← function body
            │  self._buffers["weight"]    │      ← THIS is what we
            │  self._modules["weight"]    │      ← replaced with Rust
            └─────────────────────────────┘
          → frame teardown                       # C: cleanup
```

Everything outside the box is the **call protocol**. Everything inside the box
is the **function body**. We replaced the inside of the box with Rust that does
the same dict lookups faster — but the box itself, and all the machinery that
gets you to it and back, is unchanged. That machinery runs ~80ns. The body runs
~50ns. We shaved the body to ~30ns. The total went from ~130ns to ~110ns — a
15% improvement on a quantity that represents 0.03% of iteration time.

The speed-bump experiment added its delay at **function entry** — measuring
sensitivity to the **number of Python function calls**, not the cost of their
bodies. Replacing bodies doesn't reduce the call count. The 30.4% sensitivity
is in the dispatch chain, not in the dict lookups inside `__getattr__`.

Replacing the type slot means removing everything from `slot_tp_getattr_hook`
downward and putting a C function directly at `tp_getattro`, so CPython calls
it with zero intermediate steps.

## Adapted Weathering: Same Methodology, Different Granularity

The weathering methodology's core strengths — evidence gates, falsifiability,
progressive replacement, correctness tests before performance measurement — are
independent of the replacement granularity. What needs to change is the unit of
work. Instead of replacing function bodies (where the overhead isn't), each
weathering cycle should target a **call protocol path**: identify a high-hit-count
Python dispatch chain (e.g., `tp_getattro` → `slot_tp_getattr_hook` →
`call_attribute` → `__getattr__`), write a C extension that replaces the type
slot directly, and gate it through the same ABBA measurement and correctness
suite. The 52 correctness tests already define the behavioural contract for
`__getattr__`, `__delattr__`, `is_namedtuple_class`, and `_get_node_type` —
those tests are reusable regardless of whether the implementation behind them is
Rust, C, or a caching strategy. The leaf-first principle still applies, but the
"leaves" are type slots rather than function bodies: replace `tp_getattro` on
`Module` before attempting to replace `tp_setattro` or FSDP's attribute
handling, because each replacement provides evidence about whether the protocol
overhead hypothesis holds.

The practical shift is from Rust/PyO3 to C written directly against CPython's
type API. `PyDescr_NewMethod` creates native method descriptors with
freelist-backed binding. `PyType_Modified` propagates slot changes through the
MRO. `tp_getattro` replacement bypasses the entire slot dispatcher. None of this
is accessible through PyO3. The Rust work is not wasted — the `fast_getattr`
logic (check `_parameters`, `_buffers`, `_modules` in order, raise
`AttributeError` with the correct format) translates line-for-line into C, and
the correctness tests catch any semantic drift. The first weathering cycle under
this revised approach would be: write a C `tp_getattro` for `Module` that does
the parameter/buffer/module lookup directly, install it via a C extension module,
verify the 52 tests still pass, then run the ABBA benchmark. If the 30.4%
sensitivity is genuinely in the dispatch chain, this single replacement should
produce a measurable QPS change — and if it doesn't, that falsifies the
hypothesis cleanly.
