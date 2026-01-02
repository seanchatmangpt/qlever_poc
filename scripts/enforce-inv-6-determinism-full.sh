#!/bin/bash
################################################################################
# EPIC 10 INVARIANT ENFORCEMENT
# INV-6: Deterministic Output (BLOCKER)
# Also enforces: INV-C6, INV-E4, INV-IMPL-3
################################################################################
#
# INVARIANT DEFINITION:
#   sha256(phase_output) reproducible across runs
#   Same source + flags + compiler → identical binaries
#   Same benchmark run multiple times → identical results
#   manifest.sha256 bit-identical across clean builds
#
# ENFORCEMENT MECHANISM:
#   1. Perform two clean builds with identical environment
#   2. Compare SHA-256 of ALL artifacts (not just manifest):
#      - Executables (find -type f -executable)
#      - Static libraries (*.a)
#      - Shared libraries (*.so)
#      - Object files (*.o)
#      - manifest.sha256 itself
#   3. Exit 1 if ANY difference detected
#
# EPIC 8 STATUS: INCOMPLETE (only manifest hash checked, not full artifacts)
# EPIC 10 STATUS: ENFORCED (full artifact comparison)
#
################################################################################

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
ARTIFACTS_DIR="${PROJECT_ROOT}/.artifacts"
BUILD_DIR="${PROJECT_ROOT}/build"
TEMP_DIR="${PROJECT_ROOT}/.determinism-check"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  EPIC 10: INV-6 Deterministic Output Enforcement              ║"
echo "║  Full Artifact Determinism Verification                       ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Clean up previous check
rm -rf "${TEMP_DIR}"
mkdir -p "${TEMP_DIR}"

# ============================================================================
# BUILD 1: First clean build
# ============================================================================
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[1/5] First Build: Cleaning workspace..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

cd "${PROJECT_ROOT}"
make clean >/dev/null 2>&1 || true

echo ""
echo "[2/5] First Build: Constructing universe..."
echo "       This may take several minutes..."
echo ""

if ! make universe 2>&1 | tee "${TEMP_DIR}/build1.log" | grep -E "(PHASE_|FATAL|ERROR)"; then
    echo -e "${RED}✗ FATAL: First build failed${NC}"
    echo "See log: ${TEMP_DIR}/build1.log"
    exit 1
fi

if [ ! -f "${ARTIFACTS_DIR}/.phase.lock" ]; then
    echo -e "${RED}✗ FATAL: First build did not complete (phase lock missing)${NC}"
    exit 1
fi

echo ""
echo "✓ First build complete"
echo ""

# ============================================================================
# CAPTURE BUILD 1 ARTIFACTS
# ============================================================================
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[3/5] Capturing Build 1 Artifacts..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

cd "${BUILD_DIR}"

# Create comprehensive hash manifest for build 1
{
    echo "# Executables"
    find . -type f -executable 2>/dev/null | sort | xargs sha256sum 2>/dev/null || true

    echo "# Static Libraries"
    find . -name "*.a" 2>/dev/null | sort | xargs sha256sum 2>/dev/null || true

    echo "# Shared Libraries"
    find . -name "*.so" -o -name "*.so.*" 2>/dev/null | sort | xargs sha256sum 2>/dev/null || true

    echo "# Object Files"
    find . -name "*.o" 2>/dev/null | sort | xargs sha256sum 2>/dev/null || true

    echo "# Phase Artifacts"
    sha256sum "${ARTIFACTS_DIR}/manifest.sha256" 2>/dev/null || true
    sha256sum "${ARTIFACTS_DIR}/compiler.id" 2>/dev/null || true
    sha256sum "${ARTIFACTS_DIR}/flags.env" 2>/dev/null || true

} > "${TEMP_DIR}/build1_artifacts.sha256"

# Count artifacts
BUILD1_COUNT=$(grep -v "^#" "${TEMP_DIR}/build1_artifacts.sha256" | wc -l)
echo "  Captured ${BUILD1_COUNT} artifacts from build 1"

# Store build 1 manifest hash
BUILD1_MANIFEST_HASH=$(sha256sum "${ARTIFACTS_DIR}/manifest.sha256" | awk '{print $1}')
echo "  Build 1 manifest hash: ${BUILD1_MANIFEST_HASH}"

# ============================================================================
# BUILD 2: Second clean build
# ============================================================================
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[4/5] Second Build: Cleaning workspace..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

cd "${PROJECT_ROOT}"
make clean >/dev/null 2>&1 || true

echo ""
echo "[4/5] Second Build: Constructing universe..."
echo "       This may take several minutes..."
echo ""

if ! make universe 2>&1 | tee "${TEMP_DIR}/build2.log" | grep -E "(PHASE_|FATAL|ERROR)"; then
    echo -e "${RED}✗ FATAL: Second build failed${NC}"
    echo "See log: ${TEMP_DIR}/build2.log"
    exit 1
fi

if [ ! -f "${ARTIFACTS_DIR}/.phase.lock" ]; then
    echo -e "${RED}✗ FATAL: Second build did not complete (phase lock missing)${NC}"
    exit 1
fi

