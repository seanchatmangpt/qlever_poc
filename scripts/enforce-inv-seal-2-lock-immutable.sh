#!/bin/bash
# EPIC 10: INV-SEAL-2 Phase Lock Immutability Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PHASE_LOCK="${PROJECT_ROOT}/.artifacts/.phase.lock"
echo "[INV-SEAL-2] Phase Lock Immutability Check..."
test -f "$PHASE_LOCK" || { echo "✗ FAIL: .phase.lock missing (build not complete)"; exit 1; }
# Verify file is read-only (444 permissions)
PERMS=$(stat -c "%a" "$PHASE_LOCK" 2>/dev/null || stat -f "%OLp" "$PHASE_LOCK" 2>/dev/null)
if [ "$PERMS" != "444" ]; then
    echo "✗ FAIL: Phase lock not immutable (permissions: $PERMS, expected: 444)"
    exit 1
fi
echo "✓ PASS: Phase lock is immutable (444 permissions)"
exit 0
