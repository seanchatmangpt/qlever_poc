#!/bin/bash
# EPIC 10: INV-B3 Test Directory Content Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "[INV-B3] Test Directory Content Check..."
test -d "${PROJECT_ROOT}/test" || { echo "✗ FAIL: test/ directory missing"; exit 1; }
# Verify test/ contains at least one test file
if find "${PROJECT_ROOT}/test" -type f \( -name "*Test.cpp" -o -name "*_test.cpp" -o -name "*test.cpp" \) 2>/dev/null | grep -q .; then
    COUNT=$(find "${PROJECT_ROOT}/test" -type f \( -name "*Test.cpp" -o -name "*_test.cpp" -o -name "*test.cpp" \) 2>/dev/null | wc -l)
    echo "✓ PASS: test/ contains $COUNT test files"
    exit 0
else
    echo "✗ FAIL: test/ directory empty or no test files found (*Test.cpp, *_test.cpp)"
    exit 1
fi
