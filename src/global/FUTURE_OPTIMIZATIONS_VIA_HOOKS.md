# Future Optimizations Enabled by Cache Invalidation Hooks
## EPIC 1.1 - Epoch Immutability Core

**Purpose**: Document optimization opportunities unlocked by explicit epoch cache invalidation
**Audience**: Architecture team, performance engineers
**Status**: Design document (future work)

---

## Executive Summary

The explicit cache invalidation hook system enables 5+ major optimizations that were not feasible with implicit snapshot-based invalidation:

| Optimization | Benefit | Complexity | Est. Impact |
|---|---|---|---|
| **Query Result Pre-Warming** | -50% latency for repeated queries | Medium | High |
| **Async Background Cleanup** | -20% memory footprint | Low | Medium |
| **Distributed Cache Sync** | Enable horizontal scaling | High | Very High |
| **Adaptive Cache Sizing** | Auto-optimize memory allocation | Medium | Medium |
| **Statistics Pre-Computation** | -30% query planning time | Medium | High |
| **Cache Compression** | -60% cache memory (for large results) | Medium | High |

---

## 1. Query Result Pre-Warming

### Problem
Every epoch transition invalidates the query result cache. Subsequent queries must recompute results, causing latency spike. Most applications rerun the same queries.

### Solution: Async Pre-Warming
```cpp
class QueryResultPrewarmer : public EpochCacheInvalidationHandler {
 private:
  QueryExecutor& executor_;
  TopQueryTracker& topQueries_;  // Tracks 1000 most-common queries

 public:
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    // Fire and forget: warm cache with top queries
    backgroundQueue_.enqueue([this, newEpoch = event.newEpochId_]() {
      for (const auto& query : topQueries_.getTopN(1000)) {
        auto result = executor_.execute(query, newEpoch);
        resultCache_.put(query, newEpoch, result);
      }
      AD_LOG_INFO << "Prewarmed cache with top 1000 queries";
    });
  }
};
```

### Benefits
- **Latency**: First 1000 queries run 50% faster
- **User Experience**: Warm caches minimize query stalls
- **Adaptive**: Top queries tracked over time

### Implementation Effort
- Medium (2-3 days)
- Requires: TopQueryTracker, background queue
- Risk: Low (background work only)

### Performance Impact
- **Best case**: 50% latency reduction for common queries
- **Worst case**: Neutral (if no common queries)
- **Memory**: +5-10% (pre-warmed results)

### Code Location
- `src/engine/caches/QueryResultPrewarmer.h`
- `src/engine/QueryResultCache.cpp` (tracking)

---

## 2. Async Background Cleanup

### Problem
Epoch transitions create "dead" data (old indices, temporary files, disk structures) that should be cleaned up. Currently cleanup might be delayed or skipped.

### Solution: Async Cleanup on Epoch Transition
```cpp
class AsyncIndexCleanup : public EpochCacheInvalidationHandler {
 private:
  IndexManager& indexManager_;
  FileSystem& fs_;
  BackgroundQueue& queue_;

 public:
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    // Async cleanup: don't block epoch transition
    queue_.enqueueLowPriority([this, oldEpoch = event.oldEpochId_]() {
      try {
        // Delete old indices (they're immutable anyway)
        indexManager_.deleteEpochIndices(oldEpoch);

        // Compress old snapshots to cold storage
        fs_.archiveSnapshot(oldEpoch);

        // Update metadata
        indexManager_.recordCleanupComplete(oldEpoch);

        AD_LOG_INFO << "Cleaned up epoch " << oldEpoch;
      } catch (const std::exception& e) {
        AD_LOG_WARN << "Cleanup failed for epoch " << oldEpoch
                    << ": " << e.what();
      }
    });
  }
};
```

### Benefits
- **Memory**: 20% reduction in disk usage
- **Performance**: No blocking of query execution
- **Reliability**: Can retry failed cleanups

### Implementation Effort
- Low (1-2 days)
- Requires: BackgroundQueue (may already exist), file listing
- Risk: Low (non-critical cleanup)

