# Query Optimization: How QLever Plans Execution

Understand how QLever chooses the fastest way to execute your query.

## The Problem: Multiple Execution Plans

Every SPARQL query can be executed in many different ways:

```sparql
SELECT ?author ?book WHERE {
  ?book a wd:Q571 .           # It's a book
  ?book wdt:P50 ?author .     # Has author
  ?author a wd:Q5 .           # Author is a person
}
```

**Plan A:** Process in order
```
1. Scan all books (1M results)
2. For each book, find author (1M lookups)
3. Filter to only humans (800K results)
```
**Cost:** 1M + 1M = 2M operations

**Plan B:** Reorder conditions
```
1. Scan all humans (8B results)
2. For each human, find books they authored (8B lookups)
3. Filter to only books (800K results)
```
**Cost:** 8B + 8B = 16B operations ❌ Much slower!

**Plan C:** Smart ordering (what QLever does)
```
1. Scan all books (1M results)
2. For each book, find author (1M lookups)
3. Filter authors to humans (800K results)
```
**Cost:** 1M + 1M = 2M operations ✅ Fastest

QLever automatically chooses Plan C.

## Cost-Based Query Planning

QLever uses **cost-based optimization**: it estimates the cost of different execution plans and picks the cheapest one.

### How QLever Estimates Cost

For each pattern, QLever estimates how many results it will produce:

```sparql
?book a wd:Q571 .      # Estimate: ~1M books
?author a wd:Q5 .      # Estimate: ~8B humans
?book wdt:P50 ?author  # Estimate: ~0.8 books per author
```

**Cardinality** = estimated number of results

### Join Order Optimization

QLever applies these heuristics:

1. **Filter first** — Patterns with low cardinality (small result sets) processed first
   ```sparql
   # Good: Start with most selective
   ?book a wd:Q571 .           # Low cardinality (~1M)
   ?book wdt:P50 ?author .     # Medium cardinality
   ?author a wd:Q5 .           # High cardinality (~8B) - filter from small set
   ```

2. **Join connected components** — Process related data together
   ```sparql
   # Group related patterns
   ?book a wd:Q571 .
   ?book wdt:P50 ?author .     # Connected to book via ?book

   # Then separately
   ?country a wd:Q6256 .       # Not connected to book
   ```

3. **Estimate result size** — Use index statistics to predict intermediate result sizes

### Example: Join Order Decision

**Query:** Find French authors of English books

```sparql
SELECT ?author WHERE {
  ?book a wd:Q571 .                    # 1M books
  ?book wdt:P407 wd:Q1860 .            # 100K English books
  ?book wdt:P50 ?author .              # 100K (author, book) pairs
  ?author wdt:P27 wd:Q142 .            # 80K French authors
  ?author a wd:Q5 .                    # 8B humans (not needed here!)
}
```

**QLever's Plan:**
```
1. Index scan: ?book wdt:P407 wd:Q1860     # 100K
   (language index is very selective)
2. Join: For each book, get author          # 100K lookups
3. Filter: ?author wdt:P27 wd:Q142         # ~80K remain
4. Skip: ?author a wd:Q5                    # Redundant (already filtered)
```

Result: ~200K operations, returns 80K results

**If QLever naively did it:**
```
1. Scan all humans (8B)
2. Filter to French (80K)
3. Find books for each author (80K × 100K combinations)
4. Filter to English (small percentage remain)
Result: 8B operations ❌ Catastrophic!
```

## Triple Pattern Matching

Every SPARQL pattern matches zero or more triples:

```sparql
?book wdt:P50 ?author .
```

Matches all triples where:
- Predicate = `wdt:P50` (author property)
- Subject = anything (the book)
- Object = anything (the author)

QLever uses index permutations to find these efficiently:

| Pattern | Best Index | Time |
|---------|-----------|------|
| `?book wdt:P50 ?author` | PSO (Predicate first) | O(log N) |
| `?book a ?type` | SPO (Subject first) | O(log N) |
| `?person ?prop wd:Q142` | OSP (Object first) | O(log N) |

## When Optimization Can't Help

Some queries are inherently expensive:

```sparql
# Cartesian product: every person with every book
SELECT ?person ?book WHERE {
  ?person a wd:Q5 .        # 8B results
  ?book a wd:Q571 .        # 1M results
}
# Result: 8B × 1M = impossible to compute
```

**Solution:** Add a connecting condition

```sparql
SELECT ?person ?book WHERE {
  ?person a wd:Q5 .
  ?person wdt:P50 ?book .  # Actually wrote the book
  ?book a wd:Q571 .
}
```

## Performance Tips for Complex Queries

### 1. Provide Constraints Early

```sparql
# Slow: Large intermediate results
SELECT ?author WHERE {
  ?book wdt:P50 ?author .
  ?author wdt:P27 ?country .
  FILTER(?country = wd:Q142)  # Filter late
}

# Fast: Constrain before joining
SELECT ?author WHERE {
  ?author wdt:P27 wd:Q142 .   # Only French (80K)
  ?book wdt:P50 ?author .     # Find their books
}
```

### 2. Use Most Selective Patterns First

```sparql
# Slow: Start with 1B entities
SELECT ?person WHERE {
  ?person a ?type .            # 1B results
  ?person wdt:P166 wd:Q7191 .  # Nobel laureates
}

# Fast: Start with small set
SELECT ?person WHERE {
  ?person wdt:P166 wd:Q7191 .  # 500 Nobel laureates
  ?person a ?type .            # Just 500 lookups
}
```

### 3. Avoid Unnecessary Patterns

```sparql
# Slow: Three patterns
SELECT ?author WHERE {
  ?book a wd:Q571 .
  ?book a ?type .              # Redundant! We know it's a book
  ?book wdt:P50 ?author .
}

# Fast: Two patterns
SELECT ?author WHERE {
  ?book a wd:Q571 .
  ?book wdt:P50 ?author .
}
```

## How to See QLever's Plan

Some QLever instances support EXPLAIN to see the execution plan:

```sparql
EXPLAIN
SELECT ?author WHERE {
  ?book a wd:Q571 .
  ?book wdt:P50 ?author .
}
```

Returns information about:
- Which index will be used
- Estimated cardinality at each step
- Join order
- Estimated total cost

## Next Steps

- **Speed up your queries** → [How-to: Performance](../how-to/performance.md)
- **Understand the system** → [How QLever Works](./architecture.md)
- **Learn SPARQL** → [Tutorial: Your First Query](../tutorials/02-first-query.md)

---

**Key Insight:** Query performance isn't about the query language—it's about helping the optimizer. More constraints and specific patterns = better optimization = faster results.
