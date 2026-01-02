#!/bin/bash
# EPIC 10: INV-F2 Manifest Sorted Enforcement
set -e
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST="${PROJECT_ROOT}/.artifacts/manifest.sha256"
echo "[INV-F2] Manifest Sort Order Check..."
test -f "$MANIFEST" || { echo "✗ FAIL: manifest.sha256 missing"; exit 1; }
# Verify manifest is sorted (sort -c checks if already sorted)
if sort -c "$MANIFEST" 2>/dev/null; then
    echo "✓ PASS: Manifest is sorted alphabetically"
    exit 0
else
    echo "✗ FAIL: Manifest is NOT sorted"
    echo "Expected sorted order, got:"
    head -5 "$MANIFEST"
    exit 1
fi
