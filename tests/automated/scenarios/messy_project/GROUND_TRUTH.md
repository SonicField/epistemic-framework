# Ground Truth: Messy Project Test Scenario

This file documents what exists in this scenario for test verification.
The discovery command should NOT read this file - it's for the test evaluator only.

## Terminal Goal (as human would state it)

"Parallelise data loading for a machine learning pipeline."

## Artefacts That Should Be Found

| File | Purpose | Status | Expected Verdict |
|------|---------|--------|------------------|
| `src/loader_v1.py` | First attempt at parallel loading | Failed - race condition | Discard |
| `src/loader_v2.py` | Fixed version with locks | Works | Keep |
| `src/loader_v3_experimental.py` | Lock-free attempt | Incomplete | Evaluate |
| `old/notes.txt` | Early design thinking | Partial | Extract key decisions |
| `old/benchmark_results.csv` | Performance measurements | Valid data | Keep |
| `README.md` | Outdated description | Misleading | Update or discard |
| `tests/test_loader.py` | Tests for v2 | Works | Keep with loader_v2 |

## Artefacts That Should Be Ignored

| File | Why |
|------|-----|
| `__pycache__/` | Build artefact |
| `.DS_Store` | System file |
| `old/scratch.py` | Random experiments, no value |

## Key Decisions Discovery Should Surface

1. Lock-based approach chosen over lock-free (v2 vs v3)
2. Batch size of 32 was optimal (in benchmark_results.csv)
3. Thread pool size of 4 matched CPU cores

## Open Questions Discovery Should Identify

1. Should v3 (lock-free) be continued or abandoned?
2. Is the current test coverage sufficient?
3. What's the deployment target?
