# Epoch.cpp Modification Examples

This file shows the exact code changes needed to integrate `EpochMetrics` into the existing `Epoch.cpp` implementation.

## Step 0: Add Include Directive

**File:** `/home/user/qlever/src/global/Epoch.cpp`

Add at the top after existing includes:

```cpp
#include "ad_utility/global/EpochMetrics.h"
#include "ad_utility/util/Log.h"  // For logging
```

---

## Step 1: Modify `getCurrentEpochIdForQuery()`

### Before:
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

### After:
```cpp
EpochId EpochManager::getCurrentEpochIdForQuery() const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SERVE) {
    throw std::logic_error(
        "Query attempted outside SERVE epoch");
  }
  EpochId id = lock->epochId_;
  lock.release();  // Release state lock before metrics lock

  // Record successful query start (safe now that we've validated state)
  globalEpochMetrics.wlock()->recordQueryStart();

  return id;
}
```

**Why this change:**
- Records every successful query execution
- Maintains the dual-lock ordering: state lock first, then metrics lock
- Enables tracking of query load per epoch

---

## Step 2: Modify `checkAllowedToMutate()`

### Before:
```cpp
void EpochManager::checkAllowedToMutate() const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::INGEST) {
    throw std::logic_error(
        "Write attempted outside INGEST epoch");
  }
}
```

### After:
```cpp
void EpochManager::checkAllowedToMutate() const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::INGEST) {
    lock.release();  // Release state lock before recording rejection

    // Record rejected write attempt
    globalEpochMetrics.wlock()->recordRejectedWrite();

    AD_LOG_DEBUG << "Write rejected: current_state="
                 << (int)lock->state_ << "\n";

    throw std::logic_error(
        "Write attempted outside INGEST epoch");
  }
}
```

**Why this change:**
- Records every write rejection with timestamp
- Helps detect concurrent mutation attempts during SERVE phase
- Enables monitoring of application correctness

---

## Step 3: Modify `transitionToServe()`

### Before:
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

### After:
```cpp
void EpochManager::transitionToServe() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SEAL) {
    throw std::logic_error(
        "Cannot transition to SERVE from non-SEAL state");
  }
  lock->state_ = EpochState::SERVE;
  uint64_t count = ++(lock->transitionCount_);
  lock.release();  // Release state lock before metrics

  // Record epoch transition and reset query counter
  globalEpochMetrics.wlock()->recordEpochTransition();

  // Log the state transition
  AD_LOG_INFO << "Epoch transition to SERVE: transition_count=" << count
              << "\n";
}
```

**Why this change:**
- Marks epoch "sealing" - when SERVE phase begins
- Resets query counter for new epoch
- Logs all epoch transitions for audit trail

---

## Step 4: Modify `restart()`

### Before:
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

### After:
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
  lock.release();  // Release state lock before metrics

  // Record epoch restart
  globalEpochMetrics.wlock()->recordEpochRestart(oldEpochId);

  // Log the restart
  AD_LOG_INFO << "Epoch restart: old_epoch_id=" << oldEpochId
              << ", new_epoch_id=" << newEpochId << "\n";
}
```

**Why this change:**
- Records epoch cycle completion
- Provides epoch ID for double-counting prevention
- Logs all epoch restarts for audit trail

---

## Complete Modified Epoch.cpp

Here's the complete modified file with all changes integrated:

```cpp
#include "ad_utility/global/Epoch.h"
#include "ad_utility/global/EpochMetrics.h"
#include "ad_utility/util/Log.h"

namespace ad_utility {

// Global singleton for epoch management
ad_utility::Synchronized<EpochManager> globalEpochManager;

// Transition: INIT -> INGEST
void EpochManager::transitionToIngest() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::INIT) {
    throw std::logic_error(
        "Cannot transition to INGEST from non-INIT state");
  }
  lock->state_ = EpochState::INGEST;
  AD_LOG_DEBUG << "Epoch transition to INGEST\n";
}

// Transition: INGEST -> SEAL
void EpochManager::transitionToSeal() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::INGEST) {
    throw std::logic_error(
        "Cannot transition to SEAL from non-INGEST state");
  }
  lock->state_ = EpochState::SEAL;
  AD_LOG_DEBUG << "Epoch transition to SEAL\n";
}

// Transition: SEAL -> SERVE (increments transition counter)
void EpochManager::transitionToServe() {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SEAL) {
    throw std::logic_error(
        "Cannot transition to SERVE from non-SEAL state");
  }
  lock->state_ = EpochState::SERVE;
  uint64_t count = ++(lock->transitionCount_);
  lock.release();

  // Record epoch transition and reset query counter
  globalEpochMetrics.wlock()->recordEpochTransition();

  AD_LOG_INFO << "Epoch transition to SERVE: transition_count=" << count
              << "\n";
}

