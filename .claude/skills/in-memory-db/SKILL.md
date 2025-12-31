---
name: in-memory-db
description: Build and use QLever as an embedded in-memory database (libqlever). Use when embedding QLever without HTTP servers, managing RDF indexes, or optimizing memory usage for database operations.
---

# In-Memory Database Skill for QLever

Provides comprehensive guidance on using QLever as an embedded, in-memory RDF database through libqlever. Similar to how oxigraph uses rocksdb, QLever can be embedded directly in C++ applications for serverless RDF querying.

## Overview: libqlever vs Server Mode

QLever offers two deployment modes:

### Traditional Server Mode
```
HTTP Server → QLever Engine → RDF Index → Disk Storage
```
- Suitable for large datasets exceeding RAM
- Network-based query interface (SPARQL endpoint)
- Persistent storage on disk
- Multi-client access

### Embedded Mode (libqlever)
```
Application Code → libqlever → QLever Engine → In-Memory Index
```
- Perfect for: Medium-sized datasets in RAM
- Direct C++ API (no HTTP overhead)
- Fast startup (no server initialization)
- Single-process access
- Similar architecture to oxigraph with rocksdb

## Getting Started with libqlever

### Basic Workflow

**1. Include the Library**
```cpp
#include "libqlever/Qlever.h"
```

**2. Build an Index from RDF Data**
```cpp
#include <fstream>
#include <libqlever/Qlever.h>

// Create Qlever instance with configuration
qlever::QueryExecutionContext context;

// Build index from RDF file (.ttl, .nt)
std::string indexPath = "my_index";
qlever::buildIndex("data.ttl", indexPath);
```

**3. Load Index**
```cpp
qlever::Qlever db;
db.loadIndex(indexPath);
```

**4. Execute SPARQL Queries**
```cpp
std::string sparqlQuery = R"(
  PREFIX foaf: <http://xmlns.com/foaf/0.1/>
  SELECT ?name WHERE {
    ?person foaf:name ?name .
  }
  LIMIT 10
)";

auto result = db.query(sparqlQuery);
```

**5. Process Results**
```cpp
// Results returned as IdTable (memory-efficient result containers)
// See engine documentation for result processing
```

## Key Components

### 1. Qlever Main Class

**Location**: `src/libqlever/Qlever.h`

**Core Methods**:

```cpp
class Qlever {
public:
  // Index management
  void loadIndex(const std::string& indexPath);
  void buildIndex(const std::string& rdfPath,
                  const std::string& outputPath);

  // Query execution
  QueryResult query(const std::string& sparqlQuery);
  QueryResult query(const ParsedQuery& parsedQuery);

  // Configuration
  void setMemoryLimit(size_t bytes);
  void setTimeLimit(std::chrono::milliseconds ms);

  // Statistics
  size_t getIndexSize() const;
  size_t getMemoryUsage() const;
};
```

**Common Configurations**:
```cpp
qlever::Qlever db;

// Set memory constraints
db.setMemoryLimit(4_GB);  // Max memory for query execution

// Set timeout (though manual cancellation not yet supported)
// db.setTimeLimit(std::chrono::seconds(30));

// Optional: Detailed timing information
db.setMediaType(qlever::MediaType::qleverJson);  // Detailed analysis
```

### 2. Index Building

**Input Formats Supported**:
- Turtle (.ttl)
- N-Triples (.nt)
- N-Quads (.nq)

**Index Structure** (similar to oxigraph/rocksdb):

Unlike oxigraph (which uses rocksdb as key-value store), QLever builds specialized permutation indexes:

```
Index/
├── dict.txt           # Vocabulary (IRI↔ID mappings)
├── PSO_index          # Subject→Predicate→Object
├── POS_index          # Predicate→Object→Subject
├── OSP_index          # Object→Subject→Predicate
├── textIndex/         # Full-text search index
└── spatialIndex/      # Geographic/spatial queries
```

**Building Example**:
```cpp
#include <libqlever/Qlever.h>

int main() {
  // Configure builder
  qlever::IndexBuilderOptions options;
  options.inputFile = "wikidata_subset.ttl";
  options.outputPath = "/tmp/wikidata_index";
  options.numThreads = 8;  // Parallel index building

  // Build index (can take time for large datasets)
  qlever::buildIndex(options);

  // Index is now ready to load
  return 0;
}
```

### 3. Query Execution Modes

#### Mode 1: Simple Query Strings
```cpp
auto result = db.query("SELECT * WHERE { ?s ?p ?o } LIMIT 1");
```
- Automatically parsed and planned
- Convenient but adds parsing overhead

