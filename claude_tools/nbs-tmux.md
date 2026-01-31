---
description: Interact with long-running terminal processes across multiple tool calls
allowed-tools: Bash
---

# Interactive Terminal Sessions (pty-session)

This skill enables interaction with long-running interactive processes like REPLs, debuggers, and CLI tools that require multi-step conversation.

---

## The Problem

The Bash tool is one-shot: run a command, get output, done. This prevents:
- Interacting with REPLs (python, node, gdb)
- Testing interactive CLI behaviour (e.g., AskUserQuestion prompts)
- Controlling processes that require multi-step interaction
- Monitoring long-running processes

## The Solution

Use `pty-session` - a tmux-based session manager that allows creating, interacting with, and reading from persistent terminal sessions.

---

## Commands

```bash
pty-session create <name> <command>   # Create session running command
pty-session send <name> <text>        # Send keystrokes (adds Enter by default)
pty-session read <name>               # Capture current screen content
pty-session wait <name> <pattern>     # Poll until pattern appears
pty-session kill <name>               # Terminate session
pty-session list                      # Show active sessions
pty-session help                      # Show usage
```

### Options

- `--no-enter` with `send`: Don't append Enter after text
- `--timeout=N` with `wait`: Timeout in seconds (default 60)
- `--scrollback=N` with `read`: Lines of history to capture (default 100)

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | General error |
| 2 | Session not found |
| 3 | Timeout (wait command) |
| 4 | Invalid arguments |

---

## Usage Pattern

### Basic Interaction

```bash
# Start a Python REPL
pty-session create mypy 'python3'

# Wait for prompt
pty-session wait mypy '>>>'

# Send commands
pty-session send mypy 'x = 42'
pty-session send mypy 'print(x * 2)'

# Read output
pty-session read mypy

# Clean up
pty-session kill mypy
```

### Testing Interactive Behaviour

```bash
# Start claude in a test directory
cd /path/to/test/repo
pty-session create test_claude 'claude'

# Wait for ready
pty-session wait test_claude '❯' --timeout=30

# Send command
pty-session send test_claude '/nbs'

# Wait for and detect AskUserQuestion
pty-session wait test_claude 'investigation' --timeout=120

# Read to see full context
OUTPUT=$(pty-session read test_claude)

# Respond if question detected
if echo "$OUTPUT" | grep -q "Are you.*investigation"; then
    pty-session send test_claude 'Yes'
fi

# Clean up
pty-session kill test_claude
```

### Monitoring Long-Running Process

```bash
# Start build
pty-session create build 'make -j8'

# Check progress periodically
pty-session read build | tail -10

# Wait for completion
pty-session wait build 'Build complete' --timeout=600

# Clean up
pty-session kill build
```

---

## Important Notes

### Session Isolation

- All sessions are prefixed with `pty_` internally
- Sessions created via `pty-session` are isolated from user's tmux sessions
- Running `pty-session list` only shows pty-session sessions
- Running `pty-session kill` cannot affect non-pty-session tmux sessions

### Timing Considerations

- After `send`, the command may not have executed yet
- Use `wait` to block until expected output appears
- Use `read` to capture current state without waiting

### Nested Tmux

- Works correctly when already running inside tmux
- Creates detached sessions that don't interfere with the parent

### Double-Enter Issue

- Some TUI applications need Enter sent twice
- If a command doesn't execute, try: `pty-session send name '' ` (send empty string to trigger extra Enter)

---

## When to Use This

- Interacting with REPLs (Python, Node, GDB, psql)
- Testing interactive CLI behaviour
- Automating multi-step terminal workflows
- Running processes that need monitoring and intervention
- Any situation where you need to send input and read output across multiple tool calls

---

## Location

The `pty-session` script is at: `~/claude_docs/nbs-framework/bin/pty-session`

Ensure it's in your PATH or use the full path.
