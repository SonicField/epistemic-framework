# Plan: pty-session Improvements

## Terminal Goal

Add three quality-of-life improvements to pty-session without changing its nature: status in list, blocking read, and last-output caching for dead sessions.

## Changes

### 1. List shows status

**Current:**
```
Active pty-session sessions:
  worker-lexer
  git-push
```

**New:**
```
Active pty-session sessions:
  worker-lexer    running
  git-push        killed
```

**Implementation:**
- Running sessions: query `tmux list-sessions`
- Killed sessions: check cache directory for saved state

**Files changed:**
- `bin/pty-session`: `cmd_list()` function

### 2. Read with `--wait` flag

**Current:** Must guess timing with `sleep N && pty-session read name`

**New:** `pty-session read --wait name` blocks until session exits, then returns final output

**Implementation:**
- Poll `tmux has-session` every 0.5s
- When session no longer exists, read from cache
- If already dead, read cache immediately
- Timeout after reasonable duration (300s default, configurable)

**Files changed:**
- `bin/pty-session`: `cmd_read()` function
- Add `--timeout=N` option for wait duration

### 3. Dead sessions cache last output

**Current:** Session exits → disappears → `read` fails with "Session not found"

**New:** Session exits → last output cached → `read` returns it once → cache deleted

**Implementation:**
- Create `~/.pty-session/cache/` directory
- **REVISED APPROACH** (tmux hooks don't fire reliably):
  - `kill` command captures pane content BEFORE killing session
  - Stores to cache, then kills session
  - No hooks needed - simpler and more reliable
- Cache operations:
  - Capture pane to `~/.pty-session/cache/$name.output`
  - Save timestamp to `~/.pty-session/cache/$name.timestamp`
  - Exit code not available (session killed explicitly, not exited naturally)
- `read` checks:
  1. Session alive? Read from tmux
  2. Cache exists? Read cache, delete cache files
  3. Neither? Error

**Files changed:**
- `bin/pty-session`:
  - `cmd_kill()` - capture to cache before killing
  - `cmd_read()` - check cache if session not found
  - `cmd_list()` - include cached sessions
  - Add cache cleanup on successful read

**Cache structure:**
```
~/.pty-session/
  cache/
    worker-name.output     # Last screen content
    worker-name.timestamp  # When session was killed
```

### 4. Documentation updates

**Files to update:**
- `docs/pty-session.md`:
  - Document `--wait` flag for read
  - Document exit status in list output
  - Explain cache behaviour
  - Add cache cleanup notes

**New sections:**
- "Session Exit Behaviour" - what happens when session ends
- "Cache Directory" - where output is stored, when it's cleaned

### 5. Testing

**Manual test scenarios:**

1. **Status display:**
   - Create session, verify "running" in list
   - Kill session, verify "exited (0)" in list
   - Create session that fails, verify "exited (1)" in list

2. **Blocking read:**
   - Create session running `sleep 5 && echo done && exit`
   - Run `read --wait` - should block ~5s, return output
   - Try `read --wait` with already-dead session - should return cached output

3. **Cache behaviour:**
   - Create session, let it exit
   - Run `read` - should get cached output
   - Run `read` again - should fail (cache consumed)

4. **Timeout:**
   - Create long-running session
   - Run `read --wait --timeout=5` - should timeout after 5s

**Adversarial tests:**

1. **Race conditions:**
   - Session exits between list and read calls
   - Multiple reads of same dead session (concurrent)
   - Cache file permissions issues

2. **Malicious input:**
   - Session names with special chars: `../../etc/passwd`
   - Session names with spaces
   - Session names with newlines

3. **Resource exhaustion:**
   - Create 100 sessions, let them exit, verify cache doesn't grow unbounded
   - Large output (1MB+) in cached session

4. **Hook failures:**
   - Exit hook fails to capture (tmux gone, disk full)
   - Verify read still handles gracefully

**Test files:**
```
tests/automated/test_pty_session_improvements.sh
tests/manual/qa_pty_session.md (update existing)
```

## Implementation Order

1. Cache infrastructure (directory creation in `cmd_kill`)
2. Update `kill` to capture before killing
3. Update `read` to check cache
4. Add `--wait` flag to `read`
5. Update `list` to show killed sessions
6. Documentation
7. Tests (normal)
8. Tests (adversarial)

## Exit Criteria

- [x] Plan written
- [ ] Implementation complete
- [ ] All three features work as described
- [ ] Documentation updated
- [ ] Manual tests pass
- [ ] Adversarial tests pass
- [ ] No regressions in existing functionality

## Falsification Criteria

**Feature 1 (status in list):**
- Create session, kill it, run list → should show "killed"
- If it shows "running" or not listed, feature failed

**Feature 2 (blocking read):**
- Create session that exits after 3s, run `read --wait` → should return within 4s with output
- If it returns immediately or hangs forever, feature failed

**Feature 3 (dead session cache):**
- Create session, let it exit, run read → should get output
- Run read again → should fail with "Session not found"
- If first read fails or second succeeds, feature failed

## Risk Assessment

**Low risk:**
- Changes are additive (new flags, new behaviour for dead sessions)
- Existing commands unchanged
- Cache is opt-in (only populated when session exits)

**Medium risk:**
- Hook installation could fail on some tmux versions
- Cache directory permissions on shared systems
- Race conditions between exit and cache capture

**Mitigation:**
- Test hook installation, provide graceful degradation
- Use `mkdir -p` with safe permissions (700)
- Atomic file operations where possible
