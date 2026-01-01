# N3 Examples and Tutorials

This directory contains comprehensive N3 (Notation3) examples for learning and testing QLever with RDF data.

## Table of Contents

- [Overview](#overview)
- [Directory Structure](#directory-structure)
- [Tutorial Examples](#tutorial-examples)
- [Real-World Examples](#real-world-examples)
- [Advanced Examples](#advanced-examples)
- [Loading Examples into QLever](#loading-examples-into-qlever)
- [Sample SPARQL Queries](#sample-sparql-queries)
- [Additional Resources](#additional-resources)

## Overview

N3 (Notation3) is a compact and readable format for expressing RDF data. It extends Turtle with additional features like variables and rules. These examples demonstrate various N3 patterns, from basic syntax to complex real-world use cases.

**What's Included:**
- 13 comprehensive N3 example files
- Tutorial progression from basic to advanced
- Real-world use cases (FOAF, Schema.org, Linked Data, Knowledge Graphs)
- Advanced patterns (Collections, Property Lists, Blank Nodes, Language Tags)
- Sample SPARQL queries for each dataset
- Loading scripts for QLever

## Directory Structure

```
examples/
├── README-N3.md                    # This file
├── n3-queries.rq                   # Sample SPARQL queries
├── load-all-n3.sh                  # Script to load all examples
├── n3-tutorial/                    # Step-by-step tutorials
│   ├── 01-basic.n3                 # Basic N3 syntax
│   ├── 02-persons.n3               # People and relationships
│   ├── 03-locations.n3             # Geographic data
│   ├── 04-mixed-types.n3           # Various datatypes
│   └── 05-large-dataset.n3         # 1000+ triples
├── n3-real-world/                  # Real-world use cases
│   ├── foaf-data.n3                # Friend of a Friend vocabulary
│   ├── schema-org.n3               # Schema.org structured data
│   ├── linked-data.n3              # Linked data with external datasets
│   └── knowledge-graph.n3          # Academic knowledge graph
└── n3-advanced/                    # Advanced patterns
    ├── collections-and-lists.n3    # RDF collections and containers
    ├── property-lists.n3           # Property list syntax
    ├── blank-nodes.n3              # Blank node patterns
    └── language-tags.n3            # Multilingual data
```

## Tutorial Examples

### 1. Basic N3 (`n3-tutorial/01-basic.n3`)

**Purpose:** Introduction to N3 syntax fundamentals

**Key Concepts:**
- Prefix declarations (`@prefix`)
- Simple triples (subject-predicate-object)
- Typed literals (`"value"^^xsd:type`)
- Language tags (`"text"@lang`)
- Comments (`# comment`)

**Triple Count:** ~30

**Example Query:**
```sparql
# Get all people and their names
SELECT ?person ?name WHERE {
  ?person rdf:type ex:Person .
  ?person ex:name ?name .
}
```

### 2. Persons (`n3-tutorial/02-persons.n3`)

**Purpose:** Modeling people with relationships

**Key Concepts:**
- Complex person profiles
- Relationships (foaf:knows, ex:manages)
- Blank nodes for addresses
- Team structures
- FOAF vocabulary usage

**Triple Count:** ~120

**Example Query:**
```sparql
# Find all employees in Engineering department
SELECT ?person ?name ?jobTitle WHERE {
  ?person rdf:type ex:Person .
  ?person ex:name ?name .
  ?person ex:department "Engineering" .
  ?person ex:jobTitle ?jobTitle .
}
```

### 3. Locations (`n3-tutorial/03-locations.n3`)

**Purpose:** Geographic data and spatial relationships

**Key Concepts:**
- Geographic hierarchies (Country > State > City)
- Coordinates (latitude/longitude)
- Points of interest
- Climate data
- GeoNames vocabulary

**Triple Count:** ~200

**Example Query:**
```sparql
# Find all cities in California with their populations
SELECT ?city ?population WHERE {
  ?city rdf:type ex:City .
  ?city ex:state ex:california .
  ?city ex:population ?population .
}
ORDER BY DESC(?population)
```

### 4. Mixed Types (`n3-tutorial/04-mixed-types.n3`)

**Purpose:** Comprehensive datatype demonstration

**Key Concepts:**
- All XSD datatypes (string, integer, decimal, boolean, date, time, etc.)
- Language-tagged strings in multiple languages
- Date and time formats
- Binary data (base64, hex)
- Real-world examples (products, books, users)

**Triple Count:** ~200

**Example Query:**
```sparql
# Find all products in stock with price < 1000
SELECT ?product ?name ?price WHERE {
  ?product rdf:type ex:Product .
  ?product ex:name ?name .
  ?product ex:price ?price .
  ?product ex:inStock "true"^^xsd:boolean .
  FILTER(?price < 1000)
}
ORDER BY ?price
```

### 5. Large Dataset (`n3-tutorial/05-large-dataset.n3`)

**Purpose:** Performance testing with realistic e-commerce data

**Key Concepts:**
- 1000+ triples
- E-commerce domain (products, customers, orders, reviews)
- Realistic data patterns
- Performance testing scenarios

**Triple Count:** ~1500

**Example Query:**
```sparql
# Find top-rated products with most reviews
SELECT ?product ?name ?rating ?reviewCount WHERE {
  ?product rdf:type ex:Product .
  ?product ex:name ?name .
  ?product ex:rating ?rating .
  ?product ex:reviewCount ?reviewCount .
  FILTER(?rating > 4.5)
}
ORDER BY DESC(?reviewCount)
LIMIT 10
```

## Real-World Examples

### 1. FOAF Data (`n3-real-world/foaf-data.n3`)

**Purpose:** Friend of a Friend vocabulary for social networks

**Key Concepts:**
- Person profiles with FOAF properties
- Social relationships (foaf:knows)
- Online accounts and presence
- Publications and projects
- Groups and organizations

**Triple Count:** ~300

**Use Cases:**
- Social network representation
- Academic profiles
- Professional networking
- Distributed identity

**Example Query:**
```sparql
# Find all people who know each other (mutual connections)
SELECT ?person1 ?name1 ?person2 ?name2 WHERE {
  ?person1 foaf:knows ?person2 .
  ?person2 foaf:knows ?person1 .
  ?person1 foaf:name ?name1 .
  ?person2 foaf:name ?name2 .
  FILTER(str(?person1) < str(?person2))  # Avoid duplicates
}
```

### 2. Schema.org (`n3-real-world/schema-org.n3`)

**Purpose:** Structured data for search engines and SEO

**Key Concepts:**
- Organization and Person schemas
- Products and offers
- Events and venues
- Articles and blog posts
- Recipes with structured instructions
- Movies and entertainment

**Triple Count:** ~400

**Use Cases:**
- Rich snippets in search results
- E-commerce product listings
- Event promotion
- Recipe websites
- Content publishing

**Example Query:**
```sparql
# Find all events in San Francisco with their dates
SELECT ?event ?name ?startDate ?location WHERE {
  ?event rdf:type schema:Event .
  ?event schema:name ?name .
  ?event schema:startDate ?startDate .
  ?event schema:location ?loc .
  ?loc schema:address ?addr .
  ?addr schema:addressLocality "San Francisco" .
}
ORDER BY ?startDate
```

### 3. Linked Data (`n3-real-world/linked-data.n3`)

**Purpose:** Best practices for Linked Open Data

**Key Concepts:**
- External dataset linking (DBpedia, Wikidata, GeoNames)
- owl:sameAs for entity resolution
- Multiple vocabulary integration
- VoID dataset metadata
- Cross-dataset queries

**Triple Count:** ~350

**Use Cases:**
- Knowledge base integration
- Entity resolution
- Data federation
- Linked Open Data publishing

**Example Query:**
```sparql
# Find all people with their DBpedia and Wikidata links
SELECT ?person ?name ?dbpedia ?wikidata WHERE {
  ?person rdf:type foaf:Person .
  ?person foaf:name ?name .
  ?person owl:sameAs ?dbpedia .
  ?person owl:sameAs ?wikidata .
  FILTER(CONTAINS(str(?dbpedia), "dbpedia.org"))
  FILTER(CONTAINS(str(?wikidata), "wikidata.org"))
}
```

### 4. Knowledge Graph (`n3-real-world/knowledge-graph.n3`)

**Purpose:** Domain-specific academic knowledge graph

**Key Concepts:**
- Universities and departments
- Researchers with h-index and citations
- Research papers and conferences
- Grants and funding
- Patents and innovations
- Collaboration networks

**Triple Count:** ~500

**Use Cases:**
- Academic research tracking
- Collaboration discovery
- Citation analysis
- Grant management
- Research impact assessment

**Example Query:**
```sparql
# Find researchers and their collaboration network
SELECT ?researcher ?name ?coauthor ?coauthorName ?paperCount WHERE {
  ?researcher rdf:type ex:Researcher .
  ?researcher foaf:name ?name .
  ?researcher ex:coAuthor ?coauthor .
  ?coauthor foaf:name ?coauthorName .
  {
    SELECT ?researcher ?coauthor (COUNT(?paper) AS ?paperCount) WHERE {
      ?paper dcterms:creator ?researcher .
      ?paper dcterms:creator ?coauthor .
      FILTER(?researcher != ?coauthor)
    }
    GROUP BY ?researcher ?coauthor
  }
}
ORDER BY DESC(?paperCount)
```

## Advanced Examples

### 1. Collections and Lists (`n3-advanced/collections-and-lists.n3`)

**Purpose:** RDF collection patterns

**Key Concepts:**
- RDF Lists (ordered collections)
- Collection syntax with parentheses `( item1 item2 )`
- rdf:Bag (unordered collections)
- rdf:Seq (ordered sequences)
- rdf:Alt (alternatives)
- Nested collections
- Empty collections

**Triple Count:** ~400

**Example Query:**
```sparql
# Find all lists and their first item
SELECT ?list ?firstItem WHERE {
  ?list rdf:first ?firstItem .
}
```

### 2. Property Lists (`n3-advanced/property-lists.n3`)

**Purpose:** Syntactic sugar for readable N3

**Key Concepts:**
- Semicolon separator for multiple predicates
- Comma separator for multiple objects
- Blank node property lists `[ ]`
- Nested property lists
- Formatting best practices

**Triple Count:** ~500

**Example Query:**
```sparql
# Find products with multiple colors
SELECT ?product ?name (GROUP_CONCAT(?color; separator=", ") AS ?colors) WHERE {
  ?product rdf:type ex:Product .
  ?product ex:name ?name .
  ?product ex:color ?color .
}
GROUP BY ?product ?name
```

### 3. Blank Nodes (`n3-advanced/blank-nodes.n3`)

**Purpose:** Blank node usage patterns

**Key Concepts:**
- Named blank nodes (`_:label`)
- Anonymous blank nodes (`[ ]`)
- Blank node reuse
- N-ary relations
- Reification patterns
- Provenance information
- When to use blank nodes vs named nodes

**Triple Count:** ~350

**Example Query:**
```sparql
# Find all statements with provenance
SELECT ?subject ?predicate ?object ?source ?timestamp WHERE {
  ?statement rdf:type rdf:Statement .
  ?statement rdf:subject ?subject .
  ?statement rdf:predicate ?predicate .
  ?statement rdf:object ?object .
  ?statement ex:source ?source .
  ?statement ex:timestamp ?timestamp .
}
```

### 4. Language Tags (`n3-advanced/language-tags.n3`)

**Purpose:** Multilingual data representation

**Key Concepts:**
- Language tags (en, fr, es, de, etc.)
- Regional variants (en-US, en-GB, zh-CN, etc.)
- Multiple scripts (Latin, Cyrillic, Chinese, Arabic, etc.)
- Multilingual content (products, articles, UI)
- Best practices for i18n

**Triple Count:** ~300

**Example Query:**
```sparql
# Get product names in all available languages
SELECT ?product ?name ?lang WHERE {
  ?product rdf:type ex:Product .
  ?product ex:name ?name .
  BIND(LANG(?name) AS ?lang)
}
ORDER BY ?product ?lang
```

## Loading Examples into QLever

### Quick Start

Load all examples at once:
```bash
cd examples
./load-all-n3.sh
```

### Load Individual Files

Using QLever's IndexBuilderMain:

```bash
# Create index from N3 file
IndexBuilderMain -i my-index -F ttl -f n3-tutorial/01-basic.n3

# Create index from multiple files
IndexBuilderMain -i combined-index -F ttl -f "n3-tutorial/*.n3"

# Create index with settings
IndexBuilderMain -i my-index -F ttl -f n3-real-world/foaf-data.n3 \
  -s settings.json
```

### Using QLever Server

After building the index:

```bash
# Start QLever server
ServerMain -i my-index -p 7001

# Query via HTTP
curl -X POST http://localhost:7001/ \
  -H "Content-Type: application/sparql-query" \
  --data "SELECT * WHERE { ?s ?p ?o } LIMIT 10"
```

## Sample SPARQL Queries

See `n3-queries.rq` for comprehensive query examples for each dataset.

### Basic Queries

**Count all triples:**
```sparql
SELECT (COUNT(*) AS ?count) WHERE {
  ?s ?p ?o .
}
```

**Find all classes:**
```sparql
SELECT DISTINCT ?class WHERE {
  ?s rdf:type ?class .
}
ORDER BY ?class
```

**Find all predicates:**
```sparql
SELECT DISTINCT ?predicate WHERE {
  ?s ?predicate ?o .
}
ORDER BY ?predicate
```

### Intermediate Queries

**Filter by datatype:**
```sparql
SELECT ?s ?o WHERE {
  ?s ?p ?o .
  FILTER(DATATYPE(?o) = xsd:decimal)
}
```

**Language filtering:**
```sparql
SELECT ?subject ?text WHERE {
  ?subject rdfs:label ?text .
  FILTER(LANG(?text) = "en")
}
```

**Date range queries:**
```sparql
SELECT ?event ?name ?date WHERE {
  ?event rdf:type ex:Event .
  ?event ex:name ?name .
  ?event ex:date ?date .
  FILTER(?date >= "2024-01-01"^^xsd:date && ?date <= "2024-12-31"^^xsd:date)
}
```

### Advanced Queries

**Property paths:**
```sparql
# Find all people connected within 2 hops
SELECT ?person1 ?person2 WHERE {
  ?person1 foaf:knows/foaf:knows ?person2 .
}
```

**Aggregation:**
```sparql
# Average rating by category
SELECT ?category (AVG(?rating) AS ?avgRating) WHERE {
  ?product ex:category ?category .
  ?product ex:rating ?rating .
}
GROUP BY ?category
ORDER BY DESC(?avgRating)
```

**Optional patterns:**
```sparql
# Get people with optional email
SELECT ?person ?name ?email WHERE {
  ?person rdf:type ex:Person .
  ?person ex:name ?name .
  OPTIONAL { ?person ex:email ?email }
}
```

**UNION queries:**
```sparql
# Get all organization names (companies or universities)
SELECT ?org ?name WHERE {
  {
    ?org rdf:type ex:Company .
    ?org ex:companyName ?name .
  } UNION {
    ?org rdf:type ex:University .
    ?org foaf:name ?name .
  }
}
```

## Dataset Statistics

| Dataset | Triples (approx) | Entities | Complexity |
|---------|------------------|----------|------------|
| 01-basic.n3 | 30 | 8 | Beginner |
| 02-persons.n3 | 120 | 15 | Beginner |
| 03-locations.n3 | 200 | 25 | Intermediate |
| 04-mixed-types.n3 | 200 | 20 | Intermediate |
| 05-large-dataset.n3 | 1500 | 200+ | Advanced |
| foaf-data.n3 | 300 | 10 | Intermediate |
| schema-org.n3 | 400 | 20 | Intermediate |
| linked-data.n3 | 350 | 30 | Advanced |
| knowledge-graph.n3 | 500 | 40 | Advanced |
| collections-and-lists.n3 | 400 | 30 | Advanced |
| property-lists.n3 | 500 | 25 | Intermediate |
| blank-nodes.n3 | 350 | 30 | Advanced |
| language-tags.n3 | 300 | 20 | Intermediate |
| **TOTAL** | **~5150** | **~500** | - |

## Expected Query Results

### Tutorial Examples

**01-basic.n3 - Basic queries:**
- 8 persons/entities
- 5 different entity types
- Language-tagged strings in 3+ languages

**02-persons.n3 - Relationship queries:**
- 6 people with complete profiles
- 3 teams
- Multiple foaf:knows relationships
- Salary range: $35k - $105k

**03-locations.n3 - Geographic queries:**
- 4 countries
- 3 states
- 11 cities
- 7 points of interest

**04-mixed-types.n3 - Datatype queries:**
- 10+ different XSD datatypes
- Multilingual labels (8+ languages)
- Date range: historical to future dates

**05-large-dataset.n3 - Performance queries:**
- 200+ products across 5 categories
- 30+ customers with membership levels
- 25+ orders with items
- 25+ reviews with ratings

## Commands Cheat Sheet

**Build index from N3:**
```bash
IndexBuilderMain -i myindex -F ttl -f path/to/file.n3
```

**Start server:**
```bash
ServerMain -i myindex -p 7001
```

**Query via curl:**
```bash
curl -X POST http://localhost:7001/ \
  -H "Accept: application/sparql-results+json" \
  -H "Content-Type: application/sparql-query" \
  --data "SELECT * WHERE { ?s ?p ?o } LIMIT 10"
```

**Count triples:**
```bash
curl -X POST http://localhost:7001/ \
  -H "Content-Type: application/sparql-query" \
  --data "SELECT (COUNT(*) AS ?count) WHERE { ?s ?p ?o }"
```

## Validation and Testing

**Validate N3 syntax:**
```bash
# Using rapper (from raptor2-utils)
rapper -i turtle -o ntriples file.n3 > /dev/null

# Using riot (from Apache Jena)
riot --validate file.n3
```

**Check for common issues:**
```bash
# Check for undefined prefixes
grep -n "@prefix" file.n3

# Check for unclosed brackets
grep -c "\[" file.n3
grep -c "\]" file.n3

# Check for balanced parentheses
grep -o "(" file.n3 | wc -l
grep -o ")" file.n3 | wc -l
```

## Additional Resources

### N3 and Turtle Documentation
- [W3C Turtle Specification](https://www.w3.org/TR/turtle/)
- [N3 Notation Primer](https://www.w3.org/TeamSubmission/n3/)
- [RDF 1.1 Primer](https://www.w3.org/TR/rdf11-primer/)

### SPARQL Resources
- [SPARQL 1.1 Query Language](https://www.w3.org/TR/sparql11-query/)
- [SPARQL Examples](https://www.w3.org/TR/sparql11-query/#examples)
- [QLever Documentation](https://github.com/ad-freiburg/qlever)

### Vocabularies Used
- [FOAF Vocabulary](http://xmlns.com/foaf/spec/)
- [Schema.org](https://schema.org/)
- [Dublin Core](https://www.dublincore.org/specifications/dublin-core/dcmi-terms/)
- [SKOS](https://www.w3.org/2004/02/skos/)

### Tools
- **Visualization:** [WebVOWL](http://vowl.visualdataweb.org/webvowl.html)
- **Validation:** [RDF Validator](http://www.w3.org/RDF/Validator/)
- **Conversion:** [RDF Translator](http://rdf-translator.appspot.com/)
- **SPARQL IDE:** [Yasgui](https://yasgui.triply.cc/)

## Troubleshooting

**Issue: "Undefined prefix" error**
- Check all `@prefix` declarations at the top of the file
- Ensure prefixes match exactly (case-sensitive)

**Issue: "Syntax error" when loading**
- Validate N3 syntax with rapper or riot
- Check for unbalanced brackets, parentheses
- Ensure all triples end with a period

**Issue: "No results" from query**
- Verify index was built successfully
- Check that query prefixes match data prefixes
- Try simpler query first: `SELECT * WHERE { ?s ?p ?o } LIMIT 10`

**Issue: Performance problems with large datasets**
- Build index with appropriate settings
- Use LIMIT in queries during testing
- Consider index optimization options

## Contributing

To add new examples:

1. Choose the appropriate directory (tutorial/real-world/advanced)
2. Follow existing naming conventions
3. Include comprehensive comments explaining concepts
4. Add realistic, meaningful data
5. Update this README with dataset info
6. Add sample queries to `n3-queries.rq`
7. Test loading and querying before committing

## License

These examples are provided as part of the QLever project and follow the same license (Apache 2.0).

---

**Last Updated:** 2026-01-01
**Version:** 1.0
**Total Files:** 13 N3 files
**Total Triples:** ~5,150
