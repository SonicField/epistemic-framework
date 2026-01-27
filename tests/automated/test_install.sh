#!/bin/bash
# Test: Verify install.sh creates correct symlinks
#
# Falsification: Exits 0 if all symlinks correct, non-zero otherwise

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
CLAUDE_COMMANDS_DIR="$HOME/.claude/commands"

# Colours for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No colour

FAILED=0

check_symlink() {
    local name="$1"
    local expected_target="$PROJECT_ROOT/claude_tools/$name"
    local actual_link="$CLAUDE_COMMANDS_DIR/$name"

    if [[ ! -L "$actual_link" ]]; then
        echo -e "${RED}FAIL${NC}: $actual_link is not a symlink"
        FAILED=1
        return
    fi

    local actual_target
    actual_target=$(readlink -f "$actual_link")
    local expected_resolved
    expected_resolved=$(readlink -f "$expected_target")

    if [[ "$actual_target" != "$expected_resolved" ]]; then
        echo -e "${RED}FAIL${NC}: $actual_link points to $actual_target, expected $expected_resolved"
        FAILED=1
        return
    fi

    echo -e "${GREEN}PASS${NC}: $name symlink correct"
}

echo "Testing install.sh symlinks..."
echo "Project root: $PROJECT_ROOT"
echo "Claude commands dir: $CLAUDE_COMMANDS_DIR"
echo ""

# Check each tool in claude_tools/
for tool in "$PROJECT_ROOT/claude_tools"/*.md; do
    if [[ -f "$tool" ]]; then
        name=$(basename "$tool")
        check_symlink "$name"
    fi
done

echo ""
if [[ $FAILED -eq 0 ]]; then
    echo -e "${GREEN}All symlinks correct${NC}"
    exit 0
else
    echo -e "${RED}Some symlinks incorrect - run bin/install.sh${NC}"
    exit 1
fi
