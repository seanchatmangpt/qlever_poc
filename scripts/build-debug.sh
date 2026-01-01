#!/bin/bash
# Build QLever in Debug mode (with debug symbols and checks)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build-debug"

echo "Building QLever (Debug mode)..."
echo "Project directory: $PROJECT_DIR"
echo "Build directory: $BUILD_DIR"
echo ""

# Create build directory if needed
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure and build
echo "Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE=Debug -GNinja ..

echo ""
echo "Building with Ninja (using $(nproc) cores)..."
cmake --build . -- -j$(nproc)

echo ""
echo "✓ Debug build complete!"
echo ""
echo "Next: Run tests from this directory:"
echo "      cd $BUILD_DIR"
echo "      ctest --output-on-failure"
echo ""
echo "Or debug with: gdb ./bin/ServerMain"
