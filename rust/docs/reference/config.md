# Configuration Reference

Store setup options and configuration parameters.

---

## StoreConfig

Configure store behavior at creation time.

```rust
use qlever::store::StoreConfig;
use std::time::Duration;

// Create configuration
let config = StoreConfig::new("http://localhost:7777")
    .with_timeout(Duration::from_secs(30));

// Build store from config
let store = Store::with_config(config)?;
```

---

## Configuration Options

### URL (Required)

Server URL in format: `http://host:port`

```rust
let config = StoreConfig::new("http://localhost:7777");
let config = StoreConfig::new("http://example.org:9999");
let config = StoreConfig::new("http://192.168.1.100:7777");
```

---

### Timeout (Optional)

Request timeout duration. Default: 30 seconds.

```rust
use std::time::Duration;

let config = StoreConfig::new("http://localhost:7777")
    .with_timeout(Duration::from_secs(60));  // 1 minute

// Short timeout (quick fail)
.with_timeout(Duration::from_secs(5))

// Long timeout (slow queries)
.with_timeout(Duration::from_secs(300))
```

---

## Best Practices

### Development

```rust
let config = StoreConfig::new("http://localhost:7777")
    .with_timeout(Duration::from_secs(30));  // Reasonable timeout
```

### Production

```rust
let config = StoreConfig::new("http://qlever-prod.example.org:7777")
    .with_timeout(Duration::from_secs(60));  // Longer timeout for large queries
```

### Testing

```rust
let config = StoreConfig::new("http://localhost:7777")
    .with_timeout(Duration::from_secs(5));  // Quick timeout to catch hangs
```

---

## Connection Management

### Reuse Store Instance

```rust
// ✅ Good: Reuse single store
let store = Store::new("http://localhost:7777")?;

for i in 0..100 {
    let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
    // ...
}
```

### Don't Reconnect

```rust
// ❌ Bad: Creates new connection each time
for i in 0..100 {
    let store = Store::new("http://localhost:7777")?;  // Reconnect!
    let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
}
```

---

## Query-Level Configuration

Configure caching per query:

```rust
use qlever::cache::QueryCache;
use std::time::Duration;

// Cache configuration
let cache = QueryCache::new(
    100,                           // Max entries
    Duration::from_secs(300)       // TTL (5 minutes)
);

let results = cache.query_with_cache(&store, "SELECT ...").await?;
```

---

## Common Configuration Scenarios

### Fast Connection, Stable Network

```rust
let config = StoreConfig::new("http://localhost:7777")
    .with_timeout(Duration::from_secs(10));

let cache = QueryCache::new(50, Duration::from_secs(600));
```

### Slow Connection, Slow Queries

```rust
let config = StoreConfig::new("http://qlever.example.org:7777")
    .with_timeout(Duration::from_secs(120));  // 2 minutes

let cache = QueryCache::new(200, Duration::from_secs(3600));
```

### Unreliable Network

```rust
let config = StoreConfig::new("http://backup-server.example.org:7777")
    .with_timeout(Duration::from_secs(30));

// Add retry logic
async fn query_with_retry(store: &Store, query: &str) -> Result<Vec<QuerySolution>> {
    for attempt in 0..3 {
        match store.query(query).await {
            Ok(results) => return Ok(results),
            Err(Error::ConnectionFailed(_)) if attempt < 2 => {
                tokio::time::sleep(Duration::from_millis(100 * 2u64.pow(attempt))).await;
                continue;
            }
            Err(e) => return Err(e),
        }
    }
    Err(Error::ConnectionFailed("Max retries exceeded".to_string()))
}
```

---

## Environment Variables

Configure via environment (optional feature):

```bash
export QLEVER_URL=http://localhost:7777
export QLEVER_TIMEOUT=30
```

```rust
use std::env;

let url = env::var("QLEVER_URL")
    .unwrap_or_else(|_| "http://localhost:7777".to_string());

let timeout = env::var("QLEVER_TIMEOUT")
    .ok()
    .and_then(|s| s.parse::<u64>().ok())
    .map(Duration::from_secs)
    .unwrap_or(Duration::from_secs(30));

let config = StoreConfig::new(&url)
    .with_timeout(timeout);
```

---

## Default Values

| Option | Default | Min | Max |
|--------|---------|-----|-----|
| Timeout | 30 sec | 1 sec | ∞ |
| Cache entries | 100 | 1 | 10000 |
| Cache TTL | 5 min | 1 sec | ∞ |

---

## See Also

- **[Performance Guide](../how-to/performance.md)** — Caching configuration
- **[API Reference](./api.md)** — StoreConfig details
- **[Quick Reference](../QUICKREF.md)** — Configuration snippets

---

**Status:** Reference guide | **Last Updated:** 2025-01-01