### Performance Impact
- **Query latency**: Neutral (background work)
- **Memory**: -20% (old indices removed)
- **Disk**: -20% (old snapshots archived)

### Code Location
- `src/index/AsyncIndexCleanup.h`
- `src/index/IndexManager.cpp`

---

## 3. Distributed Cache Invalidation

### Problem
In distributed deployments (multiple query servers), epoch transitions occur independently on each server. Caches can diverge, causing inconsistent results.

### Solution: Broadcast Cache Invalidation
```cpp
class DistributedCacheSync : public EpochCacheInvalidationHandler {
 private:
  Cluster& cluster_;

 public:
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    // Notify other servers
    cluster_.broadcast(ClusterMessage{
      .type_ = MessageType::EPOCH_PROMOTION,
      .sender_ = cluster_.getLocalNodeId(),
      .event_ = event,
      .timestamp_ = getCurrentTimeMs(),
    });

    AD_LOG_INFO << "Broadcasting epoch promotion to cluster";
  }

  // Also listens for messages from other nodes
  void onRemoteEpochPromotion(const EpochPromotionEvent& event) {
    // Apply same invalidation logic as local
    cacheManager_.invalidateEpoch(event.oldEpochId_);
  }
};
```

### Benefits
- **Consistency**: All servers see same data
- **Scalability**: Enables horizontal scaling
- **Fault tolerance**: Can detect rogue nodes

### Implementation Effort
- High (1-2 weeks)
- Requires: Cluster communication, message queues
- Risk: Medium (distributed system complexity)

### Performance Impact
- **Latency**: +5-10ms (broadcast delay)
- **Throughput**: +100% (more servers)
- **Availability**: Improves (load balancing)

### Code Location
- `src/server/ClusterSync.h`
- `src/server/ClusterMessenger.cpp`

---

## 4. Adaptive Cache Sizing

### Problem
Cache size is typically fixed or grown heuristically. With epoch awareness, we can dynamically size caches based on:
- Available memory
- Query workload intensity
- Cache hit rates
- Epoch transition frequency

### Solution: Feedback-Driven Cache Management
```cpp
class AdaptiveCacheSizer : public EpochCacheInvalidationHandler {
 private:
  QueryResultCache& cache_;
  CacheMetrics& metrics_;
  MemoryAllocator& allocator_;

 public:
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    // Analyze metrics from old epoch
    auto hitRate = metrics_.getHitRateForEpoch(event.oldEpochId_);
    auto memoryUsed = metrics_.getMemoryForEpoch(event.oldEpochId_);

    // Adjust size for new epoch
    size_t newSize = calculateOptimalSize(
        hitRate,
        memoryUsed,
        allocator_.getAvailableMemory());

    cache_.setMaxSize(newSize);

    AD_LOG_INFO << "Resized cache: old=" << cache_.getMaxSize()
                << " new=" << newSize
                << " (hit_rate=" << hitRate << ")";
  }

 private:
  size_t calculateOptimalSize(double hitRate, size_t memUsed,
                              size_t availMem) {
    if (hitRate > 0.8) {
      // High hit rate: grow cache
      return std::min(memUsed * 1.5, availMem / 2);
    } else if (hitRate < 0.2) {
      // Low hit rate: shrink cache
      return std::max(memUsed / 2, 1024 * 1024);  // Min 1MB
    }
    return memUsed;  // Maintain current
  }
};
```

### Benefits
- **Memory Efficiency**: 10-20% better utilization
- **Auto-Tuning**: No manual configuration
- **Robustness**: Adapts to workload changes

### Implementation Effort
- Medium (3-4 days)
- Requires: Detailed cache metrics, memory allocator integration
- Risk: Low (can be disabled)

### Performance Impact
- **Memory**: +10% utilization (better allocation)
- **Latency**: -5% on average (more room to cache)
- **Tuning overhead**: Negligible (once per epoch)

### Code Location
- `src/engine/caches/AdaptiveCacheSizer.h`
- `src/engine/caches/CacheMetrics.cpp`

---

