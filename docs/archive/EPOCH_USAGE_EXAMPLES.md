# EPIC 1.1 - Atomic Promotion Pattern: Usage Examples

## Example 1: Simple Promotion (No Validation)

```cpp
#include "global/Epoch.h"
#include "util/Log.h"

using namespace ad_utility;

// Basic promotion from epoch N to N+1
void promoteSimple() {
  try {
    EpochId newId = globalEpochManager.acquire()->atomicPromoteToNewEpoch();
    AD_LOG_INFO << "Successfully promoted to epoch " << newId;
  } catch (const std::logic_error& e) {
    AD_LOG_ERROR << "Promotion failed: " << e.what();
  }
}
```

## Example 2: Promotion with Validation

```cpp
// Validate index is ready before promoting
void promoteWithValidation(IndexManager& indexMgr) {
  EpochManager& epochMgr = *globalEpochManager.acquire();
  
  try {
    EpochId newId = epochMgr.atomicPromoteToNewEpoch(
        [&indexMgr](EpochId epochId) {
          // Phase 2: Validation (outside lock)
          // Validate new epoch's index is complete
          if (!indexMgr.isIndexComplete(epochId)) {
            throw std::runtime_error(
                "Index not ready for epoch " + std::to_string(epochId));
          }
          
          // Validate consistency
          uint64_t tripleCount = indexMgr.getTripleCount(epochId);
          if (tripleCount == 0) {
            throw std::runtime_error("Empty epoch");
          }
          
          AD_LOG_INFO << "Epoch " << epochId << " validated: "
                      << tripleCount << " triples";
        },
        nullptr  // No post-hook
    );
    
    AD_LOG_INFO << "Promotion successful: " << newId;
    
  } catch (const std::exception& e) {
    // Validation failed → automatic rollback occurred
    AD_LOG_ERROR << "Promotion validation failed: " << e.what();
    AD_LOG_INFO << "System rolled back to previous epoch";
  }
}
```

## Example 3: Promotion with Cache Invalidation

```cpp
// Invalidate caches after promoting
void promoteWithCacheInvalidation(QueryCache& cache) {
  EpochManager& epochMgr = *globalEpochManager.acquire();
  
  EpochId newId = epochMgr.atomicPromoteToNewEpoch(
      nullptr,  // No validation
      [&cache](EpochId epochId) {
        // Phase 4: Cache invalidation (outside lock, non-fatal)
        try {
          cache.invalidateForEpochChange(epochId);
          AD_LOG_INFO << "Cache invalidated for epoch " << epochId;
        } catch (const std::exception& e) {
          // Logged but doesn't fail promotion
          // Promotion already committed
          AD_LOG_WARN << "Cache invalidation warning: " << e.what();
        }
      }
  );
  
  AD_LOG_INFO << "Epoch " << newId << " now serving";
}
```

## Example 4: Complete Example with Both Hooks

