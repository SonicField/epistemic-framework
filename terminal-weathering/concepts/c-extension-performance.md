# C Extension Performance Discipline

AI systems write slow C extensions by default. Not wrong — slow. They reach for the patterns most represented in training data: `METH_VARARGS`, `PyArg_ParseTuple`, `Py_BuildValue`, `PyBool_FromLong`. These are the tutorial patterns. They are also the slow patterns.

The cost is not theoretical. Measured on CPython 3.15:

| Calling convention | ns/call | Overhead vs optimal |
|-------------------|---------|-------------------|
| `METH_VARARGS` + `PyArg_ParseTuple` | 184 | +96 |
| `METH_FASTCALL` + direct arg access | 88 | baseline |
| C inline (no function call) | 2–5 | — |

The `METH_VARARGS` path constructs a tuple, walks a format string character by character, unpacks the arguments back out, then discards the tuple. For a function whose actual work is two pointer comparisons, this overhead is 50x the payload. That is not a rounding error. That is the extension doing nothing, expensively.

The cause is not ignorance — it is indifference. The AI has no cost model. It treats all CPython API calls as equivalent because the training signal does not distinguish them. `PyArg_ParseTuple` and `args[0]` both "get the first argument". One allocates and parses. The other is a pointer dereference.

## The Rule

**Write against the machine, not the tutorial.**

A C extension exists because Python is too slow. If the extension itself is slow, it has no reason to exist. Every nanosecond of overhead in the call protocol is a nanosecond that could have been spent in Python with less complexity and better debuggability.

There is no acceptable level of unnecessary overhead. Not 10ns. Not 1ns. The machine does not charge by the instruction, but the instructions compound across millions of calls, and the overhead is pure waste — it does no work, produces no result, serves no purpose.

## Calling Conventions

Use the fastest available. No exceptions.

| Convention | When to use | When not to use |
|-----------|-------------|-----------------|
| `METH_FASTCALL` | Every function that takes positional arguments | Never avoid it |
| `METH_NOARGS` | Functions that take no arguments | — |
| `METH_O` | Functions that take exactly one argument | — |
| `METH_VARARGS` | **Never** | Always |

`METH_VARARGS` exists for backwards compatibility. It is not a design choice. It is a historical accident preserved for code that predates better options. New code has no excuse.

With `METH_FASTCALL`, CPython passes arguments as a C array:

```c
static PyObject *
my_func(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    /* args[0], args[1], ... — direct pointer access, no parsing */
}
```

Register the method with `_PyCFunction_CAST` to silence the type mismatch:

```c
{"my_func", _PyCFunction_CAST(my_func), METH_FASTCALL, "..."},
```

## Argument Handling

`PyArg_ParseTuple` is format-string interpretation at runtime. It is `printf` for argument unpacking — flexible, safe, slow.

| Operation | Slow path | Fast path |
|-----------|-----------|-----------|
| Get object arg | `PyArg_ParseTuple(args, "O", &obj)` | `PyObject *obj = args[0]` |
| Get int arg | `PyArg_ParseTuple(args, "i", &n)` | `long n = PyLong_AsLong(args[0])` |
| Get string arg | `PyArg_ParseTuple(args, "s", &s)` | `const char *s = PyUnicode_AsUTF8(args[0])` |
| Arity check | Implicit in format string | `if (nargs != 2) { ... }` |

Validate preconditions with asserts, not runtime checks, when the caller is internal code:

```c
assert(nargs == 2 && "my_func requires exactly 2 arguments");
assert(PyType_Check(args[1]) && "my_func: arg 2 must be a type");
```

Runtime type-checking in the hot path is defensive programming against your own code. If you are passing the wrong types to your own C functions, the correct response is to fix the caller, not to add a type check that runs a billion times to catch a bug that happens zero times.

For public API entry points where callers cannot be trusted, use the cheapest possible validation — `PyType_Check`, `PyLong_Check`, direct comparisons — not `PyArg_ParseTuple`.

## Return Values

Do not construct what already exists.