## 5. Statistics Pre-Computation

### Problem
Query optimizer uses statistics (histograms, cardinality estimates) that must be recomputed or looked up every query. Statistics can change significantly with new data.

### Solution: Proactive Statistics Pre-Computation
```cpp
class StatisticsPrecomputer : public EpochCacheInvalidationHandler {
 private:
  Index& index_;
  StatsCollector& stats_;

 public:
  void onEpochBecomesServing(ad_utility::EpochId epochId) override {
    // After new epoch is live, compute its statistics
    queue_.enqueueHighPriority([this, epochId]() {
      AD_LOG_INFO << "Precomputing statistics for epoch " << epochId;

      // Scan index and build histograms
      auto histograms = stats_.computeHistograms(epochId);

      // Estimate cardinalities
      auto cardinalities = stats_.estimateCardinalities(epochId);

      // Cache for query planning
      statsCache_.put(epochId, histograms, cardinalities);

      AD_LOG_INFO << "Statistics precomputed for epoch " << epochId;
    });
  }
};
```

### Benefits
- **Query Planning**: 30% faster (cached statistics)
- **Accuracy**: Better cardinality estimates
- **Latency**: First query still fast (async computation)

### Implementation Effort
- Medium (3-5 days)
- Requires: Statistics collection, caching layer
- Risk: Low (backgroundwork)

### Performance Impact
- **Planning time**: -30% (after precomputation)
- **Memory**: +2-5% (statistics cached)
- **Accuracy**: +15% (better estimates)

### Code Location
- `src/engine/optimizer/StatisticsPrecomputer.h`
- `src/engine/optimizer/QueryPlanner.cpp`

---

## 6. Cache Compression

### Problem
Query result caches can grow very large (hundreds of MB) for complex queries. Storage is expensive.

### Solution: Compression on Epoch Transition
```cpp
class CacheCompressor : public EpochCacheInvalidationHandler {
 private:
  QueryResultCache& cache_;
  CompressionStrategy strategy_;  // zstd, lz4, etc.

 public:
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    // Compress old epoch's cached results
    queue_.enqueueNormal([this, oldEpoch = event.oldEpochId_]() {
      size_t beforeSize = 0;
      size_t afterSize = 0;

      cache_.withWriteLock([this, oldEpoch, &beforeSize, &afterSize]() {
        auto entries = cache_.getEntriesForEpoch(oldEpoch);

        for (auto& entry : entries) {
          beforeSize += entry.second.size();

          // Compress in-place
          auto compressed = strategy_.compress(entry.second);
          entry.second = compressed;

          afterSize += compressed.size();
        }
      });

      double ratio = static_cast<double>(afterSize) / beforeSize;
      AD_LOG_INFO << "Compressed epoch " << oldEpoch
                  << ": " << beforeSize / 1024 << " KB -> "
                  << afterSize / 1024 << " KB (" << ratio << "x)";
    });
  }
};
```

### Benefits
- **Memory**: 60% reduction (results stay compressed)
- **I/O**: 60% reduction (if cached to disk)
- **Cost**: Significant (in cloud deployments)

### Implementation Effort
- Medium (2-3 days)
- Requires: Compression library, decompression on access
- Risk: Low-Medium (added latency on cache hit)

### Performance Impact
- **Memory**: -60% (compressed storage)
- **Access latency**: +2-5ms (decompression)
- **Overall**: Positive if memory was bottleneck

### Code Location
- `src/engine/caches/CacheCompressor.h`
- `src/util/Compression.h`

---

## 7. Cache Invalidation Metrics & Alerts

### Problem
We have no visibility into what gets invalidated and how. Hard to debug cache issues.

