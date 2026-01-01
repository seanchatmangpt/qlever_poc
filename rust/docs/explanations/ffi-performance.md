# FFI & Performance Deep Dive

Understanding C++ interoperability and performance characteristics.

> ⏱️ **30 minutes** | 🎯 **Advanced** | 📦 **C FFI, Performance**

---

## Overview

QLever Rust is a thin, high-performance wrapper around a C++ SPARQL engine. This guide explains:

1. How Rust and C++ interact via FFI
2. Memory safety and zero-copy semantics
3. Performance characteristics and bottlenecks
4. Optimization strategies

---

## Architecture: Rust-C++ Boundary

```
Rust Layer (Safe)
├── Store (public API)
├── QuerySolution (safe containers)
├── RDF Types (Term, Triple, Quad)
└── QueryCache (Rust memory management)
       ↓ (FFI layer)
C FFI Bindings (unsafe)
├── libqlever_query()
├── libqlever_ask()
├── libqlever_construct()
└── libqlever_describe()
       ↓ (C++ engine)
C++ Backend
├── Query Parser
├── Query Planner
├── Execution Engine
└── RDF Index
```

---

## FFI Bindings

Low-level C function bindings:

```rust
// ffi.rs - Low-level FFI layer
#[link(name = "qlever")]
extern "C" {
    pub fn libqlever_query(query: *const c_char) -> *const c_char;
    pub fn libqlever_free(ptr: *mut c_char);
    // ... other functions
}
```

**Safety considerations:**
- Raw pointers used (`*const c_char`)
- Memory lifecycle managed (allocated in C++, freed by Rust)
- Bounds checking at language boundary

---

## Memory Safety

### String Conversion (Safe)

```rust
// Rust → C++
let query = "SELECT ?x WHERE { ?x ?p ?o }";
let c_query = CString::new(query)?;  // Adds null terminator
let ptr = c_query.as_ptr();
// Send to C++
unsafe {
    let result = libqlever_query(ptr);
}

// C++ → Rust
unsafe {
    let c_str = CStr::from_ptr(result);
    let rust_string = c_str.to_string_lossy().to_string();
    libqlever_free(result as *mut c_char);  // Free C++ memory
}
```

### JSON Serialization

All data passed as JSON strings over FFI boundary:

```
Query: String
  ↓
JSON-encoded
  ↓
C++ (char*)
  ↓
C++ processes
  ↓
JSON result string
  ↓
Rust parses JSON
  ↓
QuerySolution objects
```

**Trade-off:** Extra JSON serialization overhead, but:
- Safe (no direct C++ object sharing)
- Compatible (language-agnostic)
- Debuggable (human-readable)

---

## Performance Characteristics

### Latency Breakdown

For typical query:

```
User code:           ~1 μs
Store::query()       ~10 μs (Rust overhead)
JSON serialization   ~100 μs (small payloads)
HTTP request         ~1-2 ms (network)
Server processing    ~1-3 ms (query execution)
JSON parsing         ~100 μs (result deserialization)
Total:               ~2-5 ms (typical)
```

### Memory Usage

Query results:
```
Vec<QuerySolution>
├── String (variable name) — ~50 bytes each
├── String (value) — ~100 bytes each (average)
└── HashMap overhead — ~48 bytes per entry
```

**Example:** 1000 results × 3 variables ≈ 500 KB

**Streaming alternative:**
```
Single QuerySolution in memory — ~1-2 KB
Process, discard, repeat — constant memory
```

---

## Optimization Strategies

### 1. Connection Reuse (2-5x faster)

```rust
// ✅ Good: Reuse connection
let store = Store::new("http://localhost:7777")?;
for query in queries {
    store.query(query).await?;
}

// ❌ Bad: Reconnect each time (TCP handshake overhead)
for query in queries {
    let store = Store::new("http://localhost:7777")?;
    store.query(query).await?;
}
```

### 2. Query Caching (10-100x faster for repeats)

```rust
// Cache hits avoid entire server round-trip
let cache = QueryCache::new(100, Duration::from_secs(300));
let results = cache.query_with_cache(&store, query).await?;  // 2nd call: <1 ms
```

### 3. Batch Queries (3-5x faster)

