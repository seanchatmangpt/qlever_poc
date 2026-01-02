#!/bin/bash
# QLever Rapid Rebuild Script
# Ultra-fast rebuilds for single targets using Ninja + ccache
# This is the fastest way to iterate on a single file/target

QLEVER_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-${QLEVER_ROOT}/build}"

# Parse arguments
TARGET="${1:-ServerMain}"
VERBOSE="${VERBOSE:-0}"
JOBS="${JOBS:-$(nproc)}"

# Ensure build directory exists and is configured
if [[ ! -f "${BUILD_DIR}/build.ninja" ]]; then
    echo "Build directory not initialized. Run: cd build && cmake ... && cd .."
    exit 1
fi

cd "${BUILD_DIR}"

# Enable ccache for this build
export CCACHE_DIR="${CCACHE_DIR:-$HOME/.cache/ccache}"
export CCACHE_MAXSIZE="${CCACHE_MAXSIZE:-10G}"
export CCACHE_BASEDIR="${QLEVER_ROOT}"

# Measure build time
START=$(date +%s%3N)

if [[ "$VERBOSE" == "1" ]]; then
    # Verbose output
    ninja "${TARGET}" -j "${JOBS}"
    BUILD_STATUS=$?
else
    # Quiet output (only show final result)
    ninja "${TARGET}" -j "${JOBS}" 2>&1 | {
        # Capture output but only show errors
        OUTPUT=$(cat)
        if echo "$OUTPUT" | grep -q "FAILED:"; then
            echo "$OUTPUT"
            exit 1
        elif echo "$OUTPUT" | grep -q "error:"; then
            echo "$OUTPUT"
            exit 1
        else
            # Just show the last line (progress)
            echo "$OUTPUT" | tail -3
        fi
    }
    BUILD_STATUS=$?
fi

END=$(date +%s%3N)
DURATION=$((END - START))

# Show result
if [[ $BUILD_STATUS -eq 0 ]]; then
    echo "✓ ${TARGET} built in ${DURATION}ms"

    # Show cache stats
    CACHE_HITS=$(ccache -s 2>/dev/null | grep "cache hits" | awk '{print $1}' || echo "?")
    echo "  ccache hits: ${CACHE_HITS}"
else
    echo "✗ Build failed (exit code: $BUILD_STATUS)"
    exit $BUILD_STATUS
fi
