#!/usr/bin/env bash
# EPIC 10.2 Agent 4: Bit-for-Bit Reproducibility Test
# Performs two clean builds and verifies binary determinism
# Success metric: sha256(build1) == sha256(build2)

set -euo pipefail

# Configuration
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
BUILD_DIR_1="${PROJECT_ROOT}/build_determinism_1"
BUILD_DIR_2="${PROJECT_ROOT}/build_determinism_2"
ARTIFACTS_DIR="${PROJECT_ROOT}/.artifacts/determinism"
RECEIPT_FILE="${ARTIFACTS_DIR}/build-determinism.receipt"
PHASE_LOCK_FILE="${ARTIFACTS_DIR}/.phase.lock"

# Baseline environment (locked specification)
BASELINE_OS="${BASELINE_OS:-Ubuntu 22.04}"
BASELINE_GCC="${BASELINE_GCC:-12.3.0}"
BASELINE_CMAKE="${BASELINE_CMAKE:-3.25.1}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $*" >&2
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*" >&2
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*" >&2
}

# Capture environment information
capture_environment() {
    local env_file="$1"
    mkdir -p "$(dirname "$env_file")"

    {
        echo "# Build Environment Snapshot"
        echo "timestamp=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo "hostname=$(hostname)"
        echo "os=$(lsb_release -d 2>/dev/null | cut -f2 || uname -s)"
        echo "kernel=$(uname -r)"
        echo "gcc_version=$(gcc --version | head -1)"
        echo "clang_version=$(clang --version 2>/dev/null | head -1 || echo 'N/A')"
        echo "cmake_version=$(cmake --version | head -1)"
        echo "ninja_version=$(ninja --version 2>/dev/null || echo 'N/A')"
        echo "git_commit=$(git rev-parse HEAD 2>/dev/null || echo 'N/A')"
        echo "git_branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'N/A')"
        echo "source_date_epoch=${SOURCE_DATE_EPOCH:-$(git log -1 --format=%ct 2>/dev/null || date +%s)}"
    } > "$env_file"
}

# Normalize build environment to eliminate non-determinism sources
normalize_environment() {
    # Set SOURCE_DATE_EPOCH for reproducible timestamps
    if [[ -z "${SOURCE_DATE_EPOCH:-}" ]]; then
        if git rev-parse HEAD &>/dev/null; then
            export SOURCE_DATE_EPOCH=$(git log -1 --format=%ct)
        else
            export SOURCE_DATE_EPOCH=$(date +%s)
        fi
    fi

    # Disable build-id in binaries (can include timestamps)
    export ADDITIONAL_LINKER_FLAGS="${ADDITIONAL_LINKER_FLAGS:-} -Wl,--build-id=none"

    # Normalize locale
    export LANG=C
    export LC_ALL=C

    # Disable timestamp-based features
    export DONT_UPDATE_COMPILATION_INFO=true

    log_info "Environment normalized: SOURCE_DATE_EPOCH=$SOURCE_DATE_EPOCH"
}

# Perform a clean build
perform_build() {
    local build_dir="$1"
    local build_num="$2"

    log_info "Build $build_num: Starting clean build in $build_dir"

    # Clean any previous build
    rm -rf "$build_dir"
    mkdir -p "$build_dir"

    # Configure
    log_info "Build $build_num: Configuring with CMake..."
    cd "$build_dir"

    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_CXX_FLAGS="-std=c++20 -O3 -DNDEBUG" \
          -DCMAKE_AR="$(which ar)" \
          -DCMAKE_RANLIB="$(which ranlib)" \
          -GNinja \
          -DUSE_PARALLEL=true \
          -DLOGLEVEL=INFO \
          -D_NO_TIMING_TESTS=ON \
          -DDONT_UPDATE_COMPILATION_INFO=ON \
          .. &>/dev/null || {
        log_error "Build $build_num: CMake configuration failed"
        return 1
    }

    # Build
    log_info "Build $build_num: Building with Ninja..."
    ninja -j4 &>/dev/null || {
        log_error "Build $build_num: Ninja build failed"
        return 1
    }

    log_info "Build $build_num: Build complete"
}

