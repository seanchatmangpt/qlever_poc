# Performance Characteristics: When QLever Excels

Understand QLever's performance profile, scalability, and trade-offs.

## The 30-Second Version

**QLever is fast when:**
- Queries are selective (filters reduce data early)
- Data fits in memory
- Queries follow graph structure
- You have an SSD or fast disk

**QLever struggles with:**
- Unselective queries (process 1B entities)
- Datasets bigger than RAM
- Cartesian products
- Complex aggregations on huge datasets

**Typical performance:**
- Simple queries: <10ms
- Complex queries: 100ms - 1 second
- Aggregations: 1-10 seconds
- Worst case: minutes/hours

## Scalability Profile

### Query Complexity vs. Time

```
Execution Time
     ▲
     │                    ╱╲  Aggregation + LIMIT
     │                  ╱    ╲
 1s  │                ╱        ╲
     │              ╱            ╲
     │            ╱                ╱
100ms│         ╱                  ╱
     │       ╱                   ╱ (with LIMIT)
 10ms│    ╱                     ╱
     │  ╱                      ╱
  1ms│╱________________________
     └─────────────────────────► Query Selectivity
         High                Low
```

**More selective** (fewer results) = **faster** (exponential speedup)

### Data Size vs. Performance

| Dataset Size | Index Size | Query Time | Concurrency | Typical Use |
|--------------|-----------|-----------|-------------|------------|
| <1M triples | 10 MB | <1ms | High | Local testing |
| 1M - 100M | 100 MB - 10 GB | <10ms | High | Small projects |
| 100M - 1B | 10 - 100 GB | 10-100ms | Medium | Department scale |
| 1B - 10B | 100 GB - 1 TB | 100ms - 1s | Low | Enterprise |
| >10B | >1 TB | 1s+ | Very low | Research/experimental |

**Scaling:** Near-linear with index size (good!)

### Memory vs. Query Performance

```
Query Time
    ▲
    │     No memory (disk thrashing)
    │     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
    │     ▓ Slow, unpredictable ▓
    │     ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
    │
    │     Enough for index only
    │     ─ ─ ─ ─ ─ ─ ─ ─ ─ ─
    │
    │     Index + 1 concurrent query
 1s │     ────────────────────
    │
    │     Index + 4 concurrent queries
100ms│    ───────────────────────────
    │
    │     Plenty of spare memory
 10ms│   ─────────────────────────────
    │
    └─────────────────────────────────►
        Index size              System RAM
```

**Rule of thumb:** Allocate at least 1.5x index size for good performance

## Performance Characteristics by Query Type

### Simple Lookups (Fastest)

```sparql
SELECT ?name WHERE {
  ?person rdfs:label ?name .
  FILTER(CONTAINS(?name, "Einstein"))
}
```

**Execution:** Direct index scan + filter
**Time:** 10-50ms
**Scalability:** Excellent (O(log N) + O(k) where k = results)

### Filter-Heavy Queries (Fast)

```sparql
SELECT ?person WHERE {
  ?person a wd:Q5 .
  ?person wdt:P31 wd:Q5 .
  ?person wdt:P27 wd:Q142 .
}
```

**Execution:** Index scan → index scan → index scan (all selective)
**Time:** 50-200ms
**Scalability:** Excellent

### Joins with Small Results (Fast)

```sparql
SELECT ?book ?author WHERE {
  ?book wdt:P50 ?author .
  ?author wdt:P166 wd:Q7191 .  # Nobel laureates only
}
```

**Execution:** Scan small set (500 laureates) → find their books
**Time:** 50-100ms
**Scalability:** Excellent (limited by result size)

### Aggregations on Filters (Medium)

```sparql
SELECT ?author (COUNT(?book) as ?count) WHERE {
  ?book a wd:Q571 .
  ?book wdt:P50 ?author .
}
GROUP BY ?author
HAVING COUNT(?book) > 10
```

**Execution:** Scan 1M books → group by author → aggregate
**Time:** 500ms - 5s
**Scalability:** Good (O(N) where N = books)

### Large Aggregations (Slow)

```sparql
SELECT (COUNT(*) as ?total) WHERE {
  ?x a ?type .
}
```

**Execution:** Scan entire database (~1B entities)
**Time:** 5-30s
**Scalability:** Linear (unavoidable—must count everything)

### Cartesian Products (Very Slow/Impossible)

```sparql
SELECT ?person ?book WHERE {
  ?person a wd:Q5 .        # 8B results
  ?book a wd:Q571 .        # 1M results
}
# Would produce 8B × 1M = too many results!
```

**Execution:** Fails (memory limit hit)
**Scalability:** Catastrophic (exponential)

## Comparative Performance

QLever vs. other RDF engines (typical benchmarks):

| Query Type | QLever | Virtuoso | Blazegraph |
|-----------|--------|----------|-----------|
| Simple lookup | 10ms | 50ms | 20ms |
| Complex join | 100ms | 500ms | 300ms |
| Aggregation | 1s | 3s | 2s |
| Full scan | 10s | 30s | 20s |

