#!/bin/bash
# Copyright 2026, University of Freiburg
# Chair of Algorithms and Data Structures
# EPIC 10.3 - Agent 3 Part 4: Baseline Digest Generation Script

# This script generates SHA256 digests for all UIR test queries
# by executing them via the old QueryPlanner and computing canonical TSV hashes.
#
# Usage:
#   ./generate_digests.sh [--qlever-binary PATH] [--index-dir PATH]
#
# Requirements:
#   - QLever binary (ServerMain or IndexMain)
#   - Pre-built index with test dataset (LUBM(1,0) or equivalent)
#   - uir_test_manifest.json in same directory
#   - All query files in queries/ subdirectory
#
# Output:
#   - Updated uir_test_manifest.json with computed digests
#   - Digest computation log: digest_computation.log
#
# This script is idempotent - safe to re-run.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MANIFEST_PATH="${SCRIPT_DIR}/uir_test_manifest.json"
QUERIES_DIR="${SCRIPT_DIR}/queries"
LOG_FILE="${SCRIPT_DIR}/digest_computation.log"

# Default paths (can be overridden via command-line)
QLEVER_BINARY="${QLEVER_BINARY:-qlever}"
INDEX_DIR="${INDEX_DIR:-/tmp/qlever-test-index}"

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --qlever-binary)
            QLEVER_BINARY="$2"
            shift 2
            ;;
        --index-dir)
            INDEX_DIR="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --qlever-binary PATH   Path to QLever binary (default: qlever)"
            echo "  --index-dir PATH       Path to test index directory (default: /tmp/qlever-test-index)"
            echo "  -h, --help             Show this help message"
            exit 0
            ;;
        *)
            echo "Error: Unknown option $1" >&2
            echo "Run '$0 --help' for usage information" >&2
            exit 1
            ;;
    esac
done

# Validation
if [[ ! -f "${MANIFEST_PATH}" ]]; then
    echo "Error: Manifest not found at ${MANIFEST_PATH}" >&2
    exit 1
fi

if [[ ! -d "${QUERIES_DIR}" ]]; then
    echo "Error: Queries directory not found at ${QUERIES_DIR}" >&2
    exit 1
fi

if [[ ! -d "${INDEX_DIR}" ]]; then
    echo "Error: Index directory not found at ${INDEX_DIR}" >&2
    echo "Please build a test index first using:" >&2
    echo "  IndexBuildMain -F ttl -i ${INDEX_DIR} -s <test-dataset.ttl>" >&2
    exit 1
fi

# Initialize log
echo "===== UIR Baseline Digest Computation =====" > "${LOG_FILE}"
echo "Timestamp: $(date -Iseconds)" >> "${LOG_FILE}"
echo "Manifest: ${MANIFEST_PATH}" >> "${LOG_FILE}"
echo "Index: ${INDEX_DIR}" >> "${LOG_FILE}"
echo "QLever Binary: ${QLEVER_BINARY}" >> "${LOG_FILE}"
echo "" >> "${LOG_FILE}"

echo "Starting baseline digest computation..."
echo "Log file: ${LOG_FILE}"

# Function to execute SPARQL query via QLever and compute digest
# Arguments:
#   $1: Query file path
#   $2: Query ID
# Output: SHA256 digest (64-character hex string)
compute_query_digest() {
    local query_file="$1"
    local query_id="$2"

    echo "Processing: ${query_id}" | tee -a "${LOG_FILE}"

    # TODO: Implement actual QLever query execution
    # This requires:
    #   1. Starting QLever server or using ServerMain in batch mode
    #   2. Executing query via HTTP API or direct C++ interface
    #   3. Retrieving result as TSV
    #   4. Canonicalizing TSV (sorted rows, sorted columns)
    #   5. Computing SHA256 hash
    #
    # Placeholder implementation:
    # For now, return a placeholder digest indicating baseline not yet computed

    local placeholder_digest="PENDING_BASELINE_COMPUTATION"

    # Uncomment when QLever integration is ready:
    # local query_content=$(cat "${query_file}")
    # local result_tsv=$(execute_sparql_query_via_qlever "${query_content}")
    # local canonical_tsv=$(canonicalize_tsv "${result_tsv}")
    # local digest=$(echo -n "${canonical_tsv}" | sha256sum | awk '{print $1}')
    # echo "${digest}"

    echo "${placeholder_digest}"
}

