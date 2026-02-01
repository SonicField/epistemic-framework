#!/bin/bash
# NBS Framework - Installation Script
#
# Usage: ./bin/install.sh [--prefix=PATH]
# Default prefix: ~/.nbs
#
# Creates:
#   PREFIX/commands/     - Processed command files (templates expanded)
#   PREFIX/concepts/     - Symlink to repo concepts/
#   PREFIX/docs/         - Symlink to repo docs/
#   PREFIX/templates/    - Symlink to repo templates/
#   PREFIX/bin/          - Symlink to repo bin/
#   ~/.claude/commands/* - Symlinks to PREFIX/commands/*

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Parse arguments
PREFIX="$HOME/.nbs"
for arg in "$@"; do
    case $arg in
        --prefix=*)
            PREFIX="${arg#--prefix=}"
            ;;
        --help|-h)
            echo "Usage: $0 [--prefix=PATH]"
            echo "Default prefix: ~/.nbs"
            exit 0
            ;;
        *)
            echo "Unknown argument: $arg"
            echo "Usage: $0 [--prefix=PATH]"
            exit 1
            ;;
    esac
done

# Resolve to absolute path
PREFIX="$(cd "$(dirname "$PREFIX")" 2>/dev/null && pwd)/$(basename "$PREFIX")" || PREFIX="$PREFIX"

CLAUDE_COMMANDS_DIR="$HOME/.claude/commands"

# Template processing function
process_template() {
    local template="$1"
    local output="$2"
    local nbs_root="$3"

    while IFS= read -r line || [[ -n "$line" ]]; do
        echo "${line//\{\{NBS_ROOT\}\}/$nbs_root}"
    done < "$template" > "$output"
}

echo "Installing NBS Framework..."
echo "  Prefix: $PREFIX"
echo "  Source: $PROJECT_ROOT"

# 1. Create prefix directory structure
mkdir -p "$PREFIX/commands"

# 2. Process command templates
echo "Processing command templates..."
for template in "$PROJECT_ROOT/claude_tools"/*.md; do
    if [[ -f "$template" ]]; then
        name=$(basename "$template")
        output="$PREFIX/commands/$name"
        process_template "$template" "$output" "$PREFIX"
        echo "  Processed: $name"
    fi
done

# 3. Symlink supporting directories
echo "Creating symlinks to supporting directories..."
for dir in concepts docs templates bin; do
    target="$PREFIX/$dir"
    if [[ -e "$target" || -L "$target" ]]; then
        rm -rf "$target"
    fi
    ln -s "$PROJECT_ROOT/$dir" "$target"
    echo "  Linked: $dir/"
done

# 4. Create ~/.claude/commands symlinks
echo "Installing Claude Code commands..."
mkdir -p "$CLAUDE_COMMANDS_DIR"

for cmd in "$PREFIX/commands"/*.md; do
    if [[ -f "$cmd" ]]; then
        name=$(basename "$cmd")
        target="$CLAUDE_COMMANDS_DIR/$name"

        if [[ -e "$target" || -L "$target" ]]; then
            rm "$target"
        fi

        ln -s "$cmd" "$target"
        echo "  Installed: /$name"
    fi
done

echo ""
echo "Installation complete."
echo "  Framework root: $PREFIX"
echo "  Commands: $CLAUDE_COMMANDS_DIR"
echo ""
echo "Restart Claude Code to pick up new commands."
