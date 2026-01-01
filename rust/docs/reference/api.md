# API Reference

Complete documentation of public types and methods.

---

## Store

The main entry point for executing queries.

### Constructor

```rust
pub fn new(url: &str) -> Result<Self>
```

Create a new store connected to a QLever server.

**Example:**
```rust
let store = Store::new("http://localhost:7777")?;
```

---

### Methods

#### `query`

```rust
pub async fn query(&self, sparql: &str) -> Result<Vec<QuerySolution>>
```

Execute a SPARQL SELECT query and return all results.

**Parameters:**
- `sparql` - SPARQL SELECT query string

**Returns:** `Vec<QuerySolution>` — variable bindings for each result

**Example:**
```rust
let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
```

---

#### `ask`

```rust
pub async fn ask(&self, sparql: &str) -> Result<bool>
```

Execute a SPARQL ASK query and return true/false.

**Parameters:**
- `sparql` - SPARQL ASK query string

**Returns:** `bool` — whether pattern matches

**Example:**
```rust
let exists = store.ask("ASK { ?x ?p ?o }").await?;
```

---

#### `construct`

```rust
pub async fn construct(&self, sparql: &str) -> Result<Vec<Triple>>
```

Execute a SPARQL CONSTRUCT query and return built triples.

**Parameters:**
- `sparql` - SPARQL CONSTRUCT query string

**Returns:** `Vec<Triple>` — constructed RDF triples

**Example:**
```rust
let triples = store.construct("CONSTRUCT { ?s ?p ?o } WHERE { ?s ?p ?o }").await?;
```

---

#### `describe`

```rust
pub async fn describe(&self, sparql: &str) -> Result<Vec<Quad>>
```

Execute a SPARQL DESCRIBE query and return quads.

**Parameters:**
- `sparql` - SPARQL DESCRIBE query string

**Returns:** `Vec<Quad>` — RDF quads with optional graph names

**Example:**
```rust
let quads = store.describe("DESCRIBE <http://example.org/alice>").await?;
```

---

## QuerySolution

Represents a single binding of variables from a query result.

### Methods

#### `get`

```rust
pub fn get(&self, var: &str) -> Option<String>
```

Get the value of a variable by name.

**Parameters:**
- `var` - variable name (without `?`)

**Returns:** `Option<String>` — value or None if unbound

**Example:**
```rust
if let Some(name) = solution.get("name") {
    println!("Name: {}", name);
}
```

---

#### `iter`

```rust
pub fn iter(&self) -> impl Iterator<Item = (&str, &str)>
```

Iterate over all variable bindings.

**Example:**
```rust
for (var, value) in solution.iter() {
    println!("{} = {}", var, value);
}
```

---

## RDF Model Types

### Term

Represents an RDF value (node or literal).

```rust
pub enum Term {
    NamedNode(NamedNode),
    BlankNode(BlankNode),
    Literal(Literal),
}
```

### NamedNode

An RDF IRI/URI.

```rust
let node = NamedNode::new("http://example.org/alice")?;
```

**Methods:**
- `new(iri: String) -> Result<Self>` — Create new IRI
- `iri(&self) -> &str` — Get IRI string

### BlankNode

An RDF blank node (anonymous resource).

```rust
let blank = BlankNode::new("b1")?;
```

**Methods:**
- `new(id: String) -> Result<Self>` — Create new blank node
- `id(&self) -> &str` — Get blank node ID

### Literal

An RDF literal (string, number, date, etc.).

```rust
// Simple string
let lit = Literal::new_simple("hello");

// Typed literal
let lit = Literal::new_typed("42", "http://www.w3.org/2001/XMLSchema#integer")?;

// Language-tagged literal
let lit = Literal::new_language_tagged("Hola", "es")?;
```

**Methods:**
- `new_simple(value: &str) -> Self` — Plain string literal
- `new_typed(value: &str, type_iri: &str) -> Result<Self>` — Typed literal
- `new_language_tagged(value: &str, lang: &str) -> Result<Self>` — Language-tagged
- `value(&self) -> &str` — Get literal value
- `datatype(&self) -> Option<&str>` — Get datatype IRI
- `language(&self) -> Option<&str>` — Get language tag

---

### Triple

An RDF triple (subject-predicate-object).

```rust
let subject = NamedNode::new("http://example.org/alice")?;
let predicate = NamedNode::new("http://example.org/knows")?;
let object = NamedNode::new("http://example.org/bob")?;

let triple = Triple::new(subject, predicate, object);
```

**Methods:**
- `new(subject, predicate, object) -> Self`
- `subject(&self) -> &Term`
- `predicate(&self) -> &Term`
- `object(&self) -> &Term`

---

### Quad

An RDF quad (triple with optional graph).

```rust
let quad = Quad::new(subject, predicate, object, Some(graph_name));
```

**Methods:**
- `new(subject, predicate, object, graph) -> Self`
- `subject(&self) -> &Term`
- `predicate(&self) -> &Term`
- `object(&self) -> &Term`
- `graph(&self) -> &Option<NamedNode>`