```cpp
// Full promotion with validation and cache invalidation
void promoteWithBothHooks(
    IndexManager& indexMgr,
    QueryCache& queryCache,
    SubscriberNotifier& notifier) {
  
  EpochManager& epochMgr = *globalEpochManager.acquire();
  
  // Check if another promotion in progress
  if (epochMgr.isPromotionInProgress()) {
    AD_LOG_WARN << "Promotion already in progress, skipping";
    return;
  }
  
  try {
    EpochId newId = epochMgr.atomicPromoteToNewEpoch(
        [&indexMgr](EpochId epochId) {
          // Phase 2: Validation (can be slow, no lock)
          AD_LOG_DEBUG << "Starting validation for epoch " << epochId;
          
          // Check index completeness
          if (!indexMgr.isIndexComplete(epochId)) {
            throw std::runtime_error(
                "Index incomplete: missing permutations for epoch " +
                std::to_string(epochId));
          }
          
          // Verify metadata
          auto metadata = indexMgr.getMetadata(epochId);
          if (!metadata || !metadata->isValid()) {
            throw std::runtime_error(
                "Invalid metadata for epoch " + std::to_string(epochId));
          }
          
          // Run integrity checks
          if (!indexMgr.verifyIntegrity(epochId)) {
            throw std::runtime_error(
                "Integrity check failed for epoch " + std::to_string(epochId));
          }
          
          AD_LOG_INFO << "Epoch " << epochId << " validation successful";
        },
        [&queryCache, &notifier](EpochId epochId) {
          // Phase 4: Post-promotion (can be slow, non-fatal)
          AD_LOG_DEBUG << "Invalidating caches for epoch " << epochId;
          
          try {
            // Invalidate query result cache
            queryCache.invalidateForEpochChange(epochId);
            
            // Notify subscribers
            notifier.notifyEpochChanged(epochId);
            
            // Update metrics
            metrics.recordEpochPromotion(epochId);
            
            AD_LOG_INFO << "Post-promotion actions completed for epoch "
                        << epochId;
          } catch (const std::exception& e) {
            // Non-fatal: promotion already committed
            AD_LOG_WARN << "Post-promotion action failed: " << e.what()
                        << " (non-fatal)";
          }
        }
    );
    
    AD_LOG_INFO << "Successfully promoted to epoch " << newId;
    
  } catch (const std::exception& e) {
    // Validation failed → automatic rollback occurred
    AD_LOG_ERROR << "Promotion failed: " << e.what();
    
    // Check what we rolled back to
    auto currentId = epochMgr.getEpochId();
    auto state = epochMgr.getState();
    AD_LOG_INFO << "Rolled back to epoch " << currentId
                << " in state " << (int)state;
  }
}
```

## Example 5: Timeout-Based Waiting

```cpp
// Wait for in-progress promotion to complete
void waitForPromotionComplete(std::chrono::seconds timeout) {
  EpochManager& epochMgr = *globalEpochManager.acquire();
  
  auto deadline = std::chrono::steady_clock::now() + timeout;
  
  while (epochMgr.isPromotionInProgress()) {
    if (std::chrono::steady_clock::now() > deadline) {
      throw std::runtime_error("Promotion timeout");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  
  AD_LOG_INFO << "Promotion completed";
}
```

## Example 6: Prevent Operations During Promotion

```cpp
// Check promotion status before starting expensive operation
void startLongRunningIndexBuild(IndexManager& indexMgr) {
  EpochManager& epochMgr = *globalEpochManager.acquire();
  
  if (epochMgr.isPromotionInProgress()) {
    throw std::runtime_error(
        "Cannot start index build: promotion in progress");
  }
  
  // Safe to start build
  indexMgr.startBuild();
}
```

## Example 7: Emergency Rollback

```cpp
// Manual rollback for disaster recovery (rarely needed)
void emergencyRollback() {
  EpochManager& epochMgr = *globalEpochManager.acquire();
  
  if (!epochMgr.isPromotionInProgress()) {
    AD_LOG_WARN << "No promotion in progress to rollback";
    return;
  }
  
  try {
    epochMgr.rollbackPromotion();
    AD_LOG_WARN << "Emergency rollback completed";
  } catch (const std::exception& e) {
    AD_LOG_ERROR << "Emergency rollback failed: " << e.what();
    // System may require manual intervention
  }
}
```

## Example 8: Monitoring Promotion Progress

```cpp
// Monitor promotion in progress
void monitorPromotion(int pollIntervalMs = 100) {
  EpochManager& epochMgr = *globalEpochManager.acquire();
  
  EpochId startEpochId = epochMgr.getEpochId();
  AD_LOG_INFO << "Starting epoch: " << startEpochId;
  
  while (true) {
    if (!epochMgr.isPromotionInProgress()) {
      EpochId endEpochId = epochMgr.getEpochId();
      AD_LOG_INFO << "Promotion complete: " << startEpochId
                  << " → " << endEpochId;
      break;
    }
    
    AD_LOG_DEBUG << "Promotion in progress...";
    std::this_thread::sleep_for(
        std::chrono::milliseconds(pollIntervalMs));
  }
}
```

