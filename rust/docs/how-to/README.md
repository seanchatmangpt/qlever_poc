# How-To Guides — Practical Solutions

Solution-focused guides for specific tasks. Use when you know what you want to accomplish.

## Available Guides

### [Execute Different Query Types](./query-types.md)

Learn to write SELECT, ASK, CONSTRUCT, and DESCRIBE queries.

**Topics:**
- SELECT queries (basic, filtering, ordering, LIMIT)
- ASK queries (existence checks, true/false answers)
- CONSTRUCT queries (build new triples)
- DESCRIBE queries (get resource details)
- Advanced patterns (aggregation, UNION, property paths)
- Error handling

**Time:** ~20 minutes

---

### [Optimize Query Performance](./performance.md) 🚀

Practical techniques to make queries faster.

**Topics:**
- Query caching (LRU with TTL)
- Write efficient SPARQL
- Reuse store connections
- Batch queries with concurrent execution
- Stream large result sets
- Use ASK for existence checks
- Performance monitoring
- Production settings

**Time:** ~25 minutes

---

### [Parse & Manipulate RDF Data](./rdf-data.md) *(Coming soon)*

Work with RDF terms, triples, and quads.

---

### [Handle Errors Gracefully](./error-handling.md) *(Coming soon)*

Error types and recovery strategies.

---

## Quick Reference

| Task | Guide |
|------|-------|
| Write a SPARQL query | [Query Types](./query-types.md) |
| Make queries faster | [Performance](./performance.md) |
| Cache results | [Performance](./performance.md) |
| Work with RDF data | [RDF Data](./rdf-data.md) (coming soon) |
| Handle errors | [Error Handling](./error-handling.md) (coming soon) |

---

## Choosing a Guide

**Know what you want to do?** → Find it here
**Just starting?** → Try [Getting Started Tutorial](../tutorials/)
**Need complete API?** → Check [Reference](../reference/)
**Want to understand design?** → Read [Explanations](../explanations/)

---

## Common Patterns

### 1. Execute Query & Iterate Results

```rust
let store = Store::new("http://localhost:7777")?;
let solutions = store
    .query("SELECT ?name WHERE { ?x <http://example.org/name> ?name }")
    .await?;

for solution in solutions {
    if let Some(name) = solution.get("name") {
        println!("{}", name);
    }
}
```

👉 See [Query Types](./query-types.md) for variants.

---

### 2. Cache Repeated Queries

```rust
use qlever::cache::QueryCache;
use std::time::Duration;

let cache = QueryCache::new(100, Duration::from_secs(300));
let results = cache.query_with_cache(&store, "SELECT ?x WHERE { ... }").await?;
```

👉 See [Performance](./performance.md) for details.

---

### 3. Handle Errors

```rust
use qlever::error::Error;

match store.query("SELECT ?x WHERE { ... }").await {
    Ok(solutions) => { /* process */ }
    Err(Error::QueryError(msg)) => eprintln!("SPARQL: {}", msg),
    Err(e) => eprintln!("Error: {}", e),
}
```

👉 See [Error Handling](./error-handling.md) (coming soon).

---

## Still Stuck?

1. Check [FAQ in Troubleshooting](../reference/README.md#troubleshooting)
2. Search [QLever GitHub Issues](https://github.com/ad-freiburg/qlever/issues)
3. Open a [new issue](https://github.com/seanchatmangpt/qlever/issues)

