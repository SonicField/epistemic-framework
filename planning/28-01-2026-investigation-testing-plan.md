# Investigation Dispatch Testing Plan

**Date**: 28-01-2026
**Terminal Goal**: Verify `/epistemic` dispatch correctly handles investigation context in all cases

---

## Three Outcomes to Test

| Outcome | Trigger Condition | Expected Behaviour |
|---------|-------------------|-------------------|
| **Clearly Investigating** | Branch is `investigation/*` OR `INVESTIGATION-STATUS.md` at repo root | Dispatch to investigation review |
| **Maybe Investigating** | `INVESTIGATION-STATUS.md` found elsewhere (not repo root) | Ask user to clarify |
| **Not Investigating** | No markers found | Proceed with normal review |

---

## Test Matrix

### Confirmational Tests (should trigger expected behaviour)

| Test | Setup | Expected |
|------|-------|----------|
| `test_investigation_dispatch_branch.sh` | Create `investigation/test` branch | Investigation review output |
| `test_investigation_dispatch_file.sh` | Place `INVESTIGATION-STATUS.md` at repo root + context | Investigation review output |
| `test_investigation_ask.sh` | Place `INVESTIGATION-STATUS.md` in subdirectory only | AI asks user for clarification |
| `test_investigation_normal.sh` | No markers anywhere | Normal review output |

### Adversarial Tests (should NOT trigger wrong behaviour)

| Test | Setup | Should NOT |
|------|-------|-----------|
| `test_investigation_adv_no_false_dispatch.sh` | No markers | Should NOT produce investigation review |
| `test_investigation_adv_no_false_normal.sh` | Markers at repo root | Should NOT produce normal review without asking |
| `test_investigation_adv_no_silent_ignore.sh` | Markers in subdirectory | Should NOT silently proceed without asking |

---

## Falsification Criteria

Each test has explicit pass/fail criteria:

1. **Clearly Investigating**: FAIL if output contains Status/Issues/Recommendations sections (normal review format)
2. **Maybe Investigating**: FAIL if AI proceeds without using AskUserQuestion
3. **Not Investigating**: FAIL if output mentions "investigation" or reviews hypothesis/experiments
4. **Adversarial**: Each inverts the corresponding confirmational test's criteria

---

## Implementation Order

1. [ ] Create test scenarios in `tests/automated/scenarios/`
2. [ ] Write `test_investigation_dispatch_branch.sh` (cleanest case - branch name)
3. [ ] Write adversarial counterpart
4. [ ] Write `test_investigation_dispatch_file.sh` (file at repo root)
5. [ ] Write adversarial counterpart
6. [ ] Write `test_investigation_ask.sh` (file in subdirectory)
7. [ ] Write adversarial counterpart
8. [ ] Run full test suite
9. [ ] Update progress log
10. [ ] Commit

---

## Current State

- `test_investigation_dispatch.sh` exists but is incomplete (tests file at repo root with context)
- `test_investigation_adversarial.sh` exists (tests no false dispatch)
- Detection logic in `epistemic.md` modified multiple times, not yet committed
- Need to consolidate and clarify

---

## Decision: Scope Reduction

Given time constraints, implement minimum viable coverage:

| Priority | Test | Covers |
|----------|------|--------|
| P0 | Branch-based dispatch | Cleanest positive case |
| P0 | No markers → normal review | Cleanest negative case |
| P1 | File at repo root → dispatch | Secondary positive case |
| P1 | File in subdirectory → ask | Uncertainty handling |
| P2 | Full adversarial matrix | Defence in depth |

Start with P0, assess, then P1 if time permits.