## Anti-Patterns to Avoid

### Anti-Pattern 1: Blocking in Callbacks

```cpp
// WRONG: Long-running operation in validation hook
epochMgr.atomicPromoteToNewEpoch(
    [](EpochId id) {
      // Bad: This blocks all queries!
      std::this_thread::sleep_for(std::chrono::seconds(10));
      
      // Bad: Expensive computation without lock
      for (int i = 0; i < 1000000; i++) {
        // CPU-intensive work
      }
    },
    nullptr
);
```

### Anti-Pattern 2: Acquiring Lock in Callback

```cpp
// WRONG: Trying to acquire internal lock in callback
epochMgr.atomicPromoteToNewEpoch(
    [&epochMgr](EpochId id) {
      // Bad: Attempts to acquire lock again
      // (deadlock potential if lock non-reentrant)
      auto state = epochMgr.getState();
    },
    nullptr
);
```

### Anti-Pattern 3: Failing on Post-Hook Exception

```cpp
// WRONG: Assuming post-hook can fail promotion
epochMgr.atomicPromoteToNewEpoch(
    nullptr,
    [](EpochId id) {
      throw std::runtime_error("This won't stop promotion!");
      // Exception caught, logged, promotion continues
    }
);
```

### Anti-Pattern 4: Not Checking Preconditions

```cpp
// WRONG: Calling without checking state
if (epochMgr.getState() != EpochState::SERVE) {
  throw std::runtime_error("Not in SERVE state");
}

try {
  epochMgr.atomicPromoteToNewEpoch();  // Already checked above
} catch (const std::logic_error& e) {
  // But state could change between check and call!
}

// CORRECT: Just catch the exception
try {
  epochMgr.atomicPromoteToNewEpoch();
} catch (const std::logic_error& e) {
  AD_LOG_ERROR << "Cannot promote: " << e.what();
}
```

## Testing the Atomic Promotion

```cpp
// Unit test example
#include <gtest/gtest.h>
#include "global/Epoch.h"

class AtomicPromotionTest : public ::testing::Test {
protected:
  EpochManager& epochMgr = *globalEpochManager.acquire();
};

TEST_F(AtomicPromotionTest, PromotionIncrementsEpochId) {
  EpochId initial = epochMgr.getEpochId();
  
  // Transition to SERVE first
  epochMgr.transitionToIngest();
  epochMgr.transitionToSeal();
  epochMgr.transitionToServe();
  
  // Promote
  EpochId newId = epochMgr.atomicPromoteToNewEpoch();
  
  // Verify
  EXPECT_EQ(newId, initial + 1);
  EXPECT_EQ(epochMgr.getEpochId(), initial + 1);
  EXPECT_EQ(epochMgr.getState(), EpochState::SERVE);
}

TEST_F(AtomicPromotionTest, ValidationFailureRollsBack) {
  EpochId initial = epochMgr.getEpochId();
  
  try {
    epochMgr.atomicPromoteToNewEpoch(
        [](EpochId) {
          throw std::runtime_error("Validation failed");
        },
        nullptr
    );
    FAIL() << "Should have thrown";
  } catch (const std::runtime_error& e) {
    EXPECT_STREQ(e.what(), "Validation failed");
  }
  
  // Verify rollback
  EXPECT_EQ(epochMgr.getEpochId(), initial);  // Restored
  EXPECT_EQ(epochMgr.getState(), EpochState::SERVE);  // Back to SERVE
  EXPECT_FALSE(epochMgr.isPromotionInProgress());  // Flag cleared
}
```

## Summary

The atomic promotion pattern provides safe, predictable epoch transitions with:
- **Atomicity:** All-or-nothing promotion
- **Validation:** Optional pre-promotion checks with automatic rollback
- **Non-blocking:** Callbacks outside critical sections
- **Observable:** Query progress via `isPromotionInProgress()`
- **Recoverable:** Rollback capability for failures

Use these patterns to safely manage epoch transitions in production QLever systems.
