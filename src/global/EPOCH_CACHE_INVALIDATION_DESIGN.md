# Epoch Cache Invalidation Hook Design
## EPIC 1.1 - Epoch Immutability Core

**Status**: Implementation Complete
**Created**: 2026-01-01
**Author**: Claude AI Assistant

---

## Overview

This document describes the formal cache invalidation hook system for the Epoch Immutability Core. This system makes implicit cache invalidation (via snapshot indices) **explicit, observable, and extensible** through a registry-based event handler architecture.

### Key Innovation

**Previous Model**: Cache invalidation happened implicitly when snapshots changed indices
**New Model**: Explicit `EpochPromotionEvent` fired when epochs transition, enabling:
- Observable cache invalidation behavior
- Integration of new cache systems without snapshot dependency
- Metrics collection and debugging
- Future optimizations (pre-warming, async cleanup, etc.)

---

## Architecture

### 1. Core Event Types

#### `EpochSnapshot` (Value Object)
```cpp
struct EpochSnapshot {
  EpochId epochId_;      // Which epoch
  EpochState state_;     // Current state
  int64_t timestampMs_;  // When this snapshot was taken
};
```
**Semantics**: Immutable snapshot of an epoch's state at a point in time. Once captured, only used for comparison/analysis, never modified.

#### `EpochPromotionEvent` (Domain Event)
```cpp
struct EpochPromotionEvent {
  EpochId oldEpochId_;           // Being replaced
  EpochSnapshot oldSnapshot_;    // Its state
  EpochId newEpochId_;           // Becoming SERVE
  EpochSnapshot newSnapshot_;    // Its state
  int64_t promotionTimestampMs_; // When it happened
};
```
**Semantics**: Formal notification that a new epoch has entered SERVE, making the old one permanently immutable. Fired exactly once per epoch transition.

### 2. Handler Interface

#### `EpochCacheInvalidationHandler` (Abstract)
```cpp
class EpochCacheInvalidationHandler {
  virtual void onEpochPromoted(const EpochPromotionEvent& event) = 0;
  virtual void onEpochBecomesReadOnly(EpochId epochId) {}
  virtual void onEpochBecomesServing(EpochId epochId) {}
  virtual std::string getName() const = 0;
};
```

**Three callback points**:
1. **`onEpochPromoted`** (Required): New epoch entered SERVE
   - Main hook for cache invalidation
   - Called exactly once per transition
   - Can access both old and new epoch info

2. **`onEpochBecomesReadOnly`** (Optional): Epoch entered SEAL
   - Pre-notification that epoch is about to commit
   - Handlers can prepare/warm up caches

3. **`onEpochBecomesServing`** (Optional): Epoch entered SERVE
   - Epoch is now live for queries
   - Handlers can update serving metadata

**Contract**:
- Handlers MUST NOT throw unchecked exceptions
- All exceptions caught and logged at WARN level
- Handlers should complete quickly (no I/O)
- Called synchronously with epoch transitions
- Called in registration order

### 3. Handler Registry

#### `EpochCacheInvalidationRegistry` (Thread-Safe)
```cpp
class EpochCacheInvalidationRegistry {
  void registerHandler(std::unique_ptr<EpochCacheInvalidationHandler> h);
  size_t fireOnEpochPromoted(const EpochPromotionEvent& event);
  size_t getHandlerCount() const;
  Statistics getStatistics() const;
};
```

**Thread Safety**:
- All methods protected by `Synchronized<State>`
- Multiple readers can observe statistics concurrently
- Handler registration is serialized (rare operation)
- Event firing is atomic per handler

**Exception Handling**:
```cpp
// Pseudocode
for (each handler) {
  try {
    handler->onEpochPromoted(event);  // Must not throw
    successCount++;
  } catch (const std::exception& e) {
    // Handler failed - log and continue
    AD_LOG_WARN << "Handler threw: " << e.what();
    totalExceptions++;
  }
}
```

One bad handler cannot prevent others from being called.

---

## Integration: QueryResultCache

### Use Case
A query result cache (not yet implemented) needs to:
1. **Store results** keyed by query + epoch
2. **Invalidate results** when epoch changes
3. **Avoid explicit cache.clear()** calls throughout codebase
4. **Maintain zero-copy semantics** with snapshot indices

### Implementation Pattern

