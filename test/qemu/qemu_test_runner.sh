#!/usr/bin/env bash
# EPIC 10.3 AGENT 4: ARCH-AGNOSTIC DIGEST (BIT-PARITY VALIDATION)
# QEMU Cross-Architecture Test Runner
#
# Copyright 2026, QLever Architecture Neutrality Team
#
# CRITICAL INVARIANT: ARM64 and x86_64 binaries must produce bit-identical
# results on identical inputs (seed=42). This script orchestrates the full
# validation pipeline.
#
# REQUIREMENTS:
#   - qemu-aarch64 (QEMU user-mode emulator)
#   - aarch64-linux-gnu-gcc (ARM64 cross-compiler)
#   - b3sum (BLAKE3 hashing utility)
#   - CMake 3.27+, Ninja
#
# USAGE:
#   ./qemu_test_runner.sh [options]
#
# OPTIONS:
#   --seed N          RNG seed (default: 42)
#   --count N         Number of kernel inputs (default: 1000000)
#   --build-dir DIR   Build directory (default: build)
#   --verbose         Enable verbose logging
#   --help            Show this help message
#
# EXIT CODES:
#   0  - Success (bit-parity validated)
#   1  - General error
#   42 - Bit-parity divergence detected (DivergenceAbort)

set -euo pipefail

# ==============================================================================
# CONFIGURATION
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Default configuration
SEED=42
COUNT=1000000
BUILD_DIR="${PROJECT_ROOT}/build"
QEMU_BUILD_DIR="${PROJECT_ROOT}/qemu_build_aarch64"
RESULTS_DIR="${BUILD_DIR}/qemu_results"
VERBOSE=false

# ==============================================================================
# ARGUMENT PARSING
# ==============================================================================

show_help() {
    cat <<EOF
Usage: $0 [options]

QEMU Cross-Architecture Bit-Parity Validation

Options:
    --seed N          RNG seed (default: 42)
    --count N         Number of kernel inputs (default: 1000000)
    --build-dir DIR   Build directory (default: build)
    --verbose         Enable verbose logging
    --help            Show this help message

Exit Codes:
    0  - Success (bit-parity validated)
    1  - General error
    42 - Bit-parity divergence detected (DivergenceAbort)

Example:
    $0 --seed 42 --count 1000000 --verbose

EOF
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --seed)
            SEED="$2"
            shift 2
            ;;
        --count)
            COUNT="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            echo "ERROR: Unknown argument: $1"
            show_help
            exit 1
            ;;
    esac
done

# ==============================================================================
# DEPENDENCY CHECKS
# ==============================================================================

log() {
    echo "[QEMU] $*"
}

check_dependency() {
    local cmd="$1"
    local package="$2"

    if ! command -v "$cmd" &>/dev/null; then
        echo "ERROR: $cmd not found"
        echo "Install: $package"
        return 1
    fi
    return 0
}

log "Checking dependencies..."
check_dependency qemu-aarch64 "sudo apt-get install qemu-user" || exit 1
check_dependency aarch64-linux-gnu-gcc "sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu" || exit 1
check_dependency b3sum "cargo install b3sum" || exit 1
check_dependency cmake "sudo apt-get install cmake" || exit 1
check_dependency ninja "sudo apt-get install ninja-build" || exit 1
log "All dependencies satisfied"

# ==============================================================================
# FPV GATE CHECK (BLOCKING)
# ==============================================================================

FPV_WITNESS="${PROJECT_ROOT}/fpv_witness.receipt"

if [[ ! -f "$FPV_WITNESS" ]]; then
    echo "=============================================="
    echo "ERROR: FPV GATE NOT PASSED"
    echo "=============================================="
    echo "Agent 4 is BLOCKED until Agent 2 FPV validation completes."
    echo ""
    echo "Required: ${FPV_WITNESS}"
    echo "Status:   NOT FOUND"
    echo ""
    echo "Action:   Run Agent 2 FPV validation to obtain witness:"
    echo "          1. Run Kani verification"
    echo "          2. Run RapidCheck validation (1B tests)"
    echo "          3. Measure MC/DC coverage (must be 100%)"
    echo "          4. Run 12-hour CI saturation"
    echo "          5. Generate fpv_witness.receipt"
    echo "=============================================="
    exit 1
fi

log "FPV witness found: $FPV_WITNESS"
log "Agent 4 gate UNLOCKED"

# ==============================================================================
# PHASE 1: BUILD x86_64 BINARY
# ==============================================================================

log "=============================================="
log "PHASE 1/5: Building x86_64 binary"
log "=============================================="

mkdir -p "$BUILD_DIR"
cd "$PROJECT_ROOT"

if [[ "$VERBOSE" == true ]]; then
    cmake -G Ninja -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    ninja -C "$BUILD_DIR" BitParityValidator
else
    cmake -G Ninja -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1
    ninja -C "$BUILD_DIR" BitParityValidator >/dev/null 2>&1
fi

log "x86_64 build complete"