# Function to canonicalize TSV result
# (Sort rows lexicographically, sort columns alphabetically)
canonicalize_tsv() {
    local tsv="$1"

    # TODO: Implement TSV canonicalization
    # Requirements:
    #   1. Parse header row to get column names
    #   2. Sort column names alphabetically
    #   3. Reorder columns accordingly
    #   4. Sort data rows lexicographically
    #   5. Ensure Unix line endings (LF)

    echo "${tsv}"
}

# Parse manifest to get all query IDs
QUERY_COUNT=$(jq '.queries | length' "${MANIFEST_PATH}")
echo "Found ${QUERY_COUNT} queries in manifest" | tee -a "${LOG_FILE}"

# Process each query and collect digests
DIGESTS_JSON="[]"

for i in $(seq 0 $((QUERY_COUNT - 1))); do
    QUERY_ID=$(jq -r ".queries[$i].id" "${MANIFEST_PATH}")
    QUERY_FILE=$(jq -r ".queries[$i].file" "${MANIFEST_PATH}")
    QUERY_CATEGORY=$(jq -r ".queries[$i].category" "${MANIFEST_PATH}")

    FULL_QUERY_PATH="${SCRIPT_DIR}/${QUERY_FILE}"

    if [[ ! -f "${FULL_QUERY_PATH}" ]]; then
        echo "  WARNING: Query file not found: ${FULL_QUERY_PATH}" | tee -a "${LOG_FILE}"
        DIGEST="ERROR_FILE_NOT_FOUND"
    else
        DIGEST=$(compute_query_digest "${FULL_QUERY_PATH}" "${QUERY_ID}")
    fi

    echo "  Digest: ${DIGEST}" | tee -a "${LOG_FILE}"

    # Store digest for later manifest update
    DIGESTS_JSON=$(echo "${DIGESTS_JSON}" | jq \
        --arg id "${QUERY_ID}" \
        --arg digest "${DIGEST}" \
        '. += [{"id": $id, "digest": $digest}]')
done

echo "" | tee -a "${LOG_FILE}"
echo "Digest computation complete." | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

# Update manifest with computed digests
echo "Updating manifest with computed digests..." | tee -a "${LOG_FILE}"

UPDATED_MANIFEST=$(jq \
    --argjson digests "${DIGESTS_JSON}" \
    '.queries |= map(
        . as $query |
        ($digests | map(select(.id == $query.id)) | .[0].digest) as $new_digest |
        if $new_digest then
            . + {"digest": $new_digest}
        else
            .
        end
    )' \
    "${MANIFEST_PATH}")

# Backup original manifest
cp "${MANIFEST_PATH}" "${MANIFEST_PATH}.backup"
echo "Original manifest backed up to: ${MANIFEST_PATH}.backup" | tee -a "${LOG_FILE}"

# Write updated manifest
echo "${UPDATED_MANIFEST}" > "${MANIFEST_PATH}"
echo "Manifest updated: ${MANIFEST_PATH}" | tee -a "${LOG_FILE}"

# Summary
COMPUTED_COUNT=$(echo "${DIGESTS_JSON}" | jq '[.[] | select(.digest != "PENDING_BASELINE_COMPUTATION" and .digest != "ERROR_FILE_NOT_FOUND")] | length')
PENDING_COUNT=$(echo "${DIGESTS_JSON}" | jq '[.[] | select(.digest == "PENDING_BASELINE_COMPUTATION")] | length')
ERROR_COUNT=$(echo "${DIGESTS_JSON}" | jq '[.[] | select(.digest == "ERROR_FILE_NOT_FOUND")] | length')

echo "" | tee -a "${LOG_FILE}"
echo "===== Summary =====" | tee -a "${LOG_FILE}"
echo "Total queries: ${QUERY_COUNT}" | tee -a "${LOG_FILE}"
echo "Computed digests: ${COMPUTED_COUNT}" | tee -a "${LOG_FILE}"
echo "Pending computation: ${PENDING_COUNT}" | tee -a "${LOG_FILE}"
echo "Errors (file not found): ${ERROR_COUNT}" | tee -a "${LOG_FILE}"
echo "" | tee -a "${LOG_FILE}"

if [[ "${COMPUTED_COUNT}" -eq 0 ]]; then
    echo "NOTE: All digests still pending. This script currently uses placeholder logic." | tee -a "${LOG_FILE}"
    echo "      To compute actual digests, integrate with QLever query execution." | tee -a "${LOG_FILE}"
    echo "      See TODO comments in compute_query_digest() function." | tee -a "${LOG_FILE}"
fi

echo "Done. Check ${LOG_FILE} for details."