# Compute SHA256 hashes of all build artifacts
compute_artifact_hashes() {
    local build_dir="$1"
    local output_file="$2"

    log_info "Computing SHA256 hashes for artifacts in $build_dir"

    # Find all executables and libraries, sorted for deterministic ordering
    cd "$build_dir"
    {
        # Main executables
        find . -type f -name "ServerMain" -o -name "IndexBuilderMain" -o -name "VocabularyMergerMain" -o -name "PrintIndexVersionMain" -o -name "N3VerifierMain" -o -name "LibQLeverExample" 2>/dev/null | sort
        # Static libraries
        find . -type f -name "*.a" 2>/dev/null | sort
        # Shared libraries
        find . -type f -name "*.so" -o -name "*.so.*" 2>/dev/null | sort
        # Test binaries
        find ./test -type f -executable 2>/dev/null | sort
    } | while read -r file; do
        if [[ -f "$file" && -r "$file" ]]; then
            sha256sum "$file" 2>/dev/null || true
        fi
    done | sort -k2 > "$output_file"

    local artifact_count=$(wc -l < "$output_file")
    log_info "Computed hashes for $artifact_count artifacts"
}

# Compare two hash manifests
compare_manifests() {
    local manifest1="$1"
    local manifest2="$2"
    local diff_file="$3"

    log_info "Comparing build manifests..."

    if diff -u "$manifest1" "$manifest2" > "$diff_file" 2>&1; then
        log_info "✓ Manifests are IDENTICAL - builds are deterministic!"
        return 0
    else
        log_error "✗ Manifests DIFFER - builds are NOT deterministic"
        log_error "Diff saved to: $diff_file"

        # Show a summary of differences
        local added=$(grep -c "^+" "$diff_file" || echo 0)
        local removed=$(grep -c "^-" "$diff_file" || echo 0)
        log_error "Changes: +$added lines, -$removed lines"

        return 1
    fi
}

