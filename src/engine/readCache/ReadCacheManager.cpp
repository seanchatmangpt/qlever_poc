// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
// Created for EPIC 3 Task 6 - Epoch Promote Atomic Cache Swap

#include "engine/readCache/ReadCacheManager.h"

#include <chrono>

#include "util/Log.h"

namespace ad_utility {

// ============================================================================
// ReadCacheManager Implementation
// ============================================================================

ReadCacheManager::ReadCacheManager() {
  // Initialize with empty cache instances
  // Use default constructor parameters for each cache type
  bytesCache_.store(std::make_shared<BytesCacheType>());
  planCache_.store(std::make_shared<PlanCacheType>());
  negativeCache_.store(std::make_shared<NegativeCacheType>());

  AD_LOG_INFO << "ReadCacheManager initialized with empty caches";
}

ReadCacheManager& ReadCacheManager::getInstance() {
  static ReadCacheManager instance;
  return instance;
}

std::shared_ptr<BytesCacheType> ReadCacheManager::getBytesCacheRef() {
  return bytesCache_.load();
}

std::shared_ptr<PlanCacheType> ReadCacheManager::getPlanCacheRef() {
  return planCache_.load();
}

std::shared_ptr<NegativeCacheType> ReadCacheManager::getNegativeCacheRef() {
  return negativeCache_.load();
}

void ReadCacheManager::onEpochPromoted(EpochId oldEpochId,
                                       EpochId newEpochId) {
  // Record pre-swap metrics
  auto oldBytesCache = bytesCache_.load();
  auto oldPlanCache = planCache_.load();
  auto oldNegativeCache = negativeCache_.load();

  // Get BytesCache stats
  size_t oldBytesSize = oldBytesCache->size();
  size_t oldBytesCount = oldBytesCache->entryCount();

  // Get PlanCache stats
  auto planStats = oldPlanCache->getStats();
  size_t oldPlanSize = planStats.totalBytes;
  size_t oldPlanCount = planStats.numEntries;

  // Get NegativeCache stats
  auto negStats = oldNegativeCache->getStats();
  size_t oldNegativeSize =
      negStats.num_entries * 16;  // Approximate size (16 bytes per entry)
  size_t oldNegativeCount = negStats.num_entries;

  AD_LOG_INFO << "Epoch promoted from " << oldEpochId << " to " << newEpochId
              << ". Pre-swap cache state: BytesCache=" << oldBytesSize
              << " bytes (" << oldBytesCount << " entries), PlanCache="
              << oldPlanSize << " bytes (" << oldPlanCount
              << " entries), NegativeCache=" << oldNegativeSize << " bytes ("
              << oldNegativeCount << " entries)";

  // Create new empty cache instances
  auto newBytesCache = std::make_shared<BytesCacheType>();
  auto newPlanCache = std::make_shared<PlanCacheType>();
  auto newNegativeCache = std::make_shared<NegativeCacheType>();

  // Atomic swap - this is the critical section
  // After these operations:
  // - New queries get new empty caches
  // - Old queries holding shared_ptrs still have access to old caches
  // - Old caches will be destroyed when last shared_ptr is released
  bytesCache_.store(newBytesCache);
  planCache_.store(newPlanCache);
  negativeCache_.store(newNegativeCache);

  // Update metrics
  metrics_.withWriteLock([&](auto& m) {
    m.totalPromotions_++;
    m.totalBytesInvalidated_ += oldBytesSize;
    m.totalPlanEntriesInvalidated_ += oldPlanCount;
    m.totalNegativeEntriesInvalidated_ += oldNegativeCount;
  });

  AD_LOG_INFO << "Cache swap completed for epoch " << newEpochId
              << ". New caches installed, old caches will be released when "
                 "last reader drops reference.";

  // Old caches (oldBytesCache, oldPlanCache, oldNegativeCache) go out of scope
  // here They will be destroyed when their shared_ptr ref count reaches 0
}

void ReadCacheManager::setupEpochHooks() {
  // Create handler that delegates to this manager
  auto handler = std::make_unique<ReadCacheInvalidationHandler>(*this);

  // Register with global registry
  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&handler](auto& registry) {
        registry.registerHandler(std::move(handler));
      });

  AD_LOG_INFO << "ReadCacheManager registered with epoch invalidation registry";
}

ReadCacheManager::CacheMetrics ReadCacheManager::getMetrics() const {
  auto currentBytesCache = bytesCache_.load();
  auto currentPlanCache = planCache_.load();
  auto currentNegativeCache = negativeCache_.load();

  // Get current stats from each cache
  auto planStats = currentPlanCache->getStats();
  auto negStats = currentNegativeCache->getStats();

  return metrics_.withReadLock([&](const auto& m) {
    return CacheMetrics{
        .totalPromotions = m.totalPromotions_,
        .totalBytesInvalidated = m.totalBytesInvalidated_,
        .totalPlanEntriesInvalidated = m.totalPlanEntriesInvalidated_,
        .totalNegativeEntriesInvalidated = m.totalNegativeEntriesInvalidated_,
        .currentBytesSize = currentBytesCache->size(),
        .currentPlanSize = planStats.totalBytes,
        .currentNegativeSize = negStats.num_entries * 16,
    };
  });
}

void ReadCacheManager::clearAllCachesForTesting() {
  // Create new empty instances
  auto newBytesCache = std::make_shared<BytesCacheType>();
  auto newPlanCache = std::make_shared<PlanCacheType>();
  auto newNegativeCache = std::make_shared<NegativeCacheType>();

  // Swap
  bytesCache_.store(newBytesCache);
  planCache_.store(newPlanCache);
  negativeCache_.store(newNegativeCache);

  AD_LOG_DEBUG << "All caches cleared for testing";
}

// ============================================================================
// ReadCacheInvalidationHandler Implementation
// ============================================================================

void ReadCacheInvalidationHandler::onEpochPromoted(
    const EpochPromotionEvent& event) {
  AD_LOG_DEBUG << "ReadCacheInvalidationHandler received promotion event: "
               << event.toString();

  // Delegate to manager
  manager_.onEpochPromoted(event.oldEpochId_, event.newEpochId_);
}

}  // namespace ad_utility
