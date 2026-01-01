# Execute Different Query Types

How to run SELECT, ASK, CONSTRUCT, and DESCRIBE queries.

> ⏱️ **20 minutes** | 🎯 **Intermediate** | 📦 **SPARQL 1.1**

---

## TL;DR {#tldr}

```rust
// SELECT — Get variables
let results = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;

// ASK — True/false
let exists = store.ask("ASK { ?x ?p ?o }").await?;

// CONSTRUCT — Build triples
let triples = store.construct("CONSTRUCT { ?s ?p ?o } WHERE { ... }").await?;

// DESCRIBE — Get details about resource
let quads = store.describe("DESCRIBE <http://example.org/alice>").await?;
```

**When to use what:**
| Query Type | Use When | Latency |
|-----------|----------|---------|
| **SELECT** | You want variable values | Fast |
| **ASK** | You just need yes/no answer | ⚡ Fastest |
| **CONSTRUCT** | You want to build new triples | Medium |
| **DESCRIBE** | You want all facts about resource | Medium |

---

## SELECT Queries

Return matching variables from the dataset.

### Basic SELECT

```rust
let solutions = store
    .query("SELECT ?subject ?predicate WHERE { ?subject ?predicate ?object }")
    .await?;

for solution in solutions {
    let subject = solution.get("subject").unwrap();
    let predicate = solution.get("predicate").unwrap();
    println!("{} {}", subject, predicate);
}
```

### SELECT with Filtering

```rust
let solutions = store
    .query("
        SELECT ?person ?age
        WHERE {
            ?person <http://example.org/age> ?age .
            FILTER (?age > 21)
        }
    ")
    .await?;

for solution in solutions {
    println!("{:?}", solution);
}
```

### SELECT DISTINCT (Remove Duplicates)

```rust
let solutions = store
    .query("
        SELECT DISTINCT ?name
        WHERE {
            ?person <http://example.org/name> ?name .
        }
    ")
    .await?;

// Returns each name only once
```

### SELECT with Ordering & Limits

```rust
let solutions = store
    .query("
        SELECT ?name ?age
        WHERE {
            ?person <http://example.org/name> ?name ;
                    <http://example.org/age> ?age .
        }
        ORDER BY DESC(?age)
        LIMIT 10
    ")
    .await?;
```

### SELECT with Optional

Match patterns even if some variables aren't bound:

```rust
let solutions = store
    .query("
        SELECT ?name ?email
        WHERE {
            ?person <http://example.org/name> ?name .
            OPTIONAL {
                ?person <http://example.org/email> ?email .
            }
        }
    ")
    .await?;

for solution in solutions {
    let name = solution.get("name").unwrap();
    // email might be None if not present
    if let Some(email) = solution.get("email") {
        println!("{}: {}", name, email);
    } else {
        println!("{}: (no email)", name);
    }
}
```

---

## ASK Queries

Get a yes/no answer instead of variable bindings.

```rust
let exists = store
    .ask("ASK { ?x a <http://example.org/Person> }")
    .await?;

if exists {
    println!("At least one person exists");
} else {
    println!("No people found");
}
```

### Use Cases

- **Data validation**: `ASK { ?x ?p ?y }` — "Is dataset non-empty?"
- **Existence checks**: `ASK { <http://example.org/alice> <http://example.org/knows> ?x }`
- **Constraints**: `ASK { ?person <http://example.org/age> ?a FILTER (?a > 100) }`

---

## CONSTRUCT Queries

Build new RDF triples from the dataset.

### Basic CONSTRUCT

```rust
let triples = store
    .construct("
        CONSTRUCT {
            ?x <http://example.org/fullName> ?name .
        }
        WHERE {
            ?x <http://example.org/firstName> ?first ;
               <http://example.org/lastName> ?last .
            BIND(CONCAT(?first, ' ', ?last) AS ?name)
        }
    ")
    .await?;

// Result is a Vec<Triple>
for triple in triples {
    println!("{} {} {}", triple.subject, triple.predicate, triple.object);
}
```

### CONSTRUCT with Multiple Patterns

```rust
let quads = store
    .construct("
        CONSTRUCT {
            ?s <http://example.org/related> ?o .
            ?s <http://example.org/type> ?type .
        }
        WHERE {
            ?s a ?type ;
               <http://example.org/knows> ?o .
        }
    ")
    .await?;
```

