// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
// Created for EPIC 3 Task 6 - Epoch Promote Atomic Cache Swap

#ifndef QLEVER_ENGINE_READCACHE_READCACHEMANAGER_H
#define QLEVER_ENGINE_READCACHE_READCACHEMANAGER_H

#include <atomic>
#include <memory>

#include "ad_utility/Epoch.h"
#include "ad_utility/Synchronized.h"
#include "engine/readCache/BytesCache.h"
#include "engine/readCache/NegativeCache.h"
#include "engine/readCache/PlanCache.h"
#include "global/EpochCacheInvalidationHook.h"

namespace ad_utility {

// Type aliases for cache types from different namespaces
using BytesCacheType = ad_utility::readCache::BytesCache;
using PlanCacheType = ::readCache::PlanCache;
using NegativeCacheType = ::readCache::NegativeCache;

// ============================================================================
// ReadCacheManager - Singleton manager for read-path caches
// ============================================================================
//
// Manages three cache types with atomic swap on epoch promotion:
// - BytesCache: Caches serialized operation results
// - PlanCache: Caches query execution plans
// - NegativeCache: Caches negative lookups
//
// CRITICAL INVARIANT: 0% cross-epoch hits
// - When epoch promotes, all old caches are atomically swapped with new empty
//   caches
// - Old cache instances are kept alive via shared_ptr while readers hold
//   references
// - New queries automatically get new cache instances
//
// Thread Safety:
// - All public methods are thread-safe
// - Cache swap is atomic via std::atomic<shared_ptr>
// - No locks needed for cache access (lock-free reads)
//
class ReadCacheManager {
 private:
  // Atomic cache instances - allow lock-free reads and atomic swap
  std::atomic<std::shared_ptr<BytesCacheType>> bytesCache_;
  std::atomic<std::shared_ptr<PlanCacheType>> planCache_;
  std::atomic<std::shared_ptr<NegativeCacheType>> negativeCache_;

  // Metrics for observability
  struct Metrics {
    uint64_t totalPromotions_ = 0;
    uint64_t totalBytesInvalidated_ = 0;
    uint64_t totalPlanEntriesInvalidated_ = 0;
    uint64_t totalNegativeEntriesInvalidated_ = 0;
  };

  ad_utility::Synchronized<Metrics> metrics_;

  // Private constructor - singleton pattern
  ReadCacheManager();

 public:
  // Get singleton instance
  static ReadCacheManager& getInstance();

  // Prevent copying and moving
  ReadCacheManager(const ReadCacheManager&) = delete;
  ReadCacheManager& operator=(const ReadCacheManager&) = delete;
  ReadCacheManager(ReadCacheManager&&) = delete;
  ReadCacheManager& operator=(ReadCacheManager&&) = delete;

  // Get shared_ptr to current cache instances
  // These are safe to hold across epoch boundaries - old caches are kept alive
  // via shared_ptr while readers hold references
  std::shared_ptr<BytesCacheType> getBytesCacheRef();
  std::shared_ptr<PlanCacheType> getPlanCacheRef();
  std::shared_ptr<NegativeCacheType> getNegativeCacheRef();

  // Epoch promotion hook - atomically swap caches
  // Called by EpochCacheInvalidationHandler when epoch promotes
  //
  // Mechanism:
  // 1. Create new empty cache instances
  // 2. Atomically swap: old_caches → released, new_caches → stored
  // 3. Old caches drop out of scope when last reader releases shared_ptr
  //
  // Guarantees:
  // - 0% cross-epoch hits (new epoch sees only new caches)
  // - No data races (atomic swap)
  // - No use-after-free (shared_ptr keeps old caches alive)
  // - No brief window where old cache is partially available
  void onEpochPromoted(EpochId oldEpochId, EpochId newEpochId);

  // Register epoch promotion hook with global registry
  // Should be called once at server startup
  void setupEpochHooks();

  // Observability - get metrics
  struct CacheMetrics {
    uint64_t totalPromotions;
    uint64_t totalBytesInvalidated;
    uint64_t totalPlanEntriesInvalidated;
    uint64_t totalNegativeEntriesInvalidated;
    size_t currentBytesSize;
    size_t currentPlanSize;
    size_t currentNegativeSize;
  };

  CacheMetrics getMetrics() const;

  // Testing: clear all caches immediately (no epoch check)
  void clearAllCachesForTesting();
};

// ============================================================================
// ReadCacheInvalidationHandler - EpochCacheInvalidationHandler implementation
// ============================================================================
//
// Connects ReadCacheManager to epoch promotion events
//
class ReadCacheInvalidationHandler : public EpochCacheInvalidationHandler {
 private:
  ReadCacheManager& manager_;

 public:
  explicit ReadCacheInvalidationHandler(ReadCacheManager& manager)
      : manager_(manager) {}

  void onEpochPromoted(const EpochPromotionEvent& event) override;

  std::string getName() const override {
    return "ReadCacheInvalidationHandler";
  }
};

}  // namespace ad_utility

#endif  // QLEVER_ENGINE_READCACHE_READCACHEMANAGER_H
