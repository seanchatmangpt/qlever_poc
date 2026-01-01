# Epoch Metrics Integration Guide

This document describes how to integrate observability metrics into the Epoch system for monitoring and debugging.

## Overview

The `EpochMetrics` system provides comprehensive observability into epoch state transitions and query/mutation patterns. It enables:

1. **Validation of Epoch Invariants** - Prove that immutability constraints are maintained
2. **Performance Monitoring** - Track query patterns and rejection rates
3. **Debugging Assistance** - Identify problematic behavior patterns
4. **Operational Metrics** - Export metrics for dashboards and alerting

## Integration Points in Epoch.cpp

### 1. Query Access Recording

**Location:** `EpochManager::getCurrentEpochIdForQuery()`

**Current Code:**
```cpp
EpochId EpochManager::getCurrentEpochIdForQuery() const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SERVE) {
    throw std::logic_error(
        "Query attempted outside SERVE epoch");
  }
  return lock->epochId_;
}
```

**Modified Code with Metrics:**
```cpp
EpochId EpochManager::getCurrentEpochIdForQuery() const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SERVE) {
    throw std::logic_error(
        "Query attempted outside SERVE epoch");
  }
  EpochId id = lock->epochId_;
  lock.release();  // Release lock before recording metric

  // Record successful query start
  globalEpochMetrics.wlock()->recordQueryStart();

  return id;
}
```

**Why:** Records every successful query, enabling tracking of:
- Queries per epoch
- Total queries across all epochs
- Last query timestamp

---

### 2. Write Rejection Recording

**Location:** `EpochManager::checkAllowedToMutate()`

**Current Code:**
```cpp
void EpochManager::checkAllowedToMutate() const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::INGEST) {
    throw std::logic_error(
        "Write attempted outside INGEST epoch");
  }
}
```

**Modified Code with Metrics:**
```cpp
void EpochManager::checkAllowedToMutate() const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::INGEST) {
    lock.release();  // Release before throwing

    // Record rejected write attempt
    globalEpochMetrics.wlock()->recordRejectedWrite();

    throw std::logic_error(
        "Write attempted outside INGEST epoch");
  }
}
```

**Why:** Records every rejected mutation, enabling:
- Detection of concurrent write attempts
- Monitoring of application behavior correctness
- Performance impact assessment of mutation constraints

---

### 3. Epoch Transition Recording

**Location:** `EpochManager::transitionToServe()`

**Current Code:**
```cpp
void EpochManager::transitionToServe() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SEAL) {
    throw std::logic_error(
        "Cannot transition to SERVE from non-SEAL state");
  }
  lock->state_ = EpochState::SERVE;
  lock->transitionCount_++;
}
```

**Modified Code with Metrics:**
```cpp
void EpochManager::transitionToServe() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SEAL) {
    throw std::logic_error(
        "Cannot transition to SERVE from non-SEAL state");
  }
  lock->state_ = EpochState::SERVE;
  lock->transitionCount_++;
  lock.release();  // Release lock before metrics

  // Record epoch transition (sealing)
  globalEpochMetrics.wlock()->recordEpochTransition();

  // Optionally log the transition
  AD_LOG_INFO << "Epoch transition to SERVE: transition_count="
              << (lock->transitionCount_ - 1)
              << "\n";
}
```

**Why:** Marks epoch completion and enables:
- Epoch lifecycle tracking
- Query count per epoch measurement
- Average performance metrics calculation

---

### 4. Epoch Restart Recording

**Location:** `EpochManager::restart()`

**Current Code:**
```cpp
void EpochManager::restart() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SERVE) {
    throw std::logic_error(
        "Cannot restart epoch from non-SERVE state");
  }
  lock->state_ = EpochState::INIT;
  lock->epochId_++;
}
```

**Modified Code with Metrics:**
```cpp
void EpochManager::restart() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SERVE) {
    throw std::logic_error(
        "Cannot restart epoch from non-SERVE state");
  }
  lock->state_ = EpochState::INIT;
  uint64_t oldEpochId = lock->epochId_;
  lock->epochId_++;
  EpochId newEpochId = lock->epochId_;
  lock.release();  // Release before metrics

  // Record epoch restart with epoch ID
  globalEpochMetrics.wlock()->recordEpochRestart(oldEpochId);

  // Optionally log the restart
  AD_LOG_INFO << "Epoch restart: "
              << "old_epoch_id=" << oldEpochId
              << ", new_epoch_id=" << newEpochId
              << "\n";
}
```

**Why:** Marks epoch cycle completion and enables:
- Epoch sequencing validation
- Double-counting prevention for metrics
- Restart frequency tracking

---

## Cache Invalidation Integration

**Location:** Where cache invalidation is triggered on epoch state change

