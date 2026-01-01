#!/bin/bash
# Build QLever in Release mode (optimized for performance)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

echo "Building QLever (Release mode)..."
echo "Project directory: $PROJECT_DIR"
echo ""

# Create build directory if needed
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure and build
echo "Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..

echo ""
echo "Building with Ninja (using $(nproc) cores)..."
cmake --build . -- -j$(nproc)

echo ""
echo "✓ Release build complete!"
echo ""
echo "Next: Run tests with: ctest --output-on-failure"
echo "      Or run specific test with: ctest -R TestName --output-on-failure"