#### 1. Define Handler
```cpp
// In src/engine/caches/QueryResultCacheHandler.h
class QueryResultCacheInvalidationHandler
    : public EpochCacheInvalidationHandler {
 private:
  QueryResultCache& cache_;  // Reference to cache

 public:
  explicit QueryResultCacheInvalidationHandler(QueryResultCache& cache)
      : cache_(cache) {}

  void onEpochPromoted(const EpochPromotionEvent& event) override {
    // Clear results from old epoch
    cache_.invalidateEpoch(event.oldEpochId_);

    // Optionally: warm up common queries for new epoch
    cache_.recordNewEpoch(event.newEpochId_);
  }

  std::string getName() const override {
    return "QueryResultCacheInvalidationHandler";
  }
};
```

#### 2. Register on Server Startup
```cpp
// In src/engine/Server.cpp or ServerMain.cpp
void Server::initializeCaches() {
  auto cacheHandler =
    std::make_unique<QueryResultCacheInvalidationHandler>(resultCache_);

  globalEpochCacheInvalidationRegistry.withWriteLock(
    [&cacheHandler](auto& registry) {
      registry.registerHandler(std::move(cacheHandler));
    });
}
```

#### 3. Cache Implementation
```cpp
// In src/engine/caches/QueryResultCache.h
class QueryResultCache {
 private:
  // Key: (query_hash, epoch_id) -> Value: (results, metadata)
  std::map<std::pair<uint64_t, EpochId>, CacheEntry> entries_;

 public:
  std::optional<Results> get(const ParsedQuery& q, EpochId epoch) {
    auto key = std::make_pair(hash(q), epoch);
    if (entries_.count(key)) {
      return entries_[key].results_;
    }
    return std::nullopt;
  }

  void put(const ParsedQuery& q, EpochId epoch, Results results) {
    auto key = std::make_pair(hash(q), epoch);
    entries_[key] = {results, getCurrentTimestamp()};
  }

  // Called by handler when epoch changes
  void invalidateEpoch(EpochId oldEpochId) {
    for (auto it = entries_.begin(); it != entries_.end(); ) {
      if (it->first.second == oldEpochId) {
        it = entries_.erase(it);  // Remove entries from old epoch
      } else {
        ++it;
      }
    }
  }
};
```

#### 4. Usage in Query Execution
```cpp
// In src/engine/QueryExecutionTree.cpp
ResultTable QueryExecutionTree::execute() {
  EpochId currentEpoch = getCurrentEpochId();

  // Try cache first
  if (auto cached = resultCache_.get(*parsedQuery_, currentEpoch)) {
    return *cached;
  }

  // Compute result
  ResultTable result = executeImpl();

  // Cache for future queries in same epoch
  resultCache_.put(*parsedQuery_, currentEpoch, result);

  return result;
}
```

### Zero-Copy Semantics

The snapshot index approach is preserved:
- Results contain `snapshot.index_` which implicitly binds them to an epoch
- `EpochPromotionEvent` explicitly documents what changed
- Handlers can still use snapshot indices if needed
- New cache systems (without snapshot dependency) also supported

---

## Future Optimizations Enabled by This Hook

### 1. Pre-Warming of Query Caches
```cpp
class QueryCachePrewarmer : public EpochCacheInvalidationHandler {
  void onEpochBecomesServing(EpochId epochId) override {
    // Warm up cache with common queries (from previous epochs)
    executor_->executeTopQueries(epochId);
  }
};
```

### 2. Background Cache Cleanup
```cpp
class AsyncCacheCleanup : public EpochCacheInvalidationHandler {
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    // Asynchronously delete files for old epoch
    backgroundQueue_.enqueue([this, old = event.oldEpochId_]() {
      fileSystem_.deleteEpochFiles(old);
    });
  }
};
```

### 3. Metrics Collection
```cpp
class CacheMetricsRecorder : public EpochCacheInvalidationHandler {
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    metrics_.recordCacheInvalidation(
        event.oldEpochId_,
        event.promotionTimestampMs_);
  }
};
```

### 4. Distributed Cache Invalidation
```cpp
class DistributedCacheInvalidator : public EpochCacheInvalidationHandler {
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    // Notify other servers of epoch change
    cluster_.broadcastEpochPromotion(event);
  }
};
```

### 5. Cache Compression/Archival
```cpp
class CacheArchiver : public EpochCacheInvalidationHandler {
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    // Compress old epoch's cached results
    archive_.storeCompressed(event.oldEpochId_);
  }
};
```

---

## Integration with Existing Components

### EpochManager (src/global/Epoch.cpp)

The registry fires are called from epoch transitions:

