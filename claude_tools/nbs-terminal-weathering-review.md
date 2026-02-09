---
description: Correctness review for terminal weathering conversions
allowed-tools: Read, Glob, Grep, AskUserQuestion, Bash(git log:*), Bash(git status:*), Bash(git diff:*), Bash(git branch:*)
---

# Terminal Weathering Review

You are reviewing a terminal weathering session. This review is dispatched by `/nbs` when it detects a `weathering/*` branch or `.nbs/terminal-weathering/` directory.

**Apply this review IN ADDITION TO the normal NBS review, not instead of it.**

Read `{{NBS_ROOT}}/terminal-weathering/concepts/terminal-weathering.md` if you have not already this session.

---

## Correctness Checks

| Check | What to look for |
|-------|-----------------|
| **Shared types** | Are types crossing the conversion boundary identified? Are they verified compatible across both implementations? |
| **Reference semantics** | Has reference/pointer indirection been analysed? Do aliasing and mutation visibility behave identically? |
| **Type identity** | Have `isinstance`, `type()`, and class identity checks been verified against Rust-backed objects? |
| **Overlay mechanism** | Is there a clear mechanism for both implementations to coexist? Is it documented? |
| **Existing test suite** | Has the full existing test suite been run against both implementations? Not just new tests. |
| **Correctness vs performance** | Is the Assess phase checking correctness before performance? Correctness gate must pass before performance is even considered. |
| **Failed conversion analysis** | Are failed/reverted conversions documented with what they taught? Are negative results being reported? |

---

## Leaf Discipline

AIs revert to traditional development thinking under context pressure. This is the most common drift pattern in terminal weathering. Watch for these specific failure modes:

| Drift | Correction |
|-------|-----------|
| **Converting coupled units instead of leaves** | A leaf is a single function or class, not "the minimum viable coupled subsystem." If the candidate has dependencies on unconverted Python code, mock the boundary. Do not convert the dependency too. |
| **Avoiding mocks to "do it properly"** | Mocks at the conversion boundary are not a shortcut — they are the methodology. Each leaf must be proven correct in isolation before fusing. |
| **Treating boundary overhead as a problem** | Python↔Rust boundary crossings cause slowdown (potentially severe). This is expected and irrelevant during the correctness phase. If the AI is optimising for performance before all leaves are correct, it has lost the plot. |
| **Fusing before correctness is proven** | Correctness → Fuse → Performance is a phase sequence, not a balance. Fusing is only permitted after every leaf passes its correctness gate independently. |
| **Conflating correctness and performance** | "It works but it's 84% slower" is a **success** during the correctness phase. If the AI treats this as failure, it has confused the terminal goal of the current phase. |

---

## The Phase Separation

Terminal weathering has two distinct phases that must not be conflated:

**Correctness phase**: Convert each leaf individually. Mock dependencies at the boundary. Prove semantic identity with the Python implementation. Accept any performance penalty — it does not matter yet.

**Fuse phase**: Once all leaves in a region are individually proven correct, remove the mocks and connect the Rust implementations directly. This is where performance improvement appears.

If the AI is discussing performance during the correctness phase, it has drifted. Pull it back.

---

## Output

Include a **Terminal Weathering Correctness** section in the review output, after the normal NBS review dimensions:

```markdown
## Terminal Weathering Correctness

### Leaf Discipline
[Are conversions targeting actual leaves? Are mocks in place? Is the AI conflating correctness with performance?]

### Boundary Safety
[Are shared types, reference semantics, and type identity being checked?]

### Phase Clarity
[Is the current phase (correctness vs fuse) clear? Is the AI working within it?]
```

---

## The Contract

Correctness first. Performance is a consequence of correct fusing, not a goal of individual conversion. An AI that produces a semantically identical but slower Rust implementation has succeeded. An AI that produces a faster but subtly different implementation has failed.
