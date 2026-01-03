#!/bin/bash
# Ninja Build - Simplified
# Usage: ./scripts/ninja-build.sh [ninja-args...]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build}"

# Verify CMake is configured
if [ ! -f "$BUILD_DIR/build.ninja" ] && [ ! -f "$BUILD_DIR/Makefile" ]; then
    echo "FATAL: CMake not configured. Run: cmake -B build -GNinja .." >&2
    exit 1
fi

# Run Ninja
cd "$BUILD_DIR"
ninja "$@"