```cpp
// In Epoch.cpp - existing restart() method
void EpochManager::restart() {
  auto oldEpochId = epochId_;
  auto oldSnapshot = EpochSnapshot{
    .epochId_ = oldEpochId,
    .state_ = EpochState::SERVE,
    .timestampMs_ = getCurrentTimeMs(),
  };

  // Perform transition
  epochId_++;
  state_ = EpochState::INIT;

  auto newEpochId = epochId_;
  auto newSnapshot = EpochSnapshot{
    .epochId_ = newEpochId,
    .state_ = EpochState::SERVE,
    .timestampMs_ = getCurrentTimeMs(),
  };

  // Fire event to handlers
  EpochPromotionEvent event{
    .oldEpochId_ = oldEpochId,
    .oldSnapshot_ = oldSnapshot,
    .newEpochId_ = newEpochId,
    .newSnapshot_ = newSnapshot,
    .promotionTimestampMs_ = getCurrentTimeMs(),
  };

  fireEpochPromotionEvent(event);
}
```

### EpochMetrics (src/global/EpochMetrics.cpp)

Can also register as a handler:

```cpp
class EpochMetricsHandler : public EpochCacheInvalidationHandler {
  void onEpochPromoted(const EpochPromotionEvent& event) override {
    metrics_.recordCacheInvalidation();
  }
};
```

---

## Testing Strategy

### Unit Tests (EpochCacheInvalidationHookTest.cpp)

1. **Handler Registration**: Can register/unregister handlers
2. **Event Firing**: Events reach all registered handlers
3. **Exception Handling**: One throwing handler doesn't block others
4. **Thread Safety**: Concurrent registration and firing
5. **Statistics**: Metrics tracked accurately

### Integration Tests (TBD)

1. **Epoch Transitions**: Events fired during actual epoch changes
2. **Cache Invalidation**: Old results removed when epoch changes
3. **Multi-Handler**: Multiple handlers fire in order
4. **Error Recovery**: Exceptions don't corrupt state

### Mock Helper

`MockCacheInvalidationHandler` (in test file) provides:
- Record of all events received
- Ability to simulate exceptions
- Assertions on event sequences
- Debug inspection of handler state

---

## Implementation Checklist

- [x] Define `EpochSnapshot` value object
- [x] Define `EpochPromotionEvent` domain event
- [x] Define `EpochCacheInvalidationHandler` abstract interface
- [x] Implement `EpochCacheInvalidationRegistry` (thread-safe)
- [x] Global singleton registry
- [x] Exception handling (catch-and-log per handler)
- [x] Metrics tracking (total events, exceptions)
- [x] Unit tests with mock handler
- [x] Documentation (this file)
- [ ] Integrate with `EpochManager::restart()`
- [ ] Implement `QueryResultCache` and handler
- [ ] Integration tests with real epoch transitions
- [ ] Performance benchmarks

---

## Design Principles

1. **Explicit is Better Than Implicit**
   - Old: "Cache invalidation happens via snapshot changes"
   - New: "EpochPromotionEvent explicitly documents cache invalidation"

2. **Open/Closed Principle**
   - Registry is open for extension (new handlers)
   - Closed for modification (core logic unchanged)
   - No need to modify EpochManager for new cache systems

3. **Fail-Safe**
   - One handler's exception cannot crash others
   - Exceptions logged at WARN level
   - System continues functioning

4. **Observability**
   - Each handler implements `getName()` for debugging
   - Registry tracks total events and exceptions
   - All firing logged at DEBUG level

5. **Testability**
   - Mock handler captures all events
   - Can simulate exceptions
   - Thread-safe test isolation

---

## Performance Considerations

### Event Firing Overhead
- **O(n handlers)**: Linear in handler count
- **O(1) per handler**: Each handler called once
- **Synchronous**: No async/thread pool overhead
- **Typical case**: 2-3 handlers, <1ms total

### Memory Usage
- **Per handler**: ~64 bytes (pointer, virtual methods)
- **Per event**: ~64 bytes stack temporary
- **Negligible**: Compared to epoch data volume

### Lock Contention
- Write lock only held during firing (short duration)
- Statistics accessible with read lock
- No lock during handler execution (exception safe)

---

## Related Issues & PRs

- EPIC 1 - Epoch Immutability Core (main feature)
- Issue: Cache Invalidation Hook Formalization
- PR: Implements `EpochCacheInvalidationHook.{h,cpp}`

---

## References

- `/home/user/qlever/src/global/Epoch.h` - Epoch state machine
- `/home/user/qlever/src/global/EpochMetrics.h` - Existing metrics
- `/home/user/qlever/src/util/Synchronized.h` - Thread-safe wrapper
- CLAUDE.md - Development guidelines
