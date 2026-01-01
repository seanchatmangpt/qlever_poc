// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
// Created for EPIC 1.1 - Epoch Immutability Core

#ifndef AD_UTILITY_EPOCH_CACHE_INVALIDATION_HOOK_H
#define AD_UTILITY_EPOCH_CACHE_INVALIDATION_HOOK_H

#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "ad_utility/Epoch.h"
#include "ad_utility/Synchronized.h"

namespace ad_utility {

// ============================================================================
// CORE EVENT TYPES
// ============================================================================

// Metadata snapshot of an epoch (immutable once epoch enters SERVE)
struct EpochSnapshot {
  EpochId epochId_;
  EpochState state_;
  int64_t timestampMs_;

  std::string toString() const {
    return "EpochSnapshot(id=" + std::to_string(epochId_) + ", state=" +
           std::string(state_ == EpochState::SERVE ? "SERVE" : "OTHER") +
           ", ts=" + std::to_string(timestampMs_) + ")";
  }
};

// Formal event: epoch promotion from one SERVE state to another
// Fired when: EpochManager::restart() completes and a new epoch enters SERVE
// Semantics: Old epoch is now immutable; new epoch is live for queries
struct EpochPromotionEvent {
  // Previous serving epoch (now becoming read-only)
  EpochId oldEpochId_;
  EpochSnapshot oldSnapshot_;

  // New serving epoch (now accepting queries)
  EpochId newEpochId_;
  EpochSnapshot newSnapshot_;

  // When this promotion occurred
  int64_t promotionTimestampMs_;

  std::string toString() const {
    return "EpochPromotionEvent(old=" + std::to_string(oldEpochId_) +
           " -> new=" + std::to_string(newEpochId_) +
           ", promotedAt=" + std::to_string(promotionTimestampMs_) + ")";
  }
};

// ============================================================================
// HANDLER INTERFACE
// ============================================================================

// Abstract handler for epoch-scoped cache invalidation.
// Implementations:
// - MUST not throw exceptions (all exceptions are caught and logged)
// - MUST complete quickly (firing is synchronous with epoch transitions)
// - MAY use the epochId to invalidate old data
// - SHOULD use metrics to track invalidations
class EpochCacheInvalidationHandler {
 public:
  virtual ~EpochCacheInvalidationHandler() = default;

  // Called when a new epoch is promoted to SERVE state
  // This is the primary hook for cache invalidation.
  //
  // Preconditions:
  // - newEpochId > oldEpochId (monotonic)
  // - Both epoch states are valid
  // - Called exactly once per epoch transition
  //
  // Expected implementations:
  // 1. Clear any caches keyed by oldEpochId
  // 2. Update internal bookkeeping to track newEpochId
  // 3. Record metrics (counts, timestamps)
  // 4. Optional: Trigger async work (e.g., background cleanup)
  virtual void onEpochPromoted(const EpochPromotionEvent& event) = 0;

  // Optional: Called when an epoch transitions from INGEST to SEAL
  // Signals that epoch is about to become read-only.
  // Default: no-op
  virtual void onEpochBecomesReadOnly(EpochId epochId) {}

  // Optional: Called when an epoch becomes the serving epoch (SEAL -> SERVE)
  // Signals that queries will now target this epoch.
  // Default: no-op
  virtual void onEpochBecomesServing(EpochId epochId) {}

  // Human-readable name for debugging/logging
  virtual std::string getName() const = 0;
};

// ============================================================================
// HANDLER REGISTRY (THREAD-SAFE)
// ============================================================================

// Thread-safe registry of cache invalidation handlers.
// Handlers are called in registration order, with exceptions caught
// and logged. Firing is synchronous.
class EpochCacheInvalidationRegistry {
 private:
  // Internal state protected by Synchronized
  struct State {
    std::vector<std::unique_ptr<EpochCacheInvalidationHandler>> handlers_;
    uint64_t totalFiredEvents_ = 0;
    uint64_t totalExceptions_ = 0;
  };

  Synchronized<State> state_;

 public:
  EpochCacheInvalidationRegistry() = default;
  ~EpochCacheInvalidationRegistry() = default;

  // Register a handler. Takes ownership via unique_ptr.
  // Thread-safe. O(1).
  void registerHandler(std::unique_ptr<EpochCacheInvalidationHandler> handler);

  // Fire onEpochPromoted event to all registered handlers.
  // - Catches all exceptions per-handler
  // - Logs exceptions at WARN level
  // - Returns count of handlers that fired successfully
  // Thread-safe. O(n handlers).
  size_t fireOnEpochPromoted(const EpochPromotionEvent& event);

  // Fire onEpochBecomesReadOnly event to all registered handlers.
  // Thread-safe. O(n handlers).
  size_t fireOnEpochBecomesReadOnly(EpochId epochId);

  // Fire onEpochBecomesServing event to all registered handlers.
  // Thread-safe. O(n handlers).
  size_t fireOnEpochBecomesServing(EpochId epochId);

  // Get count of registered handlers
  size_t getHandlerCount() const;

  // Get statistics on events fired
  struct Statistics {
    size_t handlerCount_;
    uint64_t totalEventsFired_;
    uint64_t totalExceptionsCaught_;
  };

  Statistics getStatistics() const;

  // Clear all registered handlers (primarily for testing)
  void clearHandlers();
};

// ============================================================================
// GLOBAL SINGLETON
// ============================================================================

// Thread-safe global registry for epoch cache invalidation handlers.
// Usage:
//   globalEpochCacheInvalidationRegistry.withWriteLock([](auto& registry) {
//     registry.registerHandler(std::make_unique<MyHandler>());
//   });
extern ad_utility::Synchronized<EpochCacheInvalidationRegistry>
    globalEpochCacheInvalidationRegistry;

// Convenience function to fire epoch promotion event
// Automatically acquires lock and fires to global registry
inline void fireEpochPromotionEvent(const EpochPromotionEvent& event) {
  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&event](auto& registry) { registry.fireOnEpochPromoted(event); });
}

}  // namespace ad_utility

#endif  // AD_UTILITY_EPOCH_CACHE_INVALIDATION_HOOK_H
