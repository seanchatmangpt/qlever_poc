# Architecture & Design

Understanding how the QLever Rust library is structured and why design decisions were made.

---

## Overview

The QLever Rust library is a **thin, type-safe wrapper** around a C++ backend. It emphasizes:

1. **Type safety** — Rust's type system prevents RDF errors at compile time
2. **Zero-copy where possible** — Share memory with C++ backend
3. **Minimal overhead** — Lightweight FFI layer
4. **Async-first** — Non-blocking I/O with Tokio
5. **Familiar API** — Similar to Oxigraph for easy adoption

---

## Layered Architecture

```
┌─────────────────────────────────────────────┐
│  User Application Code                      │
├─────────────────────────────────────────────┤
│  Public API Layer (Store, QuerySolution)   │
├─────────────────────────────────────────────┤
│  RDF Model Types (Term, Triple, Quad)      │
├─────────────────────────────────────────────┤
│  Query Execution (query, ask, construct)   │
├─────────────────────────────────────────────┤
│  FFI Layer (C bindings to libqlever)       │
├─────────────────────────────────────────────┤
│  C++ Backend (QLever SPARQL Engine)        │
└─────────────────────────────────────────────┘
```

---

## Core Modules

### `lib.rs` — Root Module

The library root, re-exporting public API:

```rust
pub use store::Store;
pub use model::{NamedNode, BlankNode, Literal, Term, Triple, Quad};
pub use query::QuerySolution;
pub use cache::QueryCache;
pub use error::{Error, Result};
```

Users typically only import from the root:
```rust
use qlever::{Store, NamedNode, Literal};
```

---

### `store.rs` — Store Interface

Defines the public `Store` trait and in-memory fallback implementation.

```rust
pub trait Store {
    async fn query(&self, sparql: &str) -> Result<Vec<QuerySolution>>;
    async fn ask(&self, sparql: &str) -> Result<bool>;
    // ... other methods
}
```

**Purpose:** Abstraction layer allowing multiple backends (in-memory, remote, etc.)

---

### `qlever_store.rs` — QLever Backend

The concrete implementation that talks to QLever servers via HTTP.

```rust
pub struct Store {
    url: String,
    client: HttpClient,
}

impl Store {
    pub async fn query(&self, sparql: &str) -> Result<Vec<QuerySolution>> {
        // Make HTTP request to QLever server
        // Parse JSON response
        // Convert to QuerySolution objects
    }
}
```

**Key responsibilities:**
- HTTP communication
- JSON parsing
- Error handling from server

---

### `model.rs` — RDF Types

Core RDF data structures:

```rust
pub enum Term {
    NamedNode(NamedNode),    // <http://example.org/x>
    BlankNode(BlankNode),    // _:b1
    Literal(Literal),        // "hello", 42, etc.
}

pub struct Triple {
    subject: Term,
    predicate: Term,
    object: Term,
}

pub struct Quad {
    subject: Term,
    predicate: Term,
    object: Term,
    graph: Option<NamedNode>,
}
```

**Design principle:** Mirror RDF 1.1 specification exactly. Type system enforces constraints (e.g., predicate must be NamedNode in Triple).

---

### `query.rs` — Result Types

Query result containers:

```rust
pub struct QuerySolution {
    bindings: HashMap<String, String>,
}

impl QuerySolution {
    pub fn get(&self, var: &str) -> Option<String> {
        self.bindings.get(var).map(|s| s.clone())
    }
}
```

**Design:** Lightweight wrapper around HashMap. String values (not parsed) to avoid multiple formats.

---

### `cache.rs` — Query Caching

Optional LRU cache with TTL:

```rust
pub struct QueryCache {
    cache: LruCache<String, (Instant, Vec<QuerySolution>)>,
    capacity: usize,
    ttl: Duration,
}
```

**Why separate?** Caching is optional; users add explicitly via `cache.query_with_cache()` instead of transparently caching (which can hide bugs).

---

### `error.rs` — Error Types

Rust error enum:

```rust
pub enum Error {
    QueryError(String),        // SPARQL syntax error
    InvalidUrl(String),        // Bad server URL
    ConnectionFailed(String),  // Network error
    SerializationError(String),// Bad JSON response
    IoError(String),           // File I/O error
}

pub type Result<T> = std::result::Result<T, Error>;
```

**Pattern:** Flat enum (not nested types). All errors convertible to string for simplicity.

---

### `ffi.rs` — C FFI Bindings

Low-level bindings to C++ libqlever:

```rust
#[link(name = "qlever")]
extern "C" {
    pub fn libqlever_query(query: *const c_char) -> *const c_char;
    pub fn libqlever_free(ptr: *mut c_char);
    // ... other FFI functions
}
```

**Design:** Minimal bindings, only what's needed. Higher-level API (Store) built on top.

---

## Request/Response Flow

### 1. User Code

```rust
let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
```

### 2. Store::query() (qlever_store.rs)

```rust
// Build HTTP request
let url = format!("{}/api/query", self.url);
let body = json!({ "query": sparql });

// Send HTTP request
let response = http_client.post(url)
    .json(&body)
    .send()
    .await?;

// Parse response
let json = response.json::<JsonResponse>().await?;

// Convert to QuerySolution
let solutions = json.results.iter()
    .map(|binding| QuerySolution::from(binding))
    .collect();
```

### 3. Server Processing

QLever C++ server:
- Parses SPARQL
- Plans execution
- Executes query
- Returns JSON

### 4. Return to User

```rust
Ok(solutions)  // Vec<QuerySolution>
```

---

## Design Decisions & Rationale

### 1. Async/Await First

**Decision:** All I/O operations are `async fn`.

