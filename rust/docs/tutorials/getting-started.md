# Getting Started with QLever Rust

A step-by-step guide to set up and execute your first SPARQL query.

**Time:** ~15 minutes | **Prerequisites:** Rust 1.70+ | **Level:** Beginner

---

## Step 1: Set Up QLever Server

You need a running QLever server. The easiest way is Docker:

```bash
# Pull the latest image
docker pull qlever:latest

# Start the server (runs on localhost:7777)
docker run -d \
  -p 7777:7777 \
  --name qlever \
  qlever:latest
```

Verify it's running:

```bash
curl http://localhost:7777/api/info
# Should return JSON with server information
```

---

## Step 2: Create a Rust Project

```bash
cargo new my-qlever-app
cd my-qlever-app
```

---

## Step 3: Add Dependencies

Edit `Cargo.toml`:

```toml
[dependencies]
qlever = { path = "../rust" }  # Adjust path if needed
tokio = { version = "1", features = ["full"] }
```

---

## Step 4: Write Your First Query

Replace `src/main.rs` with:

```rust
use qlever::Store;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Connect to QLever server
    let store = Store::new("http://localhost:7777")?;

    // Execute a simple query
    let solutions = store
        .query("SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 5")
        .await?;

    println!("Results:");
    for (i, solution) in solutions.iter().enumerate() {
        println!("  Result {}: {:?}", i + 1, solution);
    }

    Ok(())
}
```

---

## Step 5: Run It

```bash
cargo run
```

You'll see results from your QLever dataset:

```
Results:
  Result 1: {"s": "http://...", "p": "http://...", "o": "..."}
  ...
```

**Congratulations!** You just executed your first SPARQL query. 🎉

---

## What Just Happened?

1. **Connected to server**: `Store::new()` creates a connection handle
2. **Wrote SPARQL query**: The `SELECT` statement with a `LIMIT` clause
3. **Executed async**: `await?` waits for the server response (non-blocking)
4. **Got results**: Converted to `Vec<QuerySolution>` and iterated

---

## Understanding the Result

Each `QuerySolution` is a map of variable names → RDF values:

```rust
for solution in solutions {
    // Get specific variable
    if let Some(value) = solution.get("s") {
        println!("Subject: {}", value);
    }

    // Iterate all variables
    for (var_name, value) in solution.iter() {
        println!("{} = {}", var_name, value);
    }
}
```

---

## Core Concepts

### RDF Triples

RDF data is stored as **triples**: `(subject, predicate, object)`

```
<http://example.org/alice>  <http://example.org/knows>  <http://example.org/bob>
```

In SPARQL, use `?variables` to match parts of triples.

### SPARQL Queries

Four main query types:

| Type | Purpose | Example |
|------|---------|---------|
| **SELECT** | Return matching variables | `SELECT ?name WHERE { ... }` |
| **ASK** | True/false answer | `ASK { ?s ?p ?o }` |
| **CONSTRUCT** | Build new triples | `CONSTRUCT { ?s ?p ?o } WHERE { ... }` |
| **DESCRIBE** | Get metadata about resource | `DESCRIBE ?x` |

### Variables

Variables start with `?`:
- `?subject`, `?predicate`, `?object` — standard
- `?x`, `?y` — shorthand
- Match against any RDF value

---

## Next: Write a Real Query

The example above just returns *everything*. Let's query the actual data:

### Find All Names

```rust
let solutions = store
    .query("
        SELECT ?name
        WHERE {
            ?person <http://xmlns.com/foaf/0.1/name> ?name
        }
    ")
    .await?;

for solution in solutions {
    if let Some(name) = solution.get("name") {
        println!("Person: {}", name);
    }
}
```

### Find People and Their Ages

```rust
let solutions = store
    .query("
        SELECT ?person ?age
        WHERE {
            ?person <http://xmlns.com/foaf/0.1/name> ?name ;
                    <http://example.org/age> ?age .
        }
    ")
    .await?;

for solution in solutions {
    let person = solution.get("person").unwrap();
    let age = solution.get("age").unwrap();
    println!("{} is {} years old", person, age);
}
```

---

## Common Issues & Fixes

### "Connection refused"

**Problem:** `Error: Connection refused`

**Fix:** Ensure QLever server is running:

```bash
curl http://localhost:7777/api/info
```

If it fails, start the Docker container (see Step 1).

### "Invalid query"

**Problem:** `Error: Invalid SPARQL`

**Fix:** Check syntax:
- Is `WHERE` present?
- Are triple patterns correct? `?s ?p ?o`
- Misspelled IRIs?

Use an online SPARQL validator: https://sparql.org/query-validator

### "No results"

**Problem:** Query runs but returns empty results.

**Fix:**
- Check your SPARQL patterns against actual data
- Try a broader query: `SELECT ?s ?p ?o WHERE { ?s ?p ?o } LIMIT 1`
- Verify predicates and classes match your dataset

---

## Performance Tips

1. **Use LIMIT** for testing:
   ```sparql
   SELECT ?s WHERE { ?s ?p ?o } LIMIT 10
   ```

2. **Be specific** with patterns:
   ```sparql
   -- Good: specific type
   SELECT ?name WHERE {
     ?person a foaf:Person ;
             foaf:name ?name .
   }

   -- Bad: too broad
   SELECT ?x WHERE { ?x ?p ?y }
   ```

3. **Cache results** for repeated queries:
   ```rust
   use qlever::cache::QueryCache;

   let cache = QueryCache::new(100, Duration::from_secs(300));
   cache.query_with_cache(&store, "SELECT ...").await?;
   ```

---

## Continue Learning

Now that you've executed your first query:

1. **Learn query types**: Read [Execute Different Query Types](../how-to/query-types.md)
2. **Work with RDF data**: Read [Parse & Manipulate RDF Data](../how-to/rdf-data.md)
3. **Understand the API**: Read [API Reference](../reference/api.md)
4. **Want complex patterns?** See [How-To: Advanced Patterns](../how-to/query-types.md#advanced-patterns)

---

## Troubleshooting Guide

| Error | Cause | Solution |
|-------|-------|----------|
| `InvalidUrl` | Invalid server URL | Use `http://localhost:7777` |
| `QueryError("...")` | Bad SPARQL syntax | Validate at https://sparql.org/query-validator |
| `ConnectionFailed` | Server not running | Start Docker container or server |
| `SerializationError` | Invalid response | May indicate server issue; check logs |

---

**Ready for more?** Jump to [How-To Guides](../how-to/) for practical solutions.
