#!/bin/bash
# EPIC 8.2 Agent 10: Immutability Validation Script
# Validates AX-4: Immutable Artifacts
# Verifies all artifacts have read-only permissions (444)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "========================================="
echo "ARTIFACT IMMUTABILITY VALIDATION"
echo "========================================="

cd "$PROJECT_ROOT"

violations=0

# List of artifacts that must be immutable
artifacts=(
  ".artifacts/compiler.id"
  ".artifacts/flags.env"
  ".artifacts/manifest.sha256"
  ".artifacts/.phase.lock"
)

echo ""
echo "Checking artifact permissions..."

for artifact in "${artifacts[@]}"; do
  if [ -f "$artifact" ]; then
    perms=$(stat -c "%a" "$artifact" 2>/dev/null || stat -f "%Lp" "$artifact" 2>/dev/null)

    if [ "$perms" = "444" ]; then
      echo "  ✓ $artifact: $perms (read-only)"
    else
      echo "  ✗ $artifact: $perms (WRITABLE - expected 444)"
      violations=$((violations + 1))
    fi
  else
    echo "  ⚠ $artifact: NOT FOUND (build incomplete?)"
  fi
done

echo ""
echo "========================================="
if [ $violations -eq 0 ]; then
  echo "✓ PASS: All artifacts immutable"
  exit 0
else
  echo "✗ FAIL: $violations artifacts are writable"
  echo ""
  echo "VIOLATION: Artifacts must be read-only (chmod 444)"
  echo "This ensures they cannot be modified after sealing"
  exit 1
fi