# ==============================================================================
# PHASE 2: RUN x86_64 BINARY
# ==============================================================================

log "=============================================="
log "PHASE 2/5: Running x86_64 binary"
log "=============================================="

mkdir -p "$RESULTS_DIR"
X86_OUTPUT="${RESULTS_DIR}/x86_64_results.bin"

if [[ "$VERBOSE" == true ]]; then
    "$BUILD_DIR/test/qemu/BitParityValidator" \
        --seed "$SEED" \
        --count "$COUNT" \
        --output "$X86_OUTPUT" \
        --verbose
else
    "$BUILD_DIR/test/qemu/BitParityValidator" \
        --seed "$SEED" \
        --count "$COUNT" \
        --output "$X86_OUTPUT"
fi

log "x86_64 execution complete: $X86_OUTPUT"

# ==============================================================================
# PHASE 3: CROSS-COMPILE ARM64 BINARY
# ==============================================================================

log "=============================================="
log "PHASE 3/5: Cross-compiling ARM64 binary"
log "=============================================="

TOOLCHAIN="${PROJECT_ROOT}/cmake/toolchains/aarch64-linux-gnu.cmake"

# Create toolchain file if it doesn't exist
if [[ ! -f "$TOOLCHAIN" ]]; then
    mkdir -p "$(dirname "$TOOLCHAIN")"
    cat >"$TOOLCHAIN" <<'EOF'
# ARM64 Cross-Compilation Toolchain
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a -frandom-seed=42")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv8-a -frandom-seed=42")
EOF
    log "Created ARM64 toolchain: $TOOLCHAIN"
fi

if [[ "$VERBOSE" == true ]]; then
    cmake -G Ninja \
        -B "$QEMU_BUILD_DIR" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DCMAKE_BUILD_TYPE=Release \
        -DUSE_PRECOMPILED_HEADERS=OFF
    ninja -C "$QEMU_BUILD_DIR" BitParityValidator
else
    cmake -G Ninja \
        -B "$QEMU_BUILD_DIR" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DCMAKE_BUILD_TYPE=Release \
        -DUSE_PRECOMPILED_HEADERS=OFF >/dev/null 2>&1
    ninja -C "$QEMU_BUILD_DIR" BitParityValidator >/dev/null 2>&1
fi

log "ARM64 build complete"

# ==============================================================================
# PHASE 4: RUN ARM64 BINARY IN QEMU
# ==============================================================================

log "=============================================="
log "PHASE 4/5: Running ARM64 binary in QEMU"
log "=============================================="

ARM64_OUTPUT="${RESULTS_DIR}/arm64_results.bin"

if [[ "$VERBOSE" == true ]]; then
    qemu-aarch64 \
        -L /usr/aarch64-linux-gnu \
        "$QEMU_BUILD_DIR/test/qemu/BitParityValidator" \
        --seed "$SEED" \
        --count "$COUNT" \
        --output "$ARM64_OUTPUT" \
        --verbose
else
    qemu-aarch64 \
        -L /usr/aarch64-linux-gnu \
        "$QEMU_BUILD_DIR/test/qemu/BitParityValidator" \
        --seed "$SEED" \
        --count "$COUNT" \
        --output "$ARM64_OUTPUT"
fi

log "ARM64 execution complete: $ARM64_OUTPUT"

# ==============================================================================
# PHASE 5: VALIDATE BIT-PARITY
# ==============================================================================

log "=============================================="
log "PHASE 5/5: Validating bit-parity"
log "=============================================="

# Compute BLAKE3 digests
X86_DIGEST=$(b3sum "$X86_OUTPUT" | cut -d' ' -f1)
ARM64_DIGEST=$(b3sum "$ARM64_OUTPUT" | cut -d' ' -f1)

log "x86_64 digest: $X86_DIGEST"
log "ARM64 digest:  $ARM64_DIGEST"

# Compare digests
if [[ "$X86_DIGEST" == "$ARM64_DIGEST" ]]; then
    echo "=============================================="
    echo "✅ BIT-PARITY VALIDATED"
    echo "=============================================="
    echo "Architectures: ARM64 == x86_64"
    echo "Seed:          $SEED (deterministic)"
    echo "Kernel inputs: $COUNT"
    echo "Divergences:   0"
    echo "Digest:        $X86_DIGEST"
    echo "=============================================="
    exit 0
else
    echo "=============================================="
    echo "❌ BIT-PARITY DIVERGENCE DETECTED"
    echo "=============================================="
    echo "x86_64 digest: $X86_DIGEST"
    echo "ARM64 digest:  $ARM64_DIGEST"
    echo ""
    echo "AGENT 9 REVIEW REQUIRED:"
    echo "  - Check qleverest::vmath SIMD implementations"
    echo "  - Verify AVX-512 and NEON produce identical results"
    echo "  - Review Instruction Mask for hardware leakage"
    echo ""
    echo "BUILD ABORTED (DivergenceAbort)"
    echo "=============================================="
    exit 42
fi
