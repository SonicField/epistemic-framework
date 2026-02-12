# The Verification Cycle

Safety comes from verbs, not nouns. Correctness emerges from actions - checking, validating, asserting, testing - not from static structures like type systems or design patterns.

"This value was validated" matters. "This has type ValidatedInput" does not, unless validation actually occurred. The verb happened or it did not. That is provable.

## The Cycle

```
Design → Plan → Deconstruct → [Test → Code → Document] → Next
```

Each phase has entry and exit criteria. Skipping phases is not speed; it is debt with compound interest.

| Phase | Entry | Exit |
|-------|-------|------|
| Design | Problem exists | Success criteria defined |
| Plan | Criteria clear | Testable steps identified |
| Deconstruct | Steps identified | Each step has test criterion |
| Test | Criterion defined | Test written and fails |
| Code | Test fails correctly | Test passes, assertions hold |
| Document | Test passes | Learnings recorded |

## The Decomposition Criterion

If you cannot write a test for a step, either:
- You have not decomposed far enough, or
- You do not yet understand what you are building

"Implement authentication" is not testable. These are:
- Validate password meets complexity rules
- Hash password with salt
- Compare hash against stored hash
- Generate session token on success

Each step independently testable. Each test defines success.

## Assertions Are Not Optional

Assertions are executable specifications. A triggered assertion is proof of a bug.

Three levels:

| Level | Purpose | Example |
|-------|---------|---------|
| Precondition | What must be true on entry | `assert amount > 0` |
| Postcondition | What must be true on exit | `assert balance_before + balance_after == total` |
| Invariant | What must always be true | `assert account.balance >= 0` |

Every assertion message answers: What was expected? What occurred? Why does it matter?

## Types Are Hints, Not Guarantees

Type systems are incomplete. They cannot express "this list is sorted", "this connection is authenticated", or "this balance is non-negative." A type annotation constrains the compiler's view of the code — it does not constrain reality.

The danger: type systems create false confidence. "It type-checks, therefore it is correct" substitutes a noun (the type) for the verb (the check). A function that accepts `ValidatedInput` and returns `SafeOutput` has told you nothing about whether validation occurred or safety was achieved.

```python
# The noun says it is safe. Is it?
def process(data: ValidatedInput) -> SafeOutput:
    return transform(data)  # No verification occurred

# The verb checks.
def process(data: ValidatedInput) -> SafeOutput:
    assert data.is_actually_validated(), \
        f"ValidatedInput not validated: {data.source}"
    result = transform(data)
    assert result.meets_safety_criteria(), \
        f"Transform produced unsafe output: {result.summary()}"
    return result
```

Use types as documentation. Rely on assertions for correctness. The type hints intent; the assertion verifies it.

## Integration-First Testing

Mocks hide integration failures. A test where every dependency is mocked proves only that the mock behaves as expected — which is a tautology.

The methodology:

1. Write integration tests against the real system first
2. Add targeted unit tests only for complex isolated logic

What mocks hide: network latency and timeouts, concurrency and race conditions, resource contention and deadlocks, configuration mismatches, real failure modes and error messages. A clean unit test suite is not evidence of a working system.

The anti-pattern:

```python
# Everything mocked — tests pass, integration fails
@mock.patch('database')
@mock.patch('network')
@mock.patch('filesystem')
def test_everything_mocked():
    pass  # This test proves nothing about real behaviour

# Test the real system
def test_with_real_database(test_db):
    result = service.process(test_db)
    assert result.saved_to_db()
```

Mock at system boundaries (external APIs you do not control) and at conversion boundaries during porting (where the mock proves the ported piece matches the original before fusing). Outside these two cases, test for real.

## Dynamic Analysis

Static checks — type systems, linters, compilation — are nouns. They describe what the code *should* be. Dynamic analysis tools are verbs. They observe what the code *actually does* at runtime.

Every class of runtime bug that static analysis cannot catch must have a corresponding dynamic analysis verb. Examples by language:

- **C/C++**: ASan (memory errors), TSan (data races), UBSan (undefined behaviour), Valgrind (leaks)
- **Rust**: Miri (unsafe verification), sanitizer builds
- **Python**: `pytest` with fail-fast, `python -X dev` mode, Hypothesis (property-based testing)
- **Concurrent code**: thread/race analysis (TSan, Go race detector, Helgrind)
- **Untrusted input**: fuzz testing (libFuzzer, AFL, cargo-fuzz, Atheris)

If a project has no mechanism to detect memory errors, data races, or undefined behaviour at runtime, those bugs are invisible. Invisible bugs are the most dangerous kind — they compound silently until they manifest as data corruption or security vulnerabilities.

Dynamic analysis is not optional hardening. It is a verification verb on the same level as testing and assertions.

## The Practical Question

Before moving to the next step: what did I learn? Write it down. If you cannot articulate what you learned, you were not paying attention.

---

## Pillar Check

Have you read all six pillars in this session?

- goals.md
- falsifiability.md
- rhetoric.md
- bullshit-detection.md
- verification-cycle.md *(you are here)*
- zero-code-contract.md

If you cannot clearly recall reading each one, read them now. Next: `zero-code-contract.md`
