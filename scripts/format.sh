#!/bin/bash
# Format code without committing

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Formatting code (clang-format + codespell)..."
echo ""

cd "$PROJECT_DIR"

# Run pre-commit formatters on all files
pre-commit run --all-files

echo ""
echo "✓ Code formatted!"
echo ""
echo "Tip: Files are now formatted. Stage and commit when ready:"
echo "  git add <files>"
echo "  git commit -m 'message'"