// Restart: SERVE -> INIT, increment epoch
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
  lock.release();

  // Record epoch restart
  globalEpochMetrics.wlock()->recordEpochRestart(oldEpochId);

  AD_LOG_INFO << "Epoch restart: old_epoch_id=" << oldEpochId
              << ", new_epoch_id=" << newEpochId << "\n";
}

// Query: assert SERVE, return epoch ID
EpochId EpochManager::getCurrentEpochIdForQuery() const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::SERVE) {
    throw std::logic_error(
        "Query attempted outside SERVE epoch");
  }
  EpochId id = lock->epochId_;
  lock.release();

  // Record successful query start
  globalEpochMetrics.wlock()->recordQueryStart();

  return id;
}

// Mutation: assert INGEST
void EpochManager::checkAllowedToMutate() const {
  auto lock = state_.acquire();
  if (lock->state_ != EpochState::INGEST) {
    lock.release();

    // Record rejected write attempt
    globalEpochMetrics.wlock()->recordRejectedWrite();

    AD_LOG_DEBUG << "Write rejected: outside INGEST epoch\n";

    throw std::logic_error(
        "Write attempted outside INGEST epoch");
  }
}

// Observable state - copy current state safely
EpochState EpochManager::getState() const {
  auto lock = state_.acquire();
  return lock->state_;
}

// Observable epoch ID
EpochId EpochManager::getEpochId() const {
  auto lock = state_.acquire();
  return lock->epochId_;
}

// Observable transition count
uint64_t EpochManager::getTransitionCount() const {
  auto lock = state_.acquire();
  return lock->transitionCount_;
}

}  // namespace ad_utility
```

---

## CMakeLists.txt Update

**File:** `/home/user/qlever/src/global/CMakeLists.txt`

Ensure that `EpochMetrics.cpp` is included in the build. The file should list:

```cmake
add_library(global_library
    Constants.cpp
    Epoch.cpp
    EpochMetrics.cpp  # <-- ADD THIS LINE
    RuntimeParameters.cpp
)
```

Or if using a more implicit collection:

```cmake
file(GLOB GLOBAL_SOURCES "*.cpp")
add_library(global_library ${GLOBAL_SOURCES})
```

---

## Complete Modified CMakeLists.txt

```cmake
# Global utilities library
add_library(global_library
    Constants.cpp
    Epoch.cpp
    EpochMetrics.cpp
    RuntimeParameters.cpp
)

