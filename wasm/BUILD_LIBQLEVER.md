# Building QLever WASM with libqlever (Emscripten)

This guide explains how to compile QLever's libqlever C++ code to WebAssembly using Emscripten.

## Architecture

```
JavaScript (TypeScript)
    ↓ (calls)
wasm_wrapper.ts (DataFactory, QueryBuilder, Store)
    ↓ (calls)
libqlever_bindings.rs (Rust FFI via wasm-bindgen)
    ↓ (calls)
QleverWasmWrapper.cpp (C++ wrapper)
    ↓ (calls)
libqlever C++ (Qlever, Index, QueryPlanner)
```

## Prerequisites

### Required Software

1. **Emscripten SDK** (for WASM compilation)
   ```bash
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh  # Activate in current shell
   ```

2. **Rust** (for wasm-bindgen and Cargo)
   ```bash
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   source $HOME/.cargo/env
   rustup target add wasm32-emscripten
   ```

3. **Node.js & npm** (for JavaScript/TypeScript tooling)
   ```bash
   npm install -g wasm-pack
   npm install -g wasm-bindgen-cli
   ```

4. **CMake 3.27+**
   ```bash
   cmake --version  # Should be 3.27 or later
   ```

5. **Build Tools**
   - GCC 11+ or Clang 16+
   - Ninja or Make

### System Libraries

For Ubuntu/Debian:
```bash
sudo apt-get install libboost-dev libicu-dev libzstd-dev
```

For macOS:
```bash
brew install boost icu4c zstd cmake ninja
```

## Building Steps

### Step 1: Activate Emscripten

Every time you start a new shell, activate Emscripten:

```bash
source /path/to/emsdk/emsdk_env.sh
```

Verify installation:
```bash
emcc --version
rustup target list | grep emscripten
```

### Step 2: Set Up Build Environment

Navigate to the QLever root directory:

```bash
cd /path/to/qlever
export EMSCRIPTEN_ROOT=/path/to/emsdk/upstream/emscripten
```

### Step 3: Build Using the Build Script

Option A: Using the provided build script (recommended):

```bash
cd wasm
chmod +x build_with_emscripten.sh
./build_with_emscripten.sh
```

The script will:
- Check for Emscripten availability
- Create a `wasm_build` directory
- Configure CMake with Emscripten toolchain
- Build libqlever WASM module
- Output WASM files to `wasm_build/lib/`

Option B: Manual build using npm script:

```bash
cd wasm
npm install
npm run build:libqlever
```

### Step 4: Verify Build Output

Check that WASM files were created:

```bash
ls -lh wasm_build/lib/
# Expected output:
# -rw-r--r-- libqlever_wasm.wasm (2-5 MB)
# -rw-r--r-- libqlever_wasm.js   (50-100 KB)
```

File sizes (uncompressed):
- `libqlever_wasm.wasm`: 2-5 MB (binary WASM module)
- `libqlever_wasm.js`: ~100 KB (JavaScript glue code)

Gzipped sizes:
- `.wasm.gz`: 500 KB - 1.5 MB
- `.js.gz`: 20-30 KB

### Step 5: Copy WASM Files to Package

```bash
cp wasm_build/lib/libqlever_wasm.wasm pkg/
cp wasm_build/lib/libqlever_wasm.js pkg/
```

## Building for Different Configurations

### Development Build (with debug symbols)

```bash
./build_with_emscripten.sh --dev
```

This preserves:
- Debug symbols
- Assertion checks
- Larger binary (useful for debugging)

### Release Build (optimized)

```bash
./build_with_emscripten.sh
# or
./build_with_emscripten.sh --release
```

This produces:
- Fully optimized code (-O3)
- Link-time optimization (LTO)
- Stripped symbols
- Smaller binary size

### Custom Configuration

For fine-tuned control, edit `build_with_emscripten.sh` and modify:

```bash
cmake "$QLEVER_ROOT" \
    -DCMAKE_TOOLCHAIN_FILE="$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -flto -s WASM=1" \
    # Add your custom flags here
    -GNinja
```

