#!/bin/bash
# EPIC 10: INV-F4 Manifest Immutability Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST="${PROJECT_ROOT}/.artifacts/manifest.sha256"
echo "[INV-F4] Manifest Immutability Check..."
test -f "$MANIFEST" || { echo "✗ FAIL: manifest.sha256 missing"; exit 1; }
# Verify file is read-only (444 permissions)
PERMS=$(stat -c "%a" "$MANIFEST" 2>/dev/null || stat -f "%OLp" "$MANIFEST" 2>/dev/null)
if [ "$PERMS" != "444" ]; then
    echo "✗ FAIL: Manifest not immutable (permissions: $PERMS, expected: 444)"
    exit 1
fi
# Verify timestamp hasn't changed after seal (compare with .phase.lock)
if [ -f "${PROJECT_ROOT}/.artifacts/.phase.lock" ]; then
    MANIFEST_TIME=$(stat -c "%Y" "$MANIFEST" 2>/dev/null || stat -f "%m" "$MANIFEST" 2>/dev/null)
    LOCK_TIME=$(stat -c "%Y" "${PROJECT_ROOT}/.artifacts/.phase.lock" 2>/dev/null || stat -f "%m" "${PROJECT_ROOT}/.artifacts/.phase.lock" 2>/dev/null)
    if [ "$MANIFEST_TIME" -gt "$LOCK_TIME" ]; then
        echo "✗ FAIL: Manifest modified after phase lock (timestamp violation)"
        exit 1
    fi
fi
echo "✓ PASS: Manifest is immutable (444 permissions, timestamp preserved)"
exit 0
