# How QLever Works: Architecture Overview

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

## Data Loading Pipeline

```
┌─────────────────────────────────────────────────────┐
│ INPUT: RDF Files (TTL, NT, RDF/XML, JSON-LD, etc) │
└──────────────────┬──────────────────────────────────┘
                   │
                   ▼
         ┌──────────────────────┐
         │ PARSE & VALIDATE     │
         └──────────┬───────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ VOCABULARY EXTRACTION│
         │ - Assign IDs         │
         │ - Track unique values│
         └──────────┬───────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ TRIPLE ENCODING      │
         │ Subject → ID         │
         │ Predicate → ID       │
         │ Object → ID          │
         └──────────┬───────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ PERMUTATION BUILDING │
         │ Sort by S-P-O        │
         │ Sort by P-S-O        │
         │ (add more as needed) │
         └──────────┬───────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ COMPRESSION (opt)    │
         │ - FSST for strings   │
         │ - Zstd for blocks    │
         └──────────┬───────────┘
                    │
                    ▼
         ┌──────────────────────┐
         │ OUTPUT: INDEX        │
         │ (on disk)            │
         └──────────────────────┘
```

## Different Permutation Use Cases

### SPO (Subject-Predicate-Object) Default

**When to use:** Most queries follow subject first

```
Query: Give me everything about Alice
Scan: Find Alice (one place in SPO)
Result: All her properties adjacent
```

**Storage:** Grouped by subject
```
Alice: age=30, knows=Bob, city=NYC
Bob: age=25, knows=Carol, city=SF
Carol: age=28
```

### PSO (Predicate-Subject-Object)

**When to use:** Frequent "find all X property"

```
Query: Find all people who know someone
Scan: Find "knows" property (one place)
Result: All subject-object pairs for "knows"
```

### OSP (Object-Subject-Predicate)

**When to use:** Frequent reverse lookups

```
Query: Find all things that are books
Scan: Find book ID (one place)
Result: All subjects with this object
```

## Query Optimization Examples

### Example 1: Naive vs. Optimized

**Naive (slow):**
```sparql
SELECT ?name WHERE {
  ?person a ?type .           # Get all entities (1B)
  ?person rdfs:label ?name .  # Get their names
  FILTER(?type = wd:Q5)       # Filter to humans (only 8B needed)
}
```

Process: 1B entities → filter → 8B humans

**Optimized (fast):**
```sparql
SELECT ?name WHERE {
  ?person a wd:Q5 .           # Get only humans (8B, OSP index)
  ?person rdfs:label ?name .  # Get their names
}
```

Process: 8B humans → get names (1/125th the work)

QLever automatically reorders conditions to do this!

### Example 2: Join Order Matters

**Bad order (1B × 500M operations):**
```
1. Scan all people (1B)
2. For each, find their country (might not exist)
3. For each country, check if it's large
```

**Good order (500M × 8 operations):**
```
1. Scan all large countries (8)
2. For each, find people who live there (500M)
```

1,000,000x difference!

## Bottlenecks & Solutions

| Problem | Solution |
|---------|----------|
| Slow query | Check join order, add filters first |
| Out of memory | Increase server memory, reduce dataset |
| Slow indexing | Use more threads, compress data |
| Large index | Enable compression |
| Slow text search | Limit indexed predicates |
| Slow spatial queries | Ensure spatial indexing enabled |

## Next Steps

- **Query Optimization** → [Explanation: Query Optimization](./optimization.md)
- **Performance** → [How-to: Performance](../how-to/performance.md)
- **Troubleshooting** → [How-to: Configuration](../how-to/configuration.md)

---

**Key Insight:** QLever's speed comes from:
1. Integer IDs (small, fast)
2. Pre-sorted permutations (sequential access)
3. Smart query planning (optimal order)
4. Parallel execution (multi-core)
5. Memory management (large datasets on small hardware)

Combined, these give 10-1000x speedup vs. naive approaches!