**Rationale:**
- Non-blocking I/O essential for modern apps
- Tokio is Rust standard async runtime
- Allows batching multiple queries concurrently

---

### 2. String-Based Result Values

**Decision:** `QuerySolution` stores results as `HashMap<String, String>`.

**Rationale:**
- Simpler API (no format conversions)
- Server returns JSON strings anyway
- Type conversion happens at use site if needed
- Avoids premature parsing

**Trade-off:** Users convert to RDF types manually if needed.

---

### 3. Separate QueryCache

**Decision:** Caching is opt-in via `QueryCache`, not built into `Store`.

**Rationale:**
- Makes caching visible to user
- Users understand when cache hits occur
- Different apps have different caching needs
- Prevents subtle bugs from transparent caching

---

### 4. Thin FFI Layer

**Decision:** Minimal C FFI; most logic in Rust.

**Rationale:**
- Easier to maintain
- Rust compiler catches errors
- Most work is marshalling JSON, not C computation
- QLever server does actual heavy lifting

---

### 5. Oxigraph-Compatible API

**Decision:** Types and method names match oxigraph where possible.

**Rationale:**
- Familiar to existing Rust RDF developers
- Easy migration between libraries
- Reduces learning curve

---

## Memory Model

### Ownership

```
Store (owns HTTP client, connection state)
  ├── HttpClient
  └── Configuration

QuerySolution (owned by user)
  └── HashMap<String, String>

Triple/Quad (owned by user)
  └── Term (can be NamedNode, BlankNode, Literal)
```

**Principle:** Everything is owned, no borrowing across async boundaries. Safe concurrent access via Arc<> if needed by user.

---

### Zero-Copy Semantics

**Goal:** Minimize copying between C++ and Rust.

**Current approach:**
- JSON strings from C++ copied once to Rust String
- User receives owned Vec<QuerySolution>
- Streaming APIs (ResultIterator) avoid materializing all results

**Future opportunity:**
- Direct memory mapping for large result sets
- Currently limited by JSON serialization format

---

## Concurrency Model

### Thread Safety

All public types are `Send + Sync`:

```rust
pub struct Store { /* ... */ }
unsafe impl Send for Store {}
unsafe impl Sync for Store {}
```

**Implication:** Safe to share `Arc<Store>` across threads/tasks.

### Async Runtime

Works with **any** Tokio-based async runtime:

```rust
// Native app
#[tokio::main]
async fn main() { ... }

// Web framework (Actix, Axum, etc.)
// All use Tokio internally
```

### No Blocking in Async

All I/O uses async/await:

```rust
// ❌ Never: store.query_blocking()
// ✅ Always: store.query().await
```

---

## Error Handling Strategy

### Result Type

All fallible operations return `Result<T>`:

```rust
pub type Result<T> = std::result::Result<T, Error>;
```

### Error Conversions

```rust
// From HTTP errors
hyper::error -> Error::ConnectionFailed

// From serialization errors
serde_json::error -> Error::SerializationError

// From SPARQL errors
Server response with error -> Error::QueryError
```

### User Responsibility

Users **must** handle `Result`:

```rust
// ❌ Won't compile: ignoring Result
store.query("...").await;

// ✅ Must use Result
let results = store.query("...").await?;

// ✅ Or explicitly handle
match store.query("...").await {
    Ok(results) => { ... }
    Err(e) => { ... }
}
```

---

## Performance Considerations

### Latency

Request/response cycle:

```
User code
  ↓ (μs)
Store::query()
  ↓ (μs) JSON serialization
HTTP request
  ↓ (ms) Network
QLever server
  ↓ (ms) Query execution
JSON response
  ↓ (μs) JSON parsing
QuerySolution objects
  ↓ (μs) User code processes results
```

**Typical breakdown:** 2-5ms = network (1-2ms) + server execution (1-3ms)

### Memory

Streaming approach for large result sets:

```rust
// Materializes all results (high memory)
let solutions = store.query("...").await?;

// Streams one-by-one (low memory)
let mut iter = store.query_streaming("...").await?;
while let Some(solution) = iter.next().await? {
    // Process one at a time
}
```

---

## Testing Strategy

### Unit Tests

Test public API in isolation:

```rust
#[test]
fn test_query_solution_get() {
    let mut solution = QuerySolution::new();
    solution.insert("x", "http://example.org/");
    assert_eq!(solution.get("x"), Some("http://example.org/"));
}
```

### Integration Tests

Test against running QLever server:

```rust
#[tokio::test]
async fn test_query_integration() {
    let store = Store::new("http://localhost:7777").expect("Server running");
    let results = store.query("SELECT ?x WHERE { ?x ?p ?o } LIMIT 1").await?;
    assert!(!results.is_empty());
}
```

---

## Future Extensions

Potential areas for expansion:

1. **Native WASM bindings** — Direct C++ WASM instead of HTTP
2. **Streaming results** — Iterator over large result sets
3. **Update operations** — SPARQL UPDATE support
4. **Graph transactions** — BEGIN/COMMIT semantics
5. **Custom serialization** — Beyond JSON
6. **Connection pooling** — Reuse TCP connections more efficiently

---

## Comparison to Oxigraph

| Feature | QLever Rust | Oxigraph |
|---------|------------|----------|
| Storage | Remote (C++) | In-memory (Rust) |
| Scale | Billions of triples | Millions of triples |
| Latency | 2-5ms (network) | <1ms (memory) |
| Deployment | Separate server | Embedded library |
| Use case | Distributed queries | Single-process |

---

## See Also

- **[FFI & Performance](./ffi-performance.md)** — Deep dive on C++ interop
- **[Async/Await Model](./async-model.md)** — Concurrency design
- **[API Reference](../reference/api.md)** — Complete type documentation

