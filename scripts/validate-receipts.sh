#!/bin/bash

# validate-receipts.sh - Deterministic receipts validator
# Verifies artifact digests against manifest entries
# Exit 0: all digests match
# Exit 1: digest mismatch or validation error
# No output on success, error messages on failure

set -o pipefail

if [[ $# -ne 1 ]]; then
    echo "VALIDATE_RECEIPTS_ERROR: manifest file required" >&2
    exit 1
fi

manifest_file="$1"

# Validate manifest file exists and is readable
if [[ ! -f "$manifest_file" ]]; then
    echo "VALIDATE_RECEIPTS_ERROR: manifest file not found: $manifest_file" >&2
    exit 1
fi

if [[ ! -r "$manifest_file" ]]; then
    echo "VALIDATE_RECEIPTS_ERROR: manifest file not readable: $manifest_file" >&2
    exit 1
fi

# Atomic validation: fail on first mismatch
while IFS= read -r line || [[ -n "$line" ]]; do
    # Skip empty lines and comments
    [[ -z "$line" || "$line" =~ ^[[:space:]]*# ]] && continue

    # Parse line: expected_hash filepath
    read -r expected_hash filepath <<< "$line"

    # Validate parsing
    if [[ -z "$expected_hash" || -z "$filepath" ]]; then
        echo "VALIDATE_RECEIPTS_ERROR: invalid manifest entry: $line" >&2
        exit 1
    fi

    # Verify file exists
    if [[ ! -f "$filepath" ]]; then
        echo "VALIDATE_RECEIPTS_ERROR: artifact not found: $filepath" >&2
        exit 1
    fi

    # Compute current digest
    current_hash=$(sha256sum "$filepath" 2>/dev/null | awk '{print $1}')

    if [[ -z "$current_hash" ]]; then
        echo "VALIDATE_RECEIPTS_ERROR: failed to compute digest: $filepath" >&2
        exit 1
    fi

    # Atomic comparison: fail immediately on mismatch
    if [[ "$expected_hash" != "$current_hash" ]]; then
        echo "VALIDATE_RECEIPTS_ERROR: digest mismatch: $filepath" >&2
        echo "VALIDATE_RECEIPTS_ERROR:   expected: $expected_hash" >&2
        echo "VALIDATE_RECEIPTS_ERROR:   received: $current_hash" >&2
        exit 1
    fi
done < "$manifest_file"

# All validations passed
exit 0
