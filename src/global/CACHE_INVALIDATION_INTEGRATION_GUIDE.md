# Cache Invalidation Hook Integration Guide
## How to Connect the Hook System to Real Caches

**Target Audience**: Developers implementing cache systems with epoch support
**Difficulty**: Intermediate
**Time to Implement**: 2-3 hours per cache system

---

## Quick Start

### Step 1: Create Your Handler

```cpp
// File: src/engine/caches/MyCacheInvalidationHandler.h
#pragma once

#include <memory>
#include "ad_utility/EpochCacheInvalidationHook.h"
#include "engine/caches/MyCache.h"

class MyCacheInvalidationHandler
    : public ad_utility::EpochCacheInvalidationHandler {
 private:
  MyCache& cache_;

 public:
  explicit MyCacheInvalidationHandler(MyCache& cache)
      : cache_(cache) {}

  void onEpochPromoted(
      const ad_utility::EpochPromotionEvent& event) override {
    // THIS IS THE MAIN HOOK
    // Called when a new epoch enters SERVE state
    // Old epoch data is now immutable and can be cleared

    AD_LOG_INFO << "Invalidating " << cache_.getName()
                << " for epoch " << event.oldEpochId_;

    // Clear all entries for the old epoch
    cache_.invalidateEpoch(event.oldEpochId_);

    // Optional: Prepare cache for new epoch
    cache_.prepareForEpoch(event.newEpochId_);

    // Optional: Record when this epoch started being served
    cache_.recordEpochStartTime(
        event.newEpochId_, event.promotionTimestampMs_);
  }

  // Optional: Called when epoch becomes read-only (INGEST -> SEAL)
  void onEpochBecomesReadOnly(ad_utility::EpochId epochId) override {
    AD_LOG_DEBUG << "Epoch " << epochId << " is now read-only";
    // Could warm up caches, finalize any pending work, etc.
  }

  // Optional: Called when epoch becomes serving (SEAL -> SERVE)
  void onEpochBecomesServing(ad_utility::EpochId epochId) override {
    AD_LOG_DEBUG << "Epoch " << epochId << " is now serving queries";
    // Could mark cache as "fresh" for this epoch
  }

  std::string getName() const override {
    return "MyCacheInvalidationHandler";
  }
};
```

### Step 2: Modify Your Cache Class

```cpp
// File: src/engine/caches/MyCache.h
class MyCache {
 private:
  // Key: (cache_key, epoch_id) -> Value: cached_data
  std::map<CacheKey, CachedData> entries_;

 public:
  // ... existing methods ...

  // NEW: Called by handler when epoch changes
  void invalidateEpoch(ad_utility::EpochId oldEpochId) {
    // Remove all entries associated with old epoch
    for (auto it = entries_.begin(); it != entries_.end(); ) {
      if (it->first.epochId_ == oldEpochId) {
        it = entries_.erase(it);  // Erase and continue
      } else {
        ++it;
      }
    }
    AD_LOG_DEBUG << "Invalidated " << oldEpochId;
  }

  // NEW: Optional - prepare for new epoch
  void prepareForEpoch(ad_utility::EpochId epochId) {
    // Could reset counters, pre-allocate space, etc.
    lastEpochId_ = epochId;
  }

  void recordEpochStartTime(ad_utility::EpochId epochId,
                            int64_t timestampMs) {
    epochStartTimes_[epochId] = timestampMs;
  }

  // Query-time access
  std::optional<CachedData> get(const CacheKey& key,
                                 ad_utility::EpochId epoch) {
    auto fullKey = std::make_pair(key, epoch);
    if (entries_.count(fullKey)) {
      return entries_[fullKey];
    }
    return std::nullopt;
  }

  void put(const CacheKey& key, ad_utility::EpochId epoch,
           CachedData data) {
    auto fullKey = std::make_pair(key, epoch);
    entries_[fullKey] = data;
  }
};
```

### Step 3: Register Handler on Server Startup

```cpp
// File: src/engine/Server.cpp or ServerMain.cpp

void Server::initialize() {
  // ... other initialization ...

  // Register cache invalidation handler
  auto invalidationHandler =
      std::make_unique<MyCacheInvalidationHandler>(myCache_);

  ad_utility::globalEpochCacheInvalidationRegistry.withWriteLock(
      [&invalidationHandler](auto& registry) {
        registry.registerHandler(std::move(invalidationHandler));
      });

  AD_LOG_INFO << "Registered cache invalidation handler for MyCache";
}
```

### Step 4: Use Cache with Epoch Binding