# Generate the determinism receipt
generate_receipt() {
    local result="$1"
    local env_file="$2"
    local manifest1="$3"
    local manifest2="$4"
    local diff_file="$5"

    mkdir -p "$(dirname "$RECEIPT_FILE")"

    {
        echo "================================================================================"
        echo "EPIC 10.2 - BUILD DETERMINISM RECEIPT"
        echo "Agent: 4 (Bit-for-Bit Reproducibility)"
        echo "================================================================================"
        echo ""
        echo "EXECUTION TIMESTAMP: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
        echo ""
        echo "--------------------------------------------------------------------------------"
        echo "BUILD ENVIRONMENT SUMMARY"
        echo "--------------------------------------------------------------------------------"
        cat "$env_file" | sed 's/^/  /'
        echo ""
        echo "--------------------------------------------------------------------------------"
        echo "BASELINE SPECIFICATION (LOCKED)"
        echo "--------------------------------------------------------------------------------"
        echo "  OS: $BASELINE_OS"
        echo "  GCC: $BASELINE_GCC"
        echo "  CMake: $BASELINE_CMAKE"
        echo "  Acceptable Variance: ±0% (bit-for-bit reproducibility required)"
        echo ""
        echo "--------------------------------------------------------------------------------"
        echo "BUILD 1 ARTIFACTS"
        echo "--------------------------------------------------------------------------------"
        echo "  Manifest: $manifest1"
        echo "  Artifact count: $(wc -l < "$manifest1")"
        echo "  Manifest SHA256: $(sha256sum "$manifest1" | awk '{print $1}')"
        echo ""
        if [[ -f "$BUILD_DIR_1/ServerMain" ]]; then
            echo "  ServerMain SHA256: $(sha256sum "$BUILD_DIR_1/ServerMain" | awk '{print $1}')"
        fi
        if [[ -f "$BUILD_DIR_1/IndexBuilderMain" ]]; then
            echo "  IndexBuilderMain SHA256: $(sha256sum "$BUILD_DIR_1/IndexBuilderMain" | awk '{print $1}')"
        fi
        echo ""
        echo "--------------------------------------------------------------------------------"
        echo "BUILD 2 ARTIFACTS"
        echo "--------------------------------------------------------------------------------"
        echo "  Manifest: $manifest2"
        echo "  Artifact count: $(wc -l < "$manifest2")"
        echo "  Manifest SHA256: $(sha256sum "$manifest2" | awk '{print $1}')"
        echo ""
        if [[ -f "$BUILD_DIR_2/ServerMain" ]]; then
            echo "  ServerMain SHA256: $(sha256sum "$BUILD_DIR_2/ServerMain" | awk '{print $1}')"
        fi
        if [[ -f "$BUILD_DIR_2/IndexBuilderMain" ]]; then
            echo "  IndexBuilderMain SHA256: $(sha256sum "$BUILD_DIR_2/IndexBuilderMain" | awk '{print $1}')"
        fi
        echo ""
        echo "--------------------------------------------------------------------------------"
        echo "DETERMINISM RESULT"
        echo "--------------------------------------------------------------------------------"
        if [[ "$result" == "PASS" ]]; then
            echo "  Result: ✓ PASS"
            echo "  Conclusion: Builds are bit-for-bit reproducible"
            echo "  Variance: 0% (perfect match)"
        else
            echo "  Result: ✗ FAIL"
            echo "  Conclusion: Builds are NOT reproducible"
            echo "  Divergence detected between builds"
        fi
        echo ""

        if [[ "$result" == "FAIL" && -f "$diff_file" && -s "$diff_file" ]]; then
            echo "--------------------------------------------------------------------------------"
            echo "DIVERGENCE ANALYSIS"
            echo "--------------------------------------------------------------------------------"
            head -100 "$diff_file" | sed 's/^/  /'
            if [[ $(wc -l < "$diff_file") -gt 100 ]]; then
                echo "  ... (truncated, see full diff at $diff_file)"
            fi
            echo ""
        fi

        echo "--------------------------------------------------------------------------------"
        echo "WORKFLOW INTEGRITY"
        echo "--------------------------------------------------------------------------------"
        local script_hash=$(sha256sum "${BASH_SOURCE[0]}" 2>/dev/null | awk '{print $1}' || echo "N/A")
        echo "  Script: ${BASH_SOURCE[0]}"
        echo "  Script SHA256: $script_hash"
        echo ""
        echo "--------------------------------------------------------------------------------"
        echo "PHASE LOCK STRUCTURE"
        echo "--------------------------------------------------------------------------------"
        echo "{"
        echo "  \"build_determinism\": {"
        echo "    \"run_1_manifest_sha256\": \"$(sha256sum "$manifest1" | awk '{print $1}')\","
        echo "    \"run_2_manifest_sha256\": \"$(sha256sum "$manifest2" | awk '{print $1}')\","
        if [[ -f "$BUILD_DIR_1/ServerMain" ]]; then
            echo "    \"run_1_engine_sha256\": \"$(sha256sum "$BUILD_DIR_1/ServerMain" | awk '{print $1}')\","
            echo "    \"run_2_engine_sha256\": \"$(sha256sum "$BUILD_DIR_2/ServerMain" | awk '{print $1}')\","
        fi
        echo "    \"match\": $(if [[ "$result" == "PASS" ]]; then echo "true"; else echo "false"; fi),"
        echo "    \"timestamp\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\""
        echo "  }"
        echo "}"
        echo ""
        echo "================================================================================"
        echo "END OF RECEIPT"
        echo "================================================================================"
    } > "$RECEIPT_FILE"

    chmod 444 "$RECEIPT_FILE"
    log_info "Receipt written to: $RECEIPT_FILE"
}

