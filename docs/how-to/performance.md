# How to: Optimize Query Performance

Speed up slow queries. Most performance gains come from query structure and index configuration.

## 80/20 Principle: What Actually Matters

**80% of performance gains come from:**
1. **Join order** — Process filters first, join strategically
2. **Memory allocation** — Give the server enough memory
3. **Index permutations** — Store data in the right order

Everything else is 20% gains.

## Quick Diagnosis

First, understand what's slow:

```bash
# See how long your query takes
qlever query --show-timing "SELECT ?x WHERE { ?x a ?type }"
```

Output shows:
- **Parse time** — Negligible (~1ms)
- **Execute time** — THE IMPORTANT PART
- **Result transfer** — Usually negligible

Focus on reducing execute time.

## Rule 1: Filter Early

**Slow:** Get all triples, then filter
```sparql
SELECT ?book ?author WHERE {
  ?book ?p ?o .        # Get everything
  ?author ?p2 ?o2 .
  FILTER(?p = rdfs:label)  # Filter later
}
```

**Fast:** Filter while searching
```sparql
SELECT ?book ?author WHERE {
  ?book rdfs:label ?title .  # Already filtering
  ?author rdfs:label ?name .
  FILTER(LANG(?name) = "en")
}
```

**Why?** QLever processes filters first. Fewer rows = faster joins.

## Rule 2: Provide Constraints

The more specific you are, the faster the query:

```sparql
# Slow: ?book could be anything
SELECT ?author WHERE {
  ?book wdt:P50 ?author .
}

# Faster: ?book is specifically a book
SELECT ?author WHERE {
  ?book a wd:Q571 .
  ?book wdt:P50 ?author .
}
```

## Rule 3: Check Your Join Order

QLever automatically optimizes, but complex queries need help:

```sparql
# Let QLever understand which is most selective
SELECT ?person ?country WHERE {
  # Most selective first (fewest matches)
  ?person wdt:P31 wd:Q5 .      # Is a human (1B matches)
  ?person wdt:P27 ?country .   # Has nationality (fewer)
  ?country a wd:Q6256 .        # Is a country (~250 matches)
}
```

QLever will optimize automatically, but being strategic helps.

## Rule 4: Memory Allocation

If queries are slow or timeout, you likely need more memory:

```bash
# Allocate more memory for the server
qlever start --memory 32GB

# Or in Qleverfile:
server:
  memory_limit: 32GB
```

How much do you need?
- Small datasets (<100M triples): 4-8GB
- Medium datasets (100M-1B): 16-32GB
- Large datasets (>1B): 32GB+

Check actual usage:
```bash
# While server is running in another terminal
qlever status
# Shows: Memory: 8.2 GB / 32 GB
```

If hovering near your limit, increase it.

## Rule 5: Index Configuration

The right index permutations make huge differences:

```yaml
# Qleverfile: Create specific permutations for your queries
index:
  permutations: [SPO, PSO, OSP]  # Optimize for your access patterns
```

**When to use which:**
- `SPO` (Subject-Predicate-Object) — Default, good for most queries
- `PSO` — Good if you frequently filter by predicate first
- `OSP` — Good for reverse lookups (find subjects with property X)

**Default (auto-optimize):**
```yaml
index:
  # QLever chooses automatically
```

## Example: Slow Query Analysis

Your query times out. Here's how to fix it:

**Original (slow):**
```sparql
SELECT ?name ?age WHERE {
  ?person a wd:Q5 .
  ?person rdfs:label ?name .
  ?person wdt:P569 ?birthDate .
  BIND(YEAR(NOW()) - YEAR(?birthDate) as ?age)
  FILTER(?age > 60)
}
```

**Analysis:**
- `a wd:Q5` filters to ~8B humans — huge!
- Calculates age for ALL humans, THEN filters
- Fix: Use specific constraints

**Optimized:**
```sparql
SELECT ?name ?age WHERE {
  # Constraint: only find Nobel laureates (much smaller set)
  ?person wdt:P166 wd:Q7191 .    # Has Nobel Prize
  ?person rdfs:label ?name .
  ?person wdt:P569 ?birthDate .

  # Now calculate age only for filtered results
  BIND(YEAR(NOW()) - YEAR(?birthDate) as ?age)
  FILTER(?age > 60)
}
```

**Result:** 100x faster (milliseconds vs. seconds)

## Checking Current Performance

```bash
# Get index statistics
qlever index-info

# Shows:
# Triples: 124,567,890
# Subjects: 45,678,901
# Avg bytes per triple: 28
# Memory used: 8.2 GB
```

Larger memory usage means better compression. This is good—it means queries are usually fast.

## When Performance is Already Good

If queries return in <1 second, you're probably fine. Don't over-optimize.

Focus on:
1. Readable queries that team understands
2. Correct results
3. Reasonable memory usage

Only optimize further if specific queries are slow.

## Advanced Optimization

If basic rules don't help:

### Use EXPLAIN to see the plan

```sparql
EXPLAIN
SELECT ?person ?name WHERE {
  ?person a wd:Q5 .
  ?person rdfs:label ?name .
}
```

Shows how QLever will execute your query (join order, estimated cost).

### Profile your queries

```bash
qlever query --profile "SELECT ?x WHERE { ?x a ?type }"
```

Shows CPU time per component. Identifies real bottleneck.

### Check for cartesian products

Unintended cartesian products (combining all rows with all rows) are catastrophic:

```sparql
# BAD: Creates cartesian product
SELECT ?person ?company WHERE {
  ?person rdfs:label ?pname .
  ?company rdfs:label ?cname .
  # No link between person and company!
}

# GOOD: Actually connects them
SELECT ?person ?company WHERE {
  ?person rdfs:label ?pname .
  ?person wdt:P108 ?company .  # Works for
  ?company rdfs:label ?cname .
}
```

## Next Steps

- **Still slow?** Check [Troubleshooting Guide](../explanation/performance.md)
- **Text search?** [How-to: Text Search](./text-search.md)
- **Geographic data?** [How-to: Spatial Queries](./spatial-queries.md)

---

**Remember:** Test with `LIMIT` first while optimizing:
```sparql
SELECT ?x WHERE { ... } LIMIT 100  # Test your logic first
```
Then remove or increase `LIMIT` for final query.
