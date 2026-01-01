# Quick Wins: Immediate Improvements to Rust Bindings

## Overview

These are practical improvements that can be implemented quickly (1-2 weeks) with the existing HTTP-based API, while planning the Phase 1 libqlever integration.

## 1. Enhanced Query Builder (WASM) ⭐⭐⭐⭐

**Current State:** Basic builder pattern with manual string construction

**Improvement:** Add SPARQL 1.1 feature support

### Implementation (1-2 days)

**File:** `wasm/src/lib.rs`

```rust
impl QueryBuilder {
    // Existing: select, where, limit, etc.

    // NEW: UNION support
    pub fn union(mut self, other: QueryBuilder) -> QueryBuilder {
        self.union_queries.push(other.build());
        self
    }

    // NEW: MINUS support
    pub fn minus(mut self, other: QueryBuilder) -> QueryBuilder {
        self.minus_queries.push(other.build());
        self
    }

    // NEW: Subqueries
    pub fn with_subquery(mut self, name: &str, subquery: QueryBuilder) -> QueryBuilder {
        self.subqueries.insert(name.to_string(), subquery.build());
        self
    }

    // NEW: Property paths
    pub fn property_path(mut self, subject: &str, path: &str, object: &str) -> QueryBuilder {
        self.where_pattern.push_str(&format!(
            "{} {} {} .",
            subject,
            path,  // e.g., "^rdf:type" or "foaf:knows+"
            object
        ));
        self
    }

    // NEW: GROUP_CONCAT aggregation
    pub fn aggregate(mut self, var: &str, function: &str) -> QueryBuilder {
        // Store aggregations for SELECT clause
        self.aggregations.push((var.to_string(), function.to_string()));
        self
    }

    // NEW: HAVING clause
    pub fn having(mut self, condition: &str) -> QueryBuilder {
        self.having_clause = Some(condition.to_string());
        self
    }

    // NEW: Negation
    pub fn filter_not_exists(mut self, pattern: &str) -> QueryBuilder {
        self.filter_conditions.push(format!("FILTER NOT EXISTS {{ {} }}", pattern));
        self
    }
}
```

**Benefits:**
- ✅ Support all common SPARQL 1.1 features
- ✅ Type-safe complex query construction
- ✅ More powerful than manual string building

### Tests

```rust
#[cfg(test)]
mod tests {
    #[test]
    fn test_union_queries() {
        let query1 = QueryBuilder::new()
            .select_query()
            .select("?x")
            .where_clause("?x a Person");

        let query2 = QueryBuilder::new()
            .select_query()
            .select("?x")
            .where_clause("?x a Company");

        let combined = QueryBuilder::new()
            .select_query()
            .select("?x")
            .union(query1)
            .union(query2)
            .build();

        assert!(combined.contains("UNION"));
    }

    #[test]
    fn test_property_paths() {
        let query = QueryBuilder::new()
            .select_query()
            .select("?s ?o")
            .property_path("?s", "^rdf:type/rdfs:subClassOf*", "?o")
            .build();

        assert!(query.contains("^rdf:type/rdfs:subClassOf*"));
    }
}
```

---

## 2. Result Streaming Support (Native) ⭐⭐⭐

**Current State:** All results loaded into memory as Vec<QuerySolution>

**Improvement:** Streaming support for large result sets

### Implementation (2-3 days)

**File:** `rust/src/streaming.rs` (NEW)

