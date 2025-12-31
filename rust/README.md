# QLever Rust Bindings

A Rust library for interacting with QLever, a high-performance RDF/SPARQL graph database. The API is designed to be similar to oxigraph, providing familiar interfaces for Rust developers working with RDF data.

## Features

- **SPARQL Query Execution**: Execute SELECT, ASK, CONSTRUCT, and DESCRIBE queries
- **RDF Data Types**: First-class support for RDF terms (named nodes, blank nodes, literals)
- **Async/Await API**: Fully async API using Tokio for non-blocking I/O
- **Server Management**: Query server statistics and get autocompletion suggestions
- **Format Support**: Support for Turtle and N-Quads RDF serialization formats
- **Type-Safe**: Strong typing for RDF concepts and query results

## Installation

Add to your `Cargo.toml`:

```toml
[dependencies]
qlever-rust = { path = "../rust" }
tokio = { version = "1", features = ["full"] }
```

## Quick Start

```rust
use qlever::Store;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Create a store connected to a QLever server
    let store = Store::new("http://localhost:7777")?;

    // Execute a SPARQL SELECT query
    let solutions = store
        .query("SELECT ?subject ?predicate WHERE { ?subject ?predicate ?object } LIMIT 10")
        .await?;

    // Iterate over results
    for solution in solutions {
        if let Some(subject) = solution.get("subject") {
            println!("Subject: {}", subject);
        }
    }

    Ok(())
}
```

## API Overview

### Store

The `Store` is the main entry point for interacting with QLever:

```rust
// Create a store
let store = Store::new("http://localhost:7777")?;

// Execute SELECT queries
let solutions = store.query("SELECT ?s ?p ?o WHERE { ?s ?p ?o }").await?;

// Execute ASK queries
let has_results = store.ask("ASK { ?s ?p ?o }").await?;

// Execute CONSTRUCT queries
let triples = store.construct("CONSTRUCT { ?s ?p ?o } WHERE { ?s ?p ?o }").await?;

// Execute DESCRIBE queries
let quads = store.describe("DESCRIBE <http://example.org/resource>").await?;

// Get server statistics
let stats = store.stats().await?;

// Get autocompletion suggestions
let suggestions = store.autocomplete("SELECT ?s WHERE { ?s", Some(22)).await?;
```

### RDF Model Types

The library provides types for representing RDF data:

```rust
use qlever::model::{NamedNode, BlankNode, Literal, Term, Triple, Quad};

// Create RDF terms
let subject = NamedNode::new("http://example.org/subject".to_string())?;
let predicate = NamedNode::new("http://example.org/predicate".to_string())?;
let object = Term::Literal(Literal::new_simple("value"));

// Create a triple
let triple = Triple::new(subject.clone(), predicate.clone(), object);

// Create a quad with optional graph name
let graph = Some(NamedNode::new("http://example.org/graph".to_string())?);
let quad = Quad::new(subject, predicate, object, graph);
```

### Query Results

Query results are returned as `QuerySolution` objects:

```rust
let solutions = store.query("SELECT ?s ?p ?o WHERE { ?s ?p ?o }").await?;

for solution in solutions {
    // Get a specific variable
    if let Some(value) = solution.get("s") {
        println!("Subject: {}", value);
    }

    // Iterate over all bindings
    for (var, value) in solution.iter() {
        println!("{} = {}", var, value);
    }

    // Convert to RDF term
    if let Some(value) = solution.get("o") {
        let term = value.to_term()?;
        // Use the term...
    }
}
```

## Configuration

You can customize the Store behavior with `StoreConfig`:

```rust
use qlever::store::StoreConfig;

let config = StoreConfig::new("http://localhost:7777")
    .with_timeout(60);

let store = Store::with_config(config)?;
```

## Error Handling

The library provides detailed error types:

```rust
use qlever::error::{Error, Result};

match store.query("SELECT * WHERE { ?s ?p ?o }").await {
    Ok(solutions) => {
        // Process results
    }
    Err(Error::QueryError(msg)) => {
        println!("Query failed: {}", msg);
    }
    Err(Error::InvalidUrl(url)) => {
        println!("Invalid URL: {}", url);
    }
    Err(e) => {
        println!("Error: {}", e);
    }
}
```

## Examples

Run the included example to see the library in action:

```bash
# Make sure QLever is running on localhost:7777
cargo run --example query
```

## API Compatibility with Oxigraph

This library is designed to have a similar API surface to oxigraph for familiarity:

| Oxigraph | QLever Rust |
|----------|------------|
| `Store` | `Store` |
| `Term`, `NamedNode`, `BlankNode`, `Literal` | Same types |
| `Triple`, `Quad` | Same types |
| `query()` | Same semantics |
| In-memory storage | Connects to remote server |

## Project Structure

```
rust/
├── src/
│   ├── lib.rs          # Main library entry point
│   ├── error.rs        # Error types
│   ├── model.rs        # RDF data types
│   ├── query.rs        # Query result types
│   └── store.rs        # Store implementation
├── examples/
│   └── query.rs        # Example usage
├── Cargo.toml          # Package manifest
└── README.md           # This file
```

## License

Same as QLever (typically GPL/Apache for academic use)

## Contributing

To contribute to the Rust bindings:

1. Follow Rust conventions and style
2. Add tests for new functionality
3. Update documentation
4. Ensure all tests pass: `cargo test`
5. Check formatting: `cargo fmt`
6. Run linter: `cargo clippy`

## Future Enhancements

- [ ] RDF parsing from files (Turtle, N-Triples, RDF/XML)
- [ ] Result streaming for large datasets
- [ ] Connection pooling and performance optimization
- [ ] Support for SPARQL Update operations
- [ ] WebSocket support for real-time updates
- [ ] Custom result serialization formats
- [ ] Better error messages and debugging

## See Also

- [QLever Documentation](https://github.com/ad-freiburg/qlever)
- [Oxigraph Documentation](https://docs.rs/oxigraph/)
- [SPARQL 1.1 Specification](https://www.w3.org/TR/sparql11-query/)
- [RDF 1.1 Concepts](https://www.w3.org/TR/rdf11-concepts/)
