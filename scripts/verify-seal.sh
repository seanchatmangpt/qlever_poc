#!/bin/bash

#
# verify-seal.sh: Verify artifact integrity using sealed manifests
#
# USAGE: verify-seal.sh <manifest_file>
#
# BEHAVIOR:
#   - Verifies all referenced artifacts still exist
#   - Re-computes SHA-256 digests and compares with manifest
#   - Fails atomically on first mismatch
#   - Returns 0 if seal intact, 1 if compromised
#

set -euo pipefail

# Configuration
readonly SCRIPT_NAME="$(basename "$0")"

# Global state for atomic failure semantics
declare -g VERIFICATION_FAILED=0
declare -g MISMATCH_COUNT=0
declare -g MISSING_COUNT=0
declare -g EXTRA_COUNT=0

# Error handling with atomic failure semantics
error_exit() {
    local message="$1"
    local exit_code="${2:-1}"
    echo "[${SCRIPT_NAME}] ERROR: ${message}" >&2
    return "${exit_code}"
}

# Log verification failure (non-fatal, collected for atomic exit)
log_mismatch() {
    local issue_type="$1"
    local details="$2"
    echo "[${SCRIPT_NAME}] INTEGRITY VIOLATION: ${issue_type}: ${details}" >&2
    VERIFICATION_FAILED=1
}

# Validate inputs atomically
validate_manifest() {
    local manifest_file="$1"

    # Atomic check 1: File exists
    if [[ ! -f "${manifest_file}" ]]; then
        error_exit "Manifest file does not exist: ${manifest_file}" 1
        return 1
    fi

    # Atomic check 2: File is readable
    if [[ ! -r "${manifest_file}" ]]; then
        error_exit "Manifest file is not readable: ${manifest_file}" 1
        return 1
    fi

    # Atomic check 3: File permissions are immutable (444)
    local perms
    perms=$(stat -c '%a' "${manifest_file}")
    if [[ "${perms}" != "444" ]]; then
        error_exit "Manifest not sealed (permissions ${perms} != 444): ${manifest_file}" 1
        return 1
    fi

    return 0
}

# Extract artifacts directory from manifest
extract_artifacts_dir() {
    local manifest_file="$1"
    local artifacts_dir

    # Extract directory from manifest header (second comment line)
    artifacts_dir=$(grep '^# Directory:' "${manifest_file}" | head -1 | sed 's/^# Directory: //')

    if [[ -z "${artifacts_dir}" ]]; then
        error_exit "Cannot determine artifacts directory from manifest: ${manifest_file}" 1
        return 1
    fi

    printf '%s' "${artifacts_dir}"
    return 0
}

# Verify artifact exists and is accessible
verify_artifact_exists() {
    local artifacts_dir="$1"
    local rel_path="$2"
    local full_path="${artifacts_dir}/${rel_path}"

    if [[ ! -f "${full_path}" ]]; then
        log_mismatch "MISSING_ARTIFACT" "File not found: ${rel_path}"
        ((MISSING_COUNT++))
        return 1
    fi

    if [[ ! -r "${full_path}" ]]; then
        log_mismatch "UNREADABLE_ARTIFACT" "Cannot read artifact: ${rel_path}"
        return 1
    fi

    return 0
}

# Verify artifact digest matches manifest
verify_artifact_digest() {
    local artifacts_dir="$1"
    local rel_path="$2"
    local expected_digest="$3"
    local full_path="${artifacts_dir}/${rel_path}"
    local computed_digest

    # Compute current digest
    computed_digest=$(sha256sum "${full_path}" | cut -d' ' -f1)

    # Compare with manifest
    if [[ "${computed_digest}" != "${expected_digest}" ]]; then
        log_mismatch "DIGEST_MISMATCH" "${rel_path}: expected ${expected_digest}, got ${computed_digest}"
        ((MISMATCH_COUNT++))
        return 1
    fi

    return 0
}

# Verify artifact permissions match manifest
verify_artifact_permissions() {
    local artifacts_dir="$1"
    local rel_path="$2"
    local expected_perms="$3"
    local full_path="${artifacts_dir}/${rel_path}"
    local actual_perms

    # Get actual permissions
    actual_perms=$(stat -c '%a' "${full_path}")

    # Compare with manifest
    if [[ "${actual_perms}" != "${expected_perms}" ]]; then
        log_mismatch "PERMISSION_MISMATCH" "${rel_path}: expected ${expected_perms}, got ${actual_perms}"
        return 1
    fi

    return 0
}