target_include_directories(global_library PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(global_library PUBLIC
    absl::strings
    absl::time
)
```

---

## Testing Integration Points

### Test 1: Verify Query Recording

```cpp
TEST(EpochIntegrationTest, QueryRecording) {
  // Reset metrics
  globalEpochMetrics.wlock()->reset();

  // Set up epoch state
  auto manager = std::make_unique<EpochManager>();

  // Start in INIT
  EXPECT_EQ(manager->getState(), EpochState::INIT);

  // Transition: INIT -> INGEST
  manager->transitionToIngest();
  EXPECT_EQ(manager->getState(), EpochState::INGEST);

  // Transition: INGEST -> SEAL
  manager->transitionToSeal();
  EXPECT_EQ(manager->getState(), EpochState::SEAL);

  // Transition: SEAL -> SERVE (records epoch transition)
  manager->transitionToServe();
  EXPECT_EQ(manager->getState(), EpochState::SERVE);

  // Record multiple queries
  for (int i = 0; i < 5; ++i) {
    auto epoch_id = manager->getCurrentEpochIdForQuery();
    EXPECT_EQ(epoch_id, 0);
  }

  // Check metrics
  auto metrics = globalEpochMetrics.rlock()->getMetrics();
  EXPECT_EQ(metrics.queriesInCurrentEpoch_, 5);
  EXPECT_EQ(metrics.totalQueriesAllEpochs_, 5);
  EXPECT_EQ(metrics.epochTransitions_, 1);
}
```

### Test 2: Verify Write Rejection Recording

```cpp
TEST(EpochIntegrationTest, WriteRejectionRecording) {
  // Reset metrics
  globalEpochMetrics.wlock()->reset();

  auto manager = std::make_unique<EpochManager>();

  // Start in INIT, NOT in INGEST
  EXPECT_EQ(manager->getState(), EpochState::INIT);

  // Try to write - should be rejected
  EXPECT_THROW(manager->checkAllowedToMutate(), std::logic_error);

  // Check metrics
  auto metrics = globalEpochMetrics.rlock()->getMetrics();
  EXPECT_EQ(metrics.writeAttemptsRejected_, 1);
  EXPECT_EQ(metrics.totalWriteRejectionsAllEpochs_, 1);
}
```

### Test 3: Verify Epoch Sequence

```cpp
TEST(EpochIntegrationTest, EpochSequenceValidation) {
  // Reset metrics
  globalEpochMetrics.wlock()->reset();

  auto manager = std::make_unique<EpochManager>();

  // Perform two complete epoch cycles
  for (int cycle = 0; cycle < 2; ++cycle) {
    // INIT -> INGEST -> SEAL -> SERVE
    manager->transitionToIngest();
    manager->transitionToSeal();
    manager->transitionToServe();

    // Record some queries
    for (int i = 0; i < 10; ++i) {
      manager->getCurrentEpochIdForQuery();
    }

    // SERVE -> INIT (restart)
    manager->restart();
  }

  // Check metrics
  auto metrics = globalEpochMetrics.rlock()->getMetrics();
  EXPECT_EQ(metrics.epochTransitions_, 2);
  EXPECT_EQ(metrics.epochRestarts_, 2);
  EXPECT_EQ(metrics.totalQueriesAllEpochs_, 20);
}
```

---

## Lock Ordering Convention

Important: When modifying the code, maintain this lock ordering:

1. **State lock first** - Validate epoch state
2. **State lock release** - Explicit release before metrics lock
3. **Metrics lock second** - Record event after state is validated

This prevents deadlock scenarios and keeps critical sections minimal.

**Pattern:**
```cpp
{
  auto state_lock = state_.acquire();
  // Check state, prepare data
  SomeData data = state_lock->getSomeData();
  state_lock.release();  // EXPLICIT RELEASE

  // Now safe to acquire metrics lock
  globalEpochMetrics.wlock()->recordEvent();
}
```

---

## Integration Verification Checklist

After making all changes:

1. **Compilation**: `cmake --build .`
   - Should compile without errors or warnings
   - All includes should resolve correctly

2. **Unit Tests**: `ctest -R EpochTest`
   - All existing epoch tests should still pass
   - New metrics tests should pass

3. **Integration Tests**: `ctest -R EpochIntegration`
   - Test query recording
   - Test write rejection recording
   - Test epoch sequence

4. **Metrics Output**: Manual verification
   - Enable detailed logging
   - Run queries and mutations
   - Verify metrics are recorded correctly

5. **Invariant Validation**: Run custom validation script
   - Verify monotonic epoch IDs
   - Verify query-only-in-SERVE property
   - Verify write rejections occur correctly

---

## Debugging Tips

### If metrics are not updating:

1. **Check includes**: Verify `EpochMetrics.h` is included in `Epoch.cpp`
2. **Check links**: Verify `EpochMetrics.cpp` is in CMakeLists.txt
3. **Check calls**: Add logging before each metric recording call
4. **Check locks**: Verify lock acquisition/release is correct

### If seeing deadlocks:

1. **Check lock ordering**: State lock must be released before metrics lock
2. **Check nested locks**: Ensure no recursive acquisitions
3. **Check exception safety**: Ensure locks are released even on exception

### If metrics are incorrect:

1. **Check reset**: Verify `reset()` is not being called unexpectedly
2. **Check double-recording**: Verify each event is recorded only once
3. **Check timestamps**: Verify timestamp update is thread-safe

---

## Performance Monitoring

After integration, monitor these metrics:

1. **Query Throughput**: `queriesInCurrentEpoch_ / duration`
2. **Write Rejection Rate**: `writeAttemptsRejected_ / total_operations`
3. **Epoch Duration**: Time between `recordEpochTransition()` calls
4. **Cache Invalidation Frequency**: `cacheInvalidations_ / epochTransitions_`

Example monitoring script:
```cpp
void monitorEpochPerformance() {
  auto metrics_before = globalEpochMetrics.rlock()->getMetrics();
  auto time_before = std::chrono::high_resolution_clock::now();

  std::this_thread::sleep_for(std::chrono::seconds(60));

  auto metrics_after = globalEpochMetrics.rlock()->getMetrics();
  auto time_after = std::chrono::high_resolution_clock::now();

  auto duration_s = std::chrono::duration_cast<std::chrono::seconds>(
      time_after - time_before).count();

  uint64_t queries_delta =
      metrics_after.totalQueriesAllEpochs_ -
      metrics_before.totalQueriesAllEpochs_;
  double qps = static_cast<double>(queries_delta) / duration_s;

  AD_LOG_INFO << "Query throughput: " << qps << " queries/sec\n";
}
```
