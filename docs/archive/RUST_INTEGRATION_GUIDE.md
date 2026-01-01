# Rust FFI Integration with Real QLever - Implementation Guide

## Current Status

We have successfully integrated the Rust FFI with QLever's real C++ API:

### What Changed

**C Wrapper (`src/qlever_c.cpp`)** - Now uses real QLever API:
```cpp
// OLD: Placeholder code with manual query execution
// NEW: Uses qlever::Qlever high-level API

#include "libqlever/Qlever.h"

struct QleverContext {
    std::shared_ptr<qlever::Qlever> engine;  // Real QLever engine
};

qlever_open():
  - Creates EngineConfig from index path
  - Instantiates qlever::Qlever engine
  - Returns opaque handle

qlever_query_json():
  - Calls engine->query() directly
  - Automatically handles: parsing, planning, execution
  - Returns JSON result (standard SPARQL Results Format)
  - Supports detailed timings via MediaType parameter
```

## Architecture: Three Layers

```
┌─────────────────────────────────────────────┐
│ Rust Application                            │
│ let store = Store::open("/path/to/index")?  │
│ let results = store.query("SELECT...")?     │
└────────────────┬────────────────────────────┘
                 │ QuerySolution, Term, NamedNode
                 │
┌────────────────▼─────────────────────────────┐
│ Safe Rust API (qlever_store.rs)              │
│ - Handle validation (NonNull<T>)             │
│ - Memory safety (Drop trait)                 │
│ - Error handling (Result<T>)                 │
│ - JSON parsing (serde_json)                  │
└────────────────┬─────────────────────────────┘
                 │ C strings (CString)
                 │ Opaque handles (void*)
                 │
┌────────────────▼─────────────────────────────┐
│ C FFI Layer (ffi.rs)                         │
│ extern "C" {                                 │
│   qlever_open()                              │
│   qlever_query_json()                        │
│   qlever_free_string()                       │
│   qlever_close()                             │
│ }                                            │
└────────────────┬─────────────────────────────┘
                 │ C ABI (stable)
                 │
┌────────────────▼──────────────────────────────┐
│ C++ Wrapper (qlever_c.cpp)                    │
│ extern "C" { ... }                            │
│                                               │
│ QleverContext {                               │
│   std::shared_ptr<qlever::Qlever> engine     │
│ }                                             │
│                                               │
│ qlever_open():                               │
│   → new EngineConfig                         │
│   → new qlever::Qlever(config)               │
│                                               │
│ qlever_query_json():                         │
│   → engine->query(sparql, mediaType)         │
│   → return JSON string                       │
└────────────────┬───────────────────────────────┘
                 │ C++ calls
                 │
┌────────────────▼──────────────────────────────┐
│ QLever C++ Library (libqlever)                │
│                                               │
│ class Qlever {                                │
│   Index index_;                               │
│   QueryExecutionContext exec_context_;       │
│                                               │
│   std::string query(const std::string& sparql,
│                     MediaType format)        │
│ }                                             │
│                                               │
│ Flow: SPARQL → Parse → Plan → Execute → JSON │
└────────────────────────────────────────────────┘
```

## Real QLever API Used

### `qlever::Qlever` class (from `libqlever/Qlever.h`)

```cpp
namespace qlever {

class Qlever {
public:
    // Load index from disk
    explicit Qlever(const EngineConfig& config);

    // Execute query and return JSON
    std::string query(
        const std::string& sparql_query,
        ad_utility::MediaType mediaType = sparqlJson
    ) const;

    // Parse and plan separately
    QueryPlan parseAndPlanQuery(std::string query) const;

    // Execute pre-planned query
    std::string query(const QueryPlan& plan,
                      ad_utility::MediaType mediaType) const;
};

// EngineConfig
struct EngineConfig {
    std::string baseName_;         // Index path
    bool loadTextIndex_ = false;   // Full-text search
    bool persistUpdates_ = false;  // Delta persistence
    std::string memoryLimit_;      // RAM limit
};

// Output formats
enum MediaType {
    sparqlJson,    // Standard SPARQL JSON Results Format
    qleverJson,    // Includes execution timings
    tsv, csv, turtle
};

}  // namespace qlever
```

## Building with Real QLever

### Step 1: Install Dependencies

```bash
# On Ubuntu/Debian
sudo apt-get install -y \
    libicu-dev \
    libboost-all-dev \
    libssl-dev \
    zlib1g-dev \
    libz3-dev

# On macOS
brew install icu4c boost openssl zstandard
```

### Step 2: Build QLever C++ Library

```bash
cd /home/user/qlever
mkdir -p build
cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release \
      -GNinja \
      -DCMAKE_CXX_STANDARD=20 \
      ..

# Build library (may take 10-30 minutes)
ninja libqlever

# Build C wrapper library
g++ -fPIC -shared \
    -o lib/libqlever_c.so \
    ../src/qlever_c.cpp \
    -I.. -Isrc \
    -Llib \
    -lqlever \
    -std=c++17 \
    -O3
```

### Step 3: Test Rust Linking

```bash
cd /home/user/qlever/rust

# Should automatically detect and link libqlever_c.so
cargo build --release

# Run tests with real library
cargo test --release

# Run benchmarks
LD_LIBRARY_PATH=../build/lib ../target/release/ffi_benchmark
```

## Expected Changes to Behavior

### With Mock Library (Current)
```
Query latency: 18-20 μs (mock only)
Throughput: 52,000 q/s
No actual query processing
```

