#!/bin/bash

# QLever WASM Build Script using Emscripten
# This script compiles libqlever C++ code to WebAssembly

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QLEVER_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
WASM_BUILD_DIR="$SCRIPT_DIR/wasm_build"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║         QLever WASM Build with Emscripten                      ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Check if Emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "❌ Error: Emscripten compiler (emcc) not found."
    echo ""
    echo "Please install Emscripten SDK:"
    echo "  1. git clone https://github.com/emscripten-core/emsdk.git"
    echo "  2. cd emsdk"
    echo "  3. ./emsdk install latest"
    echo "  4. ./emsdk activate latest"
    echo "  5. source ./emsdk_env.sh"
    echo ""
    exit 1
fi

echo "✓ Emscripten compiler found: $(emcc --version | head -n 1)"
echo ""

# Create build directory
mkdir -p "$WASM_BUILD_DIR"
cd "$WASM_BUILD_DIR"

echo "📦 Configuring CMake for Emscripten..."
echo "   Build directory: $WASM_BUILD_DIR"
echo ""

# Configure CMake with Emscripten toolchain
cmake "$QLEVER_ROOT" \
    -DCMAKE_TOOLCHAIN_FILE="$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -flto -s WASM=1" \
    -DWASM_BUILD=ON \
    -GNinja

echo ""
echo "🔨 Building libqlever for WASM..."
echo ""

# Build the WASM module
ninja qlever_wasm

echo ""
echo "✓ Build complete!"
echo ""
echo "📍 Output files:"
if [ -f "lib/libqlever_wasm.wasm" ]; then
    SIZE=$(du -h "lib/libqlever_wasm.wasm" | cut -f1)
    echo "   ✓ lib/libqlever_wasm.wasm ($SIZE)"
fi
if [ -f "lib/libqlever_wasm.js" ]; then
    SIZE=$(du -h "lib/libqlever_wasm.js" | cut -f1)
    echo "   ✓ lib/libqlever_wasm.js ($SIZE)"
fi

echo ""
echo "📋 Next steps:"
echo "   1. Copy WASM files to wasm/pkg/"
echo "   2. Update wasm/package.json to reference WASM module"
echo "   3. Test with: npm test"
echo ""