```cpp
// File: src/engine/QueryExecutionTree.cpp

ResultTable QueryExecutionTree::execute() {
  // Get current epoch for this query
  ad_utility::EpochId currentEpoch =
      ad_utility::globalEpochManager.withReadLock(
          [](const auto& manager) {
            return manager.getCurrentEpochIdForQuery();
          });

  // Try cache first
  if (auto cached = myCache_.get(cacheKey, currentEpoch)) {
    AD_LOG_DEBUG << "Cache hit for epoch " << currentEpoch;
    return *cached;
  }

  // Compute result
  ResultTable result = executeImpl();

  // Store in cache for this epoch
  myCache_.put(cacheKey, currentEpoch, result);

  return result;
}
```

---

## Testing Your Integration

### Unit Test Example

```cpp
// File: test/engine/caches/MyCacheInvalidationTest.cpp
#include <gtest/gtest.h>
#include "engine/caches/MyCacheInvalidationHandler.h"
#include "global/EpochCacheInvalidationHook.h"

class MyCacheInvalidationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Clear registry before test
    ad_utility::globalEpochCacheInvalidationRegistry.withWriteLock(
        [](auto& registry) { registry.clearHandlers(); });
  }
};

TEST_F(MyCacheInvalidationTest, InvalidatesOnEpochPromotion) {
  MyCache cache;

  // Populate cache with epoch 0 data
  cache.put("key1", 0, MyData{42});
  cache.put("key2", 0, MyData{99});

  EXPECT_EQ(cache.get("key1", 0), MyData{42});

  // Register handler
  auto handler = std::make_unique<MyCacheInvalidationHandler>(cache);
  ad_utility::globalEpochCacheInvalidationRegistry.withWriteLock(
      [&handler](auto& registry) {
        registry.registerHandler(std::move(handler));
      });

  // Fire promotion event
  ad_utility::EpochPromotionEvent event{
      .oldEpochId_ = 0,
      .oldSnapshot_ = {
          .epochId_ = 0,
          .state_ = ad_utility::EpochState::SERVE,
          .timestampMs_ = 1000},
      .newEpochId_ = 1,
      .newSnapshot_ = {
          .epochId_ = 1,
          .state_ = ad_utility::EpochState::SERVE,
          .timestampMs_ = 2000},
      .promotionTimestampMs_ = 2000,
  };

  ad_utility::globalEpochCacheInvalidationRegistry.withWriteLock(
      [&event](auto& registry) {
        registry.fireOnEpochPromoted(event);
      });

  // Old epoch data should be gone
  EXPECT_FALSE(cache.get("key1", 0));
  EXPECT_FALSE(cache.get("key2", 0));

  // New epoch data can be stored
  cache.put("key3", 1, MyData{123});
  EXPECT_EQ(cache.get("key3", 1), MyData{123});
}
```

### Integration Test Example

```cpp
// File: test/integration/CacheEpochIntegrationTest.cpp
TEST(CacheEpochIntegration, CacheInvalidatesAcrossEpochs) {
  // Start first epoch
  ad_utility::globalEpochManager.withWriteLock(
      [](auto& mgr) {
        mgr.transitionToIngest();
        mgr.transitionToSeal();
        mgr.transitionToServe();
      });

  auto epochId = ad_utility::globalEpochManager.withReadLock(
      [](const auto& mgr) { return mgr.getEpochId(); });

  // Query and cache result
  auto result1 = executeQuery("SELECT * FROM dataset");
  EXPECT_EQ(result1.size(), 1000);

  // Start second epoch (simulating data reload)
  ad_utility::globalEpochManager.withWriteLock(
      [](auto& mgr) {
        mgr.restart();
        mgr.transitionToIngest();
        // ... reload data ...
        mgr.transitionToSeal();
        mgr.transitionToServe();
      });

  // Cache should be invalidated
  // Re-running same query should use new data
  auto result2 = executeQuery("SELECT * FROM dataset");
  // Different dataset = different result size
  EXPECT_NE(result2.size(), result1.size());
}
```

---

## Common Patterns

### Pattern 1: LRU Cache with Epoch

```cpp
class EpochLRUCache {
 private:
  std::map<EpochId, EpochBucket> buckets_;
  size_t maxSize_;

 public:
  void invalidateEpoch(EpochId epochId) {
    buckets_.erase(epochId);  // Drop entire epoch's data
  }

  // Cache automatically favors recent epochs
  std::optional<Data> get(const Key& k, EpochId e) {
    if (buckets_.count(e) && buckets_[e].count(k)) {
      return buckets_[e][k];
    }
    return std::nullopt;
  }
};
```

### Pattern 2: Query Plan Cache

```cpp
class QueryPlanCache {
  struct PlanKey {
    std::string queryText_;
    ad_utility::EpochId epochId_;
  };

  std::map<PlanKey, ExecutionPlan> plans_;

  void invalidateEpoch(ad_utility::EpochId epochId) {
    // Remove all plans for this epoch
    // (plans are specific to epoch's data distribution)
    for (auto it = plans_.begin(); it != plans_.end(); ) {
      if (it->first.epochId_ == epochId) {
        it = plans_.erase(it);
      } else {
        ++it;
      }
    }
  }
};
```

