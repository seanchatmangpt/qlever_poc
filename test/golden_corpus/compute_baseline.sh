#!/bin/bash
# EPIC 10.2 - Agent 8 - Baseline Digest Computation Helper
# This script helps compute baseline digests for the golden corpus
# Prerequisites:
#   1. QLever built and available
#   2. LUBM(1,0) or test dataset loaded into QLever instance
#   3. QLever server running on localhost:7001 (or configure below)

set -euo pipefail

# Configuration
QLEVER_URL="${QLEVER_URL:-http://localhost:7001}"
CORPUS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MANIFEST="${CORPUS_DIR}/manifest.json"
TMP_MANIFEST="${CORPUS_DIR}/manifest.json.new"

echo "=============================================="
echo "Golden Corpus Baseline Digest Computation"
echo "=============================================="
echo "Corpus Directory: ${CORPUS_DIR}"
echo "QLever URL: ${QLEVER_URL}"
echo ""

# Check if jq is available
if ! command -v jq &> /dev/null; then
    echo "ERROR: jq is required but not installed."
    echo "Install with: apt-get install jq"
    exit 1
fi

# Check if QLever server is reachable
if ! curl -s "${QLEVER_URL}" &> /dev/null; then
    echo "ERROR: QLever server not reachable at ${QLEVER_URL}"
    echo "Please start QLever server first."
    exit 1
fi

# Read queries from manifest
QUERIES=$(jq -r '.queries[] | @base64' "${MANIFEST}")

# Create new manifest with computed digests
cp "${MANIFEST}" "${TMP_MANIFEST}"

for query_b64 in ${QUERIES}; do
    query_json=$(echo "${query_b64}" | base64 --decode)

    query_id=$(echo "${query_json}" | jq -r '.id')
    query_file=$(echo "${query_json}" | jq -r '.file')

    echo "Processing: ${query_id}"
    echo "  File: ${query_file}"

    # Read SPARQL query
    sparql_query=$(cat "${CORPUS_DIR}/${query_file}")

    # Execute query against QLever
    # NOTE: Adjust API endpoint and parameters based on QLever's HTTP API
    result=$(curl -s -X POST \
        -H "Content-Type: application/sparql-query" \
        -H "Accept: text/tab-separated-values" \
        --data-binary "${sparql_query}" \
        "${QLEVER_URL}/sparql" || echo "QUERY_FAILED")

    if [ "${result}" = "QUERY_FAILED" ]; then
        echo "  ERROR: Query execution failed"
        echo "  Digest: EXECUTION_FAILED"
        continue
    fi

    # Compute SHA256 digest of canonical result
    # TODO: Add canonical sorting/normalization if needed
    digest=$(echo -n "${result}" | sha256sum | awk '{print $1}')

    echo "  Digest: ${digest}"

    # Update manifest with computed digest
    jq --arg id "${query_id}" --arg digest "${digest}" \
        '(.queries[] | select(.id == $id) | .digest) = $digest' \
        "${TMP_MANIFEST}" > "${TMP_MANIFEST}.tmp"
    mv "${TMP_MANIFEST}.tmp" "${TMP_MANIFEST}"

    echo ""
done

echo "=============================================="
echo "Baseline computation complete!"
echo ""
echo "Updated manifest written to: ${TMP_MANIFEST}"
echo ""
echo "Next steps:"
echo "  1. Review ${TMP_MANIFEST}"
echo "  2. If digests look correct: mv ${TMP_MANIFEST} ${MANIFEST}"
echo "  3. Enable ValidateGoldenQueryResults test in GoldenCorpusTest.cpp"
echo "  4. Rebuild and run: ctest -R GoldenCorpusTest -V"
echo "=============================================="
