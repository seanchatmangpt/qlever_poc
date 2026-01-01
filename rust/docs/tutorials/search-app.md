# Building a Simple Search App

A real-world example: Create a searchable RDF index in 30 minutes.

> ⏱️ **30 minutes** | 🎯 **Intermediate** | 📦 **Web API, Async**

---

## Overview

Build a web service that searches an RDF dataset:

```
User Input
   ↓
Search API (async Rust)
   ↓
QLever SPARQL Engine
   ↓
Formatted Results
   ↓
User
```

---

## Step 1: Set Up Project

```bash
cargo new rdf-search-app
cd rdf-search-app
```

Edit `Cargo.toml`:

```toml
[dependencies]
qlever = { path = "../rust" }
tokio = { version = "1", features = ["full"] }
axum = "0.7"
serde = { version = "1", features = ["derive"] }
serde_json = "1"
```

---

## Step 2: Define Data Structures

Create `src/models.rs`:

```rust
use serde::{Deserialize, Serialize};

#[derive(Debug, Serialize, Deserialize)]
pub struct SearchQuery {
    pub search_term: String,
    pub limit: Option<usize>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct SearchResult {
    pub name: String,
    pub uri: String,
    pub description: Option<String>,
}

#[derive(Debug, Serialize)]
pub struct SearchResponse {
    pub results: Vec<SearchResult>,
    pub total: usize,
}
```

---

## Step 3: Create Search Service

Create `src/search.rs`:

```rust
use qlever::Store;
use crate::models::{SearchQuery, SearchResult, SearchResponse};
use std::sync::Arc;

pub async fn search_rdf(
    store: Arc<Store>,
    query: SearchQuery,
) -> Result<SearchResponse, Box<dyn std::error::Error>> {
    let limit = query.limit.unwrap_or(10);

    // Build SPARQL query to search for name/label containing search term
    let sparql = format!(
        r#"
        SELECT ?name ?uri ?description
        WHERE {{
            ?uri rdfs:label ?name ;
                 a foaf:Person .
            OPTIONAL {{ ?uri rdfs:comment ?description . }}
            FILTER (CONTAINS(LCASE(?name), LCASE("{}")))
        }}
        LIMIT {}
        "#,
        query.search_term, limit
    );

    // Execute query
    let solutions = store.query(&sparql).await?;

    // Convert to SearchResult
    let results: Vec<SearchResult> = solutions
        .iter()
        .filter_map(|sol| {
            let name = sol.get("name")?;
            let uri = sol.get("uri")?;
            let description = sol.get("description").cloned();

            Some(SearchResult {
                name,
                uri,
                description,
            })
        })
        .collect();

    let total = results.len();

    Ok(SearchResponse { results, total })
}
```

---

## Step 4: Create Web API

Edit `src/main.rs`:

```rust
mod models;
mod search;

use axum::{
    extract::{State, Query},
    response::IntoResponse,
    routing::get,
    Json, Router,
};
use qlever::Store;
use std::sync::Arc;

#[derive(Clone)]
struct AppState {
    store: Arc<Store>,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Initialize store
    let store = Arc::new(Store::new("http://localhost:7777")?);

    let state = AppState { store };

    // Define routes
    let app = Router::new()
        .route("/search", get(search_handler))
        .route("/health", get(health_handler))
        .with_state(state);

    // Start server
    let listener = tokio::net::TcpListener::bind("127.0.0.1:3000").await?;
    println!("Server running on http://127.0.0.1:3000");

    axum::serve(listener, app).await?;

    Ok(())
}

// Search endpoint
async fn search_handler(
    State(state): State<AppState>,
    Query(query): Query<models::SearchQuery>,
) -> impl IntoResponse {
    match search::search_rdf(state.store, query).await {
        Ok(response) => Json(response).into_response(),
        Err(err) => (
            axum::http::StatusCode::INTERNAL_SERVER_ERROR,
            Json(serde_json::json!({ "error": err.to_string() })),
        ).into_response(),
    }
}

// Health check
async fn health_handler() -> impl IntoResponse {
    Json(serde_json::json!({ "status": "ok" }))
}
```

