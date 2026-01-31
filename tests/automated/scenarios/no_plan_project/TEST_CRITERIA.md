# Test Scenario: No Plan Project

## Description

A minimal project with obvious NBS failures. Used to test whether `/nbs` correctly identifies known issues.

## Known Issues (Ground Truth)

The evaluator must check whether the `/nbs` output identifies these:

| Issue | Must Identify | Acceptable Variations |
|-------|---------------|----------------------|
| No plan file exists | YES | "no plan", "plan missing", "plan does not exist" |
| No progress log exists | YES | "no progress", "progress log missing" |
| Terminal goal unclear | YES | "goal unclear", "what is the goal", asks about goals |
| Recommendations provided | YES | Has a recommendations section with actionable items |

## Evaluation Criteria

For PASS, the output must:
1. Identify at least 3 of 4 known issues (or ask about them)
2. Include structured sections (Status, Issues, Recommendations)
3. Be concise (readable in under 2 minutes)
4. Not invent issues that don't exist

For FAIL:
- Misses 2+ known issues without asking about them
- No recommendations
- Verbose or unstructured
- Hallucinates problems
