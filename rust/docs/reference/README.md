# Reference — Complete API

Comprehensive documentation of types, methods, and configuration.

## Available References

### [API Reference](./api.md)

Complete documentation of all public types and methods.

**Includes:**
- `Store` — Main entry point
- `QuerySolution` — Result bindings
- RDF Model types (`Term`, `NamedNode`, `BlankNode`, `Literal`, `Triple`, `Quad`)
- `Error` types
- `QueryCache` — Caching API
- Streaming APIs
- Configuration

**Time:** Reference (look up as needed)

---

### [Error Types](./errors.md) *(Coming soon)*

Complete error catalog with examples.

---

### [Configuration](./config.md) *(Coming soon)*

Store setup options and tuning parameters.

---

## Quick Lookup

| Type | Purpose |
|------|---------|
| `Store` | Main API entry point |
| `QuerySolution` | Single query result |
| `Term` | RDF value (Node/Literal) |
| `NamedNode` | IRI/URI |
| `BlankNode` | Anonymous resource |
| `Literal` | String/number/date value |
| `Triple` | (subject, predicate, object) |
| `Quad` | Triple with optional graph |
| `Error` | Fallible operation result |
| `QueryCache` | LRU cache for queries |

---

## Common Tasks

### Create a Store

```rust
let store = Store::new("http://localhost:7777")?;
```

👉 See [Store in API Reference](./api.md#store)

---

### Execute a Query

```rust
let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;
```

👉 See [Store::query in API Reference](./api.md#query)

---

### Create RDF Terms

```rust
use qlever::model::*;

let node = NamedNode::new("http://example.org/alice")?;
let literal = Literal::new_simple("hello");
let term = Term::NamedNode(node);
```

👉 See [RDF Types in API Reference](./api.md#rdf-model-types)

---

### Build a Triple

```rust
let triple = Triple::new(subject, predicate, object);
```

👉 See [Triple in API Reference](./api.md#triple)

---

### Cache Results

```rust
let cache = QueryCache::new(100, Duration::from_secs(300));
let results = cache.query_with_cache(&store, query).await?;
```

👉 See [QueryCache in API Reference](./api.md#querycache)

---

## Browse by Category

### Query Execution
- `Store::query()` — SELECT queries
- `Store::ask()` — ASK queries
- `Store::construct()` — CONSTRUCT queries
- `Store::describe()` — DESCRIBE queries

### RDF Data
- `Term` — RDF values
- `NamedNode` — IRIs
- `BlankNode` — Anonymous nodes
- `Literal` — String/number/date
- `Triple` — (s, p, o)
- `Quad` — (s, p, o, g)

### Results
- `QuerySolution` — Variable bindings
- `ResultIterator` — Stream results
- `ChunkedResultIterator` — Stream chunks

### Advanced
- `QueryCache` — LRU caching
- `StoreConfig` — Configuration
- `Error` — Error handling

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "Type `NamedNode` not found" | `use qlever::model::NamedNode;` |
| "Result type mismatch" | Use `.get("var")` to get Option<String> |
| "Compilation error with generics" | Check lifetimes and ownership |
| "Runtime error: Connection refused" | Ensure QLever server is running |

---

## See Also

- **[Getting Started Tutorial](../tutorials/getting-started.md)** — Quick start
- **[How-To Guides](../how-to/)** — Practical solutions
- **[Architecture Explanation](../explanations/architecture.md)** — Design details