**QLever excels at:** Medium-complexity queries, selective data access

**Weak points:** Very large result sets, complex aggregations, distributed queries

## Bottleneck Analysis

### CPU-Bound (Common)

When index fits in memory:
- Query processing is CPU-limited
- Faster CPU = better performance
- Parallelization helps

**Optimize:** Use more selective queries

### Memory-Bound (Intermediate)

When intermediate results are large:
- Query is limited by memory bandwidth
- Caching helps
- Reducing result size helps dramatically

**Optimize:** Add more FILTER conditions, use LIMIT

### I/O-Bound (Less Common)

When index doesn't fit in memory:
- Disk access is the bottleneck
- Use SSD (100x faster than HDD)
- Increase RAM allocation

**Optimize:** Use better disk, increase memory

## Concurrent Query Performance

### With Adequate Memory

```
Concurrent Queries │ Time per Query │ Total Throughput
───────────────────┼────────────────┼─────────────────
        1          │     100ms      │   10 queries/s
        2          │     100ms      │   20 queries/s
        4          │     110ms      │   36 queries/s
        8          │     150ms      │   53 queries/s
       16          │     500ms      │   32 queries/s (↓)
```

**Sweet spot:** 4-8 concurrent queries (with 64GB RAM)

### With Memory Pressure

```
Concurrent Queries │ Time per Query │ Total Throughput
───────────────────┼────────────────┼─────────────────
        1          │     100ms      │   10 queries/s
        2          │     200ms      │    10 queries/s (↓)
        4          │   5000ms       │    0.8 queries/s (↓↓)
```

**When memory is tight:** Run fewer concurrent queries

## Trade-offs

### Compression vs. Speed

| Setting | Index Size | Query Speed | Build Time |
|---------|-----------|-------------|-----------|
| No compression | Large | Fastest | Fastest |
| FSST compression | 70% | ~5% slower | ~10% slower |
| Zstd compression | 40% | ~20% slower | ~30% slower |

**Decision:** Use compression if disk space is limited; skip if speed matters

### Permutations vs. Query Flexibility

| Permutations | Flexibility | Speed | Memory |
|-------------|-----------|-------|--------|
| SPO only | Limited | Fast SPO | Low |
| SPO + PSO | Good | Fast S/P | Medium |
| SPO + PSO + OSP | Excellent | Fast all | High |

**Decision:** Use more permutations if you have memory and query patterns vary

### Memory Allocation vs. Concurrent Queries

| Memory | Concurrent Queries | Per-Query Speed | Total Throughput |
|--------|------------------|-----------------|-----------------|
| 8GB | 1 | Fast | Low |
| 16GB | 2 | Good | Medium |
| 32GB | 4 | Good | Good |
| 64GB | 8 | Good | Excellent |

**Decision:** More memory = more concurrency = better total throughput

## Performance Limits

### Hard Limits

- **Memory:** Can't process more than available RAM (per query)
- **Disk:** Can't load datasets larger than disk
- **Time:** Very large unselective queries will timeout (default: 5 minutes)

### Soft Limits

- **Concurrency:** >16 queries → thrashing
- **Result size:** >10M rows → very slow to return
- **Aggregation:** Grouping >1B values → slow

## Tips for Better Performance

### 1. Use Selective Queries

```sparql
# Bad: 1B entities
SELECT ?x WHERE { ?x ?p ?o . }

# Good: Constrain early
SELECT ?x WHERE {
  ?x a wd:Q5 .        # Only humans (8B)
  ?x wdt:P27 wd:Q142  # Only French (80K) ✓
}
```

### 2. Add LIMIT for Testing

```sparql
# Test with small result set first
SELECT ?x WHERE { ?x a ?type } LIMIT 100
```

### 3. Allocate Sufficient Memory

```bash
ServerMain -i my-index -p 7023 -m 32GB
```

Min: 2x index size. Better: 3-4x index size.

### 4. Match Query Structure to Data

Different questions need different patterns:
- "Find X with property Y" → SPO index
- "Find all things with property P" → PSO index
- "Find inverse relationship" → OSP index

### 5. Monitor Performance

```bash
qlever query --show-timing "SELECT ..."
# See exactly where time is spent
```

## When to Consider Alternatives

Use something other than QLever if:

- **Frequent updates:** QLever is read-optimized (rebuilding index is slow)
- **Strict ACID transactions:** QLever is eventual-consistency
- **Very large datasets (>100B triples):** Memory usage becomes problematic
- **Geographically distributed:** QLever is single-node
- **Graph machine learning:** Use specialized graph ML tools

QLever is optimized for: **Query-heavy, read-mostly, selective queries on medium-to-large RDF datasets**

## Next Steps

- **Improve your queries** → [How-to: Performance](../how-to/performance.md)
- **Understand optimization** → [Query Optimization](./optimization.md)
- **See the architecture** → [How QLever Works](./architecture.md)

---

**Bottom Line:** QLever's performance advantage comes from efficient indexing and smart query planning. Provide good constraints, and QLever does the rest.
