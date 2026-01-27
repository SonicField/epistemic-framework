# Ground Truth: Bad Discovery Test

This scenario tests that the evaluator correctly fails when a discovery report is incomplete or wrong.

## What the Discovery Report Should Find

Same as messy_project:
- loader_v1.py (failed, race condition) → Discard
- loader_v2.py (works) → Keep
- loader_v3_experimental.py (incomplete) → Evaluate
- benchmark_results.csv → Keep
- notes.txt → Extract decisions

## What the Bad Discovery Report Claims

The mock_bad_discovery.md file deliberately:
1. Misses loader_v3 entirely
2. Claims loader_v1 works (wrong)
3. Doesn't mention benchmarks
4. Has no open questions

## Expected Test Outcome

The evaluator should return FAIL because:
- v3 not identified
- v1 incorrectly assessed
- benchmark data not mentioned
- key decisions not surfaced
