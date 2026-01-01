# Understanding RDF & SPARQL

Foundational concepts you need to understand QLever.

## What is RDF?

RDF (Resource Description Framework) is a way to describe things and their relationships. Everything is expressed as **triples**:

```
Subject --- Predicate ---> Object
```

For example:
```
Alice --- knows ---> Bob
Bob --- knows ---> Carol
Alice --- age ---> 30
```

### Real Example: A Book

In RDF:
```
ex:book123 --- rdf:type ---> wd:Q571          (it's a book)
ex:book123 --- rdfs:label ---> "The Great Gatsby"@en
ex:book123 --- ex:author ---> ex:fitzgerald   (who wrote it)
ex:book123 --- ex:year ---> 1925
ex:fitzgerald --- rdfs:label ---> "F. Scott Fitzgerald"@en
```

### Why RDF?

**Flexibility:** Add any relationship without schema changes
- SQL requires predefined columns
- RDF just adds triples

**Compatibility:** Different systems can share data
- Everyone uses the same format
- Public vocabularies (Wikidata, DBpedia, etc.)

**Semantic Web:** Machines understand meaning
- Predicates have defined meaning (rdfs:label means "name")
- Systems can reason about data

## Key RDF Concepts

### IRIs (Identifiers)

Internationalized Resource Identifiers uniquely identify things:
- Full form: `http://example.com/book/123`
- Prefixed: `ex:book123` (when `ex:` = `http://example.com/`)
- Standard IRIs:
  - `rdf:type` — "is a"
  - `rdfs:label` — "name"
  - `http://www.wikidata.org/entity/Q5` — "human"

### Literals

Values: numbers, strings, dates, etc.

```
"Alice"                                    # String
"Alice"@en                                 # English string
"30"^^xsd:integer                         # Integer
"3.14"^^xsd:float                         # Float
"2024-01-15"^^xsd:date                    # Date
```

### Blank Nodes

Anonymous resources (less common):
```
_:b1 --- ex:name ---> "John"
_:b1 --- ex:age ---> 30
```

## What is SPARQL?

SPARQL is a query language for RDF. Like SQL for databases, but for RDF graphs.

### Basic Concept

**Describe what you're looking for using patterns:**

```sparql
SELECT ?name ?age WHERE {
  ?person rdfs:label ?name .
  ?person ex:age ?age .
}
```

"Give me all (?person, ?name, ?age) triples where:
- Something is labeled with ?name
- That same something has age ?age"

## Graph Thinking

RDF is fundamentally a **graph** (network of nodes and edges):

```
        knows
   Alice ←→ Bob
    ↓         ↓
   age=30   age=25
```

SPARQL queries follow paths through this graph:

```sparql
SELECT ?friend ?age WHERE {
  ?person ex:knows ?friend .      # Step 1: Follow "knows" edge
  ?friend ex:age ?age .           # Step 2: Get their age
}
```

## Common Vocabulary Standards

### RDF Core

```
rdf:type        — "is a"
rdf:value       — "has value"
rdf:label       — (actually rdfs:label)
```

### RDFS (RDF Schema)

```
rdfs:label      — Human-readable name
rdfs:comment    — Description
rdfs:subClassOf — Hierarchy
rdfs:domain     — What can have this property
rdfs:range      — What the property points to
```

### Dublin Core

Metadata vocabulary:
```
dc:title        — Title
dc:creator      — Creator/author
dc:date         — Date
dc:description  — Description
```

### Wikidata (wd: wdt:)

The most comprehensive open knowledge base:
```
wd:Q5           — human
wd:Q571         — book
wd:Q20847       — politician
wdt:P31         — instance of (rdf:type)
wdt:P50         — author
wdt:P19         — birthplace
wdt:P27         — nationality
```

### Schema.org

Web-standard vocabulary:
```
schema:name     — Name
schema:Person   — A person
schema:Book     — A book
schema:author   — Author relationship
```

## Data Formats