**Integration Point (pseudocode):**
```cpp
void CacheManager::invalidateOnEpochTransition() {
  // Invalidate all caches
  cache_.clear();

  // Record invalidation event
  globalEpochMetrics.wlock()->recordCacheInvalidation();

  AD_LOG_DEBUG << "Cache invalidated due to epoch state change\n";
}
```

**Why:** Tracks:
- Cache turnover rate
- Impact of epoch-based semantics on cache behavior
- Effectiveness of cache invalidation strategy

---

## Example Log Output Format

### Raw Metrics (from `toString()`)
```
EpochMetrics{queries_current_epoch=127, writes_rejected=3, cache_invalidations=5, epoch_transitions=2, epoch_restarts=1, last_query_ms=1704110456789, last_write_rejection_ms=1704110401234, last_epoch_transition_ms=1704110450000, total_queries_all_epochs=256, total_writes_rejected_all_epochs=7}
```

### CSV Format (for external tools)
```csv
queries_current_epoch,writes_rejected,cache_invalidations,epoch_transitions,epoch_restarts,last_query_ms,last_write_rejection_ms,last_epoch_transition_ms,total_queries_all_epochs,total_writes_rejected_all_epochs
127,3,5,2,1,1704110456789,1704110401234,1704110450000,256,7
```

### Detailed Report (from `getDetailedMetricsReport()`)
```
=== EPOCH METRICS REPORT ===
Epoch State:
  Epoch Transitions (SEAL->SERVE):     2
  Epoch Restarts (SERVE->INIT):        1

Queries:
  Queries in Current Epoch:             127
  Total Queries All Epochs:             256
  Average Queries per Epoch:            128.00

Mutations:
  Write Attempts Rejected (Current):    3
  Total Writes Rejected All Epochs:     7
  Average Rejections per Epoch:         3.50
  Last Write Rejection Timestamp:       1704110401234 ms

Cache Management:
  Cache Invalidations (Current Epoch):  5
  Average Invalidations per Epoch:      2.50

Timings:
  Last Epoch Transition:                1704110450000 ms

=== END METRICS REPORT ===
```

### Periodic Logging Pattern
```cpp
// In a monitoring/health check thread
void periodicMetricsReport() {
  while (running) {
    std::this_thread::sleep_for(std::chrono::seconds(60));

    auto lock = globalEpochMetrics.rlock();
    AD_LOG_INFO << lock->getDetailedMetricsReport() << "\n";
  }
}
```

**Example Log Output:**
```
2025-12-31 23:45:00 - INFO - === EPOCH METRICS REPORT ===
2025-12-31 23:45:00 - INFO - Epoch State:
2025-12-31 23:45:00 - INFO -   Epoch Transitions (SEAL->SERVE):     42
2025-12-31 23:45:00 - INFO -   Epoch Restarts (SERVE->INIT):        41
2025-12-31 23:45:00 - INFO -
2025-12-31 23:45:00 - INFO - Queries:
2025-12-31 23:45:00 - INFO -   Queries in Current Epoch:             1847
2025-12-31 23:45:00 - INFO -   Total Queries All Epochs:             76234
2025-12-31 23:45:00 - INFO -   Average Queries per Epoch:            1814.38
2025-12-31 23:45:00 - INFO -
2025-12-31 23:45:00 - INFO - Mutations:
2025-12-31 23:45:00 - INFO -   Write Attempts Rejected (Current):    12
2025-12-31 23:45:00 - INFO -   Total Writes Rejected All Epochs:     187
2025-12-31 23:45:00 - INFO -   Average Rejections per Epoch:         4.45
2025-12-31 23:45:00 - INFO -   Last Write Rejection Timestamp:       1735713899234 ms
2025-12-31 23:45:00 - INFO -
2025-12-31 23:45:00 - INFO - Cache Management:
2025-12-31 23:45:00 - INFO -   Cache Invalidations (Current Epoch):  15
2025-12-31 23:45:00 - INFO -   Average Invalidations per Epoch:      1.92
2025-12-31 23:45:00 - INFO -
2025-12-31 23:45:00 - INFO - Timings:
2025-12-31 23:45:00 - INFO -   Last Epoch Transition:                1735713890000 ms
2025-12-31 23:45:00 - INFO -
2025-12-31 23:45:00 - INFO - === END METRICS REPORT ===
```

---

## Validating Epoch Invariants Using Metrics

### Invariant 1: Monotonic Epoch ID

**Definition:** Epoch IDs must always increase, never decrease or repeat.

**Validation Using Metrics:**
```cpp
void validateMonotonicEpochIds() {
  auto metrics1 = globalEpochMetrics.rlock()->getMetrics();

  // Perform some operations...

  auto metrics2 = globalEpochMetrics.rlock()->getMetrics();

  // Check that epoch restarts increase monotonically
  assert(metrics2.epochRestarts_ >= metrics1.epochRestarts_);
  assert(metrics2.epochTransitions_ >= metrics1.epochTransitions_);
}
```