| Slow | Fast | Why |
|------|------|-----|
| `PyBool_FromLong(r)` | `Py_NewRef(r ? Py_True : Py_False)` | `Py_True` and `Py_False` are immortal singletons. `PyBool_FromLong` calls into the bool constructor. |
| `Py_BuildValue("i", n)` | `PyLong_FromLong(n)` | `Py_BuildValue` parses a format string. |
| `Py_BuildValue("")` | `Py_RETURN_NONE` | — |
| `Py_BuildValue("O", obj)` | `Py_NewRef(obj)` | — |

`Py_BuildValue` is the return-value equivalent of `PyArg_ParseTuple` — format-string interpretation at runtime. Use it for complex dict/tuple construction where the alternative is verbose. Never use it for single values.

## Attribute Access

`PyObject_GetAttrString` creates a temporary string object on every call and goes through the general attribute protocol. CPython 3.14+ specialises `LOAD_ATTR_INSTANCE_VALUE` in bytecode to bypass all of this.

If you call `PyObject_GetAttrString` in a loop, you are slower than Python.

```c
/* WRONG — creates and destroys a string object per call */
for (int i = 0; i < n; i++) {
    PyObject *val = PyObject_GetAttrString(obj, "value");
    /* ... */
    Py_DECREF(val);
}

/* RIGHT — intern the string once, reuse */
static PyObject *str_value = NULL;
if (!str_value) str_value = PyUnicode_InternFromString("value");
for (int i = 0; i < n; i++) {
    PyObject *val = PyObject_GetAttr(obj, str_value);
    /* ... */
    Py_DECREF(val);
}

/* BEST — access the C struct directly, no attribute protocol at all */
for (int i = 0; i < n; i++) {
    PyObject *val = ((MyObject *)obj)->value;
    /* ... */
}
```

The best path is no path. If you control the type, store the field in a C struct and access it by pointer arithmetic. The attribute protocol exists for Python's dynamism. C code that goes through it is paying for dynamism it does not use.

## Method Calls

`PyObject_CallMethod` is convenient. It looks up the method by name (string comparison), creates a bound method object, calls it, and discards the bound method. Each call allocates and deallocates.

For hot paths, look up the method once and cache it. Or better: call the underlying C function directly if the target is a C type you control.

```c
/* WRONG — method lookup + bound method creation per call */
for (int i = 0; i < n; i++) {
    PyObject *result = PyObject_CallMethod(obj, "process", "O", arg);
}

/* BETTER — vectorcall with cached method name */
static PyObject *str_process = NULL;
if (!str_process) str_process = PyUnicode_InternFromString("process");
for (int i = 0; i < n; i++) {
    PyObject *result = PyObject_CallMethodOneArg(obj, str_process, arg);
}

/* BEST — call the C function directly */
for (int i = 0; i < n; i++) {
    int rc = MyType_process_internal((MyObject *)obj, arg);
}
```

## GC Tracking

`Py_TPFLAGS_HAVE_GC` is the default for types that contain `PyObject*` fields. It enables CPython's cyclic garbage collector to traverse and break reference cycles. The cost is not the traversal — it is the allocation.

Every GC-tracked object is prepended with a 16-byte `PyGC_Head`. This changes object size:

| Type flags | Per-object overhead | 1,000 objects |
|-----------|-------------------|--------------|
| `Py_TPFLAGS_DEFAULT` | 0 bytes | — |
| `Py_TPFLAGS_DEFAULT \| Py_TPFLAGS_HAVE_GC` | +16 bytes | +16 KB |

Measured on a linked list traversal (1,000 nodes, CPython 3.15, Intel Xeon 8339HC, L1d 32 KiB):

```
C (with GC, 48 bytes/node)       3,065 ns  (3.1 ns/node)
C (no GC, 32 bytes/node)         2,030 ns  (2.0 ns/node)
```

Identical struct, identical loop body, identical compiler. The only difference is 16 bytes per object. The GC-tracked list (48 KB) overflows L1 cache (32 KB). The non-GC list (32 KB) fits. That 50% size increase produces a 50% speed decrease — not from GC work, but from cache misses.

