# SPARQL Reference

Quick reference for SPARQL query syntax and features supported by QLever.

## Basic Query Structure

```sparql
SELECT ?variable1 ?variable2
WHERE {
  ?subject ?predicate ?object .
  # More patterns...
}
LIMIT 10
```

## SELECT Clause

```sparql
# Select specific variables
SELECT ?name ?age WHERE { ... }

# Select all variables used
SELECT * WHERE { ... }

# Count results
SELECT (COUNT(*) as ?count) WHERE { ... }

# Multiple aggregates
SELECT ?category (COUNT(*) as ?count) (AVG(?value) as ?avg) WHERE { ... }

# Distinct results
SELECT DISTINCT ?category WHERE { ... }

# Order results
ORDER BY ?name                  # Ascending (default)
ORDER BY DESC(?age)             # Descending
ORDER BY ?category DESC(?age)   # Multiple sort keys

# Limit results
LIMIT 10                        # First 10 results
OFFSET 100                      # Skip first 100
```

## Triple Patterns

Basic pattern matching:

```sparql
# Variable patterns
?subject ?predicate ?object .           # All match variables

?subject wd:Q5 ?object .                # Fixed predicate
?subject ex:age "30"^^xsd:integer .     # Fixed literal value

# RDF properties
?x a wd:Q5 .                  # rdf:type (shorthand: 'a')
?x rdfs:label ?label .        # Get label
?x rdf:value ?value .         # Get value

# Multiple patterns (AND)
?x ?y ?z .
?x dc:creator ?creator .
# Both must match
```

## FILTER Expressions

Filter results:

```sparql
# Comparison operators
FILTER(?age > 30)
FILTER(?name = "Alice")
FILTER(?age != 30)
FILTER(?age <= 65)

# Logical operators
FILTER(?age > 30 && ?age < 65)        # AND
FILTER(?name = "Alice" || ?name = "Bob")  # OR
FILTER(!(?age < 18))                  # NOT

# String functions
FILTER(CONTAINS(?name, "John"))       # Contains substring
FILTER(REGEX(?name, "^John"))         # Regex match (case-sensitive)
FILTER(REGEX(?name, "john", "i"))     # Case-insensitive

# Numeric functions
FILTER(ABS(?value) > 100)
FILTER(ROUND(?value) = 42)
FILTER(?value > CEIL(100))

# Date/time functions
FILTER(YEAR(?birthDate) > 1990)
FILTER(MONTH(?date) = 1)              # January
FILTER(DAY(?date) > 15)

# Language filters
FILTER(LANG(?label) = "en")          # English only
FILTER(LANG(?label) IN ("en", "de")) # Multiple languages

# Type checks
FILTER(isLiteral(?x))                 # Is a literal value
FILTER(isIRI(?x))                     # Is a URI
FILTER(isNumeric(?x))                 # Is numeric
```

## BIND

Create new variables:

```sparql
# Create computed variable
BIND(?age + 1 as ?nextYear)

# Create calculated field
BIND(CONCAT(?firstName, " ", ?lastName) as ?fullName)

# Conditional
BIND(IF(?age > 18, "Adult", "Minor") as ?category)

# String operations
BIND(UPPER(?name) as ?nameUpper)
BIND(LOWER(?name) as ?nameLower)
BIND(SUBSTR(?name, 1, 3) as ?initials)

# Type conversion
BIND(xsd:integer(?stringValue) as ?intValue)
```

## OPTIONAL

Match patterns optionally:

```sparql
SELECT ?person ?address ?phone WHERE {
  ?person a wd:Q5 .
  ?person rdfs:label ?name .

  # These may or may not exist
  OPTIONAL { ?person schema:address ?address . }
  OPTIONAL { ?person schema:telephone ?phone . }
}
# Returns people even if address/phone missing
```

## Graph Patterns

### Basic Graph Patterns

```sparql
SELECT ?title WHERE {
  ?book wd:P50 ?author .     # book has author
  ?author rdfs:label ?name .  # author has name
  ?book rdfs:label ?title .   # book has title
  FILTER(?name = "Smith")     # Filter by name
}
```

### UNION (OR between patterns)

```sparql
SELECT ?name WHERE {
  {
    ?person ex:familyName ?name .    # Search by family name
  }
  UNION
  {
    ?person ex:givenName ?name .     # Or given name
  }
}
# Returns matches from either pattern
```

### MINUS (Set difference)

```sparql
SELECT ?person WHERE {
  ?person a wd:Q5 .                  # All people
  MINUS
  {
    ?person wdt:P31 wd:Q156955 .     # Except students
  }
}
# Returns non-students
```

## Aggregation

### GROUP BY

```sparql
SELECT ?country (COUNT(?person) as ?count) WHERE {
  ?person a wd:Q5 .
  ?person wdt:P27 ?country .
}
GROUP BY ?country
ORDER BY DESC(?count)
LIMIT 20
```

