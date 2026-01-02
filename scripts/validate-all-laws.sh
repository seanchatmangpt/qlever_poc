#!/bin/bash
# EPIC 8.2 Agent 10: Master Law Validation Script
# Orchestrates all law validation checks
# Provides comprehensive enforcement assessment

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "╔════════════════════════════════════════╗"
echo "║   EPIC 8/8.1 LAW VALIDATION SUITE     ║"
echo "║   Agent 10: Impossibility Proof       ║"
echo "╚════════════════════════════════════════╝"

cd "$PROJECT_ROOT"

total_tests=0
failures=0

# Test execution helper
run_validation() {
  local name="$1"
  local script="$2"
  local build_required="$3"

  total_tests=$((total_tests + 1))

  echo ""
  echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
  echo "TEST $total_tests: $name"
  echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

  if [ "$build_required" = "true" ] && [ ! -f .artifacts/.phase.lock ]; then
    echo "⚠ WARNING: Test requires built artifacts, but build incomplete"
    echo "          Skipping test (run 'make universe' first)"
    return 0
  fi

  if bash "$script"; then
    echo "✓ PASSED: $name"
    return 0
  else
    echo "✗ FAILED: $name"
    failures=$((failures + 1))
    return 1
  fi
}

# ============================================
# PHASE 1: STRUCTURAL VALIDATIONS (No Build)
# ============================================

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  PHASE 1: Structural Validations      ║"
echo "║  (No build required)                  ║"
echo "╚════════════════════════════════════════╝"

run_validation \
  "Makefile Structure (AX-2, AX-6)" \
  "$SCRIPT_DIR/validate-makefile-structure.sh" \
  "false"

run_validation \
  "CI/CD Compliance (CI-1 through CI-4)" \
  "$SCRIPT_DIR/audit-ci-compliance.sh" \
  "false"

# ============================================
# PHASE 2: BUILD (If Not Already Built)
# ============================================

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  PHASE 2: Universe Construction       ║"
echo "╚════════════════════════════════════════╝"

if [ ! -f .artifacts/.phase.lock ]; then
  echo ""
  echo "Building universe (required for artifact validations)..."

  if make universe; then
    echo "✓ Universe build successful"
  else
    echo "✗ Universe build failed"
    echo ""
    echo "Cannot proceed with artifact validations"
    exit 1
  fi
else
  echo ""
  echo "✓ Universe already built (.phase.lock exists)"
  echo "  Using existing artifacts for validation"
fi

# ============================================
# PHASE 3: ARTIFACT VALIDATIONS (Build-Dependent)
# ============================================

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  PHASE 3: Artifact Validations        ║"
echo "║  (Build-dependent)                    ║"
echo "╚════════════════════════════════════════╝"

run_validation \
  "Artifact Immutability (AX-4)" \
  "$SCRIPT_DIR/validate-immutability.sh" \
  "true"

# Check if manifest validation script exists
if [ -f "$SCRIPT_DIR/validate-manifest-seal.sh" ]; then
  run_validation \
    "Manifest Seal (INV-F1-F4)" \
    "$SCRIPT_DIR/validate-manifest-seal.sh" \
    "true"
fi

# Check if receipt validation exists (from EPIC 8.1)
if [ -f "$SCRIPT_DIR/validate-receipts.sh" ]; then
  run_validation \
    "Receipt Validation (Deterministic Receipts)" \
    "$SCRIPT_DIR/validate-receipts.sh" \
    "true"
fi

# ============================================
# PHASE 4: DETERMINISM VALIDATION (Most Expensive)
# ============================================

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  PHASE 4: Determinism Validation      ║"
echo "║  (Requires two full builds)           ║"
echo "╚════════════════════════════════════════╝"

echo ""
echo "⚠ WARNING: This test will rebuild the universe twice"
echo "         This may take several minutes"
echo ""

read -p "Run determinism test? (y/N): " -n 1 -r
echo

if [[ $REPLY =~ ^[Yy]$ ]]; then
  run_validation \
    "Deterministic Output (AX-3)" \
    "$SCRIPT_DIR/validate-determinism.sh" \
    "false"
else
  echo "⊘ SKIPPED: Determinism test (user declined)"
  echo "  To run manually: ./scripts/validate-determinism.sh"
fi

# ============================================
# FINAL REPORT
# ============================================

echo ""
echo "╔════════════════════════════════════════╗"
echo "║         VALIDATION SUMMARY             ║"
echo "╚════════════════════════════════════════╝"
echo ""

passed=$((total_tests - failures))

echo "Total Tests:  $total_tests"
echo "Passed:       $passed"
echo "Failed:       $failures"
echo ""

if [ $failures -eq 0 ]; then
  echo "╔════════════════════════════════════════╗"
  echo "║  VERDICT: ENFORCEABLE                  ║"
  echo "║  All tested laws are either            ║"
  echo "║  IMPOSSIBLE or DETECTABLE              ║"
  echo "╚════════════════════════════════════════╝"
  echo ""
  echo "✓ No HIDDEN violations detected"
  echo "✓ All enforcement mechanisms operational"
  echo ""
  exit 0
else
  echo "╔════════════════════════════════════════╗"
  echo "║  VERDICT: VIOLATIONS DETECTED          ║"
  echo "║  $failures laws violated                      ║"
  echo "╚════════════════════════════════════════╝"
  echo ""
  echo "✗ Review failures above"
  echo "✗ See EPIC8_IMPOSSIBILITY_PROOF_ANALYSIS.md"
  echo ""
  exit 1
fi
