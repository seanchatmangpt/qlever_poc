# Explanations — Deep Understanding

Conceptual guides to understand *why* the API works this way and how the system is designed.

## Available Explanations

### [Architecture & Design](./architecture.md)

Comprehensive overview of library structure and design decisions.

**Topics:**
- Layered architecture (app → API → RDF types → FFI → C++ backend)
- Module breakdown (store, model, query, cache, ffi, error)
- Request/response flow
- Design rationale (async-first, zero-copy, thin FFI, etc.)
- Memory model and ownership
- Concurrency model
- Error handling strategy
- Performance considerations
- Testing strategy
- Future extensions
- Comparison to Oxigraph

**Best for:** Understanding overall design, making architectural decisions

---

### [FFI & Performance](./ffi-performance.md) *(Coming soon)*

Deep dive on C++ interoperability and performance.

**Topics:**
- C FFI binding details
- Memory safety across language boundary
- Zero-copy optimization strategies
- Performance profiling
- Benchmarking results
- Optimization opportunities

---

### [Async/Await Model](./async-model.md) *(Coming soon)*

Concurrency design and async patterns.

**Topics:**
- Why async/await?
- Tokio runtime integration
- Concurrent query execution
- Error propagation in async code
- Common async pitfalls and solutions

---

## When to Read Explanations

**Read when:**
- You want to understand *why* the API is designed this way
- You're debugging complex behavior
- You want to extend the library
- You're optimizing performance and need to understand internals
- You're evaluating whether QLever Rust is right for your use case

**Don't read if:**
- You just want to execute a query (→ [Getting Started](../tutorials/getting-started.md))
- You need to accomplish a specific task (→ [How-To Guides](../how-to/))
- You need API details (→ [Reference](../reference/))

---

## Architecture Quick Summary

```
User Application
       ↓
   Public API (Store, QuerySolution)
       ↓
RDF Model Types (Term, Triple, Quad)
       ↓
Query Execution (query, ask, construct, describe)
       ↓
FFI Layer (C bindings)
       ↓
C++ Backend (QLever SPARQL Engine)
```

**Key principle:** Thin, type-safe wrapper that emphasizes safety and ergonomics over performance (but still fast—2-5ms queries).

---

## Design Philosophy

The library prioritizes:

1. **Type Safety** — Rust's type system prevents RDF errors at compile time
2. **Async-First** — Non-blocking I/O with Tokio for modern apps
3. **Zero-Copy** — Share memory with C++ backend where possible
4. **Minimal Overhead** — Lightweight FFI layer
5. **Familiar API** — Similar to Oxigraph for easy adoption

---

## Common Questions Answered

### Why is everything async?

Non-blocking I/O essential for scalable applications. The Tokio runtime allows your app to handle thousands of queries concurrently without blocking threads.

👉 See [Async/Await Model](./async-model.md)

---

### How safe is it to call C++ from Rust?

Very safe. We use Rust's FFI layer with manual memory management isolated to `ffi.rs`. All public APIs are memory-safe.

👉 See [FFI & Performance](./ffi-performance.md)

---

### How fast is it compared to Oxigraph?

**Different targets:**
- **QLever Rust:** Remote queries (2-5ms latency) on billion-triple datasets
- **Oxigraph:** In-memory queries (<1ms latency) on million-triple datasets

Choose QLever if you need scale; choose Oxigraph if you need speed for small datasets.

👉 See [Architecture](./architecture.md#comparison-to-oxigraph)

---

### Why separate Store and QueryCache?

Separation of concerns. Caching is optional and orthogonal to querying. This makes performance characteristics explicit—users know when caching occurs.

👉 See [Architecture](./architecture.md#design-decisions--rationale)

---

## Reading Path

1. **New to architecture?** → Start with [Architecture Overview](./architecture.md)
2. **Curious about performance?** → Read [FFI & Performance](./ffi-performance.md) (coming soon)
3. **Want to understand concurrency?** → Read [Async/Await Model](./async-model.md) (coming soon)

---

## See Also

- **[Getting Started Tutorial](../tutorials/getting-started.md)** — Quick start
- **[How-To Guides](../how-to/)** — Practical solutions
- **[API Reference](../reference/)** — Type documentation

