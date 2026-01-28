# Verdict Files

This directory contains **generated test outputs** - do not commit these files.

## Why They Exist Here

Tests write verdict files to this directory because:
1. The test scripts need a writable location
2. Permissions are predictable within the repo
3. Keeping them separate from scenarios avoids confusion

## What Gets Written

| Pattern | Contents |
|---------|----------|
| `*_verdict.json` | Evaluator judgements (PASS/FAIL with reasoning) |
| `*_output.txt` | Raw command output for debugging |

## Git Status

All files in this directory are ignored via `.gitignore`. Only this README is tracked.

## Cleaning Up

```bash
rm -f tests/automated/verdicts/*.json tests/automated/verdicts/*.txt
```