```rust
// Execute concurrently, not sequentially
let (r1, r2, r3) = tokio::join!(
    store.query(q1),
    store.query(q2),
    store.query(q3),
);
// Network parallelism: 1 × network latency instead of 3 ×
```

### 4. Streaming Large Results (100x less memory)

```rust
// ❌ Bad: Loads all results at once
let all_results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;  // 500 MB?

// ✅ Good: Process one at a time
let mut results = store.query_streaming("SELECT ...").await?;
while let Some(solution) = results.next().await? {
    // ~2 KB per iteration, not 500 MB total
}
```

---

## Bottlenecks & Solutions

### Bottleneck 1: Network Latency

**Symptom:** Queries take 1-2 ms even though server returns instantly

**Cause:** Network round-trip latency (TCP, TLS, etc.)

**Solutions:**
- Batch related queries (reduce round-trips)
- Cache results (eliminate round-trip)
- Use local server if possible

### Bottleneck 2: Large Result Sets

**Symptom:** Memory usage grows linearly with result count

**Cause:** All results loaded at once into Vec<QuerySolution>

**Solutions:**
- Use `.query_streaming()` for millions of rows
- Add `LIMIT` to queries
- Filter server-side in SPARQL

### Bottleneck 3: Slow Server

**Symptom:** Queries take 1-3 seconds despite simple pattern

**Cause:** Server processing is slow (CPU, I/O bound)

**Solutions:**
- Make SPARQL more specific (type filters, predicates)
- Add LIMIT to reduce result set
- Index frequently-queried predicates
- Upgrade server hardware

### Bottleneck 4: JSON Serialization

**Symptom:** Small overhead but adds up with many small queries

**Cause:** JSON parsing/serialization on every request

**Solutions:**
- Cache results to avoid re-parsing
- Batch small queries
- Use ASK for existence checks (smaller response)

---

## Benchmarking

Measure performance in your environment:

```rust
use std::time::Instant;

async fn benchmark_query(store: &Store, query: &str) {
    let start = Instant::now();
    let results = store.query(query).await?;
    let elapsed = start.elapsed();

    println!("Query: {:.2} ms", elapsed.as_secs_f64() * 1000.0);
    println!("Results: {}", results.len());
    println!("Per-result: {:.3} μs",
        (elapsed.as_secs_f64() / results.len() as f64) * 1_000_000.0);
}
```

---

## Zero-Copy Considerations

### Current Approach

All data serialized to JSON, not zero-copy:
- JSON bytes → String → HashMap
- Simple and safe, but not zero-copy

### Future Opportunities

Direct memory sharing could enable:
- Arrow format (columnar)
- Shared memory buffers
- Memory mapping for huge result sets

**Trade-off:** Extra safety/simplicity vs raw performance

---

## Comparing to In-Memory (Oxigraph)

| Aspect | QLever Rust | Oxigraph |
|--------|------------|----------|
| **Scale** | 1 billion+ triples | 1-100 million |
| **Latency** | 2-5 ms (network) | <1 ms (memory) |
| **Throughput** | 100s of concurrent | 1000s per-thread |
| **Memory** | Server-side | Client-side |
| **Setup** | Server required | Embedded |

**Choose QLever if:** You need scale (billions of triples)
**Choose Oxigraph if:** You need speed (millions, local data)

---

## Production Recommendations

```rust
// 1. Use persistent Store connection
let store = Arc::new(Store::new("http://qlever:7777")?);

// 2. Enable aggressive caching
let cache = QueryCache::new(500, Duration::from_secs(600));

// 3. Batch related queries
let (results1, results2) = tokio::join!(
    store.query(q1),
    store.query(q2),
);

// 4. Monitor performance
let start = Instant::now();
let results = store.query(query).await?;
metrics.observe("query_latency", start.elapsed());

// 5. Stream large results
if large_query {
    let mut results = store.query_streaming(query).await?;
    while let Some(solution) = results.next().await? {
        process(solution);
    }
}
```

---

## See Also

- **[Performance How-To](../how-to/performance.md)** — Practical optimization
- **[Architecture Guide](./architecture.md)** — Design details
- **[API Reference](../reference/api.md)** — Configuration options

---

**Status:** Deep-dive guide | **Last Updated:** 2025-01-01