---

## Step 5: Test the App

```bash
# Terminal 1: Start QLever server
docker run -d -p 7777:7777 qlever:latest

# Terminal 2: Run app
cargo run

# Terminal 3: Test API
curl "http://127.0.0.1:3000/search?search_term=alice&limit=5"

# Response:
# {
#   "results": [
#     {
#       "name": "Alice",
#       "uri": "http://example.org/alice",
#       "description": "A person"
#     }
#   ],
#   "total": 1
# }
```

---

## Step 6: Add Caching (Optimize)

Update `src/main.rs`:

```rust
use qlever::cache::QueryCache;
use std::time::Duration;

#[derive(Clone)]
struct AppState {
    store: Arc<Store>,
    cache: Arc<QueryCache>,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let store = Arc::new(Store::new("http://localhost:7777")?);
    let cache = Arc::new(QueryCache::new(100, Duration::from_secs(600)));

    let state = AppState { store, cache };
    // ...
}
```

Update `src/search.rs`:

```rust
use qlever::cache::QueryCache;

pub async fn search_rdf(
    store: Arc<Store>,
    cache: Arc<QueryCache>,
    query: SearchQuery,
) -> Result<SearchResponse, Box<dyn std::error::Error>> {
    // ... build SPARQL ...

    // Use cache
    let solutions = cache.query_with_cache(&store, &sparql).await?;
    // ... rest of code ...
}
```

---

## Features to Add

### 1. Advanced Search

```sparql
SELECT ?name ?uri
WHERE {
    ?uri rdfs:label ?name .
    {
        ?uri a foaf:Person .
    } UNION {
        ?uri a foaf:Organization .
    }
    FILTER (CONTAINS(LCASE(?name), LCASE("search_term")))
}
LIMIT 10
```

### 2. Pagination

```rust
#[derive(Deserialize)]
pub struct SearchQuery {
    pub search_term: String,
    pub limit: Option<usize>,
    pub offset: Option<usize>,
}

// In SPARQL:
// ... WHERE { ... } LIMIT 10 OFFSET 20
```

### 3. Faceting

```sparql
SELECT ?type (COUNT(?uri) AS ?count)
WHERE {
    ?uri rdfs:label ?name ;
         a ?type .
    FILTER (CONTAINS(LCASE(?name), LCASE("search_term")))
}
GROUP BY ?type
```

### 4. Full-Text Search

```sparql
SELECT ?name ?score
WHERE {
    ?uri bif:contains "search_term" ;
         rdfs:label ?name .
}
ORDER BY DESC(?score)
```

---

## Production Checklist

- [ ] Add authentication/authorization
- [ ] Implement rate limiting
- [ ] Add logging for debugging
- [ ] Cache search results
- [ ] Monitor performance
- [ ] Add error handling
- [ ] Test with real data
- [ ] Load testing
- [ ] Documentation

---

## Troubleshooting

### "No results"

Check your SPARQL pattern matches your data:

```bash
curl "http://localhost:7777/api/query?query=SELECT%20%3Fx%20WHERE%20%7B%20%3Fx%20%3Fp%20%3Fo%20%7D%20LIMIT%201"
```

### "Slow searches"

Add caching and indexes:
- Cache frequently searched terms
- Add specific type filters
- Use LIMIT to reduce result set

### "Server connection failed"

Ensure QLever server running:
```bash
curl http://localhost:7777/api/info
```

---

## Next Steps

- Deploy with Docker/Kubernetes
- Add frontend (React, Vue.js, etc.)
- Integrate with other services
- Add advanced search syntax
- Implement saved searches

---

## See Also

- **[Getting Started](./getting-started.md)** — Basic setup
- **[Query Types Guide](../how-to/query-types.md)** — SPARQL patterns
- **[Performance Guide](../how-to/performance.md)** — Optimization
- **[Axum Documentation](https://docs.rs/axum/)** — Web framework
- **[Quick Reference](../QUICKREF.md)** — Code snippets

---

**Status:** Real-world example | **Last Updated:** 2025-01-01
