# Async/Await Model

Understanding QLever Rust's asynchronous design and concurrency patterns.

> ⏱️ **20 minutes** | 🎯 **Intermediate** | 📦 **Tokio, Async**

---

## Why Async/Await?

All QLever Rust operations are asynchronous (`.await`). This enables:

1. **Non-blocking I/O** — Thread doesn't wait for network
2. **Concurrency** — Thousands of concurrent queries per thread
3. **Efficiency** — Small thread pool serves many concurrent operations
4. **Scalability** — Handle many users efficiently

---

## Basic Async Pattern

```rust
#[tokio::main]  // Tokio runtime
async fn main() -> Result<()> {
    let store = Store::new("http://localhost:7777")?;

    // Must use .await on async operations
    let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;

    println!("{:?}", results);
    Ok(())
}
```

**Key insight:** `async fn` returns a Future; `.await` executes it

---

## Async vs Sync Comparison

### Blocking (Old Way)

```rust
// ❌ Blocks thread while waiting for network
fn main() {
    let results = store.query("SELECT ?x WHERE { ?x ?p ?o }");  // Blocks here!
    println!("{:?}", results);
}
```

**Problem:** Thread is idle during network I/O (wastes resources)

### Non-Blocking (New Way)

```rust
// ✅ Thread can do other work while waiting for network
#[tokio::main]
async fn main() {
    let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await;
    println!("{:?}", results);
}
```

**Benefit:** Thread free to serve other requests

---

## Concurrent Queries

Execute multiple queries in parallel:

```rust
#[tokio::main]
async fn main() -> Result<()> {
    let store = Store::new("http://localhost:7777")?;

    // Execute 3 queries concurrently
    let (people, places, things) = tokio::join!(
        store.query("SELECT ?x WHERE { ?x a foaf:Person }"),
        store.query("SELECT ?x WHERE { ?x a foaf:Place }"),
        store.query("SELECT ?x WHERE { ?x a foaf:Thing }")
    );

    println!("People: {}", people?.len());
    println!("Places: {}", places?.len());
    println!("Things: {}", things?.len());

    Ok(())
}
```

**Benefit:** All 3 queries run in parallel, total time ≈ 1 query time (not 3x)

---

## Spawning Tasks

Run async code in background:

```rust
#[tokio::main]
async fn main() -> Result<()> {
    let store = Arc::new(Store::new("http://localhost:7777")?);

    // Spawn background task
    let store_clone = store.clone();
    let task = tokio::spawn(async move {
        let results = store_clone.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
        println!("Background query done: {} results", results.len());
        Ok::<(), Box<dyn std::error::Error>>(())
    });

    // Meanwhile, main continues
    println!("Task spawned");

    // Wait for task
    task.await??;

    Ok(())
}
```

---

## Async Functions

Functions can be async too:

```rust
// Async function must return Result
async fn fetch_people(store: &Store) -> Result<Vec<QuerySolution>> {
    store.query("SELECT ?person WHERE { ?person a foaf:Person }").await
}

async fn fetch_places(store: &Store) -> Result<Vec<QuerySolution>> {
    store.query("SELECT ?place WHERE { ?place a foaf:Place }").await
}

#[tokio::main]
async fn main() -> Result<()> {
    let store = Store::new("http://localhost:7777")?;

    // Call async functions
    let people = fetch_people(&store).await?;
    let places = fetch_places(&store).await?;

    println!("People: {}, Places: {}", people.len(), places.len());
    Ok(())
}
```

---

## Error Handling in Async

Same error handling as sync, with `.await`:

```rust
// Using ? operator
async fn safe_query(store: &Store) -> Result<Vec<QuerySolution>> {
    let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
    Ok(results)
}

// Explicit error handling
async fn query_with_fallback(store: &Store) -> Result<Vec<QuerySolution>> {
    match store.query("SELECT ?x WHERE { ?x ?p ?o }").await {
        Ok(results) => Ok(results),
        Err(_) => {
            // Fallback query
            store.query("SELECT ?x WHERE { ?x ?p ?o } LIMIT 1").await
        }
    }
}
```

---

## Common Async Patterns

### Pattern 1: Concurrent Batch Queries

```rust
async fn batch_queries(store: &Store) -> Result<(Vec<_>, Vec<_>, Vec<_>)> {
    let (q1, q2, q3) = tokio::join!(
        store.query("SELECT ?x WHERE { ?x a foaf:Person }"),
        store.query("SELECT ?x WHERE { ?x a foaf:Place }"),
        store.query("SELECT ?x WHERE { ?x a foaf:Thing }")
    );

    Ok((q1?, q2?, q3?))
}
```

### Pattern 2: Streaming Results

