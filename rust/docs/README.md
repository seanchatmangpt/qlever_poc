# QLever Rust Documentation Hub

Welcome to the complete documentation for QLever Rust bindings. This documentation follows the **Diataxis** framework, organizing information by user intent.

---

## 🚀 Start Here

### ⚡ **[Quick Reference](./QUICKREF.md)** — Code Snippets (2 min)
Copy-paste patterns for all common operations. Bookmark this!

### 🗺️ **[Learning Path](./LEARNING_PATH.md)** — Choose Your Route (5 min)
Personalized guide: "I want to..." → "I should read..."

### ❓ **[FAQ](./FAQ.md)** — 50+ Common Questions (2 min)
Quick answers to common questions about setup, usage, and troubleshooting.

---

## Choose Your Path by Learning Style

### 🎓 **[Tutorials](./tutorials/)**

**Step-by-step learning guides.** Start here if you're new.

- **[Getting Started](./tutorials/getting-started.md)** — Setup, first query, core concepts (15 min)

**Best for:** Learning the basics, getting productive quickly

---

### 📋 **[How-To Guides](./how-to/)**

**Solution-focused guides.** Use when you know what you want to accomplish.

- **[Execute Different Query Types](./how-to/query-types.md)** — SELECT, ASK, CONSTRUCT, DESCRIBE
- **[Optimize Query Performance](./how-to/performance.md)** — Caching, concurrency, streaming
- **[Parse & Manipulate RDF Data](./how-to/rdf-data.md)** — Work with triples, quads
- **[Handle Errors Gracefully](./how-to/error-handling.md)** — Error patterns and recovery

**Best for:** Solving specific problems, practical examples

---

### 📚 **[Reference](./reference/)**

**Complete API documentation.** Use to look up types and methods.

- **[API Reference](./reference/api.md)** — All public types and methods
- **[Error Types](./reference/errors.md)** — Complete error catalog
- **[Configuration](./reference/config.md)** — Store setup options

**Best for:** Finding exact signatures, looking up details

---

### 💡 **[Explanations](./explanations/)**

**Conceptual guides.** Understand *why* the API is designed this way.

- **[Architecture & Design](./explanations/architecture.md)** — Overall design, module breakdown, design decisions
- **[FFI & Performance](./explanations/ffi-performance.md)** — C++ interop, performance details
- **[Async/Await Model](./explanations/async-model.md)** — Concurrency design

**Best for:** Understanding design, debugging complex behavior, extending the library

---

## Quick Start

```bash
# 1. Start QLever server
docker run -d -p 7777:7777 qlever:latest

# 2. Create Rust project
cargo new my-app && cd my-app

# 3. Add dependency
# Edit Cargo.toml:
# [dependencies]
# qlever = { path = "../rust" }
# tokio = { version = "1", features = ["full"] }

# 4. Write code
cat > src/main.rs << 'EOF'
use qlever::Store;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let store = Store::new("http://localhost:7777")?;
    let results = store.query("SELECT ?s ?p WHERE { ?s ?p ?o } LIMIT 10").await?;

    for solution in results {
        println!("{:?}", solution);
    }
    Ok(())
}
EOF

# 5. Run
cargo run
```

👉 **New to QLever?** See [Getting Started Tutorial](./tutorials/getting-started.md)

---

## Recommended Reading Order

**First time?**
1. [Getting Started Tutorial](./tutorials/getting-started.md) — 15 minutes
2. [Query Types How-To](./how-to/query-types.md) — Learn SPARQL variants
3. [Performance How-To](./how-to/performance.md) — Optimize queries

**Have specific question?**
1. Check [How-To Guides](./how-to/) for practical solutions
2. Check [Reference](./reference/) for API details
3. Check [Architecture Explanation](./explanations/architecture.md) for design

**Ready to extend the library?**
1. [Architecture & Design](./explanations/architecture.md) — Understand structure
2. [API Reference](./reference/api.md) — Know exact types
3. Explore `src/` directory in the repository

---

## Navigation Table

| I want to... | Go to... |
|---|---|
| Get started quickly | [Getting Started](./tutorials/getting-started.md) |
| Learn SPARQL queries | [Query Types How-To](./how-to/query-types.md) |
| Make queries faster | [Performance How-To](./how-to/performance.md) |
| Work with RDF data | [RDF Data How-To](./how-to/rdf-data.md) |
| Handle errors | [Error Handling How-To](./how-to/error-handling.md) |
| Look up a type | [API Reference](./reference/api.md) |
| Understand the design | [Architecture](./explanations/architecture.md) |
| Learn about FFI | [FFI & Performance](./explanations/ffi-performance.md) |

---