### Aggregate Functions

```sparql
SELECT
  ?category
  (COUNT(*) as ?count)
  (COUNT(DISTINCT ?author) as ?distinctAuthors)
  (AVG(?rating) as ?avgRating)
  (MIN(?year) as ?earliest)
  (MAX(?year) as ?latest)
  (SUM(?value) as ?total)
WHERE {
  ?book a wd:Q571 .
  ?book ex:category ?category .
  ?book ex:rating ?rating .
  ?book ex:year ?year .
}
GROUP BY ?category
```

## Property Paths

Match paths through the graph:

```sparql
# Direct edge
?x ex:knows ?y .

# One or more hops
?x ex:knows+ ?y .                # One or more "knows" relations

# Zero or more hops
?x ex:knows* ?y .                # Zero or more "knows" relations

# Alternatives (any of these properties)
?x (ex:knows | ex:acquaintance)+ ?y .

# Inverse direction
?book ^ex:author ?author .       # Who is author of this book?

# Complex combinations
?x (ex:knows | ex:worksWith)+ / ^ex:manages ?y .
```

## Prefixes

Define namespace shortcuts:

```sparql
PREFIX rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
PREFIX wd: <http://www.wikidata.org/entity/>
PREFIX wdt: <http://www.wikidata.org/prop/direct/>
PREFIX ex: <http://example.com/>

SELECT ?name WHERE {
  ?x a wd:Q5 .
  ?x rdfs:label ?name .
}
```

## Common Patterns

### Find by Name

```sparql
SELECT ?person WHERE {
  ?person rdfs:label ?name .
  FILTER(CONTAINS(?name, "Smith"))
  FILTER(LANG(?name) = "en")
}
```

### Count by Category

```sparql
SELECT ?type (COUNT(*) as ?count) WHERE {
  ?item a ?type .
}
GROUP BY ?type
ORDER BY DESC(?count)
```

### Range Query

```sparql
SELECT ?book ?year WHERE {
  ?book a wd:Q571 .
  ?book wdt:P577 ?date .
  BIND(YEAR(?date) as ?year)
  FILTER(?year >= 2000 && ?year <= 2010)
}
```

### Nearest Neighbor

```sparql
SELECT ?city ?distance WHERE {
  ?city geo:asWKT ?location .
  BIND(ST_Distance(?myPoint, ?location) as ?distance)
  FILTER(?distance < 50000)  # 50km
}
ORDER BY ?distance
LIMIT 10
```

## Advanced Features

### VALUES Clause

```sparql
SELECT ?person ?knows WHERE {
  VALUES ?person { wd:Q1234 wd:Q5678 wd:Q9012 }
  ?person ex:knows ?knows .
}
# Query specific people
```

### SUBQUERY

```sparql
SELECT ?person ?topCity WHERE {
  {
    SELECT ?city (COUNT(?person) as ?count) WHERE {
      ?person wdt:P19 ?city .
    }
    GROUP BY ?city
    ORDER BY DESC(?count)
    LIMIT 1
  }
  ?person wdt:P19 ?topCity .
}
```

## QLever Extensions

### Text Search

```sparql
SELECT ?item ?score WHERE {
  ?item qlever:text "query string" ?score .
}
ORDER BY DESC(?score)
```

### Spatial Functions

```sparql
SELECT ?city ?distance WHERE {
  ?city geo:asWKT ?location .
  BIND(ST_Distance(?point1, ?point2) as ?distance)
  FILTER(ST_Within(?location, ?polygon))
}
```

## Supported Datatypes

```sparql
# Numeric
"123"^^xsd:integer
"3.14"^^xsd:decimal
"3.14"^^xsd:float
"3.14"^^xsd:double

# String
"text"@en                   # With language tag
"text"@de
"text"^^xsd:string         # Explicitly typed

# Date/Time
"2024-01-15"^^xsd:date
"2024-01-15T10:30:00"^^xsd:dateTime
"10:30:00"^^xsd:time

# Boolean
"true"^^xsd:boolean
"false"^^xsd:boolean
```

## Limitations & QLever-Specific Behavior

**What QLever supports well:**
- SPARQL 1.1 SELECT queries
- Complex FILTER expressions
- GROUP BY aggregations
- OPTIONAL and UNION patterns
- Text search (proprietary)
- Spatial queries (S2 geometry)

**What QLever doesn't support:**
- SPARQL UPDATE (INSERT/DELETE)
- CONSTRUCT queries
- ASK queries
- DESCRIBE queries
- FEDERATED queries (non-standard)

---

For more details, see [Tutorial: Your First Query](../tutorials/02-first-query.md) or [Explanation: RDF & SPARQL](../explanation/rdf-sparql.md).
