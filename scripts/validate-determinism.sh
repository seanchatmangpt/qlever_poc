#!/bin/bash
# EPIC 8.2 Agent 10: Determinism Validation Script
# Validates AX-3: Deterministic Output
# Tests that repeated builds produce identical artifacts

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "========================================="
echo "DETERMINISM VALIDATION"
echo "========================================="

cd "$PROJECT_ROOT"

# First build
echo ""
echo "Build 1/2: Cleaning and building..."
make clean >/dev/null 2>&1
make universe >/dev/null 2>&1

if [ ! -f .artifacts/manifest.sha256 ]; then
  echo "ERROR: First build did not produce manifest"
  exit 1
fi

HASH1=$(sha256sum .artifacts/manifest.sha256 | awk '{print $1}')
echo "Build 1 manifest hash: $HASH1"

# Second build
echo ""
echo "Build 2/2: Cleaning and building..."
make clean >/dev/null 2>&1
make universe >/dev/null 2>&1

if [ ! -f .artifacts/manifest.sha256 ]; then
  echo "ERROR: Second build did not produce manifest"
  exit 1
fi

HASH2=$(sha256sum .artifacts/manifest.sha256 | awk '{print $1}')
echo "Build 2 manifest hash: $HASH2"

# Compare
echo ""
echo "========================================="
if [ "$HASH1" = "$HASH2" ]; then
  echo "✓ PASS: Deterministic build verified"
  echo "  Both builds produced identical manifests"
  echo "  Hash: $HASH1"
  exit 0
else
  echo "✗ FAIL: Non-deterministic build detected"
  echo "  Build 1: $HASH1"
  echo "  Build 2: $HASH2"
  echo ""
  echo "DIAGNOSIS:"
  echo "  Possible causes:"
  echo "  - Timestamps in artifacts"
  echo "  - Random number generation"
  echo "  - Environment variable leakage"
  echo "  - Compiler non-determinism"
  echo "  - Unsorted file system operations"
  exit 1
fi
