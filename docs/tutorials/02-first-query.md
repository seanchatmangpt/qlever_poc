# Your First SPARQL Query

Learn to write SPARQL queries by exploring a real dataset. No query language experience needed.

## What You'll Learn

- The basic structure of a SPARQL query
- How to find things in the database
- How to filter and limit results

## Prerequisites

- Completed [Quick Start](./01-quickstart.md)
- QLever server running with a dataset loaded

## The Simplest Query

Every SPARQL query answers a question. Let's start simple:

**Question:** What are some books in the database?

```sparql
SELECT ?book WHERE {
  ?book a wd:Q571 .
}
LIMIT 5
```

**Breaking it down:**
- `SELECT ?book` — Show me the book results
- `WHERE { ... }` — Here are the conditions:
  - `?book a wd:Q571` — A book (wd:Q571 is the Wikidata ID for "book")
  - `.` — End of condition
- `LIMIT 5` — Only show 5 results

**Try it:** Paste this into http://localhost:7023/gui and click "Query"

## Getting More Information

A single identifier isn't very useful. Let's also get names:

```sparql
SELECT ?book ?name WHERE {
  ?book a wd:Q571 .           # It's a book
  ?book rdfs:label ?name .    # Get its name
  FILTER(LANG(?name) = "en")  # Only English names
}
LIMIT 10
```

**What's new:**
- `?name` — A second variable to return
- `rdfs:label` — This is a standard property meaning "name"
- `FILTER(LANG(?name) = "en")` — Only English language labels
- Comments with `#` explain each line

## Following Relationships

Now let's find books *and* who wrote them:

```sparql
SELECT ?book ?title ?author ?authorName WHERE {
  ?book a wd:Q571 .                      # It's a book
  ?book rdfs:label ?title .              # Get its title
  FILTER(LANG(?title) = "en")

  ?book wdt:P50 ?author .                # Who wrote it? (P50 = author)
  ?author rdfs:label ?authorName .       # Get author's name
  FILTER(LANG(?authorName) = "en")
}
LIMIT 5
```

**Key concepts:**
- `wdt:P50` — This is a Wikidata property ID for "author"
- Multiple lines create a chain: book → has author → author has name
- Each `FILTER` applies independently

## Asking "How Many?"

Count results instead of listing them:

```sparql
SELECT (COUNT(?book) as ?total) WHERE {
  ?book a wd:Q571 .  # Count all books
}
```

Or count by category:

```sparql
SELECT ?author (COUNT(?book) as ?bookCount) WHERE {
  ?book a wd:Q571 .
  ?book wdt:P50 ?author .
  ?author rdfs:label ?name .
  FILTER(LANG(?name) = "en")
}
GROUP BY ?author
ORDER BY DESC(?bookCount)
LIMIT 20
```

**New concepts:**
- `COUNT(...)` — Count matching results
- `GROUP BY ?author` — Group results by author
- `ORDER BY DESC(...)` — Sort descending (largest first)

## Filtering Results

Only show books published after 2010:

```sparql
SELECT ?book ?title ?year WHERE {
  ?book a wd:Q571 .
  ?book rdfs:label ?title .
  FILTER(LANG(?title) = "en")

  ?book wdt:P577 ?pubDate .     # Publication date (P577)
  BIND(YEAR(?pubDate) as ?year) # Extract the year
  FILTER(?year > 2010)           # Only recent books
}
LIMIT 20
```

**New concepts:**
- `BIND(... as ?variable)` — Create a new variable from a calculation
- `YEAR(...)` — Extract year from a date
- Multiple `FILTER` conditions work together

## Optional Information

What if not all books have authors? Make it optional:

```sparql
SELECT ?book ?title ?authorName WHERE {
  ?book a wd:Q571 .
  ?book rdfs:label ?title .
  FILTER(LANG(?title) = "en")

  OPTIONAL {
    ?book wdt:P50 ?author .
    ?author rdfs:label ?authorName .
    FILTER(LANG(?authorName) = "en")
  }
}
LIMIT 20
```

**Key concept:**
- `OPTIONAL { ... }` — This section might not match, and that's OK
- If no author exists, `?authorName` will be empty but the query still returns the book

## Common Pitfall: Variable Names

Variables in SPARQL start with `?`:
- `?book` is a variable
- `wd:Q571` is NOT (it's a fixed identifier)
- `wdt:P50` is NOT (it's a fixed property)

Variables are placeholders; use any name that makes sense:
```sparql
# This is the same query with different variable names:
SELECT ?x ?y WHERE {
  ?x a wd:Q571 .
  ?x rdfs:label ?y .
}
```

## Discovering What's Available

Use this "kitchen sink" query to explore what properties exist:

```sparql
SELECT DISTINCT ?predicate (COUNT(*) as ?count) WHERE {
  ?subject ?predicate ?object .
}
ORDER BY DESC(?count)
LIMIT 50
```

This shows all relationships in your dataset, sorted by frequency.

## Next Steps

- **Performance Issues?** → [How-to: Optimize Performance](../how-to/performance.md)
- **Text Search?** → [How-to: Text Search](../how-to/text-search.md)
- **Geographic Data?** → [How-to: Spatial Queries](../how-to/spatial-queries.md)
- **Your Own Data?** → [Load Your Data](./03-load-data.md)

---

**Pro Tip:** QLever's web interface at http://localhost:7023/gui has autocompletion to help you discover properties and values. Start typing `wd:` and it will suggest available entities!
