# Generalizing 80/20 Performance Optimizations Across QLever Engine

## Executive Summary

The 80/20 performance optimizations implemented for ShEx validation (result caching, predicate indexing, metrics tracking) **can and should be generalized** across QLever's query execution engine. QLever already has sophisticated caching infrastructure (`src/util/Cache.h`, `src/util/LruCache.h`, `src/util/ConcurrentCache.h`), but the ShEx 80/20 approach provides a **lightweight, opt-in optimization pattern** that can be applied to:

1. **Query operation caching** (Join, Filter, GroupBy results)
2. **Index pattern caching** (Predicate-to-operations mapping)
3. **Operation metrics tracking** (visible performance monitoring)
4. **Pattern indexing** for cost-based optimization

This document outlines the generalization strategy.

---

## Current State: QLever's Existing Optimization Infrastructure

### 1. Existing Caching Layer

QLever already has sophisticated caching:

**File**: `src/util/Cache.h`
- Flexible cache with customizable eviction policies
- Memory-size based capacity management
- Priority queue based eviction
- **Usage**: Index queries, SPARQL expression caching

**File**: `src/util/LruCache.h`
- Simple LRU cache with fixed capacity
- Used for frequently accessed patterns
- **Usage**: Lightweight caching scenarios

**File**: `src/util/ConcurrentCache.h`
- Thread-safe wrapper around Cache
- Synchronized access with locks
- **Usage**: Multi-threaded cache access

### 2. Query Operation Structure

**File**: `src/engine/Operation.h`
- Base class for all query operations (Join, Filter, GroupBy, etc.)
- Result caching via `getResult()` method
- Runtime information tracking (`RuntimeInformation`)
- Variable-to-column mapping

**File**: `src/engine/QueryExecutionTree.h`
- Query execution plan as tree of operations
- Cache key generation
- Cost and size estimation
- Lazy evaluation support

### 3. Performance Monitoring

QLever has runtime information tracking but **no lightweight metrics dashboard** similar to ShEx's `PerformanceMetrics`.

---

## Generalization Strategy: Three-Tier Approach

### Tier 1: Extend ShEx Performance Module to Generic Operations

**Create**: `src/util/OperationPerformanceMonitor.h`

```cpp
namespace ad_utility {

class OperationPerformanceMonitor {
 public:
  struct OperationMetrics {
    std::string operationName;
    size_t executionsAttempted = 0;
    size_t executionsSucceeded = 0;
    size_t cacheHits = 0;
    size_t cacheMisses = 0;
    double avgExecutionTimeMs = 0.0;
    double cacheHitRate() const {
      return (cacheHits + cacheMisses) == 0 ? 0.0
           : cacheHits / static_cast<double>(cacheHits + cacheMisses);
    }
  };

  void recordExecution(const std::string& operationName, bool hit, double timeMs);
  OperationMetrics getMetrics(const std::string& operationName) const;
};

}  // namespace ad_utility
```

**Benefits**:
- Global operation performance visibility
- No overhead if not enabled
- Helps identify optimization opportunities
- Compatible with existing RuntimeInformation

### Tier 2: Generic Pattern Indexing

**Create**: `src/engine/PatternIndexManager.h`

```cpp
namespace qlever {

// Generic pattern indexing for any operation type
template <typename K, typename V>
class PatternIndexManager {
 public:
  // Build index from operation patterns
  void buildIndex(const std::vector<std::pair<K, std::vector<V>>>& patterns);

  // Get candidate items matching a pattern
  absl::flat_hash_set<K> getCandidates(const std::vector<V>& pattern);

  // Statistics
  struct IndexStats {
    size_t totalPatterns;
    size_t totalMappings;
    double avgCandidatesPerPattern;
  };
  IndexStats getStats() const;
};

}  // namespace qlever
```

**Applications**:
1. **Join Optimization**: Predicate-to-join mapping
   ```cpp
   PatternIndexManager<std::string, std::string> joinIndex;
   // K = join operation ID, V = predicates
   joinIndex.buildIndex(joinOperations);
   auto candidateJoins = joinIndex.getCandidates(queryPredicates);
   ```

