# Parse & Manipulate RDF Data

Working with RDF terms, triples, and quads.

> ⏱️ **20 minutes** | 🎯 **Intermediate** | 📦 **RDF 1.1**

---

## TL;DR {#tldr}

```rust
use qlever::model::*;

// Create Named Node (IRI)
let node = NamedNode::new("http://example.org/alice")?;

// Create Literal (various types)
let literal_simple = Literal::new_simple("hello");
let literal_typed = Literal::new_typed("42", "http://www.w3.org/2001/XMLSchema#integer")?;
let literal_lang = Literal::new_language_tagged("Hola", "es")?;

// Create Triple (subject, predicate, object)
let triple = Triple::new(
    node.clone(),
    NamedNode::new("http://example.org/knows")?,
    Term::Literal(literal_simple),
);

// Create Quad (triple + optional graph)
let quad = Quad::new(
    node.clone(),
    NamedNode::new("http://example.org/knows")?,
    Term::Literal(literal_simple),
    Some(NamedNode::new("http://example.org/graph")?),
);
```

---

## RDF Data Model

RDF represents knowledge as **triples**: `(subject, predicate, object)`

```
Subject:          Object:
NamedNode        NamedNode
BlankNode        BlankNode
               Literal

Predicate:
NamedNode only
```

---

## Named Nodes (IRIs)

Represent entities and properties.

```rust
use qlever::model::NamedNode;

// Create Named Node
let alice = NamedNode::new("http://example.org/alice")?;
let knows = NamedNode::new("http://example.org/knows")?;

// Common namespaces
let foaf_name = NamedNode::new("http://xmlns.com/foaf/0.1/name")?;
let rdf_type = NamedNode::new("http://www.w3.org/1999/02/22-rdf-syntax-ns#type")?;
let xsd_integer = NamedNode::new("http://www.w3.org/2001/XMLSchema#integer")?;

// Get the IRI string
let iri_str = alice.iri();
```

---

## Blank Nodes

Represent anonymous resources (no IRI).

```rust
use qlever::model::BlankNode;

// Create blank node
let anon = BlankNode::new("b1")?;
let anon2 = BlankNode::new("ref123")?;

// Get the ID
let id = anon.id();
```

**Use cases:**
- Intermediate nodes in complex structures
- Lists (RDF collections)
- Anonymous entities

---

## Literals

Represent values: strings, numbers, dates, etc.

### Simple Literals (strings)

```rust
use qlever::model::Literal;

let name = Literal::new_simple("Alice");
let name2 = Literal::new_simple("Bob");

// Get value
let value = name.value();
```

### Typed Literals

```rust
// Integer
let age = Literal::new_typed("30", "http://www.w3.org/2001/XMLSchema#integer")?;

// String (explicit)
let title = Literal::new_typed("CEO", "http://www.w3.org/2001/XMLSchema#string")?;

// Boolean
let active = Literal::new_typed("true", "http://www.w3.org/2001/XMLSchema#boolean")?;

// Date
let birth = Literal::new_typed("1990-01-15", "http://www.w3.org/2001/XMLSchema#date")?;
```

### Language-Tagged Literals

```rust
// For human-readable text in different languages
let name_en = Literal::new_language_tagged("Alice", "en")?;
let name_es = Literal::new_language_tagged("Alicia", "es")?;
let name_de = Literal::new_language_tagged("Alicia", "de")?;

// Get language tag
let lang = name_en.language();  // Some("en")
```

---

## Terms

`Term` is an enum representing any RDF value:

```rust
use qlever::model::{Term, NamedNode, BlankNode, Literal};

// Create terms
let subject = Term::NamedNode(NamedNode::new("http://example.org/alice")?);
let object = Term::Literal(Literal::new_simple("hello"));
let blank = Term::BlankNode(BlankNode::new("b1")?);

// Match on term type
match term {
    Term::NamedNode(node) => println!("IRI: {}", node.iri()),
    Term::BlankNode(blank) => println!("Blank node: {}", blank.id()),
    Term::Literal(lit) => println!("Literal: {}", lit.value()),
}
```

---

## Triples

Core RDF structure: subject-predicate-object.

```rust
use qlever::model::*;

let alice = NamedNode::new("http://example.org/alice")?;
let knows = NamedNode::new("http://example.org/knows")?;
let bob = NamedNode::new("http://example.org/bob")?;

// Create triple
let triple = Triple::new(
    Term::NamedNode(alice),
    Term::NamedNode(knows),
    Term::NamedNode(bob),
);

// Access components
let subject = triple.subject();
let predicate = triple.predicate();
let object = triple.object();

println!("{} {} {}", subject, predicate, object);
```

---

## Quads

Triple with optional graph name (named graphs).

```rust
use qlever::model::*;

let subject = NamedNode::new("http://example.org/alice")?;
let predicate = NamedNode::new("http://example.org/knows")?;
let object = NamedNode::new("http://example.org/bob")?;
let graph = Some(NamedNode::new("http://example.org/social-graph")?);

// Create quad
let quad = Quad::new(
    Term::NamedNode(subject),
    Term::NamedNode(predicate),
    Term::NamedNode(object),
    graph,
);

// Access components
let s = quad.subject();
let p = quad.predicate();
let o = quad.object();
let g = quad.graph();
```

