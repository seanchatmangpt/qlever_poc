#!/bin/bash
# EPIC 10: INV-SEAL-3 Completion Proof Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PHASE_LOCK="${PROJECT_ROOT}/.artifacts/.phase.lock"
ARTIFACTS_DIR="${PROJECT_ROOT}/.artifacts"
echo "[INV-SEAL-3] Completion Proof Check..."
test -f "$PHASE_LOCK" || { echo "✗ FAIL: .phase.lock missing"; exit 1; }
# Verify all phase artifacts exist
MISSING=""
test -f "${ARTIFACTS_DIR}/compiler.id" || MISSING="${MISSING} compiler.id"
test -f "${ARTIFACTS_DIR}/flags.env" || MISSING="${MISSING} flags.env"
test -f "${ARTIFACTS_DIR}/manifest.sha256" || MISSING="${MISSING} manifest.sha256"
if [ -n "$MISSING" ]; then
    echo "✗ FAIL: Phase lock exists but missing phase artifacts:$MISSING"
    exit 1
fi
# Verify manifest is non-empty
test -s "${ARTIFACTS_DIR}/manifest.sha256" || { echo "✗ FAIL: manifest.sha256 is empty"; exit 1; }
echo "✓ PASS: Completion proven (lock exists, all phase artifacts present)"
exit 0
