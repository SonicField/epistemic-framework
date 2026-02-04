---
description: Exit Precise Technical English mode and return to natural language
allowed-tools: Read
---

# NBS Natural

Exit PTE mode. Return to natural language communication.

---

## Effect

When invoked, `/nbs-natural`:

1. Exits PTE mode
2. AI returns to natural language output
3. Statement-level PTE discretion remains available (AI MAY still use `[PTE]` markers when precision matters)

---

## When to Use

Use `/nbs-natural` when:

- The precision-critical section is complete
- Remaining work is explanatory, not specification
- The human prefers natural language for the current context

---

## What Persists

After exiting PTE mode:

| Behaviour | Status |
|-----------|--------|
| Mode-level PTE | OFF - AI uses natural language |
| Statement-level PTE | AVAILABLE - AI may use `[PTE]` markers at discretion |
| Resolution Clauses | RETAINED - All captured resolutions remain in effect |
| Defined Terms | RETAINED - Terms defined during PTE mode remain defined |

---

## Re-entering PTE Mode

To re-enter PTE mode, invoke `/nbs-pte`.

The mode is independent. Exiting does not lose work. Entering again resumes precision mode.

---

## The Contract

PTE is for precision. Natural language is for flow.

Use PTE where ambiguity causes damage. Use natural language where it does not.

_The human decides when precision justifies the verbosity cost._
