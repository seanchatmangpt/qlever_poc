#!/bin/bash
# EPIC 10: INV-4 Build Directory Isolation Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "[INV-4] Build Directory Isolation Check..."
# Check for in-source build pollution
if [ -f "${PROJECT_ROOT}/src/CMakeCache.txt" ] || [ -f "${PROJECT_ROOT}/test/CMakeCache.txt" ]; then
    echo "✗ FAIL: In-source build detected (CMakeCache.txt in src/ or test/)"
    exit 1
fi
if [ -d "${PROJECT_ROOT}/src/CMakeFiles" ] || [ -d "${PROJECT_ROOT}/test/CMakeFiles" ]; then
    echo "✗ FAIL: In-source build detected (CMakeFiles in src/ or test/)"
    exit 1
fi
# Verify build artifacts are in build/ only
if [ -d "${PROJECT_ROOT}/build" ]; then
    if [ ! -f "${PROJECT_ROOT}/build/CMakeCache.txt" ]; then
        echo "⚠ WARNING: build/ exists but CMakeCache.txt missing (not built yet)"
    fi
fi
echo "✓ PASS: Build directory isolated (no in-source artifacts)"
exit 0
