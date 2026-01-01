# Handle Errors Gracefully

Practical patterns for error handling and recovery.

> ⏱️ **15 minutes** | 🎯 **Intermediate** | 📦 **Error Types**

---

## TL;DR {#tldr}

```rust
use qlever::error::Error;

// Always handle Result types
match store.query("SELECT ?x WHERE { ?x ?p ?o }").await {
    Ok(solutions) => println!("Got {} results", solutions.len()),
    Err(Error::QueryError(msg)) => eprintln!("SPARQL syntax error: {}", msg),
    Err(Error::ConnectionFailed(msg)) => eprintln!("Network error: {}", msg),
    Err(e) => eprintln!("Other error: {}", e),
}

// Or use ? operator for simpler code
let solutions = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
```

---

## Error Types

All operations return `Result<T>` where error is one of:

```rust
pub enum Error {
    QueryError(String),           // SPARQL syntax or execution error
    InvalidUrl(String),           // Bad server URL
    ConnectionFailed(String),     // Network/server error
    SerializationError(String),   // Invalid response format
    IoError(String),              // File I/O error
}
```

---

## Common Errors & Solutions

### "Connection refused"

```rust
// ❌ Error: Connection refused
// Cause: Server not running

// ✅ Solution: Start server
// docker run -d -p 7777:7777 qlever:latest

// Verify with:
// curl http://localhost:7777/api/info
```

### "Invalid SPARQL"

```rust
// ❌ Error: QueryError("Parse error...")
// Cause: Syntax error in SPARQL

// ✅ Solution: Check SPARQL syntax
// Use: https://sparql.org/query-validator
// Validate query before sending to server

let query = "SELECT ?x WHERE { ?x ?p ?o }";  // ✓ Valid
```

### "No results"

```rust
// ❌ No results returned
// Cause: Pattern doesn't match data

// ✅ Solution: Debug with broader query
let all = store.query("SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 1").await?;

// Or use ASK to check if data exists
let has_data = store.ask("ASK { ?x ?p ?o }").await?;
```

### "Results too large"

```rust
// ❌ Memory error with huge result sets
// Cause: Trying to load everything at once

// ✅ Solution: Use streaming
let mut results = store.query_streaming("SELECT ?x WHERE { ?x ?p ?o }").await?;
while let Some(solution) = results.next().await? {
    println!("{:?}", solution);  // Process one at a time
}

// Or use chunked iteration
let mut results = store.query_chunked("SELECT ?x WHERE { ?x ?p ?o }", 1000).await?;
while let Some(chunk) = results.next_chunk().await? {
    for solution in chunk {
        // process chunk of 1000
    }
}
```

---

## Error Handling Patterns

### Pattern 1: Match on Error Type

```rust
match store.query("...").await {
    Ok(results) => {
        // Handle success
    }
    Err(Error::QueryError(msg)) => {
        // Handle SPARQL syntax errors
        eprintln!("SPARQL error: {}", msg);
    }
    Err(Error::ConnectionFailed(msg)) => {
        // Handle network errors (retry logic?)
        eprintln!("Connection error: {}", msg);
        // Could retry with exponential backoff
    }
    Err(e) => {
        // Handle other errors
        eprintln!("Unexpected error: {}", e);
    }
}
```

### Pattern 2: ? Operator (Early Return)

```rust
async fn query_users(store: &Store) -> Result<Vec<QuerySolution>> {
    let results = store.query("SELECT ?name WHERE { ?x foaf:name ?name }").await?;
    Ok(results)
}

// In async main:
#[tokio::main]
async fn main() -> Result<()> {
    let store = Store::new("http://localhost:7777")?;
    let results = query_users(&store).await?;
    println!("{:?}", results);
    Ok(())
}
```

### Pattern 3: Custom Error Context

```rust
use qlever::error::Error;

async fn fetch_and_validate(store: &Store) -> Result<Vec<QuerySolution>> {
    let results = store
        .query("SELECT ?x WHERE { ?x a foaf:Person }")
        .await
        .map_err(|e| {
            // Add context to error
            Error::QueryError(format!("Failed to fetch persons: {}", e))
        })?;

    if results.is_empty() {
        return Err(Error::QueryError("No persons found".to_string()));
    }

    Ok(results)
}
```

### Pattern 4: Retry with Backoff

```rust
use std::time::Duration;

async fn query_with_retry(
    store: &Store,
    sparql: &str,
    max_retries: u32,
) -> Result<Vec<QuerySolution>> {
    for attempt in 0..max_retries {
        match store.query(sparql).await {
            Ok(results) => return Ok(results),
            Err(Error::ConnectionFailed(_)) if attempt < max_retries - 1 => {
                // Retry on connection errors
                let wait = Duration::from_millis(100 * 2u64.pow(attempt));
                tokio::time::sleep(wait).await;
                continue;
            }
            Err(e) => return Err(e),
        }
    }
    Err(Error::ConnectionFailed("Max retries exceeded".to_string()))
}
```

---

## Best Practices

### ✅ DO

- **Handle errors explicitly** — Don't use `.unwrap()` in production
- **Log errors** — Include context for debugging
- **Retry on network errors** — Connection issues are often temporary
- **Validate early** — Check SPARQL syntax before sending
- **Use streaming** — For potentially large result sets
- **Add timeouts** — Prevent hanging on slow networks

### ❌ DON'T

- **Ignore errors** — Don't use `.unwrap()` or `.expect()`
- **Retry on syntax errors** — If SPARQL is invalid, it won't magically fix itself
- **Log sensitive data** — Don't log connection strings or credentials
- **Block async tasks** — Don't use `.unwrap()` in async context
- **Silently fail** — Always inform user of errors

---

## Troubleshooting Guide

| Error | Meaning | Solution |
|-------|---------|----------|
| `QueryError("...")` | SPARQL syntax or execution error | Check SPARQL syntax, validate at https://sparql.org/query-validator |
| `InvalidUrl(...)` | Server URL is invalid | Use format: `http://localhost:7777` |
| `ConnectionFailed(...)` | Network/server unreachable | Ensure server running: `curl http://localhost:7777/api/info` |
| `SerializationError(...)` | Invalid JSON response | May indicate server issue; check server logs |
| `IoError(...)` | File I/O problem | Check file permissions and paths |

---

## See Also

- **[Query Types Guide](./query-types.md)** — Learn SPARQL patterns
- **[Performance Guide](./performance.md)** — Handle large result sets
- **[API Reference](../reference/api.md)** — Complete error types
- **[Quick Reference](../QUICKREF.md)** — Copy-paste patterns

---

**Status:** Reference guide | **Last Updated:** 2025-01-01
