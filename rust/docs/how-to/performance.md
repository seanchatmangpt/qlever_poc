# Optimize Query Performance

Practical techniques for faster queries: caching, connection reuse, and query optimization.

---

## Rule #1: Use Query Caching

Cache frequently-executed queries to avoid redundant server calls.

### Enable Caching

```rust
use qlever::cache::QueryCache;
use std::time::Duration;

// Create cache: 100 entries, 5-minute TTL
let cache = QueryCache::new(100, Duration::from_secs(300));

// Execute with caching
let results = cache.query_with_cache(
    &store,
    "SELECT ?name WHERE { ?person <http://example.org/name> ?name }"
).await?;
```

### Check Cache Statistics

```rust
let stats = cache.stats();
println!("Hits: {}", stats.hits);
println!("Misses: {}", stats.misses);
println!("Hit rate: {:.2}%", stats.hit_rate() * 100.0);
```

### Tune Cache Size & TTL

```rust
// Small cache, short TTL (real-time data)
let cache = QueryCache::new(10, Duration::from_secs(30));

// Large cache, long TTL (stable data)
let cache = QueryCache::new(500, Duration::from_secs(3600));

// Gigantic cache, never expires
let cache = QueryCache::new(10000, Duration::MAX);
```

---

## Rule #2: Write Efficient SPARQL

The server does most work. Write specific queries.

### ❌ Bad: Too Broad

```sparql
-- Retrieves everything
SELECT ?s ?p ?o WHERE { ?s ?p ?o }
```

### ✅ Good: Specific Types

```sparql
-- Only people with ages
SELECT ?name ?age
WHERE {
    ?person a <http://example.org/Person> ;
            <http://example.org/name> ?name ;
            <http://example.org/age> ?age .
}
```

### ✅ Good: Explicit Filters

```sparql
-- Filter server-side, not client-side
SELECT ?name ?age
WHERE {
    ?person <http://example.org/name> ?name ;
            <http://example.org/age> ?age .
    FILTER (?age > 18 && ?age < 65)
}
```

### ✅ Good: Limit Results

```sparql
-- Always limit in production
SELECT ?name
WHERE {
    ?person <http://example.org/name> ?name .
}
LIMIT 1000
```

---

## Rule #3: Reuse Store Connection

Keep one `Store` instance, don't reconnect for each query.

### ❌ Bad: Reconnect Each Time

```rust
async fn query_many() -> Result<()> {
    for i in 0..1000 {
        let store = Store::new("http://localhost:7777")?; // ❌ Reconnects!
        store.query("SELECT ?x WHERE { ?x ?p ?o } LIMIT 1").await?;
    }
    Ok(())
}
```

### ✅ Good: Reuse Single Connection

```rust
async fn query_many(store: &Store) -> Result<()> {
    for i in 0..1000 {
        store.query("SELECT ?x WHERE { ?x ?p ?o } LIMIT 1").await?; // ✅ Reuse
    }
    Ok(())
}

#[tokio::main]
async fn main() -> Result<()> {
    let store = Store::new("http://localhost:7777")?;
    query_many(&store).await?;
    Ok(())
}
```

---

## Rule #4: Use Batch Queries

Execute multiple queries concurrently with `tokio::join!` or `tokio::spawn_blocking`.

### Concurrent Execution

```rust
use tokio::join;

#[tokio::main]
async fn main() -> Result<()> {
    let store = Store::new("http://localhost:7777")?;

    // Execute 3 queries concurrently
    let (people, places, things) = join!(
        store.query("SELECT ?x WHERE { ?x a <http://example.org/Person> } LIMIT 100"),
        store.query("SELECT ?x WHERE { ?x a <http://example.org/Place> } LIMIT 100"),
        store.query("SELECT ?x WHERE { ?x a <http://example.org/Thing> } LIMIT 100")
    );

    println!("People: {}", people?.len());
    println!("Places: {}", places?.len());
    println!("Things: {}", things?.len());

    Ok(())
}
```

### Parallel with Futures

```rust
use futures::future::join_all;

async fn query_all_types(store: &Store) -> Result<Vec<Vec<QuerySolution>>> {
    let types = vec![
        "Person",
        "Place",
        "Organization",
        "Event",
    ];

    let futures = types.iter().map(|t| async move {
        store.query(&format!(
            "SELECT ?x WHERE {{ ?x a <http://example.org/{}> }}",
            t
        )).await
    });

    let results = join_all(futures).await;
    results.into_iter().collect()
}
```

---

## Rule #5: Stream Large Result Sets

For millions of results, don't load everything into memory.

### Use Streaming Iterator

```rust
use qlever::streaming::ResultIterator;

let mut results = store.query_streaming(
    "SELECT ?x WHERE { ?x ?p ?o } LIMIT 1000000"
).await?;

while let Some(solution) = results.next().await? {
    println!("{:?}", solution);
    // Process one result at a time
}
```

### Chunked Processing

```rust
use qlever::streaming::ChunkedResultIterator;

let mut results = store.query_chunked(
    "SELECT ?x ?p ?o WHERE { ?x ?p ?o }",
    1000  // Process 1000 results at a time
).await?;

while let Some(chunk) = results.next_chunk().await? {
    for solution in chunk {
        // Process chunk
    }
}
```