2. **Filter Optimization**: Variable-to-filter mapping
   ```cpp
   PatternIndexManager<FilterOp*, Variable> filterIndex;
   // K = filter operation, V = filtered variables
   filterIndex.buildIndex(filters);
   auto relevantFilters = filterIndex.getCandidates(selectedVariables);
   ```

3. **GroupBy Optimization**: Grouping-variable-to-operation mapping
   ```cpp
   PatternIndexManager<GroupByOp*, Variable> groupByIndex;
   groupByIndex.buildIndex(groupByOps);
   auto relevantGroupBys = groupByIndex.getCandidates(variables);
   ```

### Tier 3: Lightweight Result Caching for Operations

**Extend**: `src/engine/Operation.h`

Add optional lightweight caching alongside existing infrastructure:

```cpp
class Operation {
 public:
  // Existing methods...
  std::shared_ptr<const Result> getResult(bool requestLaziness = false) const;

  // NEW: Optional lightweight caching (80/20 approach)
  // Enable for frequently-executed operations
  void enableLightweightResultCaching(size_t maxCacheSize = 1000) {
    lightweightCache_ = std::make_unique<ResultCache>(maxCacheSize);
  }

  void disableLightweightResultCaching() { lightweightCache_.reset(); }

  bool isLightweightCachingEnabled() const { return lightweightCache_ != nullptr; }

  // Get cache statistics
  ResultCacheStats getLightweightCacheStats() const;

 private:
  std::unique_ptr<ResultCache> lightweightCache_;
};
```

---

## Integration Points with Existing QLever Components

### 1. Integration with QueryExecutionTree

**File**: `src/engine/QueryExecutionTree.h`

```cpp
class QueryExecutionTree {
 public:
  // Enable lightweight caching for entire operation tree
  void enableOperationCaching(size_t maxEntriesPerOp = 1000);

  // Get performance metrics for all operations
  OperationPerformanceReport getPerformanceReport() const;

  // Check if operation tree would benefit from pattern indexing
  bool shouldBuildPatternIndex() const;

 private:
  std::shared_ptr<ad_utility::OperationPerformanceMonitor> perfMonitor_;
  std::shared_ptr<PatternIndexManager<>> patternIndex_;
};
```

### 2. Integration with QueryExecutionContext

**File**: `src/engine/QueryExecutionContext.h`

```cpp
class QueryExecutionContext {
 public:
  // Configuration for 80/20 optimizations
  struct OptimizationConfig {
    bool enableLightweightCaching = true;
    size_t cacheEntriesPerOp = 1000;
    bool enablePatternIndexing = true;
    bool enablePerformanceMonitoring = true;
  };

  OptimizationConfig& getOptimizationConfig() { return optConfig_; }
  const OptimizationConfig& getOptimizationConfig() const { return optConfig_; }

 private:
  OptimizationConfig optConfig_;
  std::shared_ptr<ad_utility::OperationPerformanceMonitor> perfMonitor_;
};
```

### 3. Integration with QueryPlanner

**File**: `src/engine/QueryPlanner.cpp`

Use pattern indexing to optimize join ordering:

```cpp
// In QueryPlanner::buildPlan()
if (qec->getOptimizationConfig().enablePatternIndexing) {
  patternIndex.buildIndex(availableOperations);

  // Use pattern index to prune candidate operations
  auto candidateOps = patternIndex.getCandidates(queryPatterns);

  // Cost estimation uses fewer candidates
  for (const auto& op : candidateOps) {
    // Original cost estimation logic...
  }
}
```

---

## Specific Optimization Applications

### Application 1: Join Operation Caching

**Problem**: Same join predicates executed multiple times in query planning

**Solution**: Cache join results within a single query execution plan

```cpp
class Join : public Operation {
 public:
  void enableCaching() {
    enableLightweightResultCaching(5000);  // 5K join result cache
  }

 private:
  // Existing join implementation, now with optional caching
};
```

**Expected Improvement**: 20-30% in complex queries with repeated joins

### Application 2: Filter Operation Caching

**Problem**: Same filter constraints applied multiple times

**Solution**: Cache filter results for repeated predicates

```cpp
class Filter : public Operation {
 public:
  void enableCaching() {
    enableLightweightResultCaching(10000);  // 10K filter result cache
  }
};
```

