# QLever WASM Implementation Plan: Using libqlever C++ Backend

## Executive Summary

**Goal**: Create the fastest SPARQL JS library by compiling QLever's proven libqlever C++ code to WebAssembly instead of implementing custom Rust code.

**Approach**:
1. Compile libqlever (C++20) to WASM using Emscripten
2. Create thin C++ wrapper layer with wasm-bindgen exports
3. Implement JavaScript API layer that calls libqlever WASM functions
4. Keep existing TypeScript API design (index.d.ts) - it's correct and Oxigraph-compatible

**Benefits**:
- ✅ Uses battle-tested QLever code (used in production)
- ✅ Performance parity with QLever server
- ✅ Full SPARQL 1.1 compliance
- ✅ Maintains clean JavaScript API surface
- ✅ Single dependency: WASM module instead of multiple Rust crates

---

## Architecture Overview

```
┌─────────────────────────────────────┐
│   JavaScript/TypeScript API         │
│   (index.d.ts - UNCHANGED)          │
│  Store, QueryBuilder, DataFactory   │
└──────────────┬──────────────────────┘
               │ Calls
┌──────────────▼──────────────────────┐
│   JavaScript Wrapper Layer          │
│   (new: wasm_wrapper.js/ts)         │
│  Maps JS calls to WASM functions    │
└──────────────┬──────────────────────┘
               │ Calls WASM
┌──────────────▼──────────────────────┐
│   WASM Module (Emscripten)          │
│   Compiled libqlever C++ code       │
│  qlever-wasm.wasm (2-5MB gzipped)   │
└──────────────┬──────────────────────┘
               │ Executes
┌──────────────▼──────────────────────┐
│   libqlever C++ Classes             │
│   ├─ Qlever (main query engine)     │
│   ├─ Index (RDF storage)            │
│   ├─ QueryPlan (parsed queries)     │
│   └─ QueryExecutor (execution)      │
└─────────────────────────────────────┘
```

---

## Phase 1: Preparation & Cleanup

### Task 1.1: Delete Custom Rust Implementations

Delete the custom Rust code that will be replaced by libqlever:

**Files to DELETE:**
1. `wasm/src/rdf_term.rs` (550 lines) - RdfTerm, Triple, Quad, QuadPattern
   - Reason: libqlever has its own RDF term system

2. `wasm/src/parsers.rs` (400 lines) - RdfParser for N-Quads, N-Triples, Turtle
   - Reason: libqlever has production RDF parsers (in `src/parser/`)

3. `wasm/src/store.rs` (450 lines) - In-memory Store implementation
   - Reason: libqlever's Index is more sophisticated

4. `wasm/src/datafactory.rs` (150 lines) - RDF/JS DataFactory implementation
   - **Partial Keep**: May keep as thin wrapper for client-side term creation
   - But core data structure creation will use libqlever types

