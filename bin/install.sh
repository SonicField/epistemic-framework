#!/bin/bash
# Epistemic Framework - Tool Installation Script
#
# Installs Claude Code commands by creating symlinks in ~/.claude/commands/

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CLAUDE_COMMANDS_DIR="$HOME/.claude/commands"

# Ensure target directory exists
mkdir -p "$CLAUDE_COMMANDS_DIR"

# Install Claude tools
echo "Installing Claude Code commands..."

for tool in "$PROJECT_ROOT/claude_tools"/*.md; do
    if [[ -f "$tool" ]]; then
        name=$(basename "$tool")
        target="$CLAUDE_COMMANDS_DIR/$name"

        # Remove existing file/symlink if present
        if [[ -e "$target" || -L "$target" ]]; then
            rm "$target"
        fi

        ln -s "$tool" "$target"
        echo "  Installed: /$name (symlink created)"
    fi
done

echo "Done. Restart Claude Code to pick up new commands."
