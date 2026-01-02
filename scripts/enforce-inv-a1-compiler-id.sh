#!/bin/bash
# EPIC 10: INV-A1 Compiler Identity Format Validation
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COMPILER_ID="${PROJECT_ROOT}/.artifacts/compiler.id"
echo "[INV-A1] Compiler Identity Format Check..."
test -f "$COMPILER_ID" || { echo "✗ FAIL: compiler.id missing"; exit 1; }
# Validate format: must contain "clang" or "gcc" and version number
if grep -qE "(clang|gcc).* version [0-9]+\.[0-9]+" "$COMPILER_ID"; then
    echo "✓ PASS: Compiler ID format valid"
    cat "$COMPILER_ID"
    exit 0
else
    echo "✗ FAIL: Compiler ID format invalid (expected: clang/gcc version X.Y)"
    cat "$COMPILER_ID"
    exit 1
fi