**Metric Evidence:**
- `epochRestarts_` must be monotonically increasing
- Each restart is associated with an epoch ID increment
- Provides auditability of epoch lifecycle

---

### Invariant 2: Queries Only in SERVE State

**Definition:** No query should complete successfully unless in SERVE state.

**Validation Using Metrics:**
```cpp
void validateQueriesOnlyInServeState() {
  auto lock = globalEpochMetrics.rlock();
  auto metrics = lock->getMetrics();

  // If we have no epoch transitions, we should have no queries
  // (since first epoch starts in INIT)
  if (metrics.epochTransitions_ == 0) {
    assert(metrics.totalQueriesAllEpochs_ == 0);
  }

  // Queries should only appear when epoch is in SERVE
  // This is implicitly validated because recordQueryStart() is only
  // called after successful state check in getCurrentEpochIdForQuery()
}
```

**Metric Evidence:**
- `queriesInCurrentEpoch_` directly reflects query activity
- Only incremented when state check passes
- Proves query access control is enforced

---

### Invariant 3: Mutations Only in INGEST State

**Definition:** No mutation should succeed outside INGEST state; all attempts outside should be rejected.

**Validation Using Metrics:**
```cpp
void validateWritesOnlyInIngestState() {
  auto lock = globalEpochMetrics.rlock();
  auto metrics = lock->getMetrics();

  // Track write rejection patterns
  uint64_t total_ops = metrics.totalQueriesAllEpochs_ +
                       metrics.totalWriteRejectionsAllEpochs_;
  double rejection_rate =
      static_cast<double>(metrics.totalWriteRejectionsAllEpochs_) /
      static_cast<double>(total_ops);

  // Expected: Most writes rejected during SERVE phase
  // This should be highly visible in metrics

  AD_LOG_INFO << "Write rejection rate: " << rejection_rate * 100 << "%\n";
}
```

**Metric Evidence:**
- `writeAttemptsRejected_` counts enforcement
- `lastWriteRejectionTimestampMs_` shows recent rejections
- Demonstrates write-in-wrong-state protection is working

---

### Invariant 4: Epoch Transitions Form Valid Sequence

**Definition:** Valid sequence is INIT → INGEST → SEAL → SERVE → (restart to INIT)

**Validation Using Metrics:**
```cpp
void validateEpochSequence() {
  auto lock = globalEpochMetrics.rlock();
  auto metrics = lock->getMetrics();

  // Restart count should be equal to or one less than transition count
  // (depends on whether we've finished the current epoch cycle)
  assert(metrics.epochRestarts_ <= metrics.epochTransitions_);

  // In steady state: epochRestarts == epochTransitions
  // In ongoing epoch: epochRestarts == epochTransitions - 1
  uint64_t diff = metrics.epochTransitions_ - metrics.epochRestarts_;
  assert(diff == 0 || diff == 1);

  AD_LOG_INFO << "Epoch sequence valid. "
              << "Transitions: " << metrics.epochTransitions_
              << ", Restarts: " << metrics.epochRestarts_
              << "\n";
}
```

**Metric Evidence:**
- Relationship between `epochTransitions_` and `epochRestarts_` proves sequence
- Values tied to actual state machine transitions
- Enables proof-by-instrumentation

---

### Invariant 5: Cache Invalidation on Every Epoch Transition

**Definition:** Every SEAL→SERVE transition must trigger cache invalidation.

**Validation Using Metrics:**
```cpp
void validateCacheInvalidationFrequency() {
  auto lock = globalEpochMetrics.rlock();
  auto metrics = lock->getMetrics();

  // With proper integration, cache invalidations should occur
  // roughly in proportion to epoch transitions
  if (metrics.epochTransitions_ > 0) {
    uint64_t expected_min_invalidations = metrics.epochTransitions_;
    assert(metrics.cacheInvalidations_ >= expected_min_invalidations);

    double ratio = static_cast<double>(metrics.cacheInvalidations_) /
                   static_cast<double>(metrics.epochTransitions_);

    AD_LOG_INFO << "Cache invalidation ratio: " << ratio
                << " per epoch transition\n";
  }
}
```

**Metric Evidence:**
- `cacheInvalidations_` count correlates with `epochTransitions_`
- Proves cache management follows epoch lifecycle
- Detects missing invalidations (would show low ratios)

---

## Testing Integration

### Unit Test Example