RDF can be written in different formats:

### Turtle (.ttl) — Human-readable

```turtle
@prefix ex: <http://example.com/> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .

ex:alice foaf:name "Alice" ;
         foaf:age 30 ;
         foaf:knows ex:bob .

ex:bob foaf:name "Bob" ;
       foaf:age 25 .
```

### N-Triples (.nt) — Simple, machine-friendly

```
<http://example.com/alice> <http://xmlns.com/foaf/0.1/name> "Alice" .
<http://example.com/alice> <http://xmlns.com/foaf/0.1/age> "30"^^<http://www.w3.org/2001/XMLSchema#integer> .
```

### JSON-LD — JSON with RDF semantics

```json
{
  "@context": {
    "name": "http://xmlns.com/foaf/0.1/name",
    "age": "http://xmlns.com/foaf/0.1/age"
  },
  "name": "Alice",
  "age": 30
}
```

## How SPARQL Works

### Pattern Matching

```sparql
SELECT ?name WHERE {
  ?person rdfs:label ?name .
  FILTER(CONTAINS(?name, "John"))
}
```

"Find all values of ?name where:
- There exists a triple (?person, rdfs:label, ?name)
- AND that value contains "John""

### Multiple Patterns (AND)

```sparql
SELECT ?author WHERE {
  ?book a wd:Q571 .              # It's a book
  ?book wdt:P50 ?author .        # Has an author
  ?author wdt:P569 ?birthDate .  # Author has birth date
  FILTER(YEAR(?birthDate) > 1900)
}
```

Patterns must ALL be satisfied. Like:
- Find all X where:
  - X is a book, AND
  - X has an author, AND
  - That author was born after 1900

### Optional (left-outer join)

```sparql
SELECT ?book ?title ?author WHERE {
  ?book a wd:Q571 .
  ?book rdfs:label ?title .
  OPTIONAL { ?book wdt:P50 ?author . }
}
```

"Find books with titles. Try to find authors too, but it's OK if you don't."

### Aggregation (GROUP BY)

```sparql
SELECT ?author (COUNT(?book) as ?count) WHERE {
  ?book wdt:P50 ?author .
}
GROUP BY ?author
```

"For each author, count how many books they wrote"

## Comparing to SQL

| Concept | SQL | SPARQL |
|---------|-----|--------|
| Query | SELECT ... FROM ... WHERE | SELECT ... WHERE |
| Table | TABLE | RDF graph (implicit) |
| Row | ROW | Triple matching |
| Column | COLUMN | Variable (?name) |
| JOIN | Explicit join condition | Pattern matching on variables |
| Foreign key | PRIMARY KEY / FOREIGN KEY | Just another triple |

**SQL (rigid schema):**
```sql
SELECT author.name, COUNT(*) as book_count
FROM books
JOIN authors ON books.author_id = authors.id
GROUP BY author.id
```

**SPARQL (flexible, schema-less):**
```sparql
SELECT ?author (COUNT(?book) as ?count) WHERE {
  ?book wdt:P50 ?author .
  ?author rdfs:label ?author_name .
}
GROUP BY ?author ?author_name
```

## Why This Matters for QLever

QLever is optimized for:

1. **Graph Navigation** — Following relationships efficiently
2. **Flexible Data** — No rigid schema required
3. **Semantic Search** — Understanding meaning (ex:author, wdt:P50)
4. **Large Datasets** — Billions of triples on commodity hardware
5. **Public Data** — Works with Wikidata, DBpedia, OpenStreetMap

## Next Steps

- **Write Queries** → [Tutorial: Your First Query](../tutorials/02-first-query.md)
- **Query Syntax** → [Reference: SPARQL](../reference/sparql.md)
- **System Architecture** → [Explanation: How QLever Works](./architecture.md)

---

**Key Takeaway:** RDF is a flexible, graph-based way to represent data. SPARQL is a query language that matches patterns in that graph. QLever makes SPARQL queries fast on huge datasets.