## Documentation Map

```
QLever Rust Docs
├── 🎓 Tutorials (Learning paths)
│   └── Getting Started (15 min)
├── 📋 How-To Guides (Solutions)
│   ├── Query Types
│   ├── Performance
│   ├── RDF Data
│   └── Error Handling
├── 📚 Reference (Complete API)
│   ├── API Reference
│   ├── Error Types
│   └── Configuration
└── 💡 Explanations (Deep understanding)
    ├── Architecture
    ├── FFI & Performance
    └── Async/Await Model
```

---

## Key Concepts

### SPARQL

Query language for RDF data. Four main types:
- **SELECT** — Return matching variables
- **ASK** — True/false answer
- **CONSTRUCT** — Build new triples
- **DESCRIBE** — Get resource details

👉 See [Query Types How-To](./how-to/query-types.md)

### RDF

Data model for knowledge graphs. Core concepts:
- **Triple** — (subject, predicate, object)
- **Named Node** — IRI/URI
- **Blank Node** — Anonymous resource
- **Literal** — String/number/date value

👉 See [RDF Data How-To](./how-to/rdf-data.md)

### Async/Await

Non-blocking I/O pattern using Tokio:
- `async fn` — Function that returns Future
- `.await` — Wait for Future without blocking thread
- Enables handling thousands of concurrent operations

👉 See [Async/Await Model](./explanations/async-model.md)

---

## Common Tasks

### Execute a Query

```rust
let solutions = store
    .query("SELECT ?name WHERE { ?x <http://example.org/name> ?name }")
    .await?;

for solution in solutions {
    println!("{:?}", solution);
}
```

👉 [Query Types How-To](./how-to/query-types.md)

---

### Cache Results

```rust
let cache = QueryCache::new(100, Duration::from_secs(300));
cache.query_with_cache(&store, "SELECT ...").await?;
```

👉 [Performance How-To](./how-to/performance.md)

---

### Create RDF Terms

```rust
let node = NamedNode::new("http://example.org/alice")?;
let literal = Literal::new_simple("hello");
let triple = Triple::new(subject, predicate, object);
```

👉 [RDF Data How-To](./how-to/rdf-data.md)

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| "Connection refused" | Ensure QLever server running: `curl http://localhost:7777/api/info` |
| "Invalid SPARQL" | Check syntax at [SPARQL Validator](https://sparql.org/query-validator) |
| "No results" | Verify your SPARQL patterns match dataset; try broader query |
| "Slow queries" | Add LIMIT, use caching, read [Performance Guide](./how-to/performance.md) |

👉 See [Error Handling How-To](./how-to/error-handling.md)

---

## FAQ

**Q: Do I need a running QLever server?**
A: Yes. QLever Rust is a client library. Start server with Docker: `docker run -d -p 7777:7777 qlever:latest`

**Q: Can I use it in a web application?**
A: Yes! It's async/await compatible with all Rust web frameworks (Actix, Axum, Rocket, etc.).

**Q: How does performance compare to Oxigraph?**
A: Different use cases. QLever Rust: remote queries on billion-triple datasets (2-5ms). Oxigraph: in-memory queries on million-triple datasets (<1ms).

👉 See [Architecture](./explanations/architecture.md#comparison-to-oxigraph)

---

## Additional Resources

- **[QLever Repository](https://github.com/ad-freiburg/qlever)** — Main project
- **[SPARQL 1.1 Specification](https://www.w3.org/TR/sparql11-query/)** — SPARQL standard
- **[RDF 1.1 Concepts](https://www.w3.org/TR/rdf11-concepts/)** — RDF standard
- **[Tokio Tutorial](https://tokio.rs/)** — Async Rust runtime
- **[Oxigraph Documentation](https://docs.rs/oxigraph/)** — Alternative RDF library

---

## Getting Help

1. **Check documentation** — Start here!
2. **Search issues** — [QLever GitHub Issues](https://github.com/ad-freiburg/qlever/issues)
3. **Open an issue** — [New Issue](https://github.com/seanchatmangpt/qlever/issues/new)
4. **Ask in discussions** — [GitHub Discussions](https://github.com/seanchatmangpt/qlever/discussions)

---

## Documentation Structure

This documentation uses the **Diataxis** framework:

- **Tutorials** = Learning-oriented (how to get started)
- **How-To Guides** = Problem-oriented (how to accomplish tasks)
- **Reference** = Information-oriented (API documentation)
- **Explanations** = Understanding-oriented (why/how things work)

This structure ensures information is organized by user intent, making it easy to find what you need.

---

**Last Updated:** 2025-01-01 | **Status:** Bleeding-edge (80/20 focus)
