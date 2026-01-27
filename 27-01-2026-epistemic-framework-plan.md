# Epistemic Framework - Plan

**Date**: 27-01-2026
**Terminal Goal**: Develop an epistemic framework that improves human-AI collaboration quality at scale, externalising Alex's epistemic standards into transmissible, reusable form.

**Motivation (Pathos)**:
- Resource maximisation - better AI collaboration saves gigawatts compared to code optimisation
- Gap-filling - AI research ignores millennia of epistemic work; this is low-hanging fruit
- Transmissibility - standards that persist across sessions and can be shared

---

## Completed

1. ✓ Project structure created (`concepts/`, `claude_tools/`, `bin/`, `tests/`)
2. ✓ Git repository initialised
3. ✓ Foundation document: `goals.md`
4. ✓ Five pillar documents drafted (Falsifiability confirmed; others need review)
5. ✓ `/epistemic` command created with tiered depth
6. ✓ Install script created
7. ✓ STYLE.md for voice reference

## In Progress

8. [ ] Integrate foundation and pillars into `/epistemic` command properly
   - Currently command has embedded content that should derive from source documents
   - Need to decide: embed summary? instruct to read? hybrid?

## Outstanding

9. [ ] Review remaining pillars (Rhetoric, Verification Cycle, Zero-Code Contract, Bullshit Detection)
10. [ ] Define testing approach for the framework
11. [ ] GitHub sync
12. [ ] Progress log

---

## Strategic Decisions Needed

### How should `/epistemic` use the foundation/pillar documents?

Options:
a) **Embed summaries** - Command contains condensed versions, reads full docs when deeper analysis needed
b) **Reference only** - Command instructs AI to read docs as first step
c) **Hybrid** - Command has review dimensions inline, reads docs only for ambiguous cases (current approach)

### What does "testing" mean for this framework?

To be discussed with Alex - she has ideas.

---

## Falsification Criteria

This plan is wrong if:
- The framework doesn't change how AI collaborates with Alex (measure: before/after comparison)
- The pillars don't capture Alex's actual epistemic standards (measure: Alex review)
- The tooling is unused (measure: usage frequency)
- The voice doesn't sound like Alex (measure: Alex review)