**Object size dominates instruction count at scale.** A version with more instructions per node but smaller objects (32 bytes) was faster than the version with fewer instructions but larger objects (48 bytes). Measure before you reason.

### When to omit GC tracking

A type does not need GC tracking if it **cannot form reference cycles**. This means:

- No `PyObject*` fields, or
- All `PyObject*` fields point to types that are guaranteed acyclic (e.g., `int`, `str`, `None`), or
- The data structure is acyclic by construction and this invariant is enforced

If GC tracking is omitted, `tp_traverse` and `tp_clear` are not needed, and allocation uses `PyObject_New` instead of `PyObject_GC_New`:

```c
/* GC-tracked — 48 bytes per object, requires traverse/clear */
static PyTypeObject NodeType = {
    ...
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)Node_traverse,
    .tp_clear = (inquiry)Node_clear,
    .tp_new = Node_new,  /* uses PyObject_GC_New internally */
};

/* Not GC-tracked — 32 bytes per object, no traverse/clear */
static PyTypeObject NodeType = {
    ...
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Node_new,  /* uses PyObject_New internally */
};
```

**Warning**: omitting GC tracking on a type that *can* form cycles will cause memory leaks. The cyclic GC will never see the objects and never break the cycle. This is a correctness trade-off, not a free optimisation. Assert the acyclic invariant if you take this path.

### Evidence: boundary-crossing-bench

This data comes from a controlled benchmark comparing C and Rust/PyO3 linked list traversal. The GC tracking effect was discovered as a confound: Rust's `#[pyclass]` does not enable GC tracking by default, making Rust objects 32 bytes vs C's 48 bytes. The apparent speed advantage of Rust over C disappeared entirely when the C type was rebuilt without GC tracking. With equal object sizes, C was 1.27x faster — the remaining gap being Rust's per-node INCREF/DECREF for ownership safety. Full data: `~/local/boundary-crossing-bench/11-02-2026-boundary-bench-paper.md`.

## Reference Counting in Hot Paths

`Py_INCREF` and `Py_DECREF` are not free. In a tight traversal loop, the cost of maintaining reference counts on every object touched is measurable and irreducible.

Measured on the same linked list benchmark (1,000 nodes, CPython 3.15, equal 32-byte objects, no GC tracking):

```
C (no refcounting in loop)       2,030 ns  (2.0 ns/node)
Rust/PyO3 (INCREF+DECREF/node)   2,560 ns  (2.6 ns/node)
```

The 0.6 ns/node gap is one `Py_INCREF` and one `Py_DECREF` per iteration. C follows raw pointers with no refcount changes in the hot path — the GIL guarantees no other thread can deallocate during traversal, so the raw pointer is safe in practice. Rust/PyO3's ownership model requires incrementing the next node's refcount before releasing the current one, on every iteration.

```c
/* C — no refcount changes in the loop */
while (current != Py_None) {
    total += ((NodeObject *)current)->value;
    current = ((NodeObject *)current)->next;   /* raw pointer copy */
}
```

This matters because:

1. **The cost is per-object, not per-call.** A function that traverses 10,000 objects pays 6,000 ns in refcounting overhead alone. For a function body that is two pointer dereferences, refcounting is the dominant cost.

2. **It compounds with free-threading.** Under `--disable-gil` (CPython 3.13+), `Py_INCREF`/`Py_DECREF` become atomic operations (`lock xadd` or equivalent). The per-object cost increases. Exact figures depend on contention, but atomic refcounting is strictly more expensive than non-atomic.

3. **It is architecture-independent.** The GC tracking penalty depends on L1 cache size — on ARM cores with 64–128 KiB L1d, the GC overhead vanishes for moderate data structures because both the 32-byte and 48-byte layouts fit in cache. The INCREF/DECREF overhead does not vanish. It is the same 0.6 ns/node regardless of cache size, because it is instruction overhead, not memory pressure.

### When refcounting matters

