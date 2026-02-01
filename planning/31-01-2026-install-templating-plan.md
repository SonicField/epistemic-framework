# Plan: Install Templating System

**Date:** 31-01-2026
**Terminal Goal:** Make NBS framework installable to any location with all internal paths correctly resolved.

## Problem Statement

Current state:
- Commands contain hardcoded paths like `~/claude_docs/nbs-framework/concepts/goals.md`
- Only works if cloned to exact same location as dev machine
- Cannot test installation without modifying production `~/.nbs/`

Required state:
- Commands use `{{NBS_ROOT}}` placeholder
- `install.sh` expands templates with actual install path
- Tests can install to `/tmp/xxx` without touching `~/.nbs/`

## Design

### Template Syntax

Single placeholder: `{{NBS_ROOT}}`

Example in template:
```markdown
Read `{{NBS_ROOT}}/concepts/goals.md` for guidance.
```

After install to `~/.nbs`:
```markdown
Read `~/.nbs/concepts/goals.md` for guidance.
```

After install to `/tmp/test-123`:
```markdown
Read `/tmp/test-123/concepts/goals.md` for guidance.
```

### Template Processing (Pure Bash)

```bash
process_template() {
    local template="$1"
    local output="$2"
    local nbs_root="$3"

    while IFS= read -r line || [[ -n "$line" ]]; do
        echo "${line//\{\{NBS_ROOT\}\}/$nbs_root}"
    done < "$template" > "$output"
}
```

### Directory Structure After Install

```
~/.nbs/                          # Symlink to repo OR contains processed files
├── commands/                    # Processed command files (templates expanded)
│   ├── nbs.md
│   ├── nbs-discovery.md
│   └── ...
├── concepts/                    # Symlink or copy
├── docs/                        # Symlink or copy
└── templates/                   # Symlink or copy

~/.claude/commands/
├── nbs.md → ~/.nbs/commands/nbs.md
└── ...
```

### install.sh Changes

```bash
#!/bin/bash
# Usage: ./bin/install.sh [--prefix=PATH]
# Default prefix: ~/.nbs

PREFIX="${1#--prefix=}"
PREFIX="${PREFIX:-$HOME/.nbs}"

# 1. Create prefix directory
mkdir -p "$PREFIX/commands"

# 2. Process templates
for template in claude_tools/*.md; do
    name=$(basename "$template")
    process_template "$template" "$PREFIX/commands/$name" "$PREFIX"
done

# 3. Symlink supporting directories
ln -sf "$(pwd)/concepts" "$PREFIX/concepts"
ln -sf "$(pwd)/docs" "$PREFIX/docs"
ln -sf "$(pwd)/templates" "$PREFIX/templates"

# 4. Create ~/.claude/commands symlinks
mkdir -p ~/.claude/commands
for cmd in "$PREFIX/commands"/*.md; do
    name=$(basename "$cmd")
    ln -sf "$cmd" ~/.claude/commands/"$name"
done
```

## Falsification Criteria

### Primary Falsification (What would prove this WRONG)

After template expansion, scan ALL output files for:

| Pattern | Meaning if found |
|---------|------------------|
| `~/claude_docs/` | Old hardcoded dev path leaked through |
| `/home/alexturner/` | Absolute user path leaked through |
| `~/` not followed by `.nbs` or `.claude` | Stray home-relative path |
| `{{NBS_ROOT}}` | Template not expanded |

**If ANY of these patterns exist in expanded output, the implementation is wrong.**

### Regex Check (Automated, Deterministic)

```bash
check_no_leaked_paths() {
    local install_dir="$1"
    local errors=0

    # Check for old hardcoded paths
    if grep -rq 'claude_docs' "$install_dir/commands/"; then
        echo "FAIL: Found 'claude_docs' in expanded files"
        errors=$((errors + 1))
    fi

    # Check for absolute user paths
    if grep -rq '/home/alexturner' "$install_dir/commands/"; then
        echo "FAIL: Found '/home/alexturner' in expanded files"
        errors=$((errors + 1))
    fi

    # Check for unexpanded templates
    if grep -rq '{{NBS_ROOT}}' "$install_dir/commands/"; then
        echo "FAIL: Found unexpanded '{{NBS_ROOT}}' in expanded files"
        errors=$((errors + 1))
    fi

    # Check all paths reference install_dir
    # Extract all paths, verify they start with install_dir or ~/.claude

    return $errors
}
```