#### Mode 2: Pre-parsed Queries (for Benchmarking)
```cpp
auto parsedQuery = db.parseQuery(sparqlQuery);

// Execute multiple times without re-parsing
for (int i = 0; i < 1000; i++) {
  auto result = db.query(parsedQuery);
}
```
- Useful for performance testing
- Separates planning from execution time

**Note**: CartesianProductJoin may throw exceptions on second execution with LIMIT/OFFSET (known limitation).

#### Mode 3: Execution Tree Inspection
```cpp
auto executionTree = db.plan(sparqlQuery);

// Manually inspect/modify execution plan before running
// (Advanced usage, not recommended for most use cases)

auto result = db.executeTree(executionTree);
```

## Memory Management in libqlever

### 1. Index Memory Usage

**Typical Memory Requirements**:

```
Index Size ≈ (Triple Count × 20-30 bytes) + Vocabulary Overhead

Examples:
- 1 Million triples:   ~50-100 MB
- 10 Million triples:  ~500 MB - 1 GB
- 100 Million triples: ~5-10 GB
- 1 Billion triples:   ~50-100 GB
```

**Memory-Efficient Storage**:

QLever uses **compressed permutation indexes**:
- Multiple triple orderings (PSO, POS, OSP) for optimal query patterns
- Prefix compression within each permutation
- Zstandard compression for literal values
- Reduced duplicates through ID-based storage

### 2. Query Execution Memory

**Separate from Index Memory**:
```cpp
// Index memory: Fixed size after loading
size_t indexSize = db.getIndexSize();  // ~100 MB - 50 GB

// Execution memory: Grows with query results
db.setMemoryLimit(2_GB);  // Limits intermediate results, not index

// Typical execution memory:
// - Small query (<1M results): ~50-200 MB
// - Medium query (1M-100M results): ~500 MB - 2 GB
// - Large query (100M+ results): Needs 4+ GB
```

### 3. Memory Optimization Tips

**For Constrained Environments**:

```cpp
// 1. Use smaller datasets
// Load only necessary RDF subset
db.loadIndex("compact_index");  // Pre-filtered RDF

// 2. Limit result sizes
std::string limitedQuery = R"(
  SELECT * WHERE { ?s ?p ?o }
  LIMIT 10000
)";

// 3. Use streaming results when possible
// (Advanced: requires custom result handlers)

// 4. Monitor memory usage
std::cout << "Index: " << (db.getIndexSize() / 1024 / 1024) << " MB\n";
std::cout << "Query Memory Limit: 2 GB\n";
```

## Query Execution Analysis

### Detailed Timing Information

QLever can provide detailed analysis similar to the web UI "Analysis" tab:

```cpp
db.setMediaType(qlever::MediaType::qleverJson);

std::string result = db.query(sparqlQuery);
// Result contains JSON with:
// - Query planning time
// - Operation breakdown
// - Memory usage per operation
// - Result count and size
```

**Example Output Structure**:
```json
{
  "query": "SELECT * WHERE { ?s ?p ?o }",
  "resultsize": 1000000,
  "runtimeInMs": 245,
  "parseTimeInMs": 2,
  "planningTimeInMs": 15,
  "executionTimeInMs": 228,
  "operationBreakdown": [
    { "operation": "IndexScan", "timeInMs": 150 },
    { "operation": "Sort", "timeInMs": 78 }
  ]
}
```

### Performance Considerations

**Query Optimization Strategies**:

```sparql
-- ❌ SLOW: Cartesian product (all triples)
SELECT * WHERE {
  ?s ?p ?o
}

-- ✅ FAST: Use specific patterns
SELECT ?name WHERE {
  ?person <http://xmlns.com/foaf/0.1/name> ?name .
  ?person <http://xmlns.com/foaf/0.1/age> ?age .
  FILTER (?age > 30)
}

-- ✅ FASTER: Filter early in join order
SELECT ?name WHERE {
  ?person <http://example.org/age> ?age .
  FILTER (?age > 30) .
  ?person <http://xmlns.com/foaf/0.1/name> ?name .
}
```

## Building and Testing libqlever

### Build Configuration

```bash
# Navigate to build directory
cd build

# Ensure CMakeLists.txt includes libqlever
# (It should by default after libqlever merge)

# Build library
cmake --build . --target qlever  # Main library

# Build example
cmake --build . --target LibQLeverExample
```

### Running LibQLeverExample

**Location**: `src/libqlever/LibQLeverExample.cpp`

```bash
# Run the example executable
./LibQLeverExample

# Expected output:
# - Index building from sample RDF
# - Query execution results
# - Timing information
```

**Example Code Walkthrough**:

