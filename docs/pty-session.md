# pty-session: Interactive Terminal Session Manager

pty-session enables Claude (or any automation) to interact with long-running interactive processes across multiple invocations.

## The Problem

Claude's Bash tool executes commands in a one-shot manner. Each command runs in isolation, completes, and returns output. This fails for interactive processes that:

- Require multiple rounds of input (REPLs, debuggers, configuration wizards)
- Maintain state between commands (database clients, SSH sessions)
- Run continuously and accept input over time (development servers)

When Claude runs `python3`, the process starts, receives EOF on stdin, and exits. There is no way to send subsequent commands to the interpreter.

## How It Works

pty-session manages tmux sessions behind the scenes. Each session runs a command in a detached terminal with a proper PTY (pseudo-terminal). This allows:

- The process to stay alive between Bash invocations
- Input to be sent at any time via tmux's send-keys
- Screen content to be captured on demand via tmux's capture-pane

Sessions are prefixed with `pty_` internally. You work with simple names like `myrepl` while the tool manages the underlying `pty_myrepl` session.

## Commands

### create

Create a session running a command.

```bash
pty-session create <name> <command>
```

Example:
```bash
pty-session create pyrepl 'python3'
pty-session create debug 'gdb ./myprogram'
```

### send

Send keystrokes to a session. Enter is appended by default.

```bash
pty-session send <name> <text>
pty-session send <name> --no-enter <text>
```

The `--no-enter` flag is useful for partial input or control sequences.

Example:
```bash
pty-session send pyrepl 'x = 42'
pty-session send pyrepl 'print(x * 2)'
```

### read

Capture current screen content from a running session, or retrieve cached output from a killed session.

```bash
pty-session read <name>
pty-session read <name> --scrollback=200
pty-session read <name> --wait
pty-session read <name> --wait --timeout=300
```

**Options:**
- `--scrollback=N` - Lines of history to capture from running session (default 100)
- `--wait` - Block until session exits, then read cached output
- `--timeout=N` - With `--wait`, timeout in seconds (default 300)

**Behaviour:**
1. If session is running, capture current pane content
2. If session not running, try to read from cache (left by kill command)
3. If neither, return error

**Wait mode:**
When `--wait` is used, read blocks until the session is no longer running (killed or naturally exited), then attempts to read from cache. This is useful when you spawn a worker and want to collect its final output without polling.

### wait

Poll until a pattern appears in output, or timeout.

```bash
pty-session wait <name> <pattern>
pty-session wait <name> <pattern> --timeout=120
```

Pattern is matched using grep against captured screen content. Default timeout is 60 seconds, polling every 0.5 seconds.

Example:
```bash
pty-session wait pyrepl '>>>'
pty-session wait compile 'Build succeeded' --timeout=300
```

### kill

Terminate a session and cache its final output.

```bash
pty-session kill <name>
```

**Output caching:**
When a session is killed, its final screen content is automatically saved to `~/.pty-session/cache/`. This cache can be read once using the `read` command. After reading, the cache is automatically deleted.

**Important:** Only killed sessions have their output cached. Sessions that exit naturally (command completes) do not leave cached output.

### list

Show all pty-session sessions with their status.

```bash
pty-session list
```

**Output format:**
```
Active pty-session sessions:
  myworker             running
  oldworker            killed
```

- `running` - Session is currently active
- `killed` - Session was killed and has cached output available

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | General error |
| 2 | Session not found |
| 3 | Timeout (for wait) |
| 4 | Invalid arguments |

## Example: Python REPL

```bash
pty-session create pyrepl 'python3'
pty-session wait pyrepl '>>>'

pty-session send pyrepl 'import math'
pty-session send pyrepl 'math.sqrt(2)'
pty-session read pyrepl

pty-session kill pyrepl

# Read cached output
pty-session read pyrepl
```

## Example: Worker with Blocking Read

```bash
# Spawn worker
pty-session create worker 'python3 long_running_task.py'

# Do other work...

# Block until worker finishes, then read output
pty-session read worker --wait

# Cache is now consumed
```

## Session Exit Behaviour

**Killed sessions:**
When you kill a session with `pty-session kill`, the final screen content is captured to `~/.pty-session/cache/`. This cache persists until you read it with `pty-session read`, after which it is automatically deleted.

**Naturally exited sessions:**
If a session's command completes on its own (e.g., `echo done` finishes and exits), no cache is created. The output is lost.

To preserve output, always kill sessions explicitly.

## Cache Directory

Killed session output is stored in `~/.pty-session/cache/`:
- `<name>.output` - Final screen content (last 100 lines)
- `<name>.timestamp` - When session was killed

Cache files are automatically deleted after being read.

## Important Notes

### Security Implications

pty-session creates a fresh login shell with full user permissions. Commands run through pty-session inherit the user's complete environment and capabilities, not the sandboxed context of Claude's Bash tool.

Environment variables (like proxy settings) may differ from the Bash tool context. Commands that fail in Bash due to sandbox restrictions may succeed via pty-session. Claude can effectively do anything the user can do through their terminal.

This is powerful but requires trust. The tool exists for legitimate automation (testing, REPLs, interactive programs). It should not be used to circumvent intended safety measures.

### Session Isolation

All sessions are prefixed with `pty_` internally. They do not interfere with your personal tmux sessions.

### Timing

- The send command includes a 0.1s delay before Enter for reliable submission
- The wait command polls every 0.5 seconds
- If you send commands too quickly after create, the process may not be ready

### Nested tmux

pty-session works inside an existing tmux session. It targets sessions by name using the `-t` flag, so nested operation is not a problem.

### Cleanup

Always kill sessions when done. Orphaned sessions consume resources. Use `pty-session list` to check for lingering sessions.

## Location

```
bin/pty-session
```

Installed to `~/.claude/commands/` by `bin/install.sh`.
