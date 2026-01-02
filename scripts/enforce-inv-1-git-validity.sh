#!/bin/bash
# EPIC 10: INV-1 Git Repository Validity Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "[INV-1] Git Repository Validity Check..."
test -d "${PROJECT_ROOT}/.git" || { echo "✗ FAIL: .git directory missing"; exit 1; }
test -f "${PROJECT_ROOT}/.git/HEAD" || { echo "✗ FAIL: .git/HEAD missing"; exit 1; }
test -f "${PROJECT_ROOT}/.git/config" || { echo "✗ FAIL: .git/config missing"; exit 1; }
cd "${PROJECT_ROOT}"
git rev-parse HEAD >/dev/null 2>&1 || { echo "✗ FAIL: git rev-parse HEAD failed (corrupt repo)"; exit 1; }
echo "✓ PASS: Git repository valid"
exit 0
