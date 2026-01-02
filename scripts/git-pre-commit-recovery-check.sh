#!/bin/bash
# EPIC 8.2 Agent 3: Git Pre-Commit Hook for Recovery Pattern Prevention
# Purpose: Prevent commits that introduce recovery patterns
# Installation: cp scripts/git-pre-commit-recovery-check.sh .git/hooks/pre-commit && chmod +x .git/hooks/pre-commit

set -e

echo "Running EPIC 8.2 recovery pattern check..."

# Get list of staged C++ files
STAGED_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|h)$' || true)

if [ -z "$STAGED_FILES" ]; then
  echo "No C++ files staged, skipping recovery pattern check"
  exit 0
fi

VIOLATIONS=0

# Check 1: Exception catching without termination
echo "[1/4] Checking exception handling..."
for file in $STAGED_FILES; do
  if [ -f "$file" ]; then
    if git diff --cached "$file" | grep -E "^\+.*catch\s*\(" | grep -v "exit\|abort\|terminate" > /dev/null 2>&1; then
      echo "  ❌ VIOLATION: Exception catching without termination in $file"
      VIOLATIONS=$((VIOLATIONS + 1))
    fi
  fi
done

# Check 2: Optional fallbacks
echo "[2/4] Checking optional fallbacks..."
for file in $STAGED_FILES; do
  if [ -f "$file" ]; then
    if git diff --cached "$file" | grep -E "^\+.*\.value_or\(" > /dev/null 2>&1; then
      echo "  ❌ VIOLATION: Optional .value_or() detected in $file"
      VIOLATIONS=$((VIOLATIONS + 1))
    fi
  fi
done

# Check 3: Retry patterns
echo "[3/4] Checking retry patterns..."
for file in $STAGED_FILES; do
  if [ -f "$file" ]; then
    if git diff --cached "$file" | grep -Ei "^\+.*(retry|retries|max.*attempt)" > /dev/null 2>&1; then
      echo "  ⚠️  WARNING: Potential retry pattern in $file (manual review required)"
    fi
  fi
done

# Check 4: Fallback/default patterns
echo "[4/4] Checking fallback patterns..."
for file in $STAGED_FILES; do
  if [ -f "$file" ]; then
    if git diff --cached "$file" | grep -Ei "^\+.*(fallback|or_else|unwrap_or)" > /dev/null 2>&1; then
      echo "  ❌ VIOLATION: Fallback pattern detected in $file"
      VIOLATIONS=$((VIOLATIONS + 1))
    fi
  fi
done

# Summary
if [ $VIOLATIONS -gt 0 ]; then
  echo
  echo "═══════════════════════════════════════════════════════════"
  echo "❌ COMMIT REJECTED: $VIOLATIONS recovery pattern(s) detected"
  echo "═══════════════════════════════════════════════════════════"
  echo
  echo "Recovery patterns violate EPIC 8 fail-closed semantics."
  echo "Remove recovery patterns and use single abort path (exit 1)."
  echo
  echo "For help, see: docs/EPIC8.2_AGENT3_RECOVERY_COLLAPSE_ENFORCER.md"
  exit 1
else
  echo "✅ Recovery pattern check passed"
  exit 0
fi
