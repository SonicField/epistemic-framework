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
4. ✓ Five pillar documents drafted and reviewed
5. ✓ `/epistemic` command created with tiered depth
6. ✓ Install script created
7. ✓ STYLE.md for voice reference
8. ✓ Integrate foundation and pillars into `/epistemic` command (hybrid approach: foundation check first, read pillars when ambiguous)
9. ✓ Testing implemented
   - `tests/automated/test_install.sh` - verifies symlinks
   - `tests/automated/test_epistemic_command.sh` - AI evaluator with explicit criteria
   - `tests/manual/qa_epistemic.md` - human QA script
   - Both automated tests pass

## In Progress

(None)

## Completed (Late Update)

10. ✓ GitHub sync (user pushed manually)
11. ✓ Progress log created
12. ✓ `/epistemic-discovery` command - read-only archaeology phase
13. ✓ `/epistemic-recovery` command - step-wise action phase with confirmation
14. ✓ Tests for discovery/recovery commands
    - `test_epistemic_discovery.sh` - AI evaluator checks discovery report against ground truth
    - `test_epistemic_recovery.sh` - AI evaluator checks plan generation quality
    - `scenarios/messy_project/` - synthetic test scenario with known artefacts
15. ✓ vLLM discovery prompt template (`tests/manual/vllm_discovery_prompt.md`)

## Next

16. [ ] Run discovery/recovery tests to verify they work
17. [ ] vLLM discovery session (new Claude Code instance with meta-recording)

---

## Strategic Decisions (Resolved)

### How should `/epistemic` use the foundation/pillar documents?
**Decision**: Hybrid with foundation priority
- Step 0: Check if `goals.md` read this session; if not, read it
- Steps 1-4: Review, deepen with pillars only where clarity lacking
- Judgement call on when to read - intelligence, not algorithm

### What does "testing" mean for this framework?
**Decision**: Two categories
- **Automated**: Install script verification, command invocation, reasonableness checks via second Claude instance
- **Manual QA**: Human scripts similar to game/VR QA procedures

---

## Falsification Criteria

This plan is wrong if:
- The framework doesn't change how AI collaborates with Alex (measure: before/after comparison)
- The pillars don't capture Alex's actual epistemic standards (measure: Alex review)
- The tooling is unused (measure: usage frequency)
- The voice doesn't sound like Alex (measure: Alex review)
