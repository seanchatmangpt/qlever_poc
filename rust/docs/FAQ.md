# Frequently Asked Questions (FAQ)

Quick answers to common questions about QLever Rust.

---

## Installation & Setup

### Q: Do I need a running QLever server?

**A:** Yes. QLever Rust is a client library that connects to a separate C++ server.

To start a server:
```bash
docker run -d -p 7777:7777 qlever:latest
curl http://localhost:7777/api/info  # Verify it's running
```

### Q: Can I use QLever Rust without Docker?

**A:** Yes. You can compile and run QLever C++ server from source, but Docker is easier.

See: https://github.com/ad-freiburg/qlever

### Q: What's the minimum Rust version?

**A:** Rust 1.70 or newer.

Check your version:
```bash
rustc --version
```

Update if needed:
```bash
rustup update
```

### Q: Does QLever Rust work on Windows/Mac/Linux?

**A:** Yes. It works on all platforms where Rust and QLever C++ can compile.

Supported:
- ✅ Linux (primary target)
- ✅ macOS (tested)
- ✅ Windows (tested with WSL2)

---

## Usage

### Q: How do I execute a query?

**A:** Use the `Store` to connect and query:

```rust
let store = Store::new("http://localhost:7777")?;
let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
```

👉 See: [Getting Started](./tutorials/getting-started.md)

### Q: What query types are supported?

**A:** All SPARQL 1.1 query types:
- **SELECT** — Return variable values
- **ASK** — True/false answers
- **CONSTRUCT** — Build new triples
- **DESCRIBE** — Get resource details

👉 See: [Query Types Guide](./how-to/query-types.md)

### Q: How do I handle large result sets?

**A:** Use streaming APIs instead of loading everything into memory:

```rust
let mut results = store.query_streaming("SELECT ?x WHERE { ?x ?p ?o }").await?;
while let Some(solution) = results.next().await? {
    // Process one result at a time
}
```

👉 See: [Performance Guide](./how-to/performance.md)

### Q: Can I use QLever Rust in a web application?

**A:** Yes! It works with all Rust web frameworks (Axum, Actix, Rocket, etc.).

👉 See: [Search App Tutorial](./tutorials/search-app.md)

### Q: How do I cache query results?

**A:** Use `QueryCache` to avoid redundant server calls:

```rust
let cache = QueryCache::new(100, Duration::from_secs(300));
let results = cache.query_with_cache(&store, "SELECT ...").await?;
```

👉 See: [Performance Guide](./how-to/performance.md)

---

## Performance

### Q: Why are my queries slow?

**A:** Common causes:

1. **No LIMIT** — Add `LIMIT 10` to limit results
2. **Broad patterns** — Make SPARQL more specific
3. **No caching** — Enable `QueryCache` for repeated queries
4. **Network latency** — Batch queries with `tokio::join!`

👉 See: [Performance Guide](./how-to/performance.md)

### Q: How fast is QLever Rust?

**A:** Typical latency: **2-5 milliseconds per query**

Breakdown:
- Network: 1-2 ms
- Server processing: 1-3 ms

Result throughput: 1000+ queries/sec on modern hardware

### Q: How much memory does it use?

**A:** Depends on result size:
- Small results (100 rows): <1 MB
- Large results (1M rows): 100+ MB

**Solution:** Use `.query_streaming()` to keep memory constant

### Q: Can I run multiple queries in parallel?

**A:** Yes! Use `tokio::join!` for concurrent execution:

```rust
let (r1, r2, r3) = tokio::join!(
    store.query("..."),
    store.query("..."),
    store.query("...")
);
```

This runs 3x faster than sequential.

---

## API & Types

### Q: How do I work with RDF data?

**A:** QLever Rust provides RDF types:

```rust
let node = NamedNode::new("http://example.org/alice")?;
let literal = Literal::new_simple("hello");
let triple = Triple::new(subject, predicate, object);
```

👉 See: [RDF Data Guide](./how-to/rdf-data.md)

### Q: How do I get values from query results?

**A:** Use `.get()` on `QuerySolution`:

```rust
for solution in results {
    if let Some(name) = solution.get("name") {
        println!("Name: {}", name);
    }
}
```

### Q: What error types can I get?

**A:** 5 main error types:

- `QueryError` — SPARQL syntax/execution error
- `InvalidUrl` — Bad server URL
- `ConnectionFailed` — Network error
- `SerializationError` — Invalid response
- `IoError` — File I/O error

👉 See: [Error Handling Guide](./how-to/error-handling.md)

---

## Integration

### Q: How do I use QLever Rust in production?

**A:** Follow these practices:

