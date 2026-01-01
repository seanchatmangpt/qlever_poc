# QLever Rust — Quick Reference Card

**One-page cheatsheet** for common patterns. Copy-paste ready.

---

## Initialize Store

```rust
use qlever::Store;

// Connect to server
let store = Store::new("http://localhost:7777")?;
```

---

## Execute Queries

### SELECT Query

```rust
let results = store
    .query("SELECT ?name WHERE { ?x foaf:name ?name }")
    .await?;

for solution in results {
    if let Some(name) = solution.get("name") {
        println!("{}", name);
    }
}
```

### ASK Query

```rust
let exists = store.ask("ASK { ?x foaf:name ?name }").await?;
println!("Has data: {}", exists);
```

### CONSTRUCT Query

```rust
let triples = store.construct("
    CONSTRUCT { ?s foaf:fullName ?name }
    WHERE { ?s foaf:name ?name }
").await?;

for triple in triples {
    println!("{} {} {}", triple.subject, triple.predicate, triple.object);
}
```

### DESCRIBE Query

```rust
let quads = store.describe("DESCRIBE <http://example.org/alice>").await?;
for quad in quads {
    println!("{:?}", quad);
}
```

---

## Work with RDF Data

```rust
use qlever::model::*;

// Named Node (IRI)
let node = NamedNode::new("http://example.org/alice")?;

// Blank Node
let blank = BlankNode::new("b1")?;

// Literal (various types)
let simple = Literal::new_simple("hello");
let typed = Literal::new_typed("42", "http://www.w3.org/2001/XMLSchema#integer")?;
let tagged = Literal::new_language_tagged("Hola", "es")?;

// Build Triple
let triple = Triple::new(
    node.clone(),
    NamedNode::new("http://example.org/knows")?,
    Term::Literal(simple),
);

// Build Quad (with graph)
let quad = Quad::new(
    node.clone(),
    NamedNode::new("http://example.org/knows")?,
    Term::Literal(simple),
    Some(NamedNode::new("http://example.org/graph")?),
);
```

---

## Cache Query Results

```rust
use qlever::cache::QueryCache;
use std::time::Duration;

let cache = QueryCache::new(100, Duration::from_secs(300));
let results = cache.query_with_cache(&store, "SELECT ?x WHERE { ... }").await?;

// Check cache statistics
let stats = cache.stats();
println!("Hit rate: {:.2}%", stats.hit_rate() * 100.0);
```

---

## Batch Concurrent Queries

```rust
use tokio::join;

let (results1, results2, results3) = join!(
    store.query("SELECT ?x WHERE { ?x a foaf:Person }"),
    store.query("SELECT ?x WHERE { ?x a foaf:Place }"),
    store.query("SELECT ?x WHERE { ?x a foaf:Thing }")
);

println!("People: {}", results1?.len());
```

---

## Stream Large Results

```rust
// Process one result at a time
let mut results = store.query_streaming("SELECT ?x WHERE { ?x ?p ?o }").await?;
while let Some(solution) = results.next().await? {
    println!("{:?}", solution);
}

// Process in chunks
let mut results = store.query_chunked("SELECT ?x WHERE { ?x ?p ?o }", 1000).await?;
while let Some(chunk) = results.next_chunk().await? {
    for solution in chunk {
        // process solution
    }
}
```

---

## Error Handling

```rust
use qlever::error::Error;

match store.query("SELECT ?x WHERE { ... }").await {
    Ok(solutions) => println!("Got {} results", solutions.len()),
    Err(Error::QueryError(msg)) => eprintln!("SPARQL error: {}", msg),
    Err(Error::ConnectionFailed(msg)) => eprintln!("Network error: {}", msg),
    Err(e) => eprintln!("Error: {}", e),
}
```

---

## Common Patterns

### Get Single Variable from Result

```rust
if let Some(value) = solution.get("x") {
    println!("x = {}", value);
}
```

### Iterate All Bindings

```rust
for (var, value) in solution.iter() {
    println!("{} = {}", var, value);
}
```

### Filter Results in Rust

```rust
let filtered: Vec<_> = results
    .into_iter()
    .filter(|sol| {
        sol.get("age")
            .and_then(|age| age.parse::<i32>().ok())
            .map(|age| age > 21)
            .unwrap_or(false)
    })
    .collect();
```

### Count Results

```rust
let count = results.len();
println!("Total: {}", count);
```

---

## SPARQL Patterns

### Basic Triple Pattern

```sparql
SELECT ?name WHERE {
    ?person foaf:name ?name .
}
```

### Multiple Patterns (AND)

```sparql
SELECT ?name ?age WHERE {
    ?person foaf:name ?name ;
            ex:age ?age .
}
```

### Optional

```sparql
SELECT ?name ?email WHERE {
    ?person foaf:name ?name .
    OPTIONAL { ?person ex:email ?email . }
}
```

### Filter

```sparql
SELECT ?name ?age WHERE {
    ?person foaf:name ?name ;
            ex:age ?age .
    FILTER (?age > 21)
}
```

### DISTINCT

```sparql
SELECT DISTINCT ?type WHERE {
    ?x a ?type .
}
```

### LIMIT & ORDER

```sparql
SELECT ?name ?age WHERE {
    ?person foaf:name ?name ;
            ex:age ?age .
}
ORDER BY DESC(?age)
LIMIT 10
```

---

## Async/Await Essentials

```rust
// Mark function async
async fn my_query() -> Result<Vec<QuerySolution>> {
    let store = Store::new("http://localhost:7777")?;
    let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
    Ok(results)
}

// Use in main
#[tokio::main]
async fn main() -> Result<()> {
    let results = my_query().await?;
    Ok(())
}

// Concurrent execution
let results = tokio::join!(
    store.query("..."),
    store.query("..."),
);
```

---

## Performance Tips

✅ **DO:**
- Add `LIMIT` to queries
- Use `ASK` for existence checks
- Cache repeated queries
- Batch related queries together
- Use `.query_streaming()` for large results

❌ **DON'T:**
- Query without `LIMIT` in production
- Reconnect for each query (reuse `Store`)
- Use `SELECT COUNT(*)` when `ASK` will do
- Block the async runtime with sync operations

---

## Common Errors & Fixes

| Error | Fix |
|-------|-----|
| "Connection refused" | Ensure QLever running: `curl http://localhost:7777/api/info` |
| "Invalid SPARQL" | Check syntax at https://sparql.org/query-validator |
| "No results" | Verify patterns match your data; try `SELECT ?x ?p ?o WHERE { ?x ?p ?o } LIMIT 1` |
| "Results too large" | Use `.query_streaming()` or add `LIMIT` |
| "Compilation error: expected async" | Check you called `.await?` on query |

---

## Links

- **[Full Tutorial](./docs/tutorials/getting-started.md)** — Detailed guide
- **[Query Types How-To](./docs/how-to/query-types.md)** — More examples
- **[Performance Guide](./docs/how-to/performance.md)** — Optimization
- **[API Reference](./docs/reference/api.md)** — Complete types
- **[SPARQL Spec](https://www.w3.org/TR/sparql11-query/)** — Standard

---

**Last Updated:** 2025-01-01 | **Status:** Quick Reference