```rust
use futures::Stream;
use pin_project::pin_project;

/// Streaming iterator for large result sets
pub struct QueryResultStream {
    client: reqwest::Client,
    query: String,
    page_size: usize,
    current_offset: usize,
    buffer: std::collections::VecDeque<QuerySolution>,
    exhausted: bool,
}

impl QueryResultStream {
    pub fn new(
        client: reqwest::Client,
        query: String,
        page_size: usize,
    ) -> Self {
        QueryResultStream {
            client,
            query,
            page_size,
            current_offset: 0,
            buffer: std::collections::VecDeque::new(),
            exhausted: false,
        }
    }

    async fn fetch_next_page(&mut self) -> Result<()> {
        if self.exhausted {
            return Ok(());
        }

        // Add LIMIT and OFFSET to query
        let paginated_query = format!(
            "{} LIMIT {} OFFSET {}",
            self.query, self.page_size, self.current_offset
        );

        let results = todo!("Execute paginated query");

        if results.is_empty() {
            self.exhausted = true;
        } else {
            self.buffer.extend(results);
            self.current_offset += self.page_size;
        }

        Ok(())
    }
}

impl futures::stream::Stream for QueryResultStream {
    type Item = Result<QuerySolution>;

    fn poll_next(
        mut self: std::pin::Pin<&mut Self>,
        cx: &mut std::task::Context<'_>,
    ) -> std::task::Poll<Option<Self::Item>> {
        // Return buffered items first
        if let Some(item) = self.buffer.pop_front() {
            return std::task::Poll::Ready(Some(Ok(item)));
        }

        // Fetch next page if needed
        if !self.exhausted {
            // Use async runtime to fetch
            todo!("Implement async fetch")
        }

        if self.buffer.is_empty() {
            std::task::Poll::Ready(None)
        } else {
            std::task::Poll::Ready(Some(Ok(self.buffer.pop_front().unwrap())))
        }
    }
}

impl Store {
    pub fn query_stream(&self, query: &str) -> QueryResultStream {
        QueryResultStream::new(
            self.client.as_ref().clone(),
            query.to_string(),
            1000,  // Page size
        )
    }
}
```

**Usage:**

```rust
use futures::stream::StreamExt;

#[tokio::main]
async fn main() -> Result<()> {
    let store = Store::new("http://localhost:7777")?;

    let mut stream = store.query_stream("SELECT ?s ?p ?o WHERE { ?s ?p ?o }")?;

    while let Some(solution) = stream.next().await {
        let solution = solution?;
        println!("{:?}", solution);
    }

    Ok(())
}
```

**Benefits:**
- ✅ Handle multi-million result sets
- ✅ Constant memory usage
- ✅ Streaming semantics

---

## 3. Advanced Query Caching ⭐⭐⭐

**Current State:** No caching

**Improvement:** Built-in query result caching

### Implementation (2-3 days)

**File:** `rust/src/cache.rs` (NEW)

```rust
use std::collections::HashMap;
use std::sync::Arc;
use parking_lot::RwLock;

pub struct QueryCache {
    entries: Arc<RwLock<HashMap<String, CacheEntry>>>,
    max_size: usize,
    ttl: std::time::Duration,
}

struct CacheEntry {
    result: Vec<QuerySolution>,
    created_at: std::time::Instant,
    access_count: usize,
    last_accessed: std::time::Instant,
}

impl QueryCache {
    pub fn new(max_size: usize, ttl: std::time::Duration) -> Self {
        QueryCache {
            entries: Arc::new(RwLock::new(HashMap::new())),
            max_size,
            ttl,
        }
    }

    pub fn get(&self, query: &str) -> Option<Vec<QuerySolution>> {
        let mut entries = self.entries.write();

        if let Some(entry) = entries.get_mut(query) {
            // Check if expired
            if entry.created_at.elapsed() < self.ttl {
                entry.access_count += 1;
                entry.last_accessed = std::time::Instant::now();
                return Some(entry.result.clone());
            } else {
                entries.remove(query);
            }
        }

        None
    }

    pub fn set(&self, query: String, result: Vec<QuerySolution>) {
        let mut entries = self.entries.write();

        if entries.len() >= self.max_size {
            // Evict least-recently-used entry
            if let Some(key) = entries
                .iter()
                .min_by_key(|(_, entry)| entry.last_accessed)
                .map(|(k, _)| k.clone())
            {
                entries.remove(&key);
            }
        }

        entries.insert(
            query,
            CacheEntry {
                result,
                created_at: std::time::Instant::now(),
                access_count: 1,
                last_accessed: std::time::Instant::now(),
            },
        );
    }

    pub fn stats(&self) -> CacheStats {
        let entries = self.entries.read();
        CacheStats {
            total_entries: entries.len(),
            total_accesses: entries.iter().map(|(_, e)| e.access_count).sum(),
        }
    }

    pub fn clear(&self) {
        self.entries.write().clear();
    }
}

pub struct StoreWithCache {
    store: Store,
    cache: QueryCache,
}

impl StoreWithCache {
    pub fn new(store: Store, cache: QueryCache) -> Self {
        StoreWithCache { store, cache }
    }

    pub async fn query(&self, query: &str) -> Result<Vec<QuerySolution>> {
        // Check cache first
        if let Some(cached) = self.cache.get(query) {
            return Ok(cached);
        }

        // Execute query
        let results = self.store.query(query).await?;

        // Cache results
        self.cache.set(query.to_string(), results.clone());

        Ok(results)
    }

    pub fn cache_stats(&self) -> CacheStats {
        self.cache.stats()
    }
}

pub struct CacheStats {
    pub total_entries: usize,
    pub total_accesses: usize,
}
```

