---
diataxis_type: explanation
title: "How QLever Works: Architecture and Performance"
description: "High-level understanding of QLever's system architecture, query execution model, and performance characteristics"
audience: all
status: complete
last_updated: 2026-01-02
related_docs:
  - "explanation/optimization.md"
  - "explanation/performance.md"
  - "how-to/performance.md"
  - "reference/api.md"
  - "reference/cli.md"
  - "tutorials/01-quickstart.md"
keywords:
  - architecture
  - indexing
  - query execution
  - permutations
  - memory management
  - cost-based optimization
  - integer IDs
  - memory-mapped files
  - parallel execution
  - RDF storage
semantic_tags:
  - "architecture/system-design"
  - "architecture/data-structures"
  - "architecture/query-execution"
  - "performance/optimization"
  - "internals/indexing"
agent_priority: high
search_boost: 2.5
specifications:
  - "W3C SPARQL 1.1 Query Language"
  - "RDF 1.1 Concepts and Abstract Syntax"
---

# How QLever Works: Architecture and Performance

High-level understanding of QLever's system architecture and why it's so fast.

## System Overview

QLever consists of three main components:

```
┌─────────────────┐
│  Your RDF Data  │
│  (Turtle, N-Triples)
└────────┬────────┘
         │
         ▼
  ┌─────────────────────────┐
  │  INDEXING (One time)    │ ← IndexBuilderMain
  │ - Parse RDF data        │
  │ - Assign internal IDs   │
  │ - Build optimized index │
  │ - Create permutations   │
  └────────┬────────────────┘
           │
           ▼
  ┌─────────────────────────┐
  │ INDEXED DATABASE        │
  │ (on disk, memory-mapped)│
  └────────┬────────────────┘
           │
           ▼
  ┌─────────────────────────┐
  │ SERVER (Running)        │ ← ServerMain
  │ - Accept queries        │
  │ - Execute SPARQL        │
  │ - Return results        │
  └────────┬────────────────┘
           │
           ▼
  ┌─────────────────────────┐
  │ Your Application        │
  │ (REST API, WebUI)       │
  └─────────────────────────┘
```

## Phase 1: Data Indexing

When you run `qlever index`:

### Step 1: Vocabulary Building

QLever assigns unique integer IDs to every unique value in your data:

```
IRI "http://wikidata.org/entity/Q5" → ID 42
IRI "http://example.org/book1"      → ID 1000
Literal "Alice"@en                  → ID 2001
Literal "30"^^xsd:integer           → ID 3042
```

**Why integers?**
- Much smaller to store (4 bytes vs. 1000+ bytes per string)
- Faster comparisons and lookups
- Enables efficient compression

### Step 2: Triple Encoding

Convert RDF triples into encoded form:

```
Original:  ex:alice foaf:knows ex:bob
Encoded:   1000 → 500 → 1001  (Subject → Predicate → Object)
```

All triples become just three integers.

### Step 3: Index Permutations

Store the same triples in different orders for different query patterns:

```
SPO (Subject-Predicate-Object)
    All triples with same subject nearby
    → Fast: "Give me everything about Alice"

PSO (Predicate-Subject-Object)
    All triples with same predicate nearby
    → Fast: "Give me all 'knows' relationships"

OSP (Object-Subject-Predicate)
    All triples with same object nearby
    → Fast: "Give me all things that are known by Alice"
```

QLever usually creates 1-3 permutations based on your queries.

### Step 4: Compression

For large datasets, compress the indexes using advanced algorithms:
- FSST compression (fast string compression)
- Zstandard compression
- Bit-level packing

**Result:** Typical compression ratio 10:1 (100GB data → 10GB index)

## Phase 2: Query Execution

When you run a SPARQL query:

### Step 1: Parse

Convert SPARQL text to an internal representation:

```sparql
SELECT ?author WHERE {
  ?book a wd:Q571 .
  ?book wdt:P50 ?author .
}
```

Becomes: Query tree with operations and conditions

### Step 2: Plan Optimization

QLever generates multiple execution plans and picks the fastest one:

