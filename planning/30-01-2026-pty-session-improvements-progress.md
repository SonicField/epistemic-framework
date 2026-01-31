# Progress: pty-session Improvements

## Session 1: 2026-01-30

### Actions

**Plan created:**
- Documented three features: status in list, blocking read, cache for dead sessions
- Falsification criteria defined
- Implementation order established

**Epistemic review:**
- Identified missing progress log (created)
- Identified plan not committed (committed)
- Verified adversarial test coverage needed
- Tested tmux hook capability before implementation

**Hook testing:**
- Created `temp_hook_test.sh` to test tmux `session-closed` hooks
- **Finding:** tmux hooks don't fire reliably on session close
- Verified pre-kill capture works fine (`tmux capture-pane` before kill)
- **Decision:** Revised approach - `kill` captures to cache before killing session
- No hooks needed - simpler and more reliable
- Updated plan to reflect revised implementation

**Plan updates:**
- Changed cache mechanism from hook-based to kill-based
- Removed exit code from cache (not available for killed sessions)
- Changed list output from "exited (N)" to "killed"
- Updated implementation order

**Implementation:**
- Added `CACHE_DIR` constant
- Added cache helper functions: `cache_exists()`, `cache_session()`, `read_cache()`
- Updated `cmd_kill()` to capture output before killing
- Updated `cmd_read()` to:
  - Check cache if session not found
  - Support `--wait` flag to block until session exits
  - Support `--timeout` for wait mode
- Updated `cmd_list()` to show running/killed status
- Updated usage documentation in script header

**Testing:**
- Created `temp.sh` with 6 test cases
- All tests pass:
  1. ✓ Status shows "running" for active sessions
  2. ✓ Status shows "killed" for killed sessions
  3. ✓ Read from cache works
  4. ✓ Cache consumed after read
  5. ✓ Blocking read with --wait works
  6. ✓ --wait works with already-killed session

**Documentation:**
- Updated `docs/pty-session.md`:
  - read command: document --wait flag and cache behaviour
  - kill command: document output caching
  - list command: document status display
  - Added "Session Exit Behaviour" section
  - Added "Cache Directory" section
  - Added example for worker with blocking read

### Next Steps

1. Commit implementation and documentation
2. Create formal test file in `tests/`
3. Consider adversarial test coverage
4. Clean up temp files
5. Push to remote

### Status

- [x] Cache infrastructure
- [x] Feature 1: status in list
- [x] Feature 2: blocking read
- [x] Feature 3: dead session cache
- [x] Documentation
- [x] Tests (basic)
- [ ] Adversarial tests
- [ ] Formal test files
