#!/bin/bash
# Build QLever in fast mode (optimized for development, assumes code correctness)
#
# This script enables optimizations that skip non-essential steps:
# - Skips CompilationInfo regeneration (version info may be stale)
# - Uses optimal parallelism settings
#
# Use this for rapid development cycles when you don't need:
# - Accurate version/compilation timestamp information
# - Full validation of all build artifacts
#
# For release builds or when version info is needed, use build-release.sh instead.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

# Parse optional arguments
BUILD_TYPE="Release"
if [ "$1" = "--debug" ]; then
    BUILD_TYPE="Debug"
    BUILD_DIR="${PROJECT_DIR}/build-debug"
    shift
fi

echo "Building QLever (Fast mode, $BUILD_TYPE)..."
echo "Project directory: $PROJECT_DIR"
echo ""
echo "Fast build optimizations enabled:"
echo "  - CompilationInfo regeneration: SKIPPED"
echo "  - Optimal parallelism: ENABLED"
echo ""

# Create build directory if needed
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure CMake with fast build optimizations
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE -GNinja -DDONT_UPDATE_COMPILATION_INFO=true"

echo "Configuring CMake..."
cmake $CMAKE_ARGS ..

# Calculate optimal job count (Ninja handles +1 well for better CPU utilization)
JOBS=$(($(nproc) + 1))

echo ""
echo "Building with Ninja (using $JOBS jobs)..."
cmake --build . -- -j$JOBS

echo ""
echo "✓ Fast build complete!"
echo ""
echo "Note: This build skipped CompilationInfo regeneration."
echo "      Version info in binaries may be stale."
echo "      For release builds, use: ./scripts/build-release.sh"
echo ""
echo "Next: Run tests with: ./scripts/run-tests.sh"
echo "      Or run specific test with: ctest -R TestName --output-on-failure"