```cpp
// 1. Create sample RDF data
std::string rdfData = R"(
  @prefix ex: <http://example.org/> .
  ex:Alice ex:knows ex:Bob .
  ex:Alice ex:name "Alice" .
)";

// 2. Write to file
std::ofstream file("example.ttl");
file << rdfData;
file.close();

// 3. Build index
qlever::buildIndex("example.ttl", "example_index");

// 4. Load and query
qlever::Qlever db;
db.loadIndex("example_index");

auto result = db.query(
  "SELECT ?person WHERE { ?person ex:knows ex:Bob }"
);
```

### Unit Tests

**Location**: `test/libqlever/QleverTest.cpp`

**Running Tests**:
```bash
# Run all libqlever tests
ctest -R LibQlever --output-on-failure

# Run specific test
ctest -R LibQlever.buildIndexAndRunQuery --output-on-failure
```

**Test Coverage Includes**:
- Index building from RDF files
- Query execution on loaded indexes
- Result validation
- Memory management verification

## Limitations & Future Enhancements

### Current Limitations

| Feature | Status | Notes |
|---------|--------|-------|
| Query Timeouts | ❌ Not Supported | Must be implemented at application level |
| Manual Cancellation | ❌ Limited | No websocket cancellation in libqlever |
| Live Query Timing | ❌ Not Supported | Only final timing available (use `qleverJson` media type) |
| SPARQL Updates | ❌ Not Supported | Read-only for now |
| Transaction Support | ❌ Not Supported | Single query at a time |

### Workarounds

```cpp
// Timeout simulation
#include <thread>
#include <chrono>

std::future<QueryResult> queryFuture =
  std::async(std::launch::async,
    [&db, &query]() { return db.query(query); }
  );

auto status = queryFuture.wait_for(std::chrono::seconds(30));
if (status == std::future_status::timeout) {
  // Handle timeout (query continues in background)
  std::cerr << "Query timed out\n";
}
```

### Future Enhancements Needed

```cpp
// These are planned but not yet implemented:
// - db.cancelQuery()  // Via CancellationHandle
// - db.subscribeToResults()  // Streaming results
// - db.update(sparqlUpdate)  // SPARQL UPDATE support
// - db.createTransaction()  // Multi-query transactions
```

## Comparison with Other Embedded Databases

### vs. oxigraph (rocksdb)

| Aspect | QLever (libqlever) | oxigraph (rocksdb) |
|--------|---|---|
| Storage Backend | Compressed permutation indexes | RocksDB key-value store |
| Query Language | Full SPARQL 1.1 | Subset of SPARQL 1.1 |
| Performance | Optimized for complex joins | Optimized for key lookups |
| Memory Model | All-in-memory or hybrid | RocksDB handles spilling |
| Full-Text Search | Built-in | Requires separate index |
| Spatial Queries | Built-in (S2 geometry) | No native support |
| Update Support | Via server only | Partial SPARQL UPDATE |
| Deployment | Embedded C++ library | Rust library |

**When to use libqlever**:
- Complex SPARQL queries with multiple joins
- Full-text search requirements
- Spatial/geographic queries
- Medium datasets (fits in RAM)
- Maximum query performance

**When to use oxigraph**:
- Rust-based projects
- SPARQL UPDATE requirements
- Simpler query patterns
- RocksDB persistence benefits

## Advanced Patterns

### 1. Batch Query Execution

```cpp
std::vector<std::string> queries = {
  "SELECT COUNT(*) WHERE { ?s ?p ?o }",
  "SELECT ?type WHERE { ?s rdf:type ?type } LIMIT 100",
  "SELECT ?name WHERE { ?s foaf:name ?name } LIMIT 1000"
};

for (const auto& query : queries) {
  auto result = db.query(query);
  processResult(result);
}
```

### 2. Index Preloading

```cpp
// For applications needing instant startup:

// Main thread loads index once
qlever::Qlever db;
db.loadIndex("precomputed_index");

// Subsequent queries are fast
for (size_t i = 0; i < 1000000; ++i) {
  auto result = db.query("SELECT COUNT(*) WHERE { ?s ?p ?o }");
}
```

### 3. Memory-Constrained Environments

```cpp
// Build on powerful machine
system("qlever-index-builder --input large_dataset.ttl "
       "--output minimal_index");

// Transfer minimal index to target device
// Load on memory-constrained device
qlever::Qlever db;
db.setMemoryLimit(512_MB);  // Strict limit
db.loadIndex("minimal_index");

// Queries respect memory limit
try {
  auto result = db.query(complexQuery);
} catch (const std::exception& e) {
  std::cerr << "Query exceeded memory limit: " << e.what() << "\n";
}
```