# Create phase lock file
create_phase_lock() {
    local result="$1"
    local manifest1="$2"
    local manifest2="$3"

    mkdir -p "$(dirname "$PHASE_LOCK_FILE")"

    {
        echo "{"
        echo "  \"build_determinism\": {"
        echo "    \"run_1_manifest_sha256\": \"$(sha256sum "$manifest1" | awk '{print $1}')\","
        echo "    \"run_2_manifest_sha256\": \"$(sha256sum "$manifest2" | awk '{print $1}')\","
        if [[ -f "$BUILD_DIR_1/ServerMain" ]]; then
            echo "    \"run_1_engine_sha256\": \"$(sha256sum "$BUILD_DIR_1/ServerMain" | awk '{print $1}')\","
            echo "    \"run_2_engine_sha256\": \"$(sha256sum "$BUILD_DIR_2/ServerMain" | awk '{print $1}')\","
        fi
        echo "    \"match\": $(if [[ "$result" == "PASS" ]]; then echo "true"; else echo "false"; fi),"
        echo "    \"timestamp\": \"$(date -u +%Y-%m-%dT%H:%M:%SZ)\""
        echo "  }"
        echo "}"
    } > "$PHASE_LOCK_FILE"

    chmod 444 "$PHASE_LOCK_FILE"
    log_info "Phase lock written to: $PHASE_LOCK_FILE"
}

# Main execution
main() {
    log_info "==================================================================="
    log_info "EPIC 10.2 - Bit-for-Bit Reproducibility Test"
    log_info "Agent 4: Testing build determinism"
    log_info "==================================================================="

    # Setup
    mkdir -p "$ARTIFACTS_DIR"
    local env_file="$ARTIFACTS_DIR/environment.txt"
    local manifest1="$ARTIFACTS_DIR/build1.manifest.sha256"
    local manifest2="$ARTIFACTS_DIR/build2.manifest.sha256"
    local diff_file="$ARTIFACTS_DIR/manifest.diff"

    # Capture environment
    capture_environment "$env_file"

    # Normalize environment for reproducibility
    normalize_environment

    # Perform build 1
    log_info ""
    log_info "==================================================================="
    log_info "PHASE 1: First Clean Build"
    log_info "==================================================================="
    perform_build "$BUILD_DIR_1" "1" || {
        log_error "Build 1 failed"
        exit 1
    }
    compute_artifact_hashes "$BUILD_DIR_1" "$manifest1"

    # Perform build 2
    log_info ""
    log_info "==================================================================="
    log_info "PHASE 2: Second Clean Build"
    log_info "==================================================================="
    perform_build "$BUILD_DIR_2" "2" || {
        log_error "Build 2 failed"
        exit 1
    }
    compute_artifact_hashes "$BUILD_DIR_2" "$manifest2"

    # Compare results
    log_info ""
    log_info "==================================================================="
    log_info "PHASE 3: Determinism Verification"
    log_info "==================================================================="
    local result
    if compare_manifests "$manifest1" "$manifest2" "$diff_file"; then
        result="PASS"
    else
        result="FAIL"
    fi

    # Generate artifacts
    log_info ""
    log_info "==================================================================="
    log_info "PHASE 4: Receipt Generation"
    log_info "==================================================================="
    generate_receipt "$result" "$env_file" "$manifest1" "$manifest2" "$diff_file"
    create_phase_lock "$result" "$manifest1" "$manifest2"

    # Final result
    log_info ""
    log_info "==================================================================="
    log_info "FINAL RESULT"
    log_info "==================================================================="
    if [[ "$result" == "PASS" ]]; then
        log_info "✓ BUILD DETERMINISM: VERIFIED"
        log_info "  Builds are bit-for-bit reproducible"
        log_info "  Receipt: $RECEIPT_FILE"
        exit 0
    else
        log_error "✗ BUILD DETERMINISM: FAILED"
        log_error "  Builds are NOT reproducible"
        log_error "  Receipt: $RECEIPT_FILE"
        log_error "  Diff: $diff_file"
        exit 1
    fi
}

# Execute main function
main "$@"
