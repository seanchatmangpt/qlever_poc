#!/bin/bash
# EPIC 11 Subsystem 3 (Agent 3): Workload Pack Verification Script
#
# This script verifies that a workload pack is deterministic and reproducible.
# It validates all file hashes and the workload_id.
#
# Usage: ./verify_pack.sh
# Exit codes:
#   0 = All checks passed
#   1 = Verification failed

set -euo pipefail

PACK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PACK_DIR"

echo "==================================================================="
echo "Workload Pack Verification: deterministic_pack_1"
echo "==================================================================="
echo ""

# Expected hashes from manifest
EXPECTED_MANIFEST_HASH="974c610968c704c5a8b2d648ae07f222b3363a13bf5004772c970ec8c93421aa"
EXPECTED_WORKLOAD_ID="ab456c62a9ab2bd1c7e65758c495d7e7189f8b7ac70517bf88c86064dd9c42d3"

EXPECTED_RDF_HASH="dcde8fab296f7c313eafb5c95ac33002dcc8974fad8b3c746be040636764e971"
EXPECTED_Q01_HASH="fa3869398bc24c4652a18537ae264e7319f6edd28610f93bdf39b697028001d5"
EXPECTED_Q02_HASH="f98651608bc2602f9fad21544339829344b12c1fd729386fccdb47ce9adc154b"
EXPECTED_Q03_HASH="c87da7d4d45952fd6a845596bf9ee8ffbd1a19732d3449494290b486a83d1057"

# Verification functions
verify_hash() {
    local file=$1
    local expected=$2
    local actual=$(sha256sum "$file" | awk '{print $1}')

    if [ "$actual" = "$expected" ]; then
        echo "✓ $file: $actual"
        return 0
    else
        echo "✗ $file: MISMATCH"
        echo "  Expected: $expected"
        echo "  Actual:   $actual"
        return 1
    fi
}

# Check all files exist
echo "Step 1: Checking file presence..."
for file in manifest.json sample_data.ttl query_01_select_all_persons.sparql query_02_count_cities.sparql query_03_knows_graph.sparql; do
    if [ -f "$file" ]; then
        echo "✓ $file exists"
    else
        echo "✗ $file MISSING"
        exit 1
    fi
done
echo ""

# Verify file hashes
echo "Step 2: Verifying file hashes..."
verify_hash "sample_data.ttl" "$EXPECTED_RDF_HASH" || exit 1
verify_hash "query_01_select_all_persons.sparql" "$EXPECTED_Q01_HASH" || exit 1
verify_hash "query_02_count_cities.sparql" "$EXPECTED_Q02_HASH" || exit 1
verify_hash "query_03_knows_graph.sparql" "$EXPECTED_Q03_HASH" || exit 1
echo ""

# Verify workload_id computation
echo "Step 3: Verifying workload_id..."
COMPUTED_WORKLOAD_ID=$(echo -n "${EXPECTED_RDF_HASH}${EXPECTED_Q01_HASH}${EXPECTED_Q02_HASH}${EXPECTED_Q03_HASH}" | sha256sum | awk '{print $1}')
if [ "$COMPUTED_WORKLOAD_ID" = "$EXPECTED_WORKLOAD_ID" ]; then
    echo "✓ workload_id: $COMPUTED_WORKLOAD_ID"
else
    echo "✗ workload_id: MISMATCH"
    echo "  Expected: $EXPECTED_WORKLOAD_ID"
    echo "  Computed: $COMPUTED_WORKLOAD_ID"
    exit 1
fi
echo ""

# Verify manifest hash
echo "Step 4: Verifying manifest.json hash..."
verify_hash "manifest.json" "$EXPECTED_MANIFEST_HASH" || exit 1
echo ""

# Verify manifest is valid JSON
echo "Step 5: Verifying manifest.json is valid JSON..."
if jq empty manifest.json 2>/dev/null; then
    echo "✓ manifest.json is valid JSON"
else
    echo "✗ manifest.json is NOT valid JSON"
    exit 1
fi
echo ""

# Extract and verify workload_id from manifest
echo "Step 6: Verifying workload_id in manifest matches computed value..."
MANIFEST_WORKLOAD_ID=$(jq -r '.workload_id' manifest.json)
if [ "$MANIFEST_WORKLOAD_ID" = "$EXPECTED_WORKLOAD_ID" ]; then
    echo "✓ Manifest workload_id matches computed value"
else
    echo "✗ Manifest workload_id does NOT match"
    echo "  In manifest: $MANIFEST_WORKLOAD_ID"
    echo "  Computed:    $EXPECTED_WORKLOAD_ID"
    exit 1
fi
echo ""

echo "==================================================================="
echo "✓ ALL CHECKS PASSED"
echo "==================================================================="
echo ""
echo "Workload pack is deterministic and reproducible."
echo "workload_id: $EXPECTED_WORKLOAD_ID"
echo ""
echo "This workload can be reproduced on any machine with identical results."
exit 0
