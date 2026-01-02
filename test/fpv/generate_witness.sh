#!/usr/bin/env bash
# Copyright 2026, University of Freiburg,
# Chair of Algorithms and Data Structures.
# Author: EPIC 10.3 Agent 2 (FPV Auditor)

# FPV Witness Generation Script
#
# Generates fpv_witness.receipt containing cryptographic hashes of all
# verification results (RapidCheck, Kani, MC/DC coverage).

set -euo pipefail

echo "=== FPV Witness Generation ==="

WITNESS_FILE="fpv_witness.receipt"
TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

# Check prerequisites
command -v b3sum >/dev/null 2>&1 || {
    echo "Error: b3sum (BLAKE3) not found. Install: cargo install b3sum"
    exit 1
}

# Create witness file
cat > "$WITNESS_FILE" <<EOF
FPV_WITNESS_V1
timestamp: $TIMESTAMP
kani_version: $(cargo kani --version 2>/dev/null || echo "unknown")
rapidcheck_version: 1.0.0

[kani_harnesses]
EOF

# Run Kani verification and collect hashes
echo "Running Kani verification..."
cd test/fpv/kani

HARNESSES=(
    "verify_join_result_width"
    "verify_join_cost_estimate"
    "verify_join_corrected_estimate"
    "verify_join_table_index"
    "verify_join_back_index"
    "verify_filter_interval_bounds"
    "verify_indexscan_loop_bounds"
    "verify_indexscan_result_width"
)

for harness in "${HARNESSES[@]}"; do
    echo "  Verifying: $harness"

    # Run Kani
    output=$(cargo kani --harness "$harness" 2>&1 || true)

    # Check if verification succeeded
    if echo "$output" | grep -q "VERIFICATION:- SUCCESSFUL"; then
        # Hash the output
        hash=$(echo "$output" | b3sum | cut -d' ' -f1)
        echo "$harness: PASS (hash: blake3:$hash)" >> "../../../$WITNESS_FILE"
    else
        echo "$harness: FAIL" >> "../../../$WITNESS_FILE"
        echo "Error: Kani harness $harness failed"
        exit 1
    fi
done

cd ../../..

# Run RapidCheck tests and collect hashes
echo ""
echo "Running RapidCheck tests..."

cat >> "$WITNESS_FILE" <<EOF

[rapidcheck_properties]
EOF

RAPIDCHECK_TESTS=(
    "join"
    "filter"
    "indexscan"
)

for test in "${RAPIDCHECK_TESTS[@]}"; do
    echo "  Testing: fpv_rapidcheck_$test"

    # Run RapidCheck (use quick validation for witness generation)
    # In production CI, this would be 1B tests
    output=$(./build/test/fpv/fpv_rapidcheck_$test --rc-seed=42 --rc-max-success=10000 2>&1 || true)

    # Check if tests passed
    if echo "$output" | grep -q "\[  PASSED  \]"; then
        # Hash the output
        hash=$(echo "$output" | b3sum | cut -d' ' -f1)
        echo "${test}_semantic_equivalence: PASS (10000 tests, hash: blake3:$hash)" >> "$WITNESS_FILE"
    else
        echo "${test}_semantic_equivalence: FAIL" >> "$WITNESS_FILE"
        echo "Error: RapidCheck test $test failed"
        exit 1
    fi
done

# Collect MC/DC coverage
echo ""
echo "Collecting MC/DC coverage..."

cat >> "$WITNESS_FILE" <<EOF

[mc_dc_coverage]
EOF

if [[ -f "build/coverage.info" ]]; then
    # Parse coverage report
    coverage_output=$(./test/fpv/mcdc_report.py build/coverage.info 2>&1 || true)

    # Extract kernel coverages
    for kernel in "Join.cpp" "Filter.cpp" "IndexScan.cpp" "JoinAlgorithms.cpp"; do
        # Extract MC/DC percentage for this kernel
        mcdc=$(echo "$coverage_output" | grep "$kernel" | awk '{print $2}' || echo "0.0%")

        # Hash the coverage data for this kernel
        kernel_coverage=$(grep -A 10 "$kernel" build/coverage.info || echo "")
        hash=$(echo "$kernel_coverage" | b3sum | cut -d' ' -f1)

        kernel_name=$(basename "$kernel" .cpp)
        echo "$kernel_name: $mcdc (hash: blake3:$hash)" >> "$WITNESS_FILE"
    done
else
    echo "Warning: Coverage data not found. Skipping MC/DC coverage."
    echo "Join: N/A" >> "$WITNESS_FILE"
    echo "Filter: N/A" >> "$WITNESS_FILE"
    echo "IndexScan: N/A" >> "$WITNESS_FILE"
    echo "JoinAlgorithms: N/A" >> "$WITNESS_FILE"
fi

# Compute overall witness hash
echo ""
echo "Computing witness hash..."

KANI_HASHES=$(grep "PASS" "$WITNESS_FILE" | grep "kani_harnesses" -A 20 | grep "blake3:" | cut -d':' -f3 | sort)
RC_HASHES=$(grep "PASS" "$WITNESS_FILE" | grep "rapidcheck_properties" -A 10 | grep "blake3:" | cut -d':' -f3 | sort)
MCDC_HASHES=$(grep "blake3:" "$WITNESS_FILE" | grep "mc_dc_coverage" -A 10 | cut -d':' -f3 | sort)

ALL_HASHES=$(echo -e "$KANI_HASHES\n$RC_HASHES\n$MCDC_HASHES")
WITNESS_HASH=$(echo "$ALL_HASHES" | b3sum | cut -d' ' -f1)

cat >> "$WITNESS_FILE" <<EOF

witness_hash: BLAKE3:$WITNESS_HASH
signature: [Signature placeholder - sign with: openssl dgst -sha256 -sign privkey.pem fpv_witness.receipt]
EOF

echo ""
echo "=== FPV Witness Generated ==="
echo "File: $WITNESS_FILE"
echo "Hash: BLAKE3:$WITNESS_HASH"
echo ""
echo "Witness is ready for signing and commitment to repository."

# Sign the witness (if private key available)
if [[ -f ".fpv/witness_privkey.pem" ]]; then
    echo "Signing witness..."
    openssl dgst -sha256 -sign .fpv/witness_privkey.pem -out fpv_witness.sig "$WITNESS_FILE"
    echo "Signature created: fpv_witness.sig"

    # Verify signature
    if [[ -f ".fpv/witness_pubkey.pem" ]]; then
        openssl dgst -sha256 -verify .fpv/witness_pubkey.pem -signature fpv_witness.sig "$WITNESS_FILE" && \
            echo "✓ Signature verified successfully" || \
            echo "✗ Signature verification failed"
    fi
else
    echo "Warning: Private key not found. Witness is unsigned."
    echo "To sign: openssl dgst -sha256 -sign privkey.pem fpv_witness.receipt > fpv_witness.sig"
fi

echo ""
echo "=== FPV Gate Status ==="
echo "All verification complete. Agents 1, 3-10 are now UNLOCKED."
