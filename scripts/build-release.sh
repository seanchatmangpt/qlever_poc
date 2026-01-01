#!/bin/bash
# Build QLever in Release mode (optimized for performance)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

# Parse optional arguments
SKIP_COMPILATION_INFO=false
FAST_BUILD=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --skip-compilation-info)
            SKIP_COMPILATION_INFO=true
            shift
            ;;
        --fast)
            FAST_BUILD=true
            SKIP_COMPILATION_INFO=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --skip-compilation-info  Skip CompilationInfo.cpp regeneration (faster builds)"
            echo "  --fast                   Enable fast build mode (skip compilation info)"
            echo "  -h, --help               Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

echo "Building QLever (Release mode)..."
echo "Project directory: $PROJECT_DIR"
echo ""

# Create build directory if needed
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure CMake with optional optimizations
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release -GNinja"

if [ "$SKIP_COMPILATION_INFO" = true ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DDONT_UPDATE_COMPILATION_INFO=true"
    echo "Note: CompilationInfo regeneration skipped (version info may be stale)"
fi

echo "Configuring CMake..."
cmake $CMAKE_ARGS ..

# Calculate optimal job count (Ninja handles +1 well for better CPU utilization)
JOBS=$(($(nproc) + 1))

echo ""
echo "Building with Ninja (using $JOBS jobs)..."
cmake --build . -- -j$JOBS

echo ""
echo "✓ Release build complete!"
echo ""
if [ "$SKIP_COMPILATION_INFO" = true ]; then
    echo "Note: This build skipped CompilationInfo regeneration."
    echo "      For release builds with version info, run without --skip-compilation-info"
    echo ""
fi
echo "Next: Run tests with: ctest --output-on-failure"
echo "      Or run specific test with: ctest -R TestName --output-on-failure"