**Expected Improvement**: 15-25% in queries with repeated filters

### Application 3: GroupBy Result Caching

**Problem**: Grouping by same variable set in different contexts

**Solution**: Cache grouping results

```cpp
class GroupBy : public Operation {
 public:
  void enableCaching() {
    enableLightweightResultCaching(1000);  // 1K groupby cache (memory intensive)
  }
};
```

**Expected Improvement**: 10-20% in aggregation-heavy queries

### Application 4: Index Scan Predicate Indexing

**Problem**: Finding which index scans apply to given predicates (O(n))

**Solution**: Build predicate-to-scan mapping

```cpp
class IndexScan : public Operation {
 public:
  // In query planner
  static void buildPredicateIndex(const std::vector<IndexScan*>& scans) {
    predicateIndex_.buildIndex(scans);
  }

  // Get scans that could match these predicates (O(1))
  static auto getCandidateScans(const std::vector<std::string>& predicates) {
    return predicateIndex_.getCandidates(predicates);
  }
};
```

**Expected Improvement**: 5-15% in large schema queries (100+ index patterns)

---

## Implementation Roadmap

### Phase 1: Foundation (Week 1)
1. Create `src/util/OperationPerformanceMonitor.h`
2. Create `src/engine/PatternIndexManager.h` (template)
3. Implement basic metrics tracking
4. **Time**: 2-3 days
5. **Risk**: Low (no changes to existing code)

### Phase 2: Operation Integration (Week 2)
1. Add lightweight caching to `Operation` base class
2. Extend `QueryExecutionTree` with caching API
3. Update `QueryExecutionContext` with optimization config
4. **Time**: 2-3 days
5. **Risk**: Medium (touches core Operation class)

### Phase 3: Specific Optimizations (Week 3)
1. Enable lightweight caching in Join operations
2. Enable lightweight caching in Filter operations
3. Enable predicate indexing for IndexScan
4. **Time**: 2-3 days
5. **Risk**: Medium (tests required)

### Phase 4: Validation & Tuning (Week 4)
1. Benchmark improvements on standard queries
2. Tune cache sizes for different operation types
3. Document optimization opportunities
4. **Time**: 1-2 days
5. **Risk**: Low (no functional changes)

---

## Compatibility & Risk Assessment

### ✅ Backward Compatibility

- All optimizations are **opt-in**
- No changes to existing validation logic
- Existing code works unchanged
- Configuration via `OptimizationConfig`

### ✅ Integration with Existing Cache

- New lightweight cache complements existing `Cache.h`
- `Cache.h`: Flexible, memory-managed cache for large datasets
- New lightweight cache: Simple, opt-in cache for specific operations
- Both can coexist

### ✅ Thread Safety

- `OperationPerformanceMonitor` uses `Synchronized<>` pattern
- Pattern index is built once, then read-only
- Compatible with existing threading model

### ⚠️ Memory Considerations

- Lightweight cache must respect memory limits
- Configure per-operation cache size based on heap
- Total cache: operation_count × cache_size_per_op
- Example: 50 operations × 1K entries × 100B = 5MB (acceptable)

---

## Expected Performance Improvements

### Estimated Speedups by Query Type

| Query Type | With Lightweight Cache | With Pattern Index | Combined |
|-----------|----------------------|-------------------|----------|
| Simple SELECT | 1.0x (no benefit) | 1.0x | 1.0x |
| JOIN-heavy | 1.2-1.5x | 1.1x | 1.3-1.6x |
| FILTER-heavy | 1.3-1.8x | 1.0x | 1.3-1.8x |
| AGGREGATE | 1.1-1.3x | 1.0x | 1.1-1.3x |
| Complex multi-join | 1.5-2.5x | 1.2-1.5x | 1.8-3.5x |

### Memory Overhead

- Lightweight cache: ~100 bytes per cached entry
- Pattern index: ~50 bytes per pattern mapping
- Metrics: ~1KB per operation type
- **Total for typical query**: <10MB for 50 operations

---

## Comparison: New vs. Existing Caching

### Existing `Cache.h` (Sophisticated)

**Pros**:
- Customizable eviction policies
- Memory-size based capacity
- Production-proven

