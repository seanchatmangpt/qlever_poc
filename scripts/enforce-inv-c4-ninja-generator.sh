#!/bin/bash
# EPIC 10: INV-C4 Ninja Generator Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
echo "[INV-C4] Ninja Generator Check..."
test -d "$BUILD_DIR" || { echo "✗ FAIL: build/ directory missing"; exit 1; }
# Verify build.ninja exists
test -f "${BUILD_DIR}/build.ninja" || { echo "✗ FAIL: build.ninja missing (Ninja not used)"; exit 1; }
# Verify CMake-generated Makefile is either absent or is a wrapper for Ninja
if [ -f "${BUILD_DIR}/Makefile" ]; then
    # Check if it's a CMake-generated Ninja wrapper
    if grep -q "cmake.*--build" "${BUILD_DIR}/Makefile" 2>/dev/null; then
        echo "✓ PASS: Ninja generator used (Makefile is CMake wrapper)"
        exit 0
    else
        echo "✗ FAIL: Makefile generator used instead of Ninja"
        exit 1
    fi
else
    echo "✓ PASS: Ninja generator used (build.ninja exists, no Makefile)"
    exit 0
fi