echo ""
echo "✓ Second build complete"
echo ""

# ============================================================================
# CAPTURE BUILD 2 ARTIFACTS
# ============================================================================
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[5/5] Capturing Build 2 Artifacts..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

cd "${BUILD_DIR}"

# Create comprehensive hash manifest for build 2
{
    echo "# Executables"
    find . -type f -executable 2>/dev/null | sort | xargs sha256sum 2>/dev/null || true

    echo "# Static Libraries"
    find . -name "*.a" 2>/dev/null | sort | xargs sha256sum 2>/dev/null || true

    echo "# Shared Libraries"
    find . -name "*.so" -o -name "*.so.*" 2>/dev/null | sort | xargs sha256sum 2>/dev/null || true

    echo "# Object Files"
    find . -name "*.o" 2>/dev/null | sort | xargs sha256sum 2>/dev/null || true

    echo "# Phase Artifacts"
    sha256sum "${ARTIFACTS_DIR}/manifest.sha256" 2>/dev/null || true
    sha256sum "${ARTIFACTS_DIR}/compiler.id" 2>/dev/null || true
    sha256sum "${ARTIFACTS_DIR}/flags.env" 2>/dev/null || true

} > "${TEMP_DIR}/build2_artifacts.sha256"

# Count artifacts
BUILD2_COUNT=$(grep -v "^#" "${TEMP_DIR}/build2_artifacts.sha256" | wc -l)
echo "  Captured ${BUILD2_COUNT} artifacts from build 2"

# Store build 2 manifest hash
BUILD2_MANIFEST_HASH=$(sha256sum "${ARTIFACTS_DIR}/manifest.sha256" | awk '{print $1}')
echo "  Build 2 manifest hash: ${BUILD2_MANIFEST_HASH}"

# ============================================================================
# COMPARE ARTIFACTS
# ============================================================================
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[5/5] Comparing Artifacts..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Check artifact count consistency
if [ "$BUILD1_COUNT" -ne "$BUILD2_COUNT" ]; then
    echo -e "${RED}✗ FAIL: Artifact count mismatch${NC}"
    echo "  Build 1: $BUILD1_COUNT artifacts"
    echo "  Build 2: $BUILD2_COUNT artifacts"
    echo ""
    echo "DIAGNOSIS: Non-deterministic build output (different number of artifacts)"
    exit 1
fi

# Compare full artifact hashes
if diff -u "${TEMP_DIR}/build1_artifacts.sha256" "${TEMP_DIR}/build2_artifacts.sha256" > "${TEMP_DIR}/diff.txt"; then
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo -e "║  ${GREEN}✓ PASS: DETERMINISTIC BUILD VERIFIED${NC}                        ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "  Invariants Enforced:"
    echo "    • INV-6: Deterministic Output"
    echo "    • INV-C6: Compilation Deterministic"
    echo "    • INV-E4: Deterministic Reproducibility"
    echo "    • INV-IMPL-3: Reproducible Manifest"
    echo ""
    echo "  Verification Details:"
    echo "    • Total Artifacts: ${BUILD1_COUNT}"
    echo "    • Manifest Hash: ${BUILD1_MANIFEST_HASH}"
    echo "    • All artifact hashes identical across builds"
    echo ""
    echo "  EPIC 10 Status: BLOCKER RESOLVED"
    echo ""

    # Cleanup
    rm -rf "${TEMP_DIR}"
    exit 0
else
    echo ""
    echo "╔════════════════════════════════════════════════════════════════╗"
    echo -e "║  ${RED}✗ FAIL: NON-DETERMINISTIC BUILD DETECTED${NC}                    ║"
    echo "╚════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "  Artifacts differ between builds:"
    echo ""

    # Show differences (limit to first 20 lines)
    head -20 "${TEMP_DIR}/diff.txt"

    echo ""
    echo "  Full diff: ${TEMP_DIR}/diff.txt"
    echo ""
    echo "DIAGNOSIS:"
    echo "  Possible causes of non-determinism:"
    echo "    • Timestamps embedded in artifacts"
    echo "    • Random number generation (UUIDs, nonces)"
    echo "    • Environment variable leakage (PATH, HOME, USER)"
    echo "    • Compiler non-determinism (clang vs gcc)"
    echo "    • Unsorted file system operations"
    echo "    • Parallel build race conditions"
    echo "    • Build tool version mismatch"
    echo ""
    echo "  Investigation steps:"
    echo "    1. Check compiler.id matches: ${ARTIFACTS_DIR}/compiler.id"
    echo "    2. Check flags.env identical: ${ARTIFACTS_DIR}/flags.env"
    echo "    3. Verify SOURCE_DATE_EPOCH set in flags.env"
    echo "    4. Check for __DATE__/__TIME__ macros in source"
    echo "    5. Review build logs: ${TEMP_DIR}/build1.log vs ${TEMP_DIR}/build2.log"
    echo ""
    echo "  EPIC 10 Status: BLOCKER - DETERMINISM NOT ACHIEVED"
    echo ""

    # Do not cleanup - preserve for investigation
    echo "  Investigation artifacts preserved in: ${TEMP_DIR}/"
    exit 1
fi