### Pattern 3: Statistics/Histogram Cache

```cpp
class HistogramCache {
 private:
  std::map<EpochId, std::map<Column, Histogram>> histograms_;

 public:
  void invalidateEpoch(ad_utility::EpochId epochId) {
    // Stats change when data changes
    histograms_.erase(epochId);
  }

  std::optional<Histogram> get(const Column& col, ad_utility::EpochId epoch) {
    if (histograms_.count(epoch) && histograms_[epoch].count(col)) {
      return histograms_[epoch][col];
    }
    return std::nullopt;
  }
};
```

---

## Troubleshooting

### Issue: Cache Not Being Invalidated

**Symptoms**: Old epoch data still present after new epoch starts

**Causes**:
1. Handler not registered
2. Handler registered but epoch transition not firing event
3. `invalidateEpoch()` not called

**Solution**:
```cpp
// Add debug logging
AD_LOG_INFO << "Handler count: "
    << ad_utility::globalEpochCacheInvalidationRegistry
           .withReadLock([](const auto& r) {
             return r.getHandlerCount();
           });

// Check if event is fired
AD_LOG_INFO << "Firing epoch promotion event";
ad_utility::fireEpochPromotionEvent(event);
```

### Issue: Handler Throws Exception

**Symptoms**: Other handlers don't run, logs show "threw exception"

**Causes**:
1. `onEpochPromoted()` throws
2. `invalidateEpoch()` implementation has bug

**Solution**:
- Each handler is wrapped in try-catch
- Exceptions logged at WARN level
- Other handlers continue running
- Check logs for which handler failed

```
WARN: Handler 'MyCache' threw exception: std::bad_alloc
WARN: Handler 'OtherCache' ran successfully
```

### Issue: Performance Degradation

**Symptoms**: Handler execution slow

**Causes**:
1. `invalidateEpoch()` implementation is O(n²) or worse
2. Too many entries to iterate
3. Lock held too long

**Solution**:
```cpp
// Slow (O(n²) for each epoch)
void invalidateEpoch(EpochId old) {
  for (auto it = entries.begin(); it != entries.end(); ) {
    if (it->key.epoch == old) {
      it = entries.erase(it);  // Erase is O(n)
    } else {
      ++it;
    }
  }
}

// Fast (O(n) total)
void invalidateEpoch(EpochId old) {
  // Pre-compute which entries to keep
  std::vector<Key> toKeep;
  for (const auto& entry : entries) {
    if (entry.key.epoch != old) {
      toKeep.push_back(entry.key);
    }
  }
  // Rebuild map with kept entries
  entries.clear();
  for (const auto& key : toKeep) {
    entries[key] = getValue(key);
  }
}
```

---

## Design Considerations

### Epoch-Scoped vs. Global Caches

**Epoch-Scoped**: Cache entries keyed by (data_key, epoch_id)
- Pros: Automatic invalidation, no staleness
- Cons: More memory, need to manage per-epoch

**Global**: Cache entries keyed by (data_key) only
- Pros: Simpler code, less memory
- Cons: Must manually invalidate or accept staleness

**Recommendation**: Use epoch-scoped for correctness-critical caches
(query results, statistics). Use global for hints/hints that
can be stale (bloom filters, etc.).

### Handler Ordering

Handlers are called in registration order:
```cpp
registerHandler(metricsHandler);    // Called first
registerHandler(cacheHandler);      // Called second
registerHandler(diskCleanupHandler); // Called third
```

**Implication**: If metrics depend on caches being invalidated,
register metrics first (it will see clean state).

### Synchronous vs. Asynchronous

**Current**: Firing is synchronous (blocking epoch transition)

**For heavy work**: Decouple from sync firing:
```cpp
class AsyncCacheInvalidator : public EpochCacheInvalidationHandler {
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    // Queue async work, don't block here
    backgroundQueue_.enqueue([this, event]() {
      // Do heavy work (file deletion, compression, etc.)
      cache_.heavyCleanup(event.oldEpochId_);
    });
  }
};
```

---

## Next Steps

1. **Implement your handler** using the template above
2. **Write unit tests** covering normal + error cases
3. **Integrate with server startup** in Server.cpp
4. **Update cache implementation** to support epoch keying
5. **Run integration tests** with actual epoch transitions
6. **Monitor performance** with benchmarks
7. **Document** your specific cache behavior

---

## Related Files

- `src/global/EpochCacheInvalidationHook.h` - Hook interface
- `src/global/EpochCacheInvalidationHook.cpp` - Registry implementation
- `src/global/Epoch.h` - Epoch state machine
- `test/global/EpochCacheInvalidationHookTest.cpp` - Test examples