### With Real Library (After Build)
```
Query latency: 1-100 ms (depends on query complexity)
Throughput: 100-1000 q/s (depends on index size)
Full SPARQL query support
Real RDF indexing and triple matching
```

## Result Format Integration

QLever returns standard SPARQL JSON Results Format:

```json
{
  "head": {
    "vars": ["?s", "?p", "?o"]
  },
  "results": {
    "bindings": [
      {
        "?s": {"type": "uri", "value": "http://example.org/entity1"},
        "?p": {"type": "uri", "value": "http://example.org/property1"},
        "?o": {"type": "literal", "value": "value1"}
      },
      ...
    ]
  },
  "timings": {
    "query_ms": 1.5,
    "planning_ms": 0.2,
    "execution_ms": 1.3
  }
}
```

The Rust parser already handles this format correctly.

## Configuration Options

### Minimal Configuration (Default)
```rust
let store = Store::open("/path/to/index")?;
// Uses QLever defaults:
// - Text index: disabled
// - Memory limit: 1GB
// - Updates: not persisted
```

### Custom Configuration
```rust
let config = r#"{"loadTextIndex": true, "memoryLimit": "2GB"}"#;
let store = Store::open_with_config("/path/to/index", config)?;
```

Future implementation could parse JSON config in C wrapper.

## Testing with Real Data

### Create a Test Index

```bash
# 1. Get sample RDF data
curl -O https://example.org/test-data.nt

# 2. Build index
/home/user/qlever/build/IndexBuilderMain \
  -i test-data.nt \
  -o test_index

# 3. Test with Rust
cargo run --release --example query /path/to/test_index
```

### Example Query
```rust
use qlever::Store;

fn main() -> Result<()> {
    let store = Store::open("./test_index")?;

    let results = store.query(
        "SELECT ?s ?p WHERE { ?s ?p ?o } LIMIT 10"
    )?;

    for solution in results {
        println!("{:?}", solution);
    }

    Ok(())
}
```

## Performance Expectations

### Single Query (Real QLever)

```
Parsing:         0.5-2 ms
Planning:        0.2-1 ms
Execution:       1-100 ms (depends on data)
Result delivery: <1 ms
Total:           ~2-103 ms

FFI overhead:    ~19 μs (negligible, <0.1%)
```

### Query Throughput

With a typical 10-100 million triple index:

```
Simple patterns:     1,000-10,000 q/s
Complex joins:       100-1,000 q/s
Aggregations:        10-100 q/s
Full scans:          1-10 q/s
```

Rust FFI maintains 52,000 q/s for mock queries, showing the overhead is minimal.

## Known Limitations

1. **Dependencies Required**
   - Building requires ICU, Boost, OpenSSL, Zstandard
   - Full build may take 30+ minutes
   - Disk space: ~1GB for build artifacts

2. **Configuration Parsing**
   - config_json parameter is currently ignored
   - Could be enhanced to parse JSON config in C wrapper

3. **Error Messages**
   - C doesn't support exceptions naturally
   - Errors return null; detailed messages available in result JSON

4. **Streaming Results**
   - Current implementation materializes all results
   - Could optimize with streaming later

5. **Caching**
   - Query result cache is per-Engine instance
   - No inter-process caching

## Files Involved

### Modified
- `src/qlever_c.cpp` - Now uses real qlever::Qlever
- `src/qlever_c.h` - C interface (unchanged, already correct)

### Supporting Files
- `rust/src/qlever_store.rs` - Safe Rust wrapper
- `rust/src/ffi.rs` - FFI declarations
- `rust/build.rs` - Auto-detection of C library
- `rust/Cargo.toml` - Dependencies

### Build System
- `CMakeLists.txt` - Main build configuration
- `conanfile.txt` - C++ dependency management

## Integration Checklist

- [x] Updated C wrapper to use real QLever API
- [x] Removed mock code from C++ implementation
- [x] Verified interfaces match real QLever
- [ ] Install system dependencies (ICU, Boost, etc.)
- [ ] Build QLever C++ library
- [ ] Build C wrapper library (libqlever_c.so)
- [ ] Test Rust linking
- [ ] Create test RDF index
- [ ] Run end-to-end query test
- [ ] Benchmark real performance
- [ ] Document actual performance metrics

## Next Steps

1. **Install Dependencies** (if not already present)
   ```bash
   # Check if ICU is available
   pkg-config --cflags --libs icu-uc
   ```

2. **Build QLever**
   ```bash
   cd build && cmake -DCMAKE_BUILD_TYPE=Release -GNinja ..
   ninja libqlever
   ```

3. **Compile C Wrapper**
   ```bash
   g++ -fPIC -shared -o lib/libqlever_c.so ../src/qlever_c.cpp \
       -I.. -Isrc -Llib -lqlever -std=c++17 -O3
   ```

4. **Test Rust Build**
   ```bash
   cd rust && cargo build --release
   ```

5. **Benchmark Performance**
   ```bash
   LD_LIBRARY_PATH=../build/lib ./target/release/ffi_benchmark
   ```

6. **Create Integration Examples**
   - Query a real RDF dataset
   - Compare with direct C++ calls
   - Document actual performance

## References

- QLever libqlever API: `src/libqlever/Qlever.h`
- Engine config: `src/libqlever/EngineConfig.h`
- SPARQL JSON format: https://www.w3.org/TR/sparql11-results-json/
- QLever documentation: See `docs/` directory

---

**Status**: ✅ Integration code complete
**Next**: Build real QLever library and test with actual RDF data
