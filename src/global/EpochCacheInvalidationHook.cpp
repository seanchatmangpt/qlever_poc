// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
// Created for EPIC 1.1 - Epoch Immutability Core

#include "global/EpochCacheInvalidationHook.h"

#include <chrono>

#include "util/Log.h"

namespace ad_utility {

// Global singleton instance
ad_utility::Synchronized<EpochCacheInvalidationRegistry>
    globalEpochCacheInvalidationRegistry;

// ============================================================================
// EpochCacheInvalidationRegistry Implementation
// ============================================================================

void EpochCacheInvalidationRegistry::registerHandler(
    std::unique_ptr<EpochCacheInvalidationHandler> handler) {
  if (!handler) {
    throw std::invalid_argument(
        "EpochCacheInvalidationRegistry: handler cannot be null");
  }

  withWriteLock([&handler](auto& state) {
    state.handlers_.push_back(std::move(handler));
    AD_LOG_DEBUG << "Registered cache invalidation handler. Count: "
                 << state.handlers_.size();
  });
}

size_t EpochCacheInvalidationRegistry::fireOnEpochPromoted(
    const EpochPromotionEvent& event) {
  size_t successCount = 0;
  size_t exceptionCount = 0;

  withWriteLock([&event, &successCount, &exceptionCount](auto& state) {
    AD_LOG_INFO << "Firing EpochPromotionEvent: " << event.toString() << " to "
                << state.handlers_.size() << " handler(s)";

    for (auto& handler : state.handlers_) {
      try {
        handler->onEpochPromoted(event);
        successCount++;
      } catch (const std::exception& e) {
        exceptionCount++;
        AD_LOG_WARN << "Cache invalidation handler '" << handler->getName()
                    << "' threw exception: " << e.what();
      } catch (...) {
        exceptionCount++;
        AD_LOG_WARN << "Cache invalidation handler '" << handler->getName()
                    << "' threw unknown exception";
      }
    }

    // Update metrics
    state.totalFiredEvents_++;
    state.totalExceptions_ += exceptionCount;

    AD_LOG_DEBUG << "EpochPromotionEvent fired: " << successCount
                 << " succeeded, " << exceptionCount << " failed";
  });

  return successCount;
}

size_t EpochCacheInvalidationRegistry::fireOnEpochBecomesReadOnly(
    EpochId epochId) {
  size_t successCount = 0;

  withWriteLock([epochId, &successCount](auto& state) {
    AD_LOG_DEBUG << "Firing onEpochBecomesReadOnly for epoch " << epochId
                 << " to " << state.handlers_.size() << " handler(s)";

    for (auto& handler : state.handlers_) {
      try {
        handler->onEpochBecomesReadOnly(epochId);
        successCount++;
      } catch (const std::exception& e) {
        AD_LOG_WARN << "Handler '" << handler->getName()
                    << "' threw in onEpochBecomesReadOnly: " << e.what();
        state.totalExceptions_++;
      } catch (...) {
        AD_LOG_WARN << "Handler '" << handler->getName()
                    << "' threw unknown exception in onEpochBecomesReadOnly";
        state.totalExceptions_++;
      }
    }
  });

  return successCount;
}

size_t EpochCacheInvalidationRegistry::fireOnEpochBecomesServing(
    EpochId epochId) {
  size_t successCount = 0;

  withWriteLock([epochId, &successCount](auto& state) {
    AD_LOG_DEBUG << "Firing onEpochBecomesServing for epoch " << epochId
                 << " to " << state.handlers_.size() << " handler(s)";

    for (auto& handler : state.handlers_) {
      try {
        handler->onEpochBecomesServing(epochId);
        successCount++;
      } catch (const std::exception& e) {
        AD_LOG_WARN << "Handler '" << handler->getName()
                    << "' threw in onEpochBecomesServing: " << e.what();
        state.totalExceptions_++;
      } catch (...) {
        AD_LOG_WARN << "Handler '" << handler->getName()
                    << "' threw unknown exception in onEpochBecomesServing";
        state.totalExceptions_++;
      }
    }
  });

  return successCount;
}

size_t EpochCacheInvalidationRegistry::getHandlerCount() const {
  return withReadLock([](const auto& state) { return state.handlers_.size(); });
}

EpochCacheInvalidationRegistry::Statistics
EpochCacheInvalidationRegistry::getStatistics() const {
  return withReadLock([](const auto& state) {
    return Statistics{state.handlers_.size(), state.totalFiredEvents_,
                      state.totalExceptions_};
  });
}

void EpochCacheInvalidationRegistry::clearHandlers() {
  withWriteLock([](auto& state) {
    AD_LOG_DEBUG << "Clearing " << state.handlers_.size()
                 << " cache invalidation handlers";
    state.handlers_.clear();
  });
}

}  // namespace ad_utility
