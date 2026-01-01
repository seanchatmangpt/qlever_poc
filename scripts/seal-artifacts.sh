#!/bin/bash

#
# seal-artifacts.sh: Create immutable SHA-256 manifests for artifact integrity
#
# USAGE: seal-artifacts.sh <artifacts_directory>
#
# BEHAVIOR:
#   - Generates SHA-256 manifest of all artifacts in directory
#   - Writes manifest to immutable location with permissions 444
#   - Returns 0 on success, 1 on any failure
#   - Atomically fails on input validation errors
#

set -euo pipefail

# Configuration
readonly SCRIPT_NAME="$(basename "$0")"
readonly MANIFEST_DIR="${XDG_CONFIG_HOME:-.config}/qlever-seals"

# Error handling with atomic failure semantics
error_exit() {
    local message="$1"
    local exit_code="${2:-1}"
    echo "[${SCRIPT_NAME}] ERROR: ${message}" >&2
    return "${exit_code}"
}

# Validate inputs atomically before any side effects
validate_inputs() {
    local artifacts_dir="$1"

    # Atomic check 1: Directory exists
    if [[ ! -d "${artifacts_dir}" ]]; then
        error_exit "Artifacts directory does not exist: ${artifacts_dir}" 1
        return 1
    fi

    # Atomic check 2: Directory is readable
    if [[ ! -r "${artifacts_dir}" ]]; then
        error_exit "Artifacts directory is not readable: ${artifacts_dir}" 1
        return 1
    fi

    return 0
}

# Generate manifest file path based on directory hash
get_manifest_path() {
    local artifacts_dir="$1"
    local dir_hash

    # Normalize the directory path
    artifacts_dir="$(cd "${artifacts_dir}" && pwd)"

    # Generate deterministic hash from directory path
    dir_hash=$(printf '%s' "${artifacts_dir}" | sha256sum | cut -d' ' -f1)

    # Manifest location: immutable storage directory
    mkdir -p "${MANIFEST_DIR}"
    printf '%s/manifest-%s.sha256' "${MANIFEST_DIR}" "${dir_hash:0:16}"
}

# Generate SHA-256 manifest for all artifacts
generate_manifest() {
    local artifacts_dir="$1"
    local manifest_path="$2"
    local temp_manifest

    # Use temporary file for atomic writes
    temp_manifest="${manifest_path}.tmp.$$"

    # Atomically fail if unable to write to temp file
    if ! touch "${temp_manifest}" 2>/dev/null; then
        error_exit "Cannot write to manifest directory: ${MANIFEST_DIR}" 1
        return 1
    fi

    # Generate checksums in sorted order for determinism
    # Include file paths and permissions for integrity
    {
        printf "# Artifact Seal Manifest\n"
        printf "# Directory: %s\n" "${artifacts_dir}"
        printf "# Generated: %s\n" "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
        printf "# Format: <SHA256_DIGEST> <FILENAME> <FILEMODE> <FILESIZE>\n"
        printf "\n"

        # Find all files, compute hashes, and record permissions
        find "${artifacts_dir}" -type f -print0 | sort -z | while IFS= read -r -d '' file; do
            local sha256
            local filemode
            local filesize

            # Compute SHA-256
            sha256=$(sha256sum "${file}" | cut -d' ' -f1)

            # Get file permissions in octal
            filemode=$(stat -c '%a' "${file}")

            # Get file size in bytes
            filesize=$(stat -c '%s' "${file}")

            # Record relative path from artifacts directory
            local rel_path
            rel_path="${file#${artifacts_dir}/}"

            printf '%s %s %s %s\n' "${sha256}" "${rel_path}" "${filemode}" "${filesize}"
        done

        # Include directory structure and permissions
        printf "\n# Directory structure:\n"
        find "${artifacts_dir}" -type d -print0 | sort -z | while IFS= read -r -d '' dir; do
            local dirmode
            dirmode=$(stat -c '%a' "${dir}")
            local rel_path="${dir#${artifacts_dir}}"
            [[ -z "${rel_path}" ]] && rel_path="."
            printf 'DIR %s %s\n' "${rel_path}" "${dirmode}"
        done
    } > "${temp_manifest}"

    # Atomic move: rename temp file to final location
    if ! mv "${temp_manifest}" "${manifest_path}" 2>/dev/null; then
        rm -f "${temp_manifest}" 2>/dev/null || true
        error_exit "Cannot atomically commit manifest to: ${manifest_path}" 1
        return 1
    fi

    # Set manifest permissions to immutable read-only (444)
    if ! chmod 444 "${manifest_path}" 2>/dev/null; then
        # If we can't set permissions, remove the manifest and fail
        rm -f "${manifest_path}" 2>/dev/null || true
        error_exit "Cannot set manifest to read-only (444): ${manifest_path}" 1
        return 1
    fi

    # Verify the manifest is actually read-only
    local perms
    perms=$(stat -c '%a' "${manifest_path}")
    if [[ "${perms}" != "444" ]]; then
        rm -f "${manifest_path}" 2>/dev/null || true
        error_exit "Failed to verify read-only permissions (expected 444, got ${perms}): ${manifest_path}" 1
        return 1
    fi

    return 0
}

# Main sealing function
seal_artifacts() {
    local artifacts_dir="$1"
    local manifest_path

    # Validate inputs atomically
    if ! validate_inputs "${artifacts_dir}"; then
        return 1
    fi

    # Resolve to absolute path
    artifacts_dir="$(cd "${artifacts_dir}" && pwd)"

    # Determine manifest location
    manifest_path="$(get_manifest_path "${artifacts_dir}")"

    # Generate and write manifest
    if ! generate_manifest "${artifacts_dir}" "${manifest_path}"; then
        return 1
    fi

    # Success: output manifest location for verification
    printf '%s\n' "${manifest_path}"
    return 0
}

# Entry point
main() {
    if [[ $# -lt 1 ]]; then
        error_exit "Usage: ${SCRIPT_NAME} <artifacts_directory>" 1
        return 1
    fi

    local artifacts_dir="$1"

    # Execute sealing with full error handling
    if seal_artifacts "${artifacts_dir}"; then
        return 0
    else
        return 1
    fi
}

# Run main function
main "$@"