**Usage:**

```rust
let store = Store::new("http://localhost:7777")?;
let cache = QueryCache::new(100, Duration::from_secs(300));  // 5 min TTL
let cached_store = StoreWithCache::new(store, cache);

// First call: hits server
let results = cached_store.query("SELECT * WHERE { ?s ?p ?o } LIMIT 10").await?;

// Second call: cache hit
let results = cached_store.query("SELECT * WHERE { ?s ?p ?o } LIMIT 10").await?;

println!("Cache stats: {:?}", cached_store.cache_stats());
```

**Benefits:**
- ✅ Dramatically reduce server load
- ✅ Sub-millisecond repeated queries
- ✅ LRU eviction strategy
- ✅ TTL-based invalidation

---

## 4. Better Error Context ⭐⭐⭐

**Current State:** Generic string errors

**Improvement:** Structured error types with more context

### Implementation (1-2 days)

**File:** `rust/src/error.rs` (enhancement)

```rust
#[derive(Error, Debug)]
pub enum Error {
    #[error("HTTP request failed")]
    Http {
        #[from]
        source: reqwest::Error,
        url: Option<String>,
        query: Option<String>,
    },

    #[error("SPARQL query parsing failed: {message}")]
    QueryParsingFailed {
        message: String,
        query: String,
        position: Option<usize>,
    },

    #[error("Query execution timeout after {duration_secs}s")]
    QueryTimeout { duration_secs: u64 },

    #[error("Result format not supported: {format}")]
    UnsupportedFormat { format: String },

    #[error("Invalid configuration: {message}")]
    InvalidConfiguration {
        message: String,
        config_key: Option<String>,
    },

    #[error("Server returned {status}: {message}")]
    ServerError {
        status: u16,
        message: String,
    },

    // ... existing errors ...
}

impl Error {
    pub fn with_query(mut self, query: String) -> Self {
        match self {
            Error::Http {
                source,
                ref mut query: q,
                ..
            } => {
                *q = Some(query);
                self
            }
            _ => self,
        }
    }

    pub fn with_url(mut self, url: String) -> Self {
        match self {
            Error::Http {
                source,
                ref mut url: u,
                ..
            } => {
                *u = Some(url);
                self
            }
            _ => self,
        }
    }
}
```

**Benefits:**
- ✅ Structured error information
- ✅ Better debugging
- ✅ Error recovery strategies
- ✅ Context preservation

---

## 5. Async Trait Support (WASM) ⭐⭐

**Current State:** All methods async but monomorphic

**Improvement:** Support trait objects for better abstraction

### Implementation (1 day)

**File:** `wasm/src/lib.rs`

```rust
use async_trait::async_trait;

#[async_trait(?Send)]
pub trait SparqlClient {
    async fn query(&self, query: &str, format: &str) -> Result<String, JsValue>;
    async fn ping(&self) -> Result<bool, JsValue>;
}

#[async_trait(?Send)]
impl SparqlClient for QleverClient {
    async fn query(&self, query: &str, format: &str) -> Result<String, JsValue> {
        // Existing implementation
        todo!()
    }

    async fn ping(&self) -> Result<bool, JsValue> {
        // Existing implementation
        todo!()
    }
}

// Now client can be passed as trait object
pub async fn execute_with_client(
    client: &dyn SparqlClient,
    query: &str,
) -> Result<String, JsValue> {
    client.query(query, "json").await
}
```

**Benefits:**
- ✅ Better abstraction
- ✅ Easier testing with mocks
- ✅ Dependency injection support

---

## 6. Comprehensive Logging ⭐⭐

**Current State:** No logging

**Improvement:** Add structured logging

### Implementation (1 day)

**File:** `rust/src/lib.rs`