```
Plan A:                          Plan B:
1. Scan all "is book"      vs    1. Scan all "has author"
2. For each, find author         2. Filter to books only
   (Slow: process all things)       (Faster: smaller dataset)

QLever picks Plan B (fewer rows earlier)
```

This is **cost-based optimization**:
- Estimate size of each result
- Calculate cost of each plan
- Execute cheapest one

### Step 3: Execution

Actually run the query:

```
1. IndexScan: Find all books (using PSO index)
   Result: 1M book IDs

2. Join: For each book, find authors (using SPO index)
   Result: 800K (book, author) pairs

3. Filter: Apply SPARQL filters
   Result: Final results

4. Format: Convert integer IDs back to original values
   Return results
```

## Why QLever is Fast

### 1. Integer IDs

Everything is stored as integers, not strings:
- Lookups: O(log N) binary search instead of string comparison
- Storage: 4 bytes vs. variable-length string
- Memory: Cache fits more data

### 2. Index Permutations

Different access patterns use pre-sorted data:
- No need to re-sort (already sorted by storage order)
- Sequential disk access (very cache-efficient)
- Faster than random access by 100-1000x

### 3. Early Filtering

Filter small sets before joins:
- Process 1000 books then find authors (1000 ops)
- vs. all 1B entities then filter (1B ops)
- 1,000,000x difference!

### 4. Memory-Mapped Files

Index lives on disk but accessed like memory:
```
OS handles page loading automatically
Application sees just big fast memory
```

- Only loads needed data into RAM
- Survives process restarts
- Can handle datasets bigger than RAM

### 5. Parallel Execution

Modern CPUs have many cores:
- QLever uses multiple cores for:
  - Scanning (different range per core)
  - Joining (parallel hash join)
  - Sorting (parallel merge sort)
- ~N cores = ~N speedup (linear scaling)

### 6. Cost-Based Query Planning

Computer chooses optimal query plan:
- Different query → different optimal plan
- Automatic adjustment based on data
- No manual tuning (usually)

## Memory Management

### Virtual Memory System

QLever manages memory carefully:

```
Configured limit: 32 GB
├─ Index pages: 20 GB (accessed as needed)
├─ Query execution: 8 GB (temporary)
├─ Caching: 3 GB (frequent accesses)
└─ System: 1 GB (buffers)
```

Each operation respects the limit:
- If approaching limit: spill to disk
- Prevents out-of-memory crashes
- Automatic fallback to disk-based algorithms

## Key Design Principles

1. **Integer Encoding**: All values mapped to compact integer IDs
2. **Pre-sorted Permutations**: Multiple sort orders for different access patterns
3. **Cost-Based Planning**: Automatic query optimization
4. **Memory Mapping**: Efficient disk access without loading everything into RAM
5. **Parallel Execution**: Multi-core utilization for performance
6. **Early Filtering**: Reduce data volume before expensive operations

## Performance Characteristics

### What QLever Excels At

- **Large-scale graph queries**: Billions of triples
- **Complex joins**: Cost-based optimization handles multi-way joins
- **Text search**: Integrated full-text indexing
- **Spatial queries**: Geographic data with spatial indexing
- **Aggregations**: COUNT, SUM, AVG with GROUP BY

### Trade-offs

- **Index size**: Requires 1-3x data size for permutations (with compression)
- **Index time**: One-time cost to build permutations
- **Memory**: Benefits from large RAM but works with smaller memory via disk spilling
- **Update latency**: Optimized for read-heavy workloads; updates require reindexing

## Next Steps

- **Query Optimization Details** → [Explanation: Query Optimization](./optimization.md)
- **Performance Tuning** → [How-to: Performance](../how-to/performance.md)
- **Configuration Options** → [How-to: Configuration](../how-to/configuration.md)
- **API Reference** → [Reference: HTTP API](../reference/api.md)

---

**Key Insight**: QLever's speed comes from a combination of integer encoding, pre-sorted permutations, smart query planning, parallel execution, and careful memory management. These optimizations work together to provide 10-1000x speedup vs. naive approaches.
