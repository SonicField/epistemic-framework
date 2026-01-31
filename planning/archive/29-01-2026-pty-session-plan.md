# Plan: PTY Session Tool for Interactive Process Control

**Date**: 29-01-2026
**Terminal Goal**: Enable Claude to interact with long-running interactive processes across multiple tool calls

---

## Problem Statement

Claude's Bash tool is one-shot: run command, get output, done. This prevents:
- Interacting with REPLs (python, node, gdb)
- Testing interactive CLI behaviour (AskUserQuestion)
- Controlling processes that require multi-step interaction
- Monitoring long-running processes

## Solution

Use tmux as a session manager, wrapped in a helper script with self-documenting interface.

---

## Deliverables

### 1. `bin/pty-session` - Helper Script

Single script with subcommands:

| Subcommand | Purpose |
|------------|---------|
| `create <name> <command>` | Create detached tmux session running command |
| `send <name> <text>` | Send keystrokes (handles Enter correctly) |
| `read <name>` | Capture current screen content |
| `wait <name> <pattern> [timeout]` | Poll until pattern appears or timeout |
| `kill <name>` | Terminate session |
| `list` | Show active pty-session sessions |
| `help` | Show usage with examples |

**Key behaviours:**
- Session names prefixed with `pty_` to avoid collision with user sessions
- `send` automatically appends Enter unless `--no-enter` flag
- `wait` polls every 0.5s, default timeout 60s
- `read` captures scrollback as well as visible screen
- All commands return sensible exit codes

### 2. `claude_tools/epistemic-tmux.md` - Skill Document

Documents:
- When to use interactive sessions
- The session lifecycle pattern
- Common use cases with examples
- Gotchas and how the script handles them

### 3. Tests

Located in `tests/automated/`:

| Test | Purpose | Falsification |
|------|---------|---------------|
| `test_pty_session_basic.sh` | Create, send, read, kill cycle | Fails if any operation errors or read doesn't show sent text |
| `test_pty_session_wait.sh` | Wait for pattern to appear | Fails if wait doesn't detect pattern or times out incorrectly |
| `test_pty_session_timeout.sh` | Wait timeout works correctly | Fails if wait doesn't timeout when pattern absent |
| `test_pty_session_isolation.sh` | Sessions are isolated from each other | Fails if operations on session A affect session B |

### 4. Adversarial Tests

| Test | Purpose | Falsification |
|------|---------|---------------|
| `test_pty_session_adv_no_collision.sh` | Session names don't collide with user tmux | Fails if pty-session affects non-pty_ sessions |
| `test_pty_session_adv_cleanup.sh` | Kill cleans up properly | Fails if tmux session persists after kill |
| `test_pty_session_adv_invalid_session.sh` | Graceful handling of invalid session names | Fails if commands crash instead of returning error |

---

## Implementation Order

1. [x] Write `bin/pty-session` with create, send, read, kill, list, help
2. [x] Write `test_pty_session_basic.sh` - verify basic cycle works
3. [x] Run test, iterate until passing
4. [x] Add `wait` subcommand to script
5. [x] Write `test_pty_session_wait.sh` and `test_pty_session_timeout.sh`
6. [x] Run tests, iterate until passing
7. [x] Write adversarial tests
8. [x] Run adversarial tests, iterate until passing
9. [x] Write `claude_tools/epistemic-tmux.md` skill document
10. [ ] Update `bin/install.sh` to handle new script
11. [ ] Commit and document in progress log

---

## Success Criteria

1. All tests pass
2. I can use `pty-session` to interact with an interactive process
3. The skill document is sufficient for me to use this correctly in future sessions
4. No interference with existing tmux sessions

---

## Exit Condition

Return to investigation dispatch testing when:
- All tests (including adversarial) pass
- Script is committed
- Skill document exists

Then use `pty-session` to properly test the "ask user" behaviour for subdirectory INVESTIGATION-STATUS.md.