---

## Error Types

### Error

All fallible operations return `Result<T>` where error is:

```rust
pub enum Error {
    QueryError(String),
    InvalidUrl(String),
    ConnectionFailed(String),
    SerializationError(String),
    IoError(String),
}
```

**Handling:**
```rust
match store.query("...").await {
    Ok(results) => { /* ... */ }
    Err(Error::QueryError(msg)) => {
        eprintln!("SPARQL error: {}", msg);
    }
    Err(e) => {
        eprintln!("Error: {}", e);
    }
}
```

---

## QueryCache

Optional caching for frequently-executed queries.

### Constructor

```rust
pub fn new(capacity: usize, ttl: Duration) -> Self
```

**Parameters:**
- `capacity` — Maximum number of cached queries
- `ttl` — Time-to-live for cache entries

**Example:**
```rust
let cache = QueryCache::new(100, Duration::from_secs(300));
```

---

### Methods

#### `query_with_cache`

```rust
pub async fn query_with_cache(
    &self,
    store: &Store,
    sparql: &str,
) -> Result<Vec<QuerySolution>>
```

Execute query with caching.

**Example:**
```rust
let results = cache.query_with_cache(&store, "SELECT ?x WHERE { ... }").await?;
```

---

#### `stats`

```rust
pub fn stats(&self) -> CacheStats
```

Get cache hit/miss statistics.

**Returns:**
```rust
pub struct CacheStats {
    pub hits: u64,
    pub misses: u64,
}
```

**Example:**
```rust
let stats = cache.stats();
println!("Hit rate: {:.2}%", stats.hit_rate() * 100.0);
```

---

## Streaming APIs

For handling large result sets without loading everything into memory.

### ResultIterator

```rust
pub async fn next(&mut self) -> Result<Option<QuerySolution>>
```

Get next single result.

**Example:**
```rust
let mut results = store.query_streaming("SELECT ?x WHERE { ?x ?p ?o }").await?;
while let Some(solution) = results.next().await? {
    println!("{:?}", solution);
}
```

---

### ChunkedResultIterator

```rust
pub async fn next_chunk(&mut self) -> Result<Option<Vec<QuerySolution>>>
```

Get next chunk of results.

**Example:**
```rust
let mut results = store.query_chunked("SELECT ?x WHERE { ?x ?p ?o }", 100).await?;
while let Some(chunk) = results.next_chunk().await? {
    for solution in chunk {
        // process solution
    }
}
```

---

## Configuration

### StoreConfig

Configure store behavior.

```rust
let config = StoreConfig::new("http://localhost:7777")
    .with_timeout(std::time::Duration::from_secs(30));

let store = Store::with_config(config)?;
```

**Methods:**
- `new(url: &str) -> Self` — Create configuration
- `with_timeout(timeout: Duration) -> Self` — Set request timeout
- `build() -> Result<Store>` — Build the store

---

## Serialization Formats

### Supported Output Formats

When using `CONSTRUCT` or `DESCRIBE`:

- **N-Triples** (default for triples)
- **Turtle** (default for quads)
- **N-Quads** (quads with graph names)
- **JSON-LD** (JSON-LD serialization)

---

## Type Conversions

### String to RDF Value

```rust
// From QuerySolution string value to Term
if let Some(value_str) = solution.get("x") {
    // Parse as NamedNode
    if let Ok(node) = NamedNode::new(value_str) {
        // It's a NamedNode
    }
}
```

---

## Common Patterns

### Iterate Results with Error Handling

```rust
match store.query("SELECT ?x WHERE { ... }").await {
    Ok(solutions) => {
        for solution in solutions {
            if let Some(x) = solution.get("x") {
                println!("{}", x);
            }
        }
    }
    Err(e) => eprintln!("Error: {}", e),
}
```

### Work with RDF Terms

```rust
use qlever::model::*;

let subject = NamedNode::new("http://example.org/alice")?;
let predicate = NamedNode::new("http://example.org/age")?;
let object = Literal::new_simple("30");

let triple = Triple::new(
    Term::NamedNode(subject),
    Term::NamedNode(predicate),
    Term::Literal(object),
);
```

---

## Constants & Defaults

| Setting | Default | Notes |
|---------|---------|-------|
| Server URL | (required) | Must be provided to `Store::new()` |
| Request Timeout | 30 seconds | Configurable via `StoreConfig` |
| Cache TTL | (set per cache) | Configure via `QueryCache::new()` |
| Result Limit | (unlimited) | Add `LIMIT` to SPARQL query |

---

## See Also

- **[Getting Started](../tutorials/getting-started.md)** — Quick tutorial
- **[Query Types Guide](../how-to/query-types.md)** — How to write different queries
- **[Performance Guide](../how-to/performance.md)** — Optimization techniques
- **[Error Handling Guide](../how-to/error-handling.md)** — Error patterns

