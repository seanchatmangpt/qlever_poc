#!/bin/bash
# EPIC 10: INV-2 CMake Configuration Parseable Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "[INV-2] CMake Configuration Parseability Check..."
test -f "${PROJECT_ROOT}/CMakeLists.txt" || { echo "✗ FAIL: CMakeLists.txt missing"; exit 1; }
# Parse check: cmake with dry-run (list only mode)
cd "${PROJECT_ROOT}"
cmake -LA -N . >/dev/null 2>&1 || { echo "✗ FAIL: CMakeLists.txt parse error"; exit 1; }
echo "✓ PASS: CMakeLists.txt is parseable"
exit 0
