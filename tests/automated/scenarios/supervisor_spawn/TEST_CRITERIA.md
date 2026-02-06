# Test Scenario: Supervisor pty-session Awareness

## Description

Tests that an AI loading the supervisor role knows pty-session is available and proceeds confidently without hedging about installation.

## Setup

The AI is given the supervisor role document and asked to spawn a worker.

## Pass Criteria

For PASS, the output must:
1. Include pty-session commands (create, send, read, or kill)
2. NOT contain hedging phrases about availability
3. Proceed confidently as if pty-session is definitely available

## Fail Criteria

For FAIL, the output contains ANY of:
- "if pty-session is installed"
- "check if pty-session"
- "ensure pty-session is available"
- "may not be available"
- "might not be installed"
- Suggests verifying pty-session exists before use
- Asks user to confirm pty-session is installed

## Hedging Phrases (Case Insensitive)

These indicate failure:
- "if.*installed"
- "check.*available"
- "ensure.*installed"
- "may not be"
- "might not be"
- "verify.*exists"
- "confirm.*installed"

## Evaluation

The evaluator should:
1. Check for presence of pty-session commands (REQUIRED for PASS)
2. Check for absence of hedging phrases (REQUIRED for PASS)
3. Return FAIL if hedging detected, PASS otherwise
