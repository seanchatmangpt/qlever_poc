#!/bin/bash
# EPIC 10: INV-5 Artifacts Directory Isolation Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "[INV-5] Artifacts Directory Isolation Check..."
# Verify .artifacts/ contains no build outputs (*.o, *.a, *.so, executables)
if [ -d "${PROJECT_ROOT}/.artifacts" ]; then
    if find "${PROJECT_ROOT}/.artifacts" -type f \( -name "*.o" -o -name "*.a" -o -name "*.so" \) 2>/dev/null | grep -q .; then
        echo "✗ FAIL: Build outputs found in .artifacts/ (should be in build/)"
        exit 1
    fi
fi
# Verify build/ contains no phase artifacts (compiler.id, flags.env, manifest.sha256)
if [ -d "${PROJECT_ROOT}/build" ]; then
    if [ -f "${PROJECT_ROOT}/build/compiler.id" ] || [ -f "${PROJECT_ROOT}/build/flags.env" ] || [ -f "${PROJECT_ROOT}/build/manifest.sha256" ]; then
        echo "✗ FAIL: Phase artifacts found in build/ (should be in .artifacts/)"
        exit 1
    fi
fi
echo "✓ PASS: Artifacts directory isolated from build"
exit 0