1. **Use persistent Store connection** — Don't reconnect
2. **Enable caching** — Use `QueryCache`
3. **Batch queries** — Use `tokio::join!`
4. **Stream large results** — Avoid loading all results
5. **Add monitoring** — Track performance
6. **Handle errors** — Don't use `.unwrap()`

👉 See: [Performance Guide](./how-to/performance.md)

### Q: Can I use it with async web frameworks?

**A:** Yes! Works with all Rust async frameworks:
- ✅ Axum
- ✅ Actix-web
- ✅ Rocket
- ✅ Warp
- ✅ Tide

👉 See: [Search App Tutorial](./tutorials/search-app.md)

### Q: How do I test my code using QLever?

**A:** Use test mode with `#[tokio::test]`:

```rust
#[tokio::test]
async fn test_query() {
    let store = Store::new("http://localhost:7777").unwrap();
    let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await.unwrap();
    assert!(!results.is_empty());
}
```

### Q: Can I use it with ORMs?

**A:** Not directly (QLever Rust is not an ORM). But you can:
- Use it alongside an ORM for relational + RDF data
- Use it to populate ORM models from RDF queries
- Build a custom mapping layer

---

## Troubleshooting

### Q: "Connection refused" error

**A:** Server not running. Start it:
```bash
docker run -d -p 7777:7777 qlever:latest
curl http://localhost:7777/api/info  # Verify
```

### Q: "Invalid SPARQL" error

**A:** Check your SPARQL syntax at https://sparql.org/query-validator

Common issues:
- Missing `WHERE` clause
- Invalid variable names (must start with `?`)
- Misspelled IRIs

### Q: "No results" but query is valid

**A:** Your pattern might not match data. Debug:

```sparql
SELECT ?x ?p ?o WHERE { ?x ?p ?o } LIMIT 1
```

This shows any triple. If you get results, your data exists.

### Q: Compilation fails

**A:** Ensure Rust 1.70+:
```bash
rustc --version
rustup update  # Update if needed
```

Then rebuild:
```bash
cargo clean
cargo build
```

### Q: Memory usage keeps growing

**A:** You're likely materializing large result sets. Use streaming:

```rust
// ❌ Bad: Loads all results
let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;

// ✅ Good: Streams results
let mut results = store.query_streaming("SELECT ?x WHERE { ?x ?p ?o }").await?;
while let Some(solution) = results.next().await? {
    // Process one at a time
}
```

---

## Documentation

### Q: Which guide should I read first?

**A:** It depends on your background:

- **New to Rust?** → [Rust Book](https://doc.rust-lang.org/book/) first
- **New to SPARQL?** → [SPARQL Tutorial](https://www.w3.org/2009/sparql/wiki/Main_Page)
- **New to QLever?** → [Getting Started](./tutorials/getting-started.md)
- **In a hurry?** → [Quick Reference](./QUICKREF.md)
- **Know what you want?** → [Learning Path](./LEARNING_PATH.md)

### Q: I need to solve a specific problem

**A:** Use [Learning Path](./LEARNING_PATH.md) to find the right guide:

"I want to..." → "Read..."

Or jump to [How-To Guides](./how-to/)

### Q: I want to understand the design

**A:** Read [Architecture Guide](./explanations/architecture.md)

Then optional deep-dives:
- [FFI & Performance](./explanations/ffi-performance.md)
- [Async/Await Model](./explanations/async-model.md)

---

## Contributing

### Q: Can I contribute?

**A:** Yes! The project welcomes contributions.

See: [CONTRIBUTING.md](../../CONTRIBUTING.md)

### Q: Where do I report bugs?

**A:** Open an issue on GitHub:
https://github.com/seanchatmangpt/qlever/issues/new

Include:
- Rust version (`rustc --version`)
- Minimal reproducible example
- Error message and stack trace

### Q: How do I request a feature?

**A:** Open a GitHub issue with:
- Use case (why you need it)
- Proposed API
- Examples

---

## More Help

- **[Quick Reference](./QUICKREF.md)** — Copy-paste code snippets
- **[Learning Path](./LEARNING_PATH.md)** — Personalized routing
- **[API Reference](./reference/api.md)** — Complete type documentation
- **[Error Handling](./how-to/error-handling.md)** — Error patterns
- **[GitHub Issues](https://github.com/seanchatmangpt/qlever/issues)** — Ask the community
- **[QLever Repository](https://github.com/ad-freiburg/qlever)** — Main project

---

**Still have questions?** Open an issue or ask in [GitHub Discussions](https://github.com/seanchatmangpt/qlever/discussions)

**Last Updated:** 2025-01-01 | **Status:** Community maintained