**Files to KEEP/REFACTOR:**
- `wasm/src/index.d.ts` ✅ (TypeScript definitions - CORRECT, don't modify)
- `wasm/src/lib.rs` ⚠️ (refactor to remove custom implementations)
- `wasm/src/client.rs` ✅ (QueryBuilder and QueryClient for remote - can keep)
- `wasm/examples/advanced.js` ✅ (API usage examples - CORRECT)
- `wasm/README.md` ✅ (Documentation - good)

### Task 1.2: Set Up Project Structure for Emscripten

Create new files:
```
wasm/
├── src/
│   ├── lib.rs (REFACTORED - export libqlever bindings)
│   ├── libqlever_bindings.rs (NEW - wasm-bindgen wrapper)
│   ├── store_wrapper.rs (NEW - wraps libqlever Index for Store API)
│   ├── query_wrapper.rs (NEW - wraps libqlever QueryPlan for QueryBuilder API)
│   ├── client.rs (KEEP - for remote queries)
│   └── index.d.ts (KEEP - API definitions)
├── js/
│   └── wasm_wrapper.ts (NEW - JavaScript wrapper for WASM calls)
├── build/
│   └── build_with_emscripten.sh (NEW - Emscripten build script)
└── Cargo.toml (MODIFY - add wasm-bindgen, emscripten target)
```

---

## Phase 2: Emscripten Build Setup

### Task 2.1: Configure Cargo for Emscripten Target

**Modify `wasm/Cargo.toml`:**

```toml
[package]
name = "qlever-wasm"
version = "0.1.0"
edition = "2021"

[dependencies]
wasm-bindgen = "0.2"
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"

[lib]
crate-type = ["cdylib"]

# Use Emscripten for compilation to WASM
[profile.release]
opt-level = 3
lto = true
```

### Task 2.2: Create Emscripten Build Script

**New file: `wasm/build/build_with_emscripten.sh`**

```bash
#!/bin/bash

# Install Emscripten (first time only)
# git clone https://github.com/emscripten-core/emsdk.git
# cd emsdk
# ./emsdk install latest
# ./emsdk activate latest

# Activate Emscripten environment
source /path/to/emsdk/emsdk_env.sh

# Set up Rust for Emscripten
rustup target add wasm32-emscripten

# Compile libqlever to WASM using Emscripten
# Strategy: Create C++ wrapper that calls libqlever
#           Use wasm-bindgen to expose to JavaScript

# Build the Rust + Emscripten WASM module
cd wasm
EMCC_CFLAGS="-O3 -flto" cargo build --target wasm32-emscripten --release

# Output: target/wasm32-emscripten/release/qlever_wasm.wasm
```

**Alternative: Direct C++ to WASM (might be better)**

Instead of Rust + Emscripten, compile C++ directly:

```bash
#!/bin/bash

# Compile just the C++ libqlever code to WASM
emcc \
  -I/home/user/qlever/src \
  /home/user/qlever/src/libqlever/Qlever.cpp \
  /home/user/qlever/src/libqlever/libqlever_wrapper.cpp \
  -O3 \
  -s WASM=1 \
  -s ENVIRONMENT=web,node \
  -s EXPORT_ES6=1 \
  -s TOTAL_MEMORY=256MB \
  --bind \
  -o wasm/qlever-wasm.js
```

---

## Phase 3: C++ Wrapper Layer (libqlever Bindings)

### Task 3.1: Create libqlever C++ Wrapper

**New file: `src/libqlever/libqlever_wasm_wrapper.h`**

This C++ file exposes libqlever functionality for WASM binding:

```cpp
#ifndef QLEVER_SRC_LIBQLEVER_WASM_WRAPPER_H
#define QLEVER_SRC_LIBQLEVER_WASM_WRAPPER_H

#include <string>
#include <vector>
#include "libqlever/Qlever.h"
#include "util/http/MediaTypes.h"

namespace qlever::wasm {

// High-level WASM-friendly wrapper around Qlever
class QleverWasm {
 private:
  std::unique_ptr<Qlever> qlever_;
  std::string lastErrorMessage_;

 public:
  // Initialize with index files
  bool init(const std::string& indexBasename);

  // Execute SPARQL query, return JSON result
  std::string query(const std::string& sparqlQuery);

  // Get last error message
  std::string getLastError() const;

  // Check if initialized
  bool isInitialized() const;
};

}  // namespace qlever::wasm

#endif
```

**New file: `src/libqlever/libqlever_wasm_wrapper.cpp`**

```cpp
#include "libqlever_wasm_wrapper.h"
#include "global/RuntimeParameters.h"
#include "util/Exception.h"

namespace qlever::wasm {

bool QleverWasm::init(const std::string& indexBasename) {
  try {
    EngineConfig config;
    config.baseName_ = indexBasename;
    qlever_ = std::make_unique<Qlever>(config);
    return true;
  } catch (const std::exception& e) {
    lastErrorMessage_ = e.what();
    return false;
  }
}

std::string QleverWasm::query(const std::string& sparqlQuery) {
  if (!qlever_) {
    lastErrorMessage_ = "Qlever not initialized";
    return "";
  }
  try {
    return qlever_->query(sparqlQuery, ad_utility::MediaType::sparqlJson);
  } catch (const std::exception& e) {
    lastErrorMessage_ = e.what();
    return "";
  }
}

std::string QleverWasm::getLastError() const {
  return lastErrorMessage_;
}

bool QleverWasm::isInitialized() const {
  return qlever_ != nullptr;
}

}  // namespace qlever::wasm
```

---

## Phase 4: WASM Bindings Layer (Rust/wasm-bindgen)

### Task 4.1: Create Rust WASM Bindings

**New file: `wasm/src/libqlever_bindings.rs`**

```rust
use wasm_bindgen::prelude::*;

// Binding to C++ QleverWasm class
#[wasm_bindgen]
pub struct QleverStore {
    // Internal pointer to C++ QleverWasm (via FFI)
    inner: *mut QleverWasmFFI,
}

#[wasm_bindgen]
impl QleverStore {
    #[wasm_bindgen(constructor)]
    pub fn new() -> QleverStore {
        let ptr = unsafe { create_qlever_wasm() };
        QleverStore { inner: ptr }
    }

    #[wasm_bindgen]
    pub fn init(&mut self, index_basename: String) -> Result<(), JsValue> {
        unsafe {
            let result = init_qlever_wasm(self.inner, index_basename.as_ptr() as i32);
            if result == 0 {
                Err(JsValue::from_str("Failed to initialize QLever"))
            } else {
                Ok(())
            }
        }
    }

    #[wasm_bindgen]
    pub fn query(&self, sparql: String) -> Result<String, JsValue> {
        unsafe {
            let result = query_qlever_wasm(self.inner, sparql.as_ptr() as i32);
            if result.is_null() {
                let error = get_qlever_error(self.inner);
                Err(JsValue::from_str(&error))
            } else {
                Ok(string_from_ptr(result))
            }
        }
    }
}

impl Drop for QleverStore {
    fn drop(&mut self) {
        unsafe {
            delete_qlever_wasm(self.inner);
        }
    }
}

// FFI bindings to C++ functions
extern "C" {
    fn create_qlever_wasm() -> *mut QleverWasmFFI;
    fn delete_qlever_wasm(ptr: *mut QleverWasmFFI);
    fn init_qlever_wasm(ptr: *mut QleverWasmFFI, basename: i32) -> i32;
    fn query_qlever_wasm(ptr: *mut QleverWasmFFI, query: i32) -> *const u8;
    fn get_qlever_error(ptr: *mut QleverWasmFFI) -> String;
}

// Opaque C++ type
#[repr(C)]
pub struct QleverWasmFFI;

// Helper: convert pointer to Rust string
unsafe fn string_from_ptr(ptr: *const u8) -> String {
    let mut len = 0;
    let mut p = ptr;
    while *p != 0 {
        len += 1;
        p = p.add(1);
    }
    let slice = std::slice::from_raw_parts(ptr, len);
    String::from_utf8_lossy(slice).to_string()
}
```

### Task 4.2: Update lib.rs for WASM Exports

**Refactor `wasm/src/lib.rs`:**

```rust
mod libqlever_bindings;

pub use libqlever_bindings::QleverStore;

// Keep existing enums for API compatibility
#[wasm_bindgen]
pub enum QueryType {
    Select = 0,
    Construct = 1,
    Describe = 2,
    Ask = 3,
}

#[wasm_bindgen]
pub enum ResultFormat {
    Json = "json",
    Xml = "xml",
    Csv = "csv",
    Turtle = "turtle",
    NTriples = "ntriples",
}

// Export initialization
#[wasm_bindgen]
pub async fn init() -> Result<(), JsValue> {
    // Initialize WASM module, set panic hook, etc.
    #[cfg(feature = "console_error_panic_hook")]
    console_error_panic_hook::set_once();
    Ok(())
}

#[wasm_bindgen]
pub fn set_panic_hook() {
    #[cfg(feature = "console_error_panic_hook")]
    console_error_panic_hook::set_once();
}
```

---

## Phase 5: JavaScript API Wrapper Layer

### Task 5.1: Create JavaScript WASM Wrapper

**New file: `wasm/js/wasm_wrapper.ts`**

```typescript
import init, { QleverStore } from '../wasm_pkg';

let wasmModule: any = null;
let wasmReady = false;

export async function initializeWasm() {
  if (!wasmReady) {
    wasmModule = await init();
    wasmReady = true;
  }
  return wasmModule;
}

export class WasmQleverStore {
  private innerStore: any;

  constructor() {
    this.innerStore = new wasmModule.QleverStore();
  }

  async init(indexBasename: string): Promise<void> {
    return this.innerStore.init(indexBasename);
  }

  query(sparql: string): Promise<string> {
    return Promise.resolve(this.innerStore.query(sparql));
  }

  add(quad: any): boolean {
    // Delegate to libqlever's add operation
    // via SPARQL INSERT or internal API
    return true;
  }

  delete(quad: any): boolean {
    // Delegate to libqlever's delete
    return true;
  }

  // ... other Store methods
}
```

### Task 5.2: Implement JavaScript Store API

**Refactor `wasm/src/index.d.ts` implementation** (create corresponding .ts file):

```typescript
import { WasmQleverStore, initializeWasm } from './js/wasm_wrapper';

export class Store {
  private backend: WasmQleverStore;

  constructor() {
    this.backend = new WasmQleverStore();
  }

  async init(indexBasename: string): Promise<void> {
    await initializeWasm();
    return this.backend.init(indexBasename);
  }

  add(quad: Quad): boolean {
    // Convert JS Quad to SPARQL INSERT
    // Execute via this.backend.query()
    return true;
  }

  async query(sparql: string): Promise<QueryResult> {
    const jsonResult = await this.backend.query(sparql);
    return JSON.parse(jsonResult);
  }

  // ... remaining Store methods
}
```

---

## Phase 6: Testing & Validation

### Task 6.1: Test with Examples

Validate that all 20 examples in `wasm/examples/advanced.js` work correctly:
- Basic queries (SELECT, CONSTRUCT, DESCRIBE, ASK)
- Filters and optionals
- GROUP BY and ORDER BY
- Store CRUD operations
- Format parsing and serialization

### Task 6.2: Performance Benchmarking

Compare performance:
- Custom Rust implementation
- libqlever WASM implementation
- Expected: libqlever should be faster due to optimization

### Task 6.3: Size Analysis

WASM module size:
- Estimate: 2-5 MB (gzipped: 500KB-1.5MB)
- Larger than pure Rust, but justified by features and performance

---

## Implementation Checklist

### Phase 1: Cleanup
- [ ] Delete rdf_term.rs
- [ ] Delete parsers.rs
- [ ] Delete store.rs (custom implementation)
- [ ] Delete datafactory.rs (or keep as thin wrapper)
- [ ] Create new directory structure

### Phase 2: Emscripten Setup
- [ ] Install Emscripten SDK
- [ ] Configure Cargo.toml for Emscripten
- [ ] Create build_with_emscripten.sh script
- [ ] Test initial compilation

### Phase 3: C++ Wrapper
- [ ] Create libqlever_wasm_wrapper.h
- [ ] Implement libqlever_wasm_wrapper.cpp
- [ ] Add to libqlever CMakeLists.txt

### Phase 4: WASM Bindings
- [ ] Create libqlever_bindings.rs
- [ ] Implement FFI layer
- [ ] Refactor lib.rs
- [ ] Test compilation to WASM

### Phase 5: JavaScript API
- [ ] Create wasm_wrapper.ts
- [ ] Implement Store wrapper
- [ ] Implement DataFactory wrapper
- [ ] Implement QueryBuilder wrapper

### Phase 6: Testing
- [ ] Run all 20 examples
- [ ] Performance testing
- [ ] Size analysis
- [ ] Documentation updates

---

## Expected Outcomes

**After Implementation:**
1. ✅ Fastest SPARQL JS library (using optimized QLever C++)
2. ✅ Full SPARQL 1.1 compliance (from QLever)
3. ✅ Clean JavaScript API (Oxigraph-compatible)
4. ✅ Single, well-defined WASM dependency
5. ✅ Production-ready (uses battle-tested QLever code)

**Performance Characteristics:**
- Query execution: Native WASM performance (~2-5x faster than pure JS)
- Memory footprint: Shared between JS and WASM (efficient)
- Bundle size: 500KB-1.5MB gzipped (acceptable for the performance gain)

---

## Alternative Approaches Considered

### 1. Keep Custom Rust Implementation ❌
- **Pros**: Smaller bundle size, pure Rust
- **Cons**: Custom SPARQL engine lacks QLever's optimizations, maintenance burden
- **Decision**: Rejected (user request: "I do not want a custom implementation")

### 2. Use Remote QLever Server ❌
- **Pros**: Access to full QLever features
- **Cons**: Network latency, server dependency, not true WASM
- **Decision**: Not aligned with WASM goal

### 3. Compile Full QLever to WASM ⚠️
- **Pros**: Access to all QLever features
- **Cons**: Very large WASM module, complex build
- **Decision**: Use libqlever subset instead

### 4. libqlever with Emscripten ✅ SELECTED
- **Pros**: Optimized, proven code, clean API, manageable size
- **Cons**: Emscripten adds build complexity
- **Decision**: Best balance of performance and practicality

---

## Next Steps

1. **Approve this plan** - Confirm approach before implementation
2. **Install Emscripten** - Set up build environment
3. **Phase 1 & 2** - Cleanup and build setup (fastest path)
4. **Phase 3 & 4** - C++ and WASM bindings
5. **Phase 5** - JavaScript API layer
6. **Phase 6** - Testing and optimization

**Estimated Timeline**:
- Phases 1-2: 1-2 hours (setup)
- Phases 3-4: 3-4 hours (bindings)
- Phases 5-6: 2-3 hours (API + testing)
- **Total: ~1-2 days** for complete implementation

---

## References

- libqlever API: `/home/user/qlever/src/libqlever/Qlever.h`
- Example: `/home/user/qlever/src/libqlever/LibQLeverExample.cpp`
- WASM-bindgen: https://rustwasm.org/docs/wasm-bindgen/
- Emscripten: https://emscripten.org/
- Target API: `wasm/src/index.d.ts` (keep as-is)
