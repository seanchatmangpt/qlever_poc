#!/bin/bash
# EPIC 8.2 Agent 10: Makefile Structure Validation Script
# Validates AX-2 (Atomic Failure) and AX-6 (Sequential Phases)
# Verifies Makefile dependency graph and fail-closed semantics

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "========================================="
echo "MAKEFILE STRUCTURE VALIDATION"
echo "========================================="

cd "$PROJECT_ROOT"

violations=0

# AX-6: Check for .NOTPARALLEL directive
echo ""
echo "Checking AX-6: Sequential Phase Guarantee..."

if grep -q "^\.NOTPARALLEL" Makefile; then
  echo "  ✓ .NOTPARALLEL directive present"
else
  echo "  ✗ VIOLATION: .NOTPARALLEL directive missing"
  echo "    Without this, 'make -j' could parallelize phases"
  violations=$((violations + 1))
fi

# AX-2: Check for .IGNORE directive (should NOT exist)
echo ""
echo "Checking AX-2: Atomic Failure Semantics..."

if grep -q "^\.IGNORE" Makefile; then
  echo "  ✗ VIOLATION: .IGNORE directive found"
  echo "    This violates atomic failure - errors would be ignored"
  violations=$((violations + 1))
else
  echo "  ✓ No .IGNORE directives (atomic failure enforced)"
fi

# Check for -k flag usage (should NOT exist)
if grep -q "\-k\|--keep-going" Makefile; then
  echo "  ✗ VIOLATION: -k/--keep-going flag found"
  echo "    This violates atomic failure"
  violations=$((violations + 1))
else
  echo "  ✓ No -k flags (atomic failure enforced)"
fi

# AX-1: Check universe target dependency chain
echo ""
echo "Checking AX-1: Single Entry Point..."

if ! grep -q "^universe:" Makefile; then
  echo "  ✗ VIOLATION: universe target not found"
  violations=$((violations + 1))
else
  echo "  ✓ universe target exists"

  # Verify full dependency chain
  if ! grep "^universe:" Makefile | grep -q "phase-a phase-b phase-c phase-d phase-e phase-f"; then
    echo "  ✗ VIOLATION: universe dependency chain incomplete"
    violations=$((violations + 1))
  else
    echo "  ✓ universe depends on all phases (A→F)"
  fi
fi

# AX-6: Verify sequential phase dependencies
echo ""
echo "Checking phase dependency chain..."

phases=(a b c d e f)
for i in {1..5}; do
  phase="${phases[$i]}"
  prev="${phases[$i-1]}"

  if grep -q "^phase-$phase:" Makefile; then
    if grep "^phase-$phase:" Makefile | grep -q "phase-$prev"; then
      echo "  ✓ phase-$phase depends on phase-$prev"
    else
      echo "  ✗ VIOLATION: phase-$phase doesn't depend on phase-$prev"
      violations=$((violations + 1))
    fi
  else
    echo "  ⚠ WARNING: phase-$phase target not found"
  fi
done

# Check for set -e in phase scripts
echo ""
echo "Checking shell error propagation (set -e)..."

script_violations=0
for script in scripts/phase-*.sh; do
  if [ -f "$script" ]; then
    if head -10 "$script" | grep -q "set -e"; then
      echo "  ✓ $(basename "$script") has 'set -e'"
    else
      echo "  ✗ $(basename "$script") missing 'set -e'"
      script_violations=$((script_violations + 1))
    fi
  fi
done

if [ $script_violations -gt 0 ]; then
  echo "  ✗ VIOLATION: $script_violations phase scripts missing 'set -e'"
  violations=$((violations + 1))
fi

echo ""
echo "========================================="
if [ $violations -eq 0 ]; then
  echo "✓ PASS: Makefile structure valid"
  echo "  - Sequential phases enforced (.NOTPARALLEL)"
  echo "  - Atomic failure enforced (no .IGNORE)"
  echo "  - Single entry point (universe target)"
  echo "  - Complete dependency chain (A→B→...→F)"
  exit 0
else
  echo "✗ FAIL: $violations Makefile violations detected"
  echo ""
  echo "STRUCTURAL VIOLATIONS DETECTED"
  echo "See EPIC8_SPECIFICATION_CLOSURE.md for requirements"
  exit 1
fi
