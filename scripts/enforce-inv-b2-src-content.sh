#!/bin/bash
# EPIC 10: INV-B2 Source Directory Content Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "[INV-B2] Source Directory Content Check..."
test -d "${PROJECT_ROOT}/src" || { echo "✗ FAIL: src/ directory missing"; exit 1; }
# Verify src/ contains at least one C++ source file
if find "${PROJECT_ROOT}/src" -type f \( -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" \) 2>/dev/null | grep -q .; then
    COUNT=$(find "${PROJECT_ROOT}/src" -type f \( -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" \) 2>/dev/null | wc -l)
    echo "✓ PASS: src/ contains $COUNT C++ source files"
    exit 0
else
    echo "✗ FAIL: src/ directory empty (no .cpp/.cc files found)"
    exit 1
fi