**Cons**:
- More complex to use
- Overhead for simple caching needs
- Requires understanding of priority queues

### New Lightweight Cache (80/20)

**Pros**:
- Simple API (3-4 methods)
- No configuration needed
- Minimal overhead
- Clear performance visibility

**Cons**:
- Fixed-size, not memory-based
- LRU only (no custom policies)
- Less flexible

### Recommendation

- Use existing `Cache.h` for: Index caching, vocabulary caching, complex patterns
- Use new lightweight cache for: Operation result caching, temporary patterns
- Both serve different purposes and can coexist

---

## Key Files to Create/Modify

### New Files
1. `src/util/OperationPerformanceMonitor.h` (150 lines)
2. `src/engine/PatternIndexManager.h` (200 lines, template)
3. `src/util/ResultCache.h` (100 lines)
4. `src/engine/OptimizationConfig.h` (50 lines)

### Modified Files
1. `src/engine/Operation.h` (+20 lines public API)
2. `src/engine/QueryExecutionTree.h` (+10 lines)
3. `src/engine/QueryExecutionContext.h` (+15 lines)
4. `src/engine/Operation.cpp` (+50 lines implementation)

### Documentation
1. `docs/OPERATION_CACHING.md` (similar to ShEx guide)
2. `docs/PATTERN_INDEXING.md`
3. `docs/QUERY_OPTIMIZATION_API.md`

---

## Configuration Examples

### Enable All Optimizations

```cpp
// In query execution setup
queryContext->getOptimizationConfig().enableLightweightCaching = true;
queryContext->getOptimizationConfig().enablePatternIndexing = true;
queryContext->getOptimizationConfig().enablePerformanceMonitoring = true;

// Build execution tree with optimizations
auto tree = buildQueryExecutionTree(queryContext);
```

### Disable for Memory-Constrained Systems

```cpp
queryContext->getOptimizationConfig().enableLightweightCaching = false;
queryContext->getOptimizationConfig().enablePatternIndexing = true;  // Still use index
queryContext->getOptimizationConfig().enablePerformanceMonitoring = false;
```

### Monitor Performance

```cpp
// After query execution
auto perfReport = tree->getPerformanceReport();
for (const auto& [opName, metrics] : perfReport.operationMetrics) {
  std::cout << opName << ": "
            << (metrics.getCacheHitRate() * 100) << "% hit rate\n";
}
```

---

## Risk Mitigation

### Test Strategy

1. **Unit Tests**:
   - Cache hit/miss logic
   - Pattern index correctness
   - Metrics calculation

2. **Integration Tests**:
   - Operations with/without caching behave identically
   - Cache respects memory limits
   - Pattern index speeds up queries

3. **Performance Tests**:
   - Measure actual speedup vs. overhead
   - Profile memory usage
   - Validate improvement percentages

### Rollout Strategy

1. **Phase 1**: Implement with feature flag (default off)
2. **Phase 2**: Enable for internal testing
3. **Phase 3**: Enable for specific query types (JOIN-heavy)
4. **Phase 4**: Enable globally with monitoring

### Fallback Plan

- All optimizations are opt-in → disable if issues
- No functional changes → completely safe to disable
- Existing tests continue to pass

---

## Conclusion

The 80/20 optimization pattern proven in ShEx can be successfully generalized across QLever's query execution engine by:

1. **Creating lightweight utilities** that complement existing infrastructure
2. **Integrating with Operation and QueryExecutionTree** via optional APIs
3. **Targeting specific bottlenecks** (JOIN, FILTER, GROUP-BY operations)
4. **Maintaining backward compatibility** through opt-in configuration

**Expected Benefits**:
- 1.3-3.5x speedup on complex queries
- <10MB memory overhead
- Clear performance visibility
- Production-ready in 4 weeks

**Recommendation**: Proceed with Phase 1-2 implementation to establish foundation, then optimize high-impact operations (JOIN, FILTER) based on benchmarking results.

---

## References

- **Existing ShEx optimizations**: `docs/ShEx_Performance_80_20.md`
- **QLever caching infrastructure**: `src/util/Cache.h`, `src/util/LruCache.h`
- **Operation architecture**: `src/engine/Operation.h`
- **Query execution**: `src/engine/QueryExecutionTree.h`