```rust
async fn process_large_result(store: &Store) -> Result<()> {
    let mut results = store.query_streaming("SELECT ?x WHERE { ?x ?p ?o }").await?;

    while let Some(solution) = results.next().await? {
        // Process one result at a time
        println!("{:?}", solution);
    }

    Ok(())
}
```

### Pattern 3: Parallel Map

```rust
async fn fetch_all_types(store: Arc<Store>, types: Vec<String>) -> Result<Vec<Vec<QuerySolution>>> {
    let futures = types.into_iter().map(|t| {
        let store = store.clone();
        async move {
            store.query(&format!(
                "SELECT ?x WHERE {{ ?x a <http://example.org/{}> }}",
                t
            )).await
        }
    });

    let results: Vec<_> = futures::future::try_join_all(futures).await?;
    Ok(results)
}
```

### Pattern 4: Retry with Backoff

```rust
async fn query_with_retry(
    store: &Store,
    query: &str,
    max_retries: u32,
) -> Result<Vec<QuerySolution>> {
    for attempt in 0..max_retries {
        match store.query(query).await {
            Ok(results) => return Ok(results),
            Err(Error::ConnectionFailed(_)) if attempt < max_retries - 1 => {
                let delay = Duration::from_millis(100 * 2u64.pow(attempt));
                tokio::time::sleep(delay).await;
                continue;
            }
            Err(e) => return Err(e),
        }
    }
    Err(Error::ConnectionFailed("Max retries exceeded".to_string()))
}
```

---

## Common Pitfalls

### ❌ Forgetting .await

```rust
// ❌ Compiles but does nothing
store.query("SELECT ?x WHERE { ?x ?p ?o }");  // Returns Future, not executed!

// ✅ Actually executes
store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
```

### ❌ Using Sync Code in Async

```rust
// ❌ Blocks the entire runtime
let result = std::thread::sleep(Duration::from_secs(1));  // Bad!

// ✅ Use async sleep
tokio::time::sleep(Duration::from_secs(1)).await;
```

### ❌ Not Sharing Arc<Store>

```rust
// ❌ Each task gets own copy
for _ in 0..10 {
    tokio::spawn(async {
        let store = Store::new("http://localhost:7777")?;  // Creates 10 stores!
    });
}

// ✅ Share single instance
let store = Arc::new(Store::new("http://localhost:7777")?);
for _ in 0..10 {
    let store = store.clone();
    tokio::spawn(async move {
        let results = store.query("...").await?;
    });
}
```

---

## Performance Implications

### Async Advantage: Concurrency

```
Sync (3 sequential queries):
Query 1: [===] (5 ms)
Query 2:      [===] (5 ms)
Query 3:           [===] (5 ms)
Total: 15 ms, 1 thread blocked

Async (3 concurrent queries):
Query 1: [===]
Query 2: [===]
Query 3: [===]
Total: 5 ms, 1 thread free (can do other work!)
```

### Throughput

```
Sync: 1 query per thread
Async: 1000+ queries per thread
```

---

## Tokio Runtime

QLever Rust requires Tokio async runtime:

```rust
// Automatic runtime setup
#[tokio::main]
async fn main() {
    // Runtime automatically created and managed
}

// Or explicit
#[tokio::main(flavor = "multi_thread", worker_threads = 4)]
async fn main() {
    // Custom runtime configuration
}
```

---

## Integration with Web Frameworks

All Rust web frameworks use Tokio:

```rust
// Actix
#[actix_web::main]
async fn main() {
    let store = Store::new("http://localhost:7777")?;
    // Works seamlessly
}

// Axum
#[tokio::main]
async fn main() {
    let store = Store::new("http://localhost:7777")?;
    // Works seamlessly
}

// Rocket (async-aware)
#[launch]
fn rocket() {
    let store = Store::new("http://localhost:7777")?;
    // Works seamlessly
}
```

---

## Best Practices

### ✅ DO

- **Use `.await` on async operations** — Don't forget it
- **Share Store via Arc** — Reuse single instance
- **Batch related queries** — Run concurrently with `join!`
- **Stream large results** — Use `.query_streaming()`
- **Use async errors** — Handle with `?` operator

### ❌ DON'T

- **Block the async runtime** — Never use `std::thread::sleep`
- **Create new Stores per query** — Reuse via Arc
- **Use `.unwrap()` in async** — Errors can happen anywhere
- **Mix blocking and async** — Keep them separate

---

## See Also

- **[Performance Guide](../how-to/performance.md)** — Concurrency patterns
- **[Architecture Guide](./architecture.md)** — Async design
- **[Tokio Tutorial](https://tokio.rs/)** — Async Rust runtime
- **[Quick Reference](../QUICKREF.md)** — Async code snippets

---

**Status:** Deep-dive guide | **Last Updated:** 2025-01-01