```rust
pub use log::{debug, info, warn, error};

// In Store methods:
impl Store {
    pub async fn query(&self, query: &str) -> Result<Vec<QuerySolution>> {
        debug!("Executing query: {}", query);
        let start = std::time::Instant::now();

        let response = self.client
            .post(&format!("{}api/sparql", self.endpoint_url))
            .form(&[("query", query)])
            .send()
            .await
            .map_err(|e| {
                error!("Query failed: {}", e);
                Error::Http(e)
            })?;

        let elapsed = start.elapsed();
        info!("Query executed in {:.2}ms", elapsed.as_secs_f64() * 1000.0);

        // ... rest of implementation
    }
}
```

**Benefits:**
- ✅ Better debugging
- ✅ Performance monitoring
- ✅ Error tracking

---

## 7. Examples & Documentation ⭐⭐

**Current State:** Basic README

**Improvement:** Comprehensive examples

### Implementation (2-3 days)

**Files to Create:**

1. **`examples/basic_query.rs`**
   ```rust
   //! Basic SPARQL query example
   #[tokio::main]
   async fn main() -> Result<()> {
       let store = qlever::Store::new("http://localhost:7777")?;
       let results = store.query("SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 10").await?;
       for solution in results {
           println!("{:?}", solution);
       }
       Ok(())
   }
   ```

2. **`examples/streaming_results.rs`**
   ```rust
   //! Stream large result sets
   ```

3. **`examples/cached_queries.rs`**
   ```rust
   //! Using query result caching
   ```

4. **`examples/configuration.rs`**
   ```rust
   //! Advanced configuration
   ```

**Benefits:**
- ✅ Easy onboarding
- ✅ Best practices
- ✅ Copy-paste friendly

---

## 8. CI/CD Improvements ⭐⭐

**Current State:** Basic GitHub Actions

**Improvement:** Add more comprehensive testing

### Implementation (1-2 days)

**File:** `.github/workflows/rust-tests.yml` (new)

```yaml
name: Rust Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, macos-latest, windows-latest]
        rust: [stable, nightly]

    steps:
      - uses: actions/checkout@v2
      - uses: actions-rs/toolchain@v1
        with:
          toolchain: ${{ matrix.rust }}

      - name: Run tests
        run: cargo test --all

      - name: Run benchmarks
        run: cargo bench --no-run

      - name: Test documentation
        run: cargo test --doc

      - name: Check formatting
        run: cargo fmt -- --check

      - name: Clippy lints
        run: cargo clippy -- -D warnings
```

**Benefits:**
- ✅ Multi-platform testing
- ✅ Lint checking
- ✅ Documentation validation

---

## Priority & Timeline

| Priority | Item | Effort | Value |
|----------|------|--------|-------|
| 🔴 High | Advanced QueryBuilder (WASM) | 1-2d | ⭐⭐⭐⭐ |
| 🔴 High | Result Streaming (Native) | 2-3d | ⭐⭐⭐ |
| 🟡 Medium | Query Caching | 2-3d | ⭐⭐⭐ |
| 🟡 Medium | Better Error Context | 1-2d | ⭐⭐⭐ |
| 🟡 Medium | Examples & Docs | 2-3d | ⭐⭐ |
| 🟢 Low | Async Traits (WASM) | 1d | ⭐⭐ |
| 🟢 Low | Logging | 1d | ⭐⭐ |
| 🟢 Low | CI/CD | 1-2d | ⭐⭐ |
| **Total** | | **10-17 days** | |

## Recommended Approach

1. **Week 1:** Implement Advanced QueryBuilder + Basic Streaming
2. **Week 2:** Query Caching + Better Errors + Examples
3. **Week 3:** Documentation + CI/CD improvements

This gives you:
- ✅ More powerful API
- ✅ Better scalability
- ✅ Improved developer experience
- ✅ Foundation for Phase 1 (libqlever integration)

## Success Metrics

After implementing these quick wins:
- [ ] 50% reduction in developer onboarding time
- [ ] 10x improved performance for repeated queries (via caching)
- [ ] 100% documented API with examples
- [ ] Multi-platform CI/CD passing
- [ ] Clear error messages for debugging
- [ ] Support for large result sets (>1M rows)

---

## Next Steps

1. **Prioritize:** Choose 2-3 quick wins to implement first
2. **Create issues:** Break down each into GitHub issues
3. **Plan Phase 1:** Start detailed planning for libqlever integration
4. **Set timeline:** Schedule Phase 1 work (3-4 weeks)
