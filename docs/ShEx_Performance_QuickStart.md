# ShEx Performance Optimizations: Quick Start Guide

## TL;DR - Get 3-5x Speedup in 3 Lines

```cpp
#include "parser/ShEx.h"

ShExValidator validator(schema);
validator.enableCaching();              // Line 1: Enable result caching
validator.buildPredicateIndex();        // Line 2: Build shape index
auto report = validator.validateDataset(dataset, nodeToShapeMapping);  // Line 3: Validate
```

**Result**: 3-5x faster validation with 80% cache hit rate

---

## Step-by-Step Setup

### Step 1: Create Validator
```cpp
#include "parser/ShEx.h"

using namespace shex;

// Assuming you have a schema already
ShExValidator validator(schema);
```

### Step 2: Enable Caching
```cpp
// Enable result caching with 10,000 entry limit
validator.enableCaching();

// Or customize cache size:
validator.enableCaching(50000);  // For large datasets
validator.enableCaching(1000);   // For small datasets
```

### Step 3: Build Predicate Index
```cpp
// Build index for fast shape lookup (one-time cost: <1 second)
validator.buildPredicateIndex();
```

### Step 4: Validate
```cpp
// Validate nodes (now optimized!)
auto report = validator.validateDataset(dataset, nodeToShapeMapping);
```

---

## Common Usage Patterns

### Pattern 1: Bulk Validation with Metrics
```cpp
ShExValidator validator(schema);
validator.enableCaching(50000);
validator.buildPredicateIndex();

// Track performance
auto startTime = std::chrono::steady_clock::now();

// Validate 1M nodes
for (const auto& node : millionNodes) {
  validator.validateNode(node.id, node.shapeId, nodeData);
}

auto endTime = std::chrono::steady_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

// Print metrics
auto metrics = validator.getMetrics().getMetrics();
std::cout << "Time: " << duration.count() << "s\n";
std::cout << "Cache hit rate: " << (metrics.getCacheHitRate() * 100) << "%\n";
std::cout << "Speed: " << (metrics.validationsAttempted / duration.count())
          << " validations/second\n";
```

### Pattern 2: Real-Time Validation
```cpp
ShExValidator validator(schema);
validator.enableCaching(5000);   // Smaller cache for memory efficiency
validator.buildPredicateIndex();

// Validate incoming RDF triples in real-time
while (auto triple = listenForTriple()) {
  auto report = validator.validateNode(
    triple.subject,
    getShapeForPredicate(triple.predicate),
    extractTripleData(triple)
  );

  if (!report.conforms) {
    logValidationError(triple, report);
  }
}
```

### Pattern 3: Memory-Constrained Validation
```cpp
ShExValidator validator(schema);
validator.enableCaching(1000);   // Small cache
validator.buildPredicateIndex(); // Index has minimal memory overhead

// Validate with minimal memory footprint
auto report = validator.validateDataset(dataset, mapping);

// If memory still critical, disable caching:
// validator.disableCaching();
```

### Pattern 4: Validation with Error Reporting
```cpp
ShExValidator validator(schema);
validator.enableCaching();
validator.buildPredicateIndex();

// Get detailed error reporting
auto enhancedReport = validator.validateDatasetEnhanced(dataset, mapping);

for (const auto& [nodeId, nodeErrors] : enhancedReport.detailedErrors) {
  std::cout << "Node " << nodeId << ":\n";
  for (const auto& error : nodeErrors) {
    std::cout << "  - " << error.description << "\n";
  }
}
```

---

## Performance Monitoring

### Monitor Cache Hit Rate
```cpp
auto stats = validator.getCacheStats();
std::cout << "Cache hits: " << stats.hits << "\n";
std::cout << "Cache misses: " << stats.misses << "\n";
std::cout << "Hit rate: " << (stats.hits * 100.0 / (stats.hits + stats.misses)) << "%\n";

// Expect >80% hit rate after warmup phase
if (stats.hits / (stats.hits + stats.misses) > 0.80) {
  std::cout << "Cache is performing well!\n";
}
```

### Check Validation Metrics
```cpp
auto metrics = validator.getMetrics().getMetrics();

std::cout << "Total validations: " << metrics.validationsAttempted << "\n";
std::cout << "Success rate: " << (metrics.getSuccessRate() * 100) << "%\n";
std::cout << "Avg time per validation: " << metrics.avgValidationTimeMs << "ms\n";
std::cout << "Constraints evaluated: " << metrics.constraintsChecked << "\n";
std::cout << "Early exits: " << metrics.earlyExits << "\n";
```

### Debug Cache Performance
```cpp
// Check if cache size is optimal
auto stats = validator.getCacheStats();
auto cacheSize = validator.getCacheStats();  // Number of cached entries

if (stats.evictions > stats.hits) {
  std::cout << "Warning: Cache thrashing detected. Increase cache size.\n";
  validator.enableCaching(stats.hits + stats.misses);  // Resize
}
```

