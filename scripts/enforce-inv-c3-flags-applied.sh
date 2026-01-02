#!/bin/bash
# EPIC 10: INV-C3 Normalized Flags Applied Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CMAKE_CACHE="${PROJECT_ROOT}/build/CMakeCache.txt"
FLAGS_ENV="${PROJECT_ROOT}/.artifacts/flags.env"
echo "[INV-C3] Normalized Flags Application Check..."
test -f "$FLAGS_ENV" || { echo "✗ FAIL: flags.env missing"; exit 1; }
test -f "$CMAKE_CACHE" || { echo "✗ FAIL: CMakeCache.txt missing (build not configured)"; exit 1; }
# Extract expected flags from flags.env
source "$FLAGS_ENV"
# Check CMakeCache.txt contains normalized flags
if grep -q "CMAKE_CXX_FLAGS:STRING=.*-std=c++20.*-O3.*-DNDEBUG" "$CMAKE_CACHE"; then
    echo "✓ PASS: Normalized flags applied in CMake cache"
    grep "CMAKE_CXX_FLAGS:STRING=" "$CMAKE_CACHE"
    exit 0
else
    echo "✗ FAIL: Normalized flags NOT found in CMake cache"
    grep "CMAKE_CXX_FLAGS:STRING=" "$CMAKE_CACHE" || echo "(CMAKE_CXX_FLAGS not set)"
    exit 1
fi