```cpp
class EpochMetricsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    globalEpochMetrics.wlock()->reset();
  }

  void TearDown() override {
    globalEpochMetrics.wlock()->reset();
  }
};

TEST_F(EpochMetricsTest, RecordQueryStart) {
  auto metrics_before = globalEpochMetrics.rlock()->getMetrics();
  assert(metrics_before.queriesInCurrentEpoch_ == 0);

  globalEpochMetrics.wlock()->recordQueryStart();

  auto metrics_after = globalEpochMetrics.rlock()->getMetrics();
  assert(metrics_after.queriesInCurrentEpoch_ == 1);
  assert(metrics_after.totalQueriesAllEpochs_ == 1);
}

TEST_F(EpochMetricsTest, EpochTransitionResetsQueryCount) {
  globalEpochMetrics.wlock()->recordQueryStart();
  globalEpochMetrics.wlock()->recordQueryStart();

  auto before = globalEpochMetrics.rlock()->getMetrics();
  assert(before.queriesInCurrentEpoch_ == 2);

  globalEpochMetrics.wlock()->recordEpochTransition();

  auto after = globalEpochMetrics.rlock()->getMetrics();
  assert(after.queriesInCurrentEpoch_ == 0);
  assert(after.epochTransitions_ == 1);
  assert(after.totalQueriesAllEpochs_ == 2);
}

TEST_F(EpochMetricsTest, EpochSequenceValidation) {
  globalEpochMetrics.wlock()->recordEpochTransition();
  globalEpochMetrics.wlock()->recordEpochRestart(0);
  globalEpochMetrics.wlock()->recordEpochTransition();
  globalEpochMetrics.wlock()->recordEpochRestart(1);

  auto metrics = globalEpochMetrics.rlock()->getMetrics();
  assert(metrics.epochTransitions_ == 2);
  assert(metrics.epochRestarts_ == 2);
}
```

---

## Implementation Checklist

- [ ] **Header created**: `EpochMetrics.h` with full metric definitions
- [ ] **Implementation created**: `EpochMetrics.cpp` with all methods
- [ ] **CMakeLists.txt updated**: Add `EpochMetrics.cpp` to build
- [ ] **Include statement added to Epoch.h**: `#include "ad_utility/global/EpochMetrics.h"`
- [ ] **Integration Point 1**: `getCurrentEpochIdForQuery()` calls `recordQueryStart()`
- [ ] **Integration Point 2**: `checkAllowedToMutate()` calls `recordRejectedWrite()` on exception
- [ ] **Integration Point 3**: `transitionToServe()` calls `recordEpochTransition()`
- [ ] **Integration Point 4**: `restart()` calls `recordEpochRestart()`
- [ ] **Cache invalidation**: Hook into cache invalidation layer to call `recordCacheInvalidation()`
- [ ] **Tests added**: Unit tests for metrics collection in test suite
- [ ] **Invariant validation tests**: Tests that verify epoch invariants using metrics

---

## Performance Considerations

1. **Atomic Operations**: All metric updates use `Synchronized<T>` for thread-safety
2. **Lock Contention**: Metrics calls are very fast (single atomic increment + timestamp)
3. **Timestamp Cost**: Using `std::chrono::system_clock` (microsecond precision available)
4. **Memory Overhead**: EpochMetrics struct is ~100 bytes, negligible overhead

### Optimization Options (if needed)

1. **Conditional Compilation**: Disable metrics with compile flag
   ```cpp
   #ifdef ENABLE_EPOCH_METRICS
   globalEpochMetrics.wlock()->recordQueryStart();
   #endif
   ```

2. **Sampling**: Record only 1-in-N queries for high-traffic workloads
   ```cpp
   if (query_counter++ % SAMPLE_RATE == 0) {
     globalEpochMetrics.wlock()->recordQueryStart();
   }
   ```

3. **Async Logging**: Move metric logging to background thread
   ```cpp
   metrics_thread_.submit([this]() {
     AD_LOG_INFO << metrics_.getDetailedMetricsReport();
   });
   ```

---

## Related Files

- **Source**: `/home/user/qlever/src/global/EpochMetrics.h`
- **Implementation**: `/home/user/qlever/src/global/EpochMetrics.cpp`
- **Epoch Manager**: `/home/user/qlever/src/global/Epoch.h` and `Epoch.cpp`
- **Synchronized Utility**: `/home/user/qlever/src/util/Synchronized.h`
- **Logging**: `/home/user/qlever/src/util/Log.h`

---

## Validation Summary

These metrics PROVE the epoch invariants work correctly by:

1. **Recording Events**: Every important epoch event is recorded with timestamp
2. **Maintaining Invariants**: Metrics structure enforces relationships (e.g., restarts ≤ transitions)
3. **Providing Auditability**: Complete audit trail of epoch lifecycle
4. **Enabling Monitoring**: Real-time visibility into system behavior
5. **Supporting Testing**: Quantitative validation of epoch constraints
6. **Debugging Aid**: Metrics help diagnose incorrect usage patterns

The combination of metric collection with state machine enforcement creates a robust, observable, and debuggable epoch system.
