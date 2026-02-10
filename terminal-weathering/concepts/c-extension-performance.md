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