---

## Rule #6: Use ASK for Existence Checks

`ASK` is much faster than `SELECT` for true/false answers.

### ❌ Bad: COUNT

```rust
// Counts all results (slow!)
let solutions = store
    .query("SELECT (COUNT(*) AS ?count) WHERE { ?x ?p ?o }")
    .await?;

let count = solutions[0].get("count").unwrap();
if count == "0" {
    println!("Empty dataset");
}
```

### ✅ Good: ASK

```rust
// Returns true/false immediately (fast!)
let has_data = store.ask("ASK { ?x ?p ?o }").await?;
if has_data {
    println!("Dataset has data");
}
```

---

## Benchmarking Queries

Measure performance to identify bottlenecks.

```rust
use std::time::Instant;

#[tokio::main]
async fn main() -> Result<()> {
    let store = Store::new("http://localhost:7777")?;

    // Benchmark a query
    let start = Instant::now();
    let results = store
        .query("SELECT ?x WHERE { ?x a <http://example.org/Person> } LIMIT 1000")
        .await?;
    let elapsed = start.elapsed();

    println!("Results: {}", results.len());
    println!("Time: {:.2}ms", elapsed.as_secs_f64() * 1000.0);
    println!("Per-result: {:.3}ms",
        (elapsed.as_secs_f64() / results.len() as f64) * 1000.0);

    Ok(())
}
```

---

## Common Performance Bottlenecks

| Issue | Symptom | Solution |
|-------|---------|----------|
| No LIMIT | Slow queries | Add `LIMIT 100` |
| Broad patterns | Server does lots of work | Be specific: `?x a Type` |
| Repeated queries | Many server requests | Enable caching |
| Large results | Memory explosion | Use streaming |
| Many small queries | Network overhead | Batch with `join!` |
| Reconnecting | Connection setup cost | Reuse `Store` |

---

## Optimization Checklist

When a query is slow:

- [ ] Add `LIMIT` clause?
- [ ] Make patterns more specific (add `a` / type filters)?
- [ ] Move filters to SPARQL (`FILTER` in WHERE, not in Rust)?
- [ ] Enable caching for repeated queries?
- [ ] Use `ASK` instead of `SELECT COUNT`?
- [ ] Batch related queries with `join!`?
- [ ] Use streaming for large result sets?
- [ ] Check server load (many concurrent queries)?

---

## Caching Best Practices

```rust
// 1. Create cache once
let cache = QueryCache::new(500, Duration::from_secs(600));

// 2. Reuse for all queries
let results1 = cache.query_with_cache(&store, query1).await?;
let results2 = cache.query_with_cache(&store, query2).await?;

// 3. Monitor effectiveness
let stats = cache.stats();
if stats.hit_rate() < 0.5 {
    println!("Cache hit rate low, consider larger cache");
}

// 4. Clear if data changes
// (Cache doesn't auto-invalidate; you manage it)
// For now, create new cache instance or wait for TTL
```

---

## Production Settings

For a production app querying a billion-triple dataset:

```rust
use std::time::Duration;

// Connection pool (reuse single Store)
let store = Store::new("http://localhost:7777")?;

// Aggressive caching (500 entries, 10-minute TTL)
let cache = QueryCache::new(500, Duration::from_secs(600));

// Always use LIMIT in production
const DEFAULT_LIMIT: usize = 1000;
const MAX_LIMIT: usize = 10000;

async fn production_query(
    cache: &QueryCache,
    store: &Store,
    sparql: &str,
) -> Result<Vec<QuerySolution>> {
    // Always add LIMIT if not present
    let limited_query = if !sparql.to_uppercase().contains("LIMIT") {
        format!("{} LIMIT {}", sparql, DEFAULT_LIMIT)
    } else {
        sparql.to_string()
    };

    cache.query_with_cache(store, &limited_query).await
}
```

---

## Monitoring

Track performance in production:

```rust
struct QueryMetrics {
    query: String,
    duration_ms: f64,
    result_count: usize,
}

async fn execute_with_metrics(
    store: &Store,
    query: &str,
) -> Result<(Vec<QuerySolution>, QueryMetrics)> {
    let start = Instant::now();
    let results = store.query(query).await?;
    let count = results.len();
    let elapsed = start.elapsed();

    let metrics = QueryMetrics {
        query: query.to_string(),
        duration_ms: elapsed.as_secs_f64() * 1000.0,
        result_count: count,
    };

    // Log, send to monitoring service, etc.
    println!("Query: {} | Results: {} | Time: {:.2}ms",
        metrics.query, metrics.result_count, metrics.duration_ms);

    Ok((results, metrics))
}
```

---

## Next Steps

- **Deep dive on FFI performance**: See [FFI & Performance Explanation](../explanations/ffi-performance.md)
- **Error handling during failures**: See [Error Handling Guide](./error-handling.md)
- **API reference**: See [API Reference](../reference/api.md)

---

## Resources

- [SPARQL 1.1 Query Optimization](https://www.w3.org/TR/sparql11-query/#optimizationConsiderations)
- [QLever Performance Tuning](https://github.com/ad-freiburg/qlever#performance)
- [Tokio Concurrency Patterns](https://tokio.rs/tokio/concepts)

