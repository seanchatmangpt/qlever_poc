#!/bin/bash
# EPIC 8.2 Agent 3: Recovery Pattern Detection Script
# Purpose: Detect all recovery patterns in codebase
# Status: Static analysis enforcement for fail-closed semantics

set -e

PROJECT_ROOT="/home/user/qlever"
OUTPUT_DIR="${PROJECT_ROOT}/.artifacts/recovery_analysis"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

mkdir -p "${OUTPUT_DIR}"

echo "═══════════════════════════════════════════════════════════"
echo "EPIC 8.2: RECOVERY PATTERN DETECTION"
echo "Date: $(date)"
echo "═══════════════════════════════════════════════════════════"
echo

# Pattern Class 1: Exception Catching
echo "[1/6] Scanning for Exception Catching patterns..."
grep -r "catch\s*(" \
  --include="*.cpp" \
  --include="*.h" \
  "${PROJECT_ROOT}/src/" \
  "${PROJECT_ROOT}/test/" \
  2>/dev/null | \
  grep -v "exit\|abort\|terminate\|std::terminate" > "${OUTPUT_DIR}/pattern1_exceptions_${TIMESTAMP}.txt" || true

EXCEPTION_COUNT=$(wc -l < "${OUTPUT_DIR}/pattern1_exceptions_${TIMESTAMP}.txt")
echo "   Found: ${EXCEPTION_COUNT} catch blocks without termination"

# Pattern Class 2: Optional Fallbacks
echo "[2/6] Scanning for Optional Fallback patterns..."
grep -r "\.value_or(" \
  --include="*.cpp" \
  --include="*.h" \
  "${PROJECT_ROOT}/src/" \
  "${PROJECT_ROOT}/benchmark/" \
  2>/dev/null > "${OUTPUT_DIR}/pattern2_optional_${TIMESTAMP}.txt" || true

OPTIONAL_COUNT=$(wc -l < "${OUTPUT_DIR}/pattern2_optional_${TIMESTAMP}.txt")
echo "   Found: ${OPTIONAL_COUNT} .value_or() calls"

# Pattern Class 3: Retry Loops
echo "[3/6] Scanning for Retry Loop patterns..."
grep -ri "retry\|retries" \
  --include="*.cpp" \
  --include="*.h" \
  "${PROJECT_ROOT}/src/" \
  2>/dev/null | \
  grep -v "test/" | \
  grep -v "comment" > "${OUTPUT_DIR}/pattern3_retry_${TIMESTAMP}.txt" || true

RETRY_COUNT=$(wc -l < "${OUTPUT_DIR}/pattern3_retry_${TIMESTAMP}.txt")
echo "   Found: ${RETRY_COUNT} retry references"

# Pattern Class 4: Configuration Defaults
echo "[4/6] Scanning for Configuration Default patterns..."
grep -r "value_or\|default\|fallback" \
  --include="*.cpp" \
  --include="*.h" \
  "${PROJECT_ROOT}/src/util/ConfigManager/" \
  "${PROJECT_ROOT}/src/global/RuntimeParameters.cpp" \
  2>/dev/null > "${OUTPUT_DIR}/pattern4_config_defaults_${TIMESTAMP}.txt" || true

CONFIG_COUNT=$(wc -l < "${OUTPUT_DIR}/pattern4_config_defaults_${TIMESTAMP}.txt")
echo "   Found: ${CONFIG_COUNT} configuration default patterns"

# Pattern Class 5: Algorithm Fallbacks
echo "[5/6] Scanning for Algorithm Fallback patterns..."
grep -r "fallback\|alternative\|else.*algorithm" \
  --include="*.cpp" \
  --include="*.h" \
  "${PROJECT_ROOT}/src/engine/" \
  2>/dev/null | \
  grep -v "test/" > "${OUTPUT_DIR}/pattern5_algorithm_fallback_${TIMESTAMP}.txt" || true

ALGORITHM_COUNT=$(wc -l < "${OUTPUT_DIR}/pattern5_algorithm_fallback_${TIMESTAMP}.txt")
echo "   Found: ${ALGORITHM_COUNT} algorithm fallback patterns"

# Pattern Class 6: State Rollback
echo "[6/6] Scanning for State Rollback patterns..."
grep -ri "rollback\|revert\|undo" \
  --include="*.cpp" \
  --include="*.h" \
  "${PROJECT_ROOT}/src/" \
  2>/dev/null | \
  grep -v "test/" | \
  grep -v "comment" > "${OUTPUT_DIR}/pattern6_rollback_${TIMESTAMP}.txt" || true

ROLLBACK_COUNT=$(wc -l < "${OUTPUT_DIR}/pattern6_rollback_${TIMESTAMP}.txt")
echo "   Found: ${ROLLBACK_COUNT} rollback references"

# Summary
echo
echo "═══════════════════════════════════════════════════════════"
echo "SUMMARY"
echo "═══════════════════════════════════════════════════════════"
echo

TOTAL_VIOLATIONS=$((EXCEPTION_COUNT + OPTIONAL_COUNT + RETRY_COUNT + CONFIG_COUNT + ALGORITHM_COUNT + ROLLBACK_COUNT))

echo "Pattern Class                | Violations | Status"
echo "----------------------------|-----------|--------"
echo "1. Exception Catching       | ${EXCEPTION_COUNT}        | ❌ ILLEGAL"
echo "2. Optional Fallbacks       | ${OPTIONAL_COUNT}        | ❌ ILLEGAL"
echo "3. Retry Loops              | ${RETRY_COUNT}         | ❌ ILLEGAL"
echo "4. Configuration Defaults   | ${CONFIG_COUNT}         | ❌ ILLEGAL"
echo "5. Algorithm Fallbacks      | ${ALGORITHM_COUNT}         | ❌ ILLEGAL"
echo "6. State Rollback           | ${ROLLBACK_COUNT}         | ❌ ILLEGAL"
echo "----------------------------|-----------|--------"
echo "TOTAL                       | ${TOTAL_VIOLATIONS}       | ❌ NON-COMPLIANT"
echo

# Output files
echo "Detailed reports saved to:"
ls -lh "${OUTPUT_DIR}"/*_${TIMESTAMP}.txt | awk '{print "  -", $9, "(" $5 ")"}'
echo

# Exit status
if [ ${TOTAL_VIOLATIONS} -gt 0 ]; then
  echo "STATUS: ❌ RECOVERY PATTERNS DETECTED"
  echo "FAIL-CLOSED COMPLIANCE: 0%"
  echo
  echo "Run 'make clean && make universe' only after ALL patterns eliminated."
  exit 1
else
  echo "STATUS: ✅ NO RECOVERY PATTERNS DETECTED"
  echo "FAIL-CLOSED COMPLIANCE: 100%"
  exit 0
fi