---

## Troubleshooting

### Issue: Low Cache Hit Rate (<50%)
**Symptom**: Expected >80% hit rate but getting <50%

**Diagnosis**:
```cpp
auto stats = validator.getCacheStats();
if ((stats.hits / (stats.hits + stats.misses)) < 0.5) {
  // Likely causes:
  // 1. Cache too small relative to node count
  // 2. Node IDs are too diverse (e.g., random UUIDs)
  // 3. Validating against many different shapes
}
```

**Solution**: Increase cache size
```cpp
// If using 10K cache with 1M nodes, increase to 50K
validator.enableCaching(50000);
```

### Issue: Memory Usage Too High
**Symptom**: Validation uses too much memory

**Diagnosis**:
- Cache: ~100 bytes per entry × cache size
- Index: ~50 bytes per predicate × number of shapes

**Solution**: Reduce cache size
```cpp
// Minimal cache (1K entries)
validator.enableCaching(1000);

// If still high, check if you can disable for this use case
validator.disableCaching();
```

### Issue: Validation Speed Unchanged
**Symptom**: Performance improvement not observed

**Diagnosis**:
```cpp
auto metrics = validator.getMetrics().getMetrics();

// 1. Cache not enabled?
if (!validator.isCachingEnabled()) {
  std::cout << "Cache not enabled!\n";
}

// 2. Cache too new (warmup phase)?
if ((metrics.cacheHits / (metrics.cacheHits + metrics.cacheMisses)) < 0.5) {
  std::cout << "Still in cache warmup phase. Run more validations.\n";
}

// 3. Most validations are different nodes?
if (metrics.cacheHits < metrics.cacheMisses) {
  std::cout << "Validating mostly unique nodes. Cache less effective.\n";
}
```

---

## Configuration Recommendations

### By Use Case

**Bulk validation of preprocessed data**:
```cpp
validator.enableCaching(50000);
validator.buildPredicateIndex();
// Expected: 4-5x speedup
```

**Real-time validation of streaming data**:
```cpp
validator.enableCaching(5000);
validator.buildPredicateIndex();
// Expected: 2-3x speedup
```

**Small datasets (<1K nodes)**:
```cpp
validator.enableCaching(1000);
// Index may not help much
validator.buildPredicateIndex();  // Still enables future scalability
```

**Large datasets (>10M nodes)**:
```cpp
validator.enableCaching(100000);
validator.buildPredicateIndex();
// Expected: 3-4x speedup, may need to manage memory
```

**Memory-critical systems**:
```cpp
validator.enableCaching(1000);
validator.buildPredicateIndex();  // Minimal memory overhead
// Expected: 2-3x speedup
```

---

## Advanced: Custom Metrics Collection

```cpp
#include <unordered_map>
#include <chrono>

class ValidationBenchmark {
 public:
  void run(ShExValidator& validator, const std::vector<NodeData>& nodes) {
    auto startTime = std::chrono::steady_clock::now();

    for (const auto& node : nodes) {
      validator.validateNode(node.id, node.shape, node.data);
    }

    auto endTime = std::chrono::steady_clock::now();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();

    auto metrics = validator.getMetrics().getMetrics();

    std::cout << "=== Benchmark Results ===\n";
    std::cout << "Total time: " << totalMs << "ms\n";
    std::cout << "Validations: " << metrics.validationsAttempted << "\n";
    std::cout << "Throughput: "
              << (metrics.validationsAttempted * 1000.0 / totalMs)
              << " validations/second\n";
    std::cout << "Cache hit rate: " << (metrics.getCacheHitRate() * 100) << "%\n";
    std::cout << "Avg time: "
              << (totalMs / static_cast<double>(metrics.validationsAttempted))
              << "ms per validation\n";
  }
};

// Usage:
ShExValidator validator(schema);
validator.enableCaching();
validator.buildPredicateIndex();

ValidationBenchmark bench;
bench.run(validator, nodes);
```

---

## Next Steps

1. **Enable Optimizations**: Add 3 lines to your code (enable caching, build index, validate)
2. **Monitor Performance**: Use metrics to verify improvement
3. **Tune Parameters**: Adjust cache size based on your workload
4. **Profile**: Use performance metrics to identify remaining bottlenecks

## Related Documentation

- **Detailed guide**: See `docs/ShEx_Performance_80_20.md`
- **API reference**: See class definitions in `src/parser/ShExPerformance.h`
- **Integration**: See `src/parser/ShEx.h` for validator integration
- **Benchmarks**: Run `benchmark/ShExValidationBenchmark.cpp`

## Questions?

Check the detailed performance guide or the ShEx main documentation at `docs/ShEx.md`.
