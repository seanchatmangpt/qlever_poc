#!/bin/bash
set -e

# QLever 80/20 Build Script
# Builds only essential targets for "it still works" verification

echo "=== QLever 80/20 Optimized Build ==="

# Clean
rm -rf build
mkdir -p build

# Setup
cd build
conan install .. --output-folder=. --build=missing -s build_type=Release

# Configure (minimal options)
cmake -DCMAKE_BUILD_TYPE=Release -GNinja \
  -DBUILD_TESTING=OFF \
  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake ..

# Build only essential 20%: ServerMain + IndexBuilderMain
echo "Building ServerMain and IndexBuilderMain only..."
ninja ServerMain IndexBuilderMain -j8

echo "✓ Minimal build complete"
ls -lh ServerMain IndexBuilderMain

# Verify
./ServerMain --help > /dev/null && echo "✓ ServerMain functional"
./IndexBuilderMain --help > /dev/null && echo "✓ IndexBuilderMain functional"

echo "=== 80/20 Build Verified ==="
