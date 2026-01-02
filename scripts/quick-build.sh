#!/bin/bash
#
# Quick development build & test cycle
# Skips expensive phases (SHACL validation, full benchmarking)
# Best for rapid iteration during development
#
# Usage:
#   ./scripts/quick-build.sh                 # Build all
#   ./scripts/quick-build.sh "TestPattern"   # Build & run specific test
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}▶ Quick Build & Test${NC} (development cycle)"
echo "  - Skips SHACL validation (Phase D)"
echo "  - Skips full benchmarking (Phase E/F)"
echo "  - Uses all CPU cores for parallelization"
echo ""

# Phase A: Toolchain verification
echo -e "${BLUE}[Phase A] Toolchain Verification${NC}"
compiler=$(which clang++ || which g++ || echo "NONE")
if [ "$compiler" == "NONE" ]; then
    echo -e "${RED}✗ No C++ compiler found${NC}"
    exit 1
fi
echo -e "${GREEN}✓ Compiler: $($compiler --version | head -1)${NC}"

# Phase C: Build
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

echo ""
echo -e "${BLUE}[Phase C] Compilation${NC}"
cd "$BUILD_DIR"

if [ ! -f "Makefile" ] && [ ! -f "build.ninja" ]; then
    echo "Configuring CMake..."
    cmake -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DUSE_PARALLEL=true \
        "$PROJECT_ROOT" \
        > /dev/null
fi

build_start=$(date +%s)
echo "Building with ninja ($(nproc) jobs)..."
ninja -j$(($(nproc) + 1)) > /dev/null 2>&1 || {
    echo -e "${RED}✗ Build failed${NC}"
    ninja  # Re-run without suppression to see errors
    exit 1
}
build_end=$(date +%s)
build_time=$((build_end - build_start))

echo -e "${GREEN}✓ Build complete (${build_time}s)${NC}"

# Phase E: Tests (selective)
if [ -z "$1" ]; then
    echo ""
    echo -e "${BLUE}[Phase E] Running Unit Tests${NC}"
    test_pattern="*"  # Run all tests
else
    echo ""
    echo -e "${BLUE}[Phase E] Running Tests: $1${NC}"
    test_pattern="$1"
fi

test_start=$(date +%s)
if ctest -R "$test_pattern" \
    -j$(($(nproc) / 2)) \
    --output-on-failure \
    --timeout 300 \
    2>&1 | tail -20; then
    test_end=$(date +%s)
    test_time=$((test_end - test_start))
    echo -e "${GREEN}✓ Tests passed (${test_time}s)${NC}"
else
    echo -e "${RED}✗ Tests failed${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}✓ Quick build & test complete!${NC}"
echo "  Total time: $((build_time + test_time))s"
echo ""
echo "Next steps:"
echo "  - Run linting: make lint"
echo "  - Check format: make format-check"
echo "  - Full test suite: make test"
echo "  - Full deterministic build: make universe"