---

## Working with Query Results

Convert query results to RDF terms:

```rust
// From SPARQL query
let solutions = store.query("
    SELECT ?person ?name ?age
    WHERE {
        ?person foaf:name ?name ;
                ex:age ?age .
    }
").await?;

for solution in solutions {
    // Get string values
    let person_str = solution.get("person")?;
    let name = solution.get("name")?;
    let age_str = solution.get("age")?;

    // Parse as NamedNode if it's an IRI
    if let Ok(person_node) = NamedNode::new(person_str) {
        println!("Person: {}", person_node.iri());
    }

    // Parse age as integer
    if let Ok(age) = age_str.parse::<i32>() {
        println!("Age: {}", age);
    }
}
```

---

## Building RDF Data Programmatically

Construct data without SPARQL:

```rust
use qlever::model::*;

// Create some data
let alice = NamedNode::new("http://example.org/alice")?;
let bob = NamedNode::new("http://example.org/bob")?;
let knows = NamedNode::new("http://example.org/knows")?;
let foaf_name = NamedNode::new("http://xmlns.com/foaf/0.1/name")?;

// Build triples
let mut triples = Vec::new();

// Alice knows Bob
triples.push(Triple::new(
    Term::NamedNode(alice.clone()),
    Term::NamedNode(knows.clone()),
    Term::NamedNode(bob.clone()),
));

// Alice has name "Alice"
triples.push(Triple::new(
    Term::NamedNode(alice.clone()),
    Term::NamedNode(foaf_name.clone()),
    Term::Literal(Literal::new_simple("Alice")),
));

// Bob has name "Bob"
triples.push(Triple::new(
    Term::NamedNode(bob.clone()),
    Term::NamedNode(foaf_name.clone()),
    Term::Literal(Literal::new_simple("Bob")),
));

// Use triples...
for triple in &triples {
    println!("{} {} {}", triple.subject(), triple.predicate(), triple.object());
}
```

---

## RDF Lists

Represent ordered sequences in RDF:

```rust
// RDF represents lists as chains with rdf:first and rdf:rest
// <http://example.org/list>  rdf:first "a" ;
//                            rdf:rest  <http://example.org/list2> .
// <http://example.org/list2> rdf:first "b" ;
//                            rdf:rest  <http://example.org/list3> .
// <http://example.org/list3> rdf:first "c" ;
//                            rdf:rest  rdf:nil .

let rdf_first = NamedNode::new("http://www.w3.org/1999/02/22-rdf-syntax-ns#first")?;
let rdf_rest = NamedNode::new("http://www.w3.org/1999/02/22-rdf-syntax-ns#rest")?;
let rdf_nil = NamedNode::new("http://www.w3.org/1999/02/22-rdf-syntax-ns#nil")?;

// To work with lists, you need to traverse the chain manually
// This is typically done in SPARQL queries rather than Rust code
```

---

## Common RDF Patterns

### Property Types (Classes)

```sparql
SELECT ?person WHERE {
    ?person a foaf:Person .  -- a = rdf:type
}
```

### Property Values

```sparql
SELECT ?name WHERE {
    ?x foaf:name ?name .
}
```

### Relationships

```sparql
SELECT ?friend WHERE {
    ?person foaf:knows ?friend .
}
```

---

## Best Practices

### ✅ DO

- **Use Named Nodes for IRIs** — They validate IRI format
- **Use Literals for values** — Preserve type information
- **Use language tags** — For human-readable text in multiple languages
- **Document your vocabulary** — Comment on custom predicates
- **Use existing vocabularies** — FOAF, DCAT, Schema.org, etc.

### ❌ DON'T

- **Use Blank Nodes unnecessarily** — They're hard to reference
- **Lose type information** — Use typed literals, not string literals
- **Forget language tags** — For multilingual content
- **Invent predicates** — Use established vocabularies first

---

## Common RDF Vocabularies

| Vocab | Namespace | Use |
|-------|-----------|-----|
| **FOAF** | `http://xmlns.com/foaf/0.1/` | People and relationships |
| **DCAT** | `http://www.w3.org/ns/dcat#` | Datasets and catalogs |
| **Schema.org** | `https://schema.org/` | Structured web data |
| **Dublin Core** | `http://purl.org/dc/terms/` | Metadata (title, date, etc.) |
| **RDF** | `http://www.w3.org/1999/02/22-rdf-syntax-ns#` | RDF language (type, list, etc.) |
| **RDFS** | `http://www.w3.org/2000/01/rdf-schema#` | RDF Schema (subclass, etc.) |
| **OWL** | `http://www.w3.org/2002/07/owl#` | Ontology language |

---

## See Also

- **[Query Types Guide](./query-types.md)** — CONSTRUCT queries build RDF data
- **[Quick Reference](../QUICKREF.md)** — Code snippets
- **[API Reference](../reference/api.md)** — RDF type documentation
- **[SPARQL Guide](./query-types.md)** — SPARQL patterns for working with RDF
- **[RDF 1.1 Concepts](https://www.w3.org/TR/rdf11-concepts/)** — Official RDF spec

---

**Status:** Reference guide | **Last Updated:** 2025-01-01
