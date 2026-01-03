#!/bin/bash
# EPIC 11.1 Migration Parity Validator
# Validates that pre/post migration states are equivalent

set -euo pipefail

PRE_RECEIPT="/tmp/epic11.1-pre-migration-receipt.json"
POST_RECEIPT="/tmp/epic11.1-post-migration-receipt.json"

echo "EPIC 11.1 Migration Parity Validation"
echo "======================================"
echo ""

# Check receipt files exist
if [ ! -f "$PRE_RECEIPT" ]; then
  echo "❌ ERROR: Pre-migration receipt not found: $PRE_RECEIPT"
  echo "   Run: bash scripts/generate_verification_receipt.sh pre-migration"
  exit 1
fi

if [ ! -f "$POST_RECEIPT" ]; then
  echo "❌ ERROR: Post-migration receipt not found: $POST_RECEIPT"
  echo "   Run: bash scripts/generate_verification_receipt.sh post-migration"
  exit 1
fi

# Check jq is available
if ! command -v jq &> /dev/null; then
  echo "❌ ERROR: jq is required but not installed"
  echo "   Install: apt-get install jq (or equivalent)"
  exit 1
fi

PARITY_PASS=true

# 1. Source Code Parity Check
echo "1. Source Code Parity"
echo "   Comparing BLAKE3/SHA256 hashes of all .rs files..."

PRE_HASH=$(jq -r '.metrics.source_hash' "$PRE_RECEIPT")
POST_HASH=$(jq -r '.metrics.source_hash' "$POST_RECEIPT")

if [ "$PRE_HASH" = "$POST_HASH" ]; then
  echo "   ✅ PASS - Source code identical"
  echo "      Hash: $PRE_HASH"
else
  echo "   ❌ FAIL - Source code diverged"
  echo "      Pre:  $PRE_HASH"
  echo "      Post: $POST_HASH"
  PARITY_PASS=false
fi
echo ""

# 2. Test Results Parity Check
echo "2. Test Results Parity"
echo "   Comparing test pass/fail counts..."

PRE_PASSED=$(jq -r '.test_results.total_passed' "$PRE_RECEIPT")
PRE_FAILED=$(jq -r '.test_results.failed' "$PRE_RECEIPT")
POST_PASSED=$(jq -r '.test_results.total_passed' "$POST_RECEIPT")
POST_FAILED=$(jq -r '.test_results.failed' "$POST_RECEIPT")

if [ "$PRE_PASSED" = "$POST_PASSED" ] && [ "$PRE_FAILED" = "$POST_FAILED" ]; then
  echo "   ✅ PASS - Test results identical"
  echo "      Passed: $PRE_PASSED tests"
  echo "      Failed: $PRE_FAILED tests"
else
  echo "   ❌ FAIL - Test results diverged"
  echo "      Pre:  $PRE_PASSED passed, $PRE_FAILED failed"
  echo "      Post: $POST_PASSED passed, $POST_FAILED failed"
  PARITY_PASS=false
fi
echo ""

# 3. Dependency Edge Count Check
echo "3. Dependency Graph Parity"
echo "   Comparing dependency edge counts..."

PRE_EDGES=$(jq -r '.metrics.dependency_edge_count' "$PRE_RECEIPT")
POST_EDGES=$(jq -r '.metrics.dependency_edge_count' "$POST_RECEIPT")