# Verify artifact size matches manifest
verify_artifact_size() {
    local artifacts_dir="$1"
    local rel_path="$2"
    local expected_size="$3"
    local full_path="${artifacts_dir}/${rel_path}"
    local actual_size

    # Get actual size
    actual_size=$(stat -c '%s' "${full_path}")

    # Compare with manifest
    if [[ "${actual_size}" != "${expected_size}" ]]; then
        log_mismatch "SIZE_MISMATCH" "${rel_path}: expected ${expected_size} bytes, got ${actual_size} bytes"
        return 1
    fi

    return 0
}

# Check for extra files not in manifest
check_extra_files() {
    local artifacts_dir="$1"
    local manifest_file="$2"
    local manifest_files
    local actual_files
    local extra_files

    # Extract all files from manifest (non-DIR entries, non-comment lines)
    manifest_files=$(grep -v '^#' "${manifest_file}" | grep -v '^DIR ' | grep -v '^$' | awk '{print $2}' | sort -u)

    # Find all actual files in directory
    actual_files=$(find "${artifacts_dir}" -type f -printf '%P\n' | sort -u)

    # Find files in actual_files but not in manifest_files
    while IFS= read -r file; do
        if ! printf '%s\n' "${manifest_files}" | grep -Fxq "${file}"; then
            log_mismatch "UNEXPECTED_FILE" "File in directory but not in manifest: ${file}"
            ((EXTRA_COUNT++))
        fi
    done <<< "${actual_files}"

    return 0
}

# Verify complete seal integrity
verify_seal() {
    local manifest_file="$1"
    local artifacts_dir
    local line_num=0
    local total_files=0
    local verified_files=0

    # Validate manifest
    if ! validate_manifest "${manifest_file}"; then
        return 1
    fi

    # Extract artifacts directory
    artifacts_dir=$(extract_artifacts_dir "${manifest_file}")
    if [[ -z "${artifacts_dir}" ]]; then
        return 1
    fi

    # Verify artifacts directory still exists
    if [[ ! -d "${artifacts_dir}" ]]; then
        log_mismatch "MISSING_DIRECTORY" "Artifacts directory no longer exists: ${artifacts_dir}"
        return 1
    fi

    echo "[${SCRIPT_NAME}] Verifying seal integrity for: ${artifacts_dir}" >&2
    echo "[${SCRIPT_NAME}] Using manifest: ${manifest_file}" >&2

    # Process manifest entries
    while IFS= read -r line; do
        ((line_num++))

        # Skip comments and empty lines
        [[ "${line}" =~ ^# ]] && continue
        [[ -z "${line}" ]] && continue

        # Skip directory entries (checked separately later)
        [[ "${line}" =~ ^DIR ]] && continue

        # Parse manifest line: SHA256 FILENAME FILEMODE FILESIZE
        local sha256 rel_path filemode filesize
        read -r sha256 rel_path filemode filesize <<< "${line}"

        ((total_files++))

        # Verify artifact exists
        if ! verify_artifact_exists "${artifacts_dir}" "${rel_path}"; then
            continue
        fi

        # Verify digest (primary integrity check)
        if ! verify_artifact_digest "${artifacts_dir}" "${rel_path}" "${sha256}"; then
            continue
        fi

        # Verify permissions
        if ! verify_artifact_permissions "${artifacts_dir}" "${rel_path}" "${filemode}"; then
            continue
        fi

        # Verify size
        if ! verify_artifact_size "${artifacts_dir}" "${rel_path}" "${filesize}"; then
            continue
        fi

        # All checks passed for this artifact
        ((verified_files++))

    done < <(grep -v '^#' "${manifest_file}" | grep -v '^DIR ' | grep -v '^$')

    # Check for extra files not in manifest
    check_extra_files "${artifacts_dir}" "${manifest_file}"

    # Report results
    echo "[${SCRIPT_NAME}] Verification complete: ${verified_files}/${total_files} artifacts verified" >&2

    if [[ ${VERIFICATION_FAILED} -eq 0 ]]; then
        echo "[${SCRIPT_NAME}] ✓ Seal integrity verified" >&2
        return 0
    else
        echo "[${SCRIPT_NAME}] ✗ Seal compromised:" >&2
        echo "[${SCRIPT_NAME}]   - Digest mismatches: ${MISMATCH_COUNT}" >&2
        echo "[${SCRIPT_NAME}]   - Missing artifacts: ${MISSING_COUNT}" >&2
        echo "[${SCRIPT_NAME}]   - Unexpected files: ${EXTRA_COUNT}" >&2
        return 1
    fi
}

# Main entry point
main() {
    if [[ $# -lt 1 ]]; then
        error_exit "Usage: ${SCRIPT_NAME} <manifest_file>" 1
        return 1
    fi

    local manifest_file="$1"

    # Execute verification with atomic failure semantics
    if verify_seal "${manifest_file}"; then
        return 0
    else
        return 1
    fi
}

# Run main function
main "$@"
