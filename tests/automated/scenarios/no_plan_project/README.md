# No Plan Project

A test scenario with obvious epistemic failures. The /epistemic command should flag these.

## Expected Issues

When /epistemic reviews this project, it should identify:

1. No plan file exists
2. No progress log exists
3. Terminal goal unclear (this README doesn't state one)

## What This Tests

- Does /epistemic detect missing documentation?
- Does it ask about terminal goals when unclear?
- Is the output concise and actionable?