### Use Cases

- **Data transformation**: Normalize predicates or structure
- **Graph transformation**: Merge multiple data sources
- **Extraction**: Pull out subset of data in new structure

---

## DESCRIBE Queries

Get all information about a specific resource.

```rust
let quads = store
    .describe("DESCRIBE <http://example.org/alice>")
    .await?;

// Returns all facts about alice
for quad in quads {
    println!("{} {} {} in {}",
        quad.subject,
        quad.predicate,
        quad.object,
        quad.graph.as_ref().unwrap_or(&"default".to_string())
    );
}
```

### Multiple Resources

```rust
let quads = store
    .describe("
        DESCRIBE ?person
        WHERE {
            ?person a <http://example.org/Person> ;
                    <http://example.org/name> ?name .
            FILTER (REGEX(?name, 'Alice'))
        }
    ")
    .await?;
```

### Use Cases

- **Resource details**: Get everything about a person
- **Debugging**: Understand what properties a resource has
- **Export**: Pull complete descriptions for documentation

---

## Advanced Patterns

### Aggregation (Grouping)

```rust
let solutions = store
    .query("
        SELECT ?type (COUNT(?item) AS ?count)
        WHERE {
            ?item a ?type .
        }
        GROUP BY ?type
        ORDER BY DESC(?count)
    ")
    .await?;

for solution in solutions {
    println!("{}: {}",
        solution.get("type").unwrap(),
        solution.get("count").unwrap()
    );
}
```

### UNION (Multiple Patterns)

```rust
let solutions = store
    .query("
        SELECT ?name
        WHERE {
            {
                ?person a <http://example.org/Student> ;
                        <http://example.org/name> ?name .
            }
            UNION {
                ?person a <http://example.org/Teacher> ;
                        <http://example.org/name> ?name .
            }
        }
    ")
    .await?;
```

### Property Paths

Query chains of properties:

```rust
let solutions = store
    .query("
        SELECT ?ancestor
        WHERE {
            <http://example.org/alice> <http://example.org/parent>+ ?ancestor .
        }
    ")
    .await?;

// Matches: alice's parent, grandparent, great-grandparent, etc.
```

---

## Error Handling

Always handle query errors gracefully:

```rust
use qlever::error::Error;

match store.query("SELECT ?x WHERE { ?x ?p ?o }").await {
    Ok(solutions) => {
        println!("Got {} results", solutions.len());
    }
    Err(Error::QueryError(msg)) => {
        eprintln!("SPARQL syntax error: {}", msg);
    }
    Err(Error::ConnectionFailed(msg)) => {
        eprintln!("Server connection failed: {}", msg);
    }
    Err(e) => {
        eprintln!("Unexpected error: {}", e);
    }
}
```

---

## Performance Notes

| Query Type | Performance |
|-----------|-------------|
| SELECT (with LIMIT) | Fast (~2-5ms) |
| SELECT (no LIMIT) | Depends on result size |
| ASK | Very fast, minimizes data transfer |
| CONSTRUCT | Slower than SELECT, builds triples |
| DESCRIBE | Medium speed, depends on resource complexity |

**Tips:**
- Use `LIMIT` during development and testing
- Use `ASK` for existence checks (faster than `SELECT`)
- For large result sets, use streaming (see [Performance Guide](./performance.md))

---

## Next Steps

- **Process RDF data**: See [Parse & Manipulate RDF Data](./rdf-data.md)
- **Optimize queries**: See [Optimize Query Performance](./performance.md)
- **Handle errors**: See [Error Handling](./error-handling.md)

---

## SPARQL Resources

- [W3C SPARQL 1.1 Query Language](https://www.w3.org/TR/sparql11-query/)
- [SPARQL Tutorial](https://www.w3.org/2009/sparql/wiki/Main_Page)
- [Query Validator](https://sparql.org/query-validator) — Test syntax

---

## Quick Reference

```rust
// SELECT — get variables
let solutions = store.query("SELECT ?x WHERE { ?x ?p ?o }").await?;

// ASK — true/false
let exists = store.ask("ASK { ?x ?p ?o }").await?;

// CONSTRUCT — build triples
let triples = store.construct("CONSTRUCT { ?s ?p ?o } WHERE { ... }").await?;

// DESCRIBE — get details
let quads = store.describe("DESCRIBE ?x").await?;
```