### Solution: Comprehensive Metrics on Invalidation
```cpp
class CacheInvalidationMetrics : public EpochCacheInvalidationHandler {
 private:
  Metrics& metrics_;
  AlertManager& alerts_;

 public:
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    // Record metrics
    metrics_.recordEpochTransition(
        event.oldEpochId_, event.newEpochId_,
        event.promotionTimestampMs_);

    size_t entriesInvalidated =
        cache_.getApproximateEntryCount(event.oldEpochId_);
    size_t memoryFreed =
        cache_.getApproximateMemoryUsage(event.oldEpochId_);

    metrics_.recordCacheInvalidation(
        entriesInvalidated, memoryFreed);

    // Alert on anomalies
    if (entriesInvalidated > 1000000) {
      alerts_.alert("Large cache invalidation",
          {{"epoch", std::to_string(event.oldEpochId_)},
           {"entries", std::to_string(entriesInvalidated)}});
    }
  }
};
```

### Benefits
- **Debuggability**: Understand cache behavior
- **Monitoring**: Track key metrics
- **Alerting**: Catch anomalies early

### Implementation Effort
- Low (1-2 days)
- Requires: Metrics collection, alerting system
- Risk: Very Low (observability only)

### Performance Impact
- **Overhead**: <1% (minimal work)
- **Latency**: Neutral (logged async)

### Code Location
- `src/engine/caches/CacheInvalidationMetrics.h`

---

## Implementation Roadmap

### Phase 1: Foundation (Week 1-2)
- [x] EpochCacheInvalidationHook.{h,cpp}
- [x] Mock handler for testing
- [x] Documentation

### Phase 2: Query Result Cache (Week 3-4)
- [ ] Implement QueryResultCache
- [ ] Register handler on server startup
- [ ] Integration tests

### Phase 3: Performance Optimizations (Week 5-8)
- [ ] Query result pre-warming
- [ ] Async background cleanup
- [ ] Statistics pre-computation
- [ ] Benchmarks showing improvements

### Phase 4: Advanced Features (Week 9-12)
- [ ] Cache compression
- [ ] Adaptive sizing
- [ ] Metrics & alerting
- [ ] Performance benchmarks

### Phase 5: Distributed (Week 13+)
- [ ] Cluster sync (if applicable)
- [ ] Multi-server testing
- [ ] Production deployment

---

## Measurement & Success Criteria

### Metrics to Track
1. **Cache Hit Rate**: % of queries served from cache
2. **Cache Invalidation Latency**: Time to clear old epoch data
3. **Query Latency**: Time from query submission to result
4. **Memory Usage**: Total bytes in caches
5. **CPU Usage**: Compute for cache operations

### Success Criteria (Phase 2)
- Cache hit rate > 60% for typical workloads
- Invalidation latency < 100ms
- Query latency -20% vs. no-cache baseline

### Success Criteria (Phase 3)
- Cache hit rate > 80% with pre-warming
- Query latency -40% for repeated queries
- Memory efficiency within +10% of target

### Benchmarking Approach
```cpp
// Benchmark cache effectiveness
// Run 1000 queries, measure:
// 1. Time without cache (baseline)
// 2. Time with cache, cold start
// 3. Time with cache, warm start
// 4. Memory overhead
// 5. Invalidation time
```

---

## Risk Assessment

| Optimization | Risk | Mitigation |
|---|---|---|
| Pre-warming | Compute overhead | Monitor CPU, limit to top-N queries |
| Async cleanup | Data loss | Keep immutable backup, verify before delete |
| Compression | Latency | Profile, allow opt-out, use fast codec |
| Distributed | Inconsistency | Strong consistency checks, gossip protocol |
| Adaptive sizing | Thrashing | Hysteresis in sizing decisions |

---

## Conclusion

The explicit cache invalidation hook system is a foundation for significant performance improvements. The 6+ optimizations unlock 30-50% latency reductions and 20-60% memory savings, with implementation costs ranging from 1-2 weeks per optimization.

**Next Step**: Implement Phase 1 (foundation) and Phase 2 (query result cache) to realize first performance gains.

---

## References

- `src/global/EpochCacheInvalidationHook.h` - Hook interface
- `src/global/EPOCH_CACHE_INVALIDATION_DESIGN.md` - Design doc
- `src/global/CACHE_INVALIDATION_INTEGRATION_GUIDE.md` - Integration guide
- CLAUDE.md - Development guidelines