Refcounting overhead matters when **all three** conditions hold:

- The loop body is cheap (pointer dereferences, comparisons, arithmetic)
- The loop touches many objects (hundreds or thousands per call)
- The function is called frequently (hot path)

For functions with expensive loop bodies (I/O, allocation, Python callbacks), the refcounting cost is noise. For tight traversal loops over object graphs, it is the floor.

### Minimising refcount overhead in C

C code can avoid refcounting in traversal loops when the GIL (or other mechanism) guarantees the objects remain alive:

- **Borrowed references**: access `PyObject*` fields without INCREF when the parent object is kept alive for the duration of the loop
- **Raw pointer traversal**: `current = ((NodeObject *)current)->next` is a pointer copy with no ownership implications
- **Batch INCREF/DECREF**: if you must take a reference, do it once at the start and end of the traversal, not per node

The rule: if you control the type and know the lifetime, do not pay for reference counting you do not need. Assert the lifetime guarantee rather than paying for it on every iteration.

### Evidence: boundary-crossing-bench

The 0.6 ns/node figure comes from the same controlled benchmark. With GC tracking removed from both implementations (equal 32-byte objects), the only remaining difference was Rust's per-node `clone_ref` (INCREF) and implicit drop (DECREF) vs C's raw pointer copy. Verified by inspecting compiler output: the C loop compiles to 4 x86 instructions, the Rust loop compiles to the same 4 plus two refcount operations. The cost difference matches the instruction difference exactly. Full data: `~/local/boundary-crossing-bench/11-02-2026-boundary-bench-paper.md`.

## The AI Failure Mode

This pattern is not carelessness. It is a systematic bias.

AI systems optimise for "compiles and passes tests". `PyArg_ParseTuple` compiles. It passes tests. It handles edge cases. It is the path of least resistance for an optimiser that has no cost model.

The human writing C extensions for a decade does not reach for `PyArg_ParseTuple` because they have a cost model built from experience. They know that every layer of abstraction between the caller and the work is overhead that must be justified. They know that "it works" and "it is fast" are different claims with different evidence requirements.

The corrective is not "be more careful". It is a hard rule: **the tutorial pattern is banned**. `METH_VARARGS` is not permitted. `PyArg_ParseTuple` is not permitted. `Py_BuildValue` for single values is not permitted. `PyBool_FromLong` is not permitted. These are not options to be weighed against alternatives. They are the slow path, and slow is not acceptable.

When the purpose of C is speed, slow C is a contradiction.

## Falsifier

If a C extension function's call overhead (entry to first useful instruction) exceeds 20ns on the target hardware, the calling convention is wrong. Measure it. If you cannot measure it, you cannot claim it is fast.

## Evidence: right-foot

The `right_foot.fast_isinstance` function was initially written with `METH_VARARGS` + `PyArg_ParseTuple` + `PyBool_FromLong`. It was 2.5x slower than Python's builtin `isinstance`:

```
isinstance(obj, cls):       70 ns/call
fast_isinstance(obj, cls): 184 ns/call  ← the "fast" version
```

After replacing with `METH_FASTCALL` + direct arg access + `Py_NewRef(Py_True)`:

```
isinstance(obj, cls):       74 ns/call
fast_isinstance(obj, cls):  88 ns/call  (positive result)
fast_isinstance(obj, cls):  87 ns/call  (negative result, cache hit)
isinstance(obj, cls):      110 ns/call  (negative result, MRO walk)
```

The calling convention change alone recovered 96ns per call. The function that was slower than Python became faster than Python for negative results — because the cache returns in constant time what `isinstance` must confirm by walking the entire MRO.

The 14ns remaining gap on positive results is the `CALL_ISINSTANCE` bytecode specialisation — an optimisation path available only to the builtin, not to C extension functions. That is a hard floor for the Python-callable API. The C inline cache (no function call at all) sits at 2–5ns, well below it.

None of this was discovered by reasoning. It was discovered by measuring. The AI's first version was confidently wrong. Confidence is not evidence.
