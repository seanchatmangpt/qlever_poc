#!/bin/bash
# EPIC 10: INV-A3 No Environment Leakage Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FLAGS_ENV="${PROJECT_ROOT}/.artifacts/flags.env"
echo "[INV-A3] Environment Leakage Check..."
test -f "$FLAGS_ENV" || { echo "✗ FAIL: flags.env missing"; exit 1; }
# Verify file is read-only (444 permissions)
PERMS=$(stat -c "%a" "$FLAGS_ENV" 2>/dev/null || stat -f "%OLp" "$FLAGS_ENV" 2>/dev/null)
if [ "$PERMS" != "444" ]; then
    echo "✗ FAIL: flags.env not immutable (permissions: $PERMS, expected: 444)"
    exit 1
fi
# Verify timestamp hasn't changed (compare with .phase.lock if exists)
if [ -f "${PROJECT_ROOT}/.artifacts/.phase.lock" ]; then
    FLAGS_TIME=$(stat -c "%Y" "$FLAGS_ENV" 2>/dev/null || stat -f "%m" "$FLAGS_ENV" 2>/dev/null)
    LOCK_TIME=$(stat -c "%Y" "${PROJECT_ROOT}/.artifacts/.phase.lock" 2>/dev/null || stat -f "%m" "${PROJECT_ROOT}/.artifacts/.phase.lock" 2>/dev/null)
    if [ "$FLAGS_TIME" -gt "$LOCK_TIME" ]; then
        echo "✗ FAIL: flags.env modified after phase lock (timestamp mismatch)"
        exit 1
    fi
fi
echo "✓ PASS: No environment leakage (flags.env immutable)"
exit 0