Common CMake options:
- `-DCMAKE_BUILD_TYPE=Release|Debug`
- `-DCMAKE_CXX_FLAGS="-O3"` (optimization level)
- `-DASAN=ON` (AddressSanitizer for memory checking)

## Testing the Build

### Unit Tests

```bash
cd wasm
npm run test:libqlever
```

This will:
1. Build the libqlever WASM module
2. Run all tests in `tests/`
3. Report results

### Integration Tests

Load a sample dataset and execute queries:

```bash
node tests/integration.js
```

See `wasm/examples/` for example usage patterns.

## Troubleshooting

### Error: "emcc not found"

Make sure you've activated Emscripten in the current shell:
```bash
source /path/to/emsdk/emsdk_env.sh
which emcc  # Should show path to emcc
```

### Error: "wasm32-emscripten target not found"

Install the Emscripten target for Rust:
```bash
rustup target add wasm32-emscripten
rustup target list | grep emscripten  # Verify
```

### Build fails with memory errors

Increase available memory:
```bash
export EMCC_ALLOW_GROWTH=1
export EMCC_TOTAL_MEMORY=536870912  # 512 MB
./build_with_emscripten.sh
```

### WASM module won't load in browser

Check that:
1. WASM file is served with correct MIME type (`application/wasm`)
2. CORS headers are properly configured
3. WASM module is in the same directory as JavaScript wrapper

### Performance is slow

1. Verify you're using Release build (`-DCMAKE_BUILD_TYPE=Release`)
2. Check that LTO is enabled: `-flto` in compiler flags
3. Profile using Chrome DevTools or Node.js profiler

## File Structure After Build

```
wasm/
├── wasm_build/
│   ├── lib/
│   │   ├── libqlever_wasm.wasm    # Main WASM module
│   │   └── libqlever_wasm.js      # JavaScript wrapper
│   └── CMakeCache.txt
├── pkg/                            # wasm-bindgen output
│   ├── qlever_wasm.wasm           # Rust-compiled WASM
│   ├── qlever_wasm.js             # Rust WASM wrapper
│   └── qlever_wasm.d.ts           # TypeScript types
├── src/
│   ├── libqlever_bindings.rs      # Rust FFI layer
│   ├── wasm_wrapper.ts             # JavaScript API
│   └── index.d.ts                  # Public API types
└── build_with_emscripten.sh       # Build script
```

## API Usage After Build

Once built, use the compiled WASM module:

```javascript
import { initializeWasm, createStore, DataFactory } from './wasm_wrapper.ts';

// Initialize WASM module
await initializeWasm();

// Create store backed by libqlever
const store = createStore();
await store.init('/path/to/qlever/index');

// Execute SPARQL queries
const result = await store.query('SELECT ?s WHERE { ?s ?p ?o }');
console.log(result.results.bindings);
```

## Performance Characteristics

With libqlever WASM backend:

- **Query execution**: Native WASM speed (typically 2-5x faster than pure JavaScript)
- **Memory usage**: Shared between JS and WASM (~2-4 MB for small datasets)
- **Bundle size**: 500 KB - 1.5 MB gzipped (reasonable for features)
- **Startup time**: 500 ms - 2 sec (index loading)

For large RDF datasets (millions of triples), query latency:
- Typical: 10-500 ms per query
- Complex: 100-5000 ms per query

## Next Steps

1. **Publishing**: See `README_NPM.md` for npm package release
2. **Integration**: Use in web/Node.js applications
3. **Optimization**: Profile and optimize using Emscripten tools

## References

- [Emscripten Documentation](https://emscripten.org/)
- [wasm-bindgen Guide](https://rustwasm.org/docs/wasm-bindgen/)
- [WASM/JavaScript Interop](https://developer.mozilla.org/en-US/docs/WebAssembly/Using_the_JavaScript_API)
- [QLever Documentation](https://github.com/ad-freiburg/qlever)