## Integration Examples

### C++ Application Integration

```cpp
// my_app.cpp
#include <libqlever/Qlever.h>
#include <iostream>

class KnowledgeGraphDB {
private:
  qlever::Qlever engine_;

public:
  void initialize(const std::string& indexPath) {
    engine_.loadIndex(indexPath);
  }

  std::vector<std::string> findPeopleByAge(int minAge) {
    std::string query = R"(
      PREFIX foaf: <http://xmlns.com/foaf/0.1/>
      SELECT ?name WHERE {
        ?person foaf:name ?name ;
                foaf:age ?age .
        FILTER (?age >= $minAge)
      }
    )";

    auto result = engine_.query(query);
    // Parse and return results
    return parseResults(result);
  }
};

int main() {
  KnowledgeGraphDB db;
  db.initialize("wikidata_index");

  auto results = db.findPeopleByAge(30);
  for (const auto& person : results) {
    std::cout << person << "\n";
  }

  return 0;
}
```

### Building Custom Tools

```cpp
// analyze_rdf_dataset.cpp
// Custom tool for RDF analysis using libqlever

#include <libqlever/Qlever.h>
#include <iostream>

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage: analyze_rdf <input.ttl> <output_dir>\n";
    return 1;
  }

  std::string inputFile = argv[1];
  std::string outputDir = argv[2];

  // Build index from input file
  qlever::buildIndex(inputFile, outputDir);

  // Load and analyze
  qlever::Qlever db;
  db.loadIndex(outputDir);

  // Run analysis queries
  auto tripleCount = db.query("SELECT (COUNT(*) as ?count) WHERE { ?s ?p ?o }");
  auto typeCount = db.query("SELECT (COUNT(DISTINCT ?type) as ?count) WHERE { ?s rdf:type ?type }");

  std::cout << "Triple count: " << tripleCount << "\n";
  std::cout << "Type count: " << typeCount << "\n";

  return 0;
}
```

## Troubleshooting

### Problem: Index Loading Fails

```cpp
// Issue: "Cannot load index from path"
// Solution: Verify index exists and has correct structure

#include <filesystem>

// Check index structure
std::filesystem::path indexPath("my_index");
if (!std::filesystem::exists(indexPath / "dict.txt")) {
  std::cerr << "Invalid index: missing dict.txt\n";
}
if (!std::filesystem::exists(indexPath / "PSO_index")) {
  std::cerr << "Invalid index: missing PSO_index\n";
}

// Rebuild if necessary
qlever::buildIndex("data.ttl", "my_index");
```

### Problem: Query Returns No Results

```cpp
// Issue: SPARQL query syntax correct but no results
// Solution: Debug with simpler queries

// Step 1: Check index is loaded
auto count = db.query("SELECT (COUNT(*) as ?count) WHERE { ?s ?p ?o }");
std::cout << "Index contains: " << count << " triples\n";

// Step 2: Test pattern matching
auto sample = db.query("SELECT ?s WHERE { ?s ?p ?o } LIMIT 10");
std::cout << "Sample subjects: " << sample << "\n";

// Step 3: Check predicate URIs
auto predicates = db.query("SELECT DISTINCT ?p WHERE { ?s ?p ?o } LIMIT 100");
// Ensure URIs in query match exactly
```

### Problem: Memory Limit Exceeded

```cpp
// Issue: "Query execution exceeded memory limit"
// Solution: Optimize query or increase limit

// Option 1: Increase memory limit
db.setMemoryLimit(8_GB);  // Was 2GB

// Option 2: Add LIMIT to query
std::string optimized = R"(
  SELECT ?name WHERE {
    ?person foaf:name ?name
  }
  LIMIT 10000
)";
auto result = db.query(optimized);

// Option 3: Filter earlier
std::string better = R"(
  SELECT ?name WHERE {
    ?person foaf:age ?age ;
            foaf:name ?name .
    FILTER (?age > 30)
  }
  LIMIT 1000
)";
```

## Related Documentation

- **[CLAUDE.md](../../CLAUDE.md)** - Overall QLever development guide
- **src/libqlever/Qlever.h** - API reference
- **src/libqlever/LibQLeverExample.cpp** - Working example
- **test/libqlever/QleverTest.cpp** - Unit tests
- **src/index/** - Index implementation details
- **src/engine/** - Query execution engine

## Key Resources

- **RDF Specification**: https://www.w3.org/RDF/
- **SPARQL Query Language**: https://www.w3.org/TR/sparql11-query/
- **QLever Repository**: https://github.com/ad-freiburg/qlever
- **oxigraph Project**: https://github.com/oxigraph/oxigraph (comparison reference)