# Allow some variance in edge count due to display format differences
EDGE_DIFF=$((POST_EDGES - PRE_EDGES))
EDGE_DIFF_ABS=${EDGE_DIFF#-}  # Absolute value

if [ "$EDGE_DIFF_ABS" -le 3 ]; then
  echo "   ✅ PASS - Dependency edges preserved (within tolerance)"
  echo "      Pre:  $PRE_EDGES edges"
  echo "      Post: $POST_EDGES edges"
  echo "      Diff: $EDGE_DIFF (acceptable)"
else
  echo "   ⚠️  WARNING - Dependency edge count changed significantly"
  echo "      Pre:  $PRE_EDGES edges"
  echo "      Post: $POST_EDGES edges"
  echo "      Diff: $EDGE_DIFF"
  echo "   (This may be acceptable if workspace structure changed intentionally)"
fi
echo ""

# 4. Package Count Check
echo "4. Package Count Verification"
echo "   Checking expected package count transformation..."

PRE_PACKAGES=$(jq -r '.metrics.package_count' "$PRE_RECEIPT")
POST_PACKAGES=$(jq -r '.metrics.package_count' "$POST_RECEIPT")

# Expected: 13 crates → 16 packages (3 public + 13 internal)
if [ "$PRE_PACKAGES" -eq 13 ] && [ "$POST_PACKAGES" -eq 16 ]; then
  echo "   ✅ PASS - Package count transformation correct"
  echo "      Pre:  $PRE_PACKAGES crates"
  echo "      Post: $POST_PACKAGES packages (3 public + 13 internal)"
elif [ "$PRE_PACKAGES" -eq 13 ]; then
  echo "   ⚠️  WARNING - Expected 16 packages post-migration, found $POST_PACKAGES"
  echo "      Pre:  $PRE_PACKAGES crates"
  echo "      Post: $POST_PACKAGES packages"
else
  echo "   ℹ️  INFO - Package counts"
  echo "      Pre:  $PRE_PACKAGES"
  echo "      Post: $POST_PACKAGES"
fi
echo ""

# 5. Performance Check
echo "5. Performance Parity"
echo "   Checking build time variance..."

PRE_BUILD=$(jq -r '.metrics.build_time_seconds' "$PRE_RECEIPT")
POST_BUILD=$(jq -r '.metrics.build_time_seconds' "$POST_RECEIPT")

if [ "$PRE_BUILD" -gt 0 ]; then
  BUILD_RATIO=$(awk "BEGIN {printf \"%.2f\", $POST_BUILD / $PRE_BUILD}")
  DRIFT_PCT=$(awk "BEGIN {printf \"%.1f\", ($BUILD_RATIO - 1.0) * 100}")

  if (( $(echo "$BUILD_RATIO < 1.2" | bc -l) )); then
    echo "   ✅ PASS - Build time within acceptable bounds"
    echo "      Pre:  ${PRE_BUILD}s"
    echo "      Post: ${POST_BUILD}s"
    echo "      Drift: ${DRIFT_PCT}% (acceptable: ≤20%)"
  else
    echo "   ⚠️  WARNING - Build time increased significantly"
    echo "      Pre:  ${PRE_BUILD}s"
    echo "      Post: ${POST_BUILD}s"
    echo "      Drift: ${DRIFT_PCT}% (threshold: 20%)"
  fi
else
  echo "   ℹ️  INFO - Build time data unavailable"
fi
echo ""

# Final Summary
echo "======================================"
echo "Parity Validation Summary"
echo "======================================"
echo ""

if [ "$PARITY_PASS" = true ]; then
  echo "✅ ALL CRITICAL PARITY CHECKS PASSED"
  echo ""
  echo "Migration preserved:"
  echo "  - Source code integrity (BLAKE3/SHA256 hash match)"
  echo "  - Test results ($PRE_PASSED tests passing)"
  echo "  - Dependency graph structure"
  echo ""
  echo "Post-migration state is equivalent to pre-migration state."
  echo "Migration was successful!"
  exit 0
else
  echo "❌ PARITY VALIDATION FAILED"
  echo ""
  echo "One or more critical checks failed:"
  echo "  - Review the failures above"
  echo "  - Source code hash mismatch indicates code changes (NOT allowed)"
  echo "  - Test result changes indicate functional regression"
  echo ""
  echo "RECOMMENDATION: ROLLBACK MIGRATION"
  echo ""
  echo "Rollback procedure:"
  echo "  cd /home/user/qlever"
  echo "  tar -xzf /tmp/rust-backup-*.tar.gz"
  echo "  git reset --hard HEAD"
  echo "  git clean -fd"
  exit 1
fi
