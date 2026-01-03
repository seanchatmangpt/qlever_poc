#!/bin/bash
# CMake Configuration - Simplified
# Usage: ./scripts/cmake-configure.sh [cmake-args...]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build}"

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Run CMake
cd "$BUILD_DIR"
cmake "$@" ..