### Worker Check (AI Review, Belt)

Test creates unique temp directory:
```bash
TEST_DIR=$(mktemp -d)
# e.g. /tmp/tmp.Xa7bKc9Qz
./bin/install.sh --prefix="$TEST_DIR"
```

Worker task:
```
Read every .md file in [TEST_INSTALL_DIR]/commands/.
For each file, identify ALL file paths mentioned.
For each path found:
  - If it starts with [TEST_INSTALL_DIR]: VALID
  - If it starts with ~/.claude/commands: VALID
  - Otherwise: INVALID - report the file, line, and path

Report "ALL PATHS VALID" only if zero invalid paths found.
Do not trust. Verify each path character by character.
```

### Adversarial Test Design

**Critical requirement:** Test workers must NOT see `~/.nbs/` production installation.

How to ensure this:

1. **Worker spawned with explicit instruction**: "You are testing an installation at /tmp/test-xxx. Do NOT read from ~/.nbs/ under any circumstances."

2. **Verification**: After worker completes, check its Read tool calls. If ANY accessed `~/.nbs/`, test fails regardless of worker's report.

3. **Clean environment**: Test creates fresh temp directory, installs there, worker only reads from temp.

4. **Tripwire file**: Create `~/.nbs/commands/TRIPWIRE.md` before test. If worker reports seeing it, test fails.

## Steps

### Step 1: Audit Current Paths

**Action:** Find all hardcoded paths in command files.
**Output:** List of files and line numbers with paths to template.
**Falsification:** `grep -rn` finds all instances; manual review confirms none missed.

### Step 2: Update Command Templates

**Action:** Replace hardcoded paths with `{{NBS_ROOT}}` placeholder.
**Output:** Modified command files with placeholders.
**Falsification:**
- `grep -c '{{NBS_ROOT}}'` shows expected count
- No hardcoded paths remain (Step 1 grep returns nothing)

### Step 3: Implement install.sh

**Action:** Write template processing and installation logic.
**Output:** Updated `bin/install.sh`
**Falsification:**
- Script runs without errors
- Creates expected directory structure
- Processes templates correctly

### Step 4: Regex Verification Tests

**Action:** Create automated test script.
**Output:** `tests/automated/test_install_paths.sh`
**Falsification:**
- Test fails on intentionally broken input (planted bad path)
- Test passes on correctly templated input

### Step 5: Worker Verification Tests

**Action:** Create worker task for AI path review.
**Output:** Worker task file, test harness
**Falsification:**
- Worker finds planted bad path in adversarial test
- Worker correctly validates clean installation

### Step 6: Adversarial Integration Test

**Action:** Full end-to-end test with tripwire.
**Output:** Test that installs to temp, runs worker, verifies isolation.
**Falsification:**
- Tripwire not triggered (worker didn't access ~/.nbs/)
- All paths in temp install are valid
- Both regex and worker checks pass

### Step 7: Production Installation

**Action:** Run install.sh with default prefix.
**Output:** Working `~/.nbs/` installation.
**Falsification:**
- All previous tests pass on production install
- Commands work when invoked via `/nbs`

## What Is NOT In Scope

- Complex templating (conditionals, loops, includes)
- Multiple placeholders (only `{{NBS_ROOT}}`)
- Backwards compatibility shims
- Uninstall script (can add later if needed)

## Success Criteria

1. `./bin/install.sh --prefix=/tmp/test` creates valid isolated installation
2. All paths in installed commands reference the install prefix
3. `./bin/install.sh` (default) creates valid `~/.nbs/` installation
4. Workers running tests never access production `~/.nbs/`
5. Both regex AND worker verification pass
