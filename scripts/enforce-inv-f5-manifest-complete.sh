#!/bin/bash
# EPIC 10: INV-F5 Manifest Completeness Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST="${PROJECT_ROOT}/.artifacts/manifest.sha256"
BUILD_DIR="${PROJECT_ROOT}/build"
echo "[INV-F5] Manifest Completeness Check..."
test -f "$MANIFEST" || { echo "✗ FAIL: manifest.sha256 missing"; exit 1; }
test -d "$BUILD_DIR" || { echo "✗ FAIL: build/ directory missing"; exit 1; }
# Find all executables, libraries in build/
EXPECTED_ARTIFACTS=$(mktemp)
find "$BUILD_DIR" -type f \( -executable -o -name "*.a" -o -name "*.so" -o -name "*.so.*" \) 2>/dev/null | sort > "$EXPECTED_ARTIFACTS"
# Extract filenames from manifest (second column)
MANIFEST_ARTIFACTS=$(mktemp)
awk '{print $2}' "$MANIFEST" | sed 's|^\./||' | sort > "$MANIFEST_ARTIFACTS"
# Compare: every expected artifact should be in manifest
MISSING=$(comm -23 "$EXPECTED_ARTIFACTS" <(sed "s|^|$BUILD_DIR/|" "$MANIFEST_ARTIFACTS"))
if [ -z "$MISSING" ]; then
    ARTIFACT_COUNT=$(wc -l < "$EXPECTED_ARTIFACTS")
    echo "✓ PASS: Manifest complete ($ARTIFACT_COUNT artifacts included)"
    rm -f "$EXPECTED_ARTIFACTS" "$MANIFEST_ARTIFACTS"
    exit 0
else
    echo "✗ FAIL: Manifest incomplete - missing artifacts:"
    echo "$MISSING"
    rm -f "$EXPECTED_ARTIFACTS" "$MANIFEST_ARTIFACTS"
    exit 1
fi
