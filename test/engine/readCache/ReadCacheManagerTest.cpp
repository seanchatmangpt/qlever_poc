// Copyright 2025, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Claude AI Assistant
// Created for EPIC 3 Task 6 - Epoch Promote Atomic Cache Swap

#include <gtest/gtest.h>

#include <thread>

#include "engine/readCache/ReadCacheManager.h"
#include "global/EpochCacheInvalidationHook.h"

using namespace ad_utility;

// Type aliases for convenience
using BytesCacheType = ad_utility::readCache::BytesCache;
using PlanCacheType = ::readCache::PlanCache;
using NegativeCacheType = ::readCache::NegativeCache;

// ============================================================================
// ReadCacheManagerTest Fixture
// ============================================================================

class ReadCacheManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Clear global registry before each test
    globalEpochCacheInvalidationRegistry.withWriteLock(
        [](auto& registry) { registry.clearHandlers(); });

    // Clear caches
    ReadCacheManager::getInstance().clearAllCachesForTesting();
  }

  void TearDown() override {
    // Clean up
    globalEpochCacheInvalidationRegistry.withWriteLock(
        [](auto& registry) { registry.clearHandlers(); });
  }
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(ReadCacheManagerTest, SingletonInstance) {
  // Verify singleton pattern
  auto& instance1 = ReadCacheManager::getInstance();
  auto& instance2 = ReadCacheManager::getInstance();

  EXPECT_EQ(&instance1, &instance2);
}

TEST_F(ReadCacheManagerTest, CachesAreInitialized) {
  auto& manager = ReadCacheManager::getInstance();

  // Should be able to get references to all caches
  auto bytesCache = manager.getBytesCacheRef();
  auto planCache = manager.getPlanCacheRef();
  auto negativeCache = manager.getNegativeCacheRef();

  EXPECT_NE(bytesCache, nullptr);
  EXPECT_NE(planCache, nullptr);
  EXPECT_NE(negativeCache, nullptr);

  // Initial state should be empty
  EXPECT_EQ(bytesCache->size(), 0);
  auto planStats = planCache->getStats();
  EXPECT_EQ(planStats.totalBytes, 0);
  auto negStats = negativeCache->getStats();
  EXPECT_EQ(negStats.num_entries, 0);
}

TEST_F(ReadCacheManagerTest, CacheReferencesAreStable) {
  auto& manager = ReadCacheManager::getInstance();

  // Get references
  auto bytesCache1 = manager.getBytesCacheRef();
  auto bytesCache2 = manager.getBytesCacheRef();

  // Same cache instance before promotion
  EXPECT_EQ(bytesCache1.get(), bytesCache2.get());
}

// ============================================================================
// Epoch Promotion Tests
// ============================================================================

TEST_F(ReadCacheManagerTest, EpochPromotionSwapsCaches) {
  auto& manager = ReadCacheManager::getInstance();

  // Get reference to cache before promotion
  auto oldBytesCache = manager.getBytesCacheRef();
  auto oldPlanCache = manager.getPlanCacheRef();
  auto oldNegativeCache = manager.getNegativeCacheRef();

  // Simulate epoch promotion
  manager.onEpochPromoted(0, 1);

  // Get new references
  auto newBytesCache = manager.getBytesCacheRef();
  auto newPlanCache = manager.getPlanCacheRef();
  auto newNegativeCache = manager.getNegativeCacheRef();

  // Cache instances should be different (atomic swap happened)
  EXPECT_NE(oldBytesCache.get(), newBytesCache.get());
  EXPECT_NE(oldPlanCache.get(), newPlanCache.get());
  EXPECT_NE(oldNegativeCache.get(), newNegativeCache.get());

  // Old references should still be valid (shared_ptr keeps them alive)
  EXPECT_EQ(oldBytesCache->size(), 0);
  auto oldPlanStats = oldPlanCache->getStats();
  EXPECT_EQ(oldPlanStats.totalBytes, 0);
  auto oldNegStats = oldNegativeCache->getStats();
  EXPECT_EQ(oldNegStats.num_entries, 0);

  // New caches should be empty
  EXPECT_EQ(newBytesCache->size(), 0);
  auto newPlanStats = newPlanCache->getStats();
  EXPECT_EQ(newPlanStats.totalBytes, 0);
  auto newNegStats = newNegativeCache->getStats();
  EXPECT_EQ(newNegStats.num_entries, 0);
}

TEST_F(ReadCacheManagerTest, ZeroCrossEpochHits) {
  auto& manager = ReadCacheManager::getInstance();

  // Get cache for epoch 0
  auto epoch0Cache = manager.getBytesCacheRef();
  void* epoch0Ptr = epoch0Cache.get();

  // Promote to epoch 1
  manager.onEpochPromoted(0, 1);

  // Get cache for epoch 1
  auto epoch1Cache = manager.getBytesCacheRef();
  void* epoch1Ptr = epoch1Cache.get();

  // Verify they are different instances (0% cross-epoch reuse)
  EXPECT_NE(epoch0Ptr, epoch1Ptr);

  // Promote to epoch 2
  manager.onEpochPromoted(1, 2);

  // Get cache for epoch 2
  auto epoch2Cache = manager.getBytesCacheRef();
  void* epoch2Ptr = epoch2Cache.get();

  // Verify epoch 1 and epoch 2 caches are different
  EXPECT_NE(epoch1Ptr, epoch2Ptr);

  // All three should be different
  EXPECT_NE(epoch0Ptr, epoch2Ptr);
}

TEST_F(ReadCacheManagerTest, OldCacheKeptAliveBySharedPtr) {
  auto& manager = ReadCacheManager::getInstance();

  // Get reference to cache
  auto oldCache = manager.getBytesCacheRef();
  auto* rawPtr = oldCache.get();

  // Promote epoch
  manager.onEpochPromoted(0, 1);

  // Old cache should still be valid because we hold shared_ptr
  EXPECT_EQ(oldCache.get(), rawPtr);
  EXPECT_EQ(oldCache->size(), 0);

  // New queries get new cache
  auto newCache = manager.getBytesCacheRef();
  EXPECT_NE(newCache.get(), rawPtr);
}

TEST_F(ReadCacheManagerTest, MultiplePromotions) {
  auto& manager = ReadCacheManager::getInstance();

  std::vector<std::shared_ptr<BytesCacheType>> caches;

  // Do 5 promotions
  for (EpochId epoch = 0; epoch < 5; ++epoch) {
    // Get current cache
    caches.push_back(manager.getBytesCacheRef());

    // Promote
    manager.onEpochPromoted(epoch, epoch + 1);
  }

  // Get final cache
  caches.push_back(manager.getBytesCacheRef());

  // All caches should be different
  for (size_t i = 0; i < caches.size(); ++i) {
    for (size_t j = i + 1; j < caches.size(); ++j) {
      EXPECT_NE(caches[i].get(), caches[j].get())
          << "Cache " << i << " and " << j << " should be different";
    }
  }
}

// ============================================================================
// Metrics Tests
// ============================================================================

TEST_F(ReadCacheManagerTest, MetricsTrackPromotions) {
  auto& manager = ReadCacheManager::getInstance();

  auto initialMetrics = manager.getMetrics();
  EXPECT_EQ(initialMetrics.totalPromotions, 0);

  // Promote 3 times
  manager.onEpochPromoted(0, 1);
  manager.onEpochPromoted(1, 2);
  manager.onEpochPromoted(2, 3);

  auto finalMetrics = manager.getMetrics();
  EXPECT_EQ(finalMetrics.totalPromotions, 3);
}

TEST_F(ReadCacheManagerTest, MetricsTrackCurrentSizes) {
  auto& manager = ReadCacheManager::getInstance();

  auto metrics = manager.getMetrics();

  // Initial sizes should be 0 (stub caches)
  EXPECT_EQ(metrics.currentBytesSize, 0);
  EXPECT_EQ(metrics.currentPlanSize, 0);
  EXPECT_EQ(metrics.currentNegativeSize, 0);
}

// ============================================================================
// Hook Registration Tests
// ============================================================================

TEST_F(ReadCacheManagerTest, HookRegistrationWorks) {
  auto& manager = ReadCacheManager::getInstance();

  // Register hook
  manager.setupEpochHooks();

  // Verify handler was registered
  auto handlerCount =
      globalEpochCacheInvalidationRegistry.withReadLock(
          [](const auto& registry) { return registry.getHandlerCount(); });

  EXPECT_EQ(handlerCount, 1);
}

TEST_F(ReadCacheManagerTest, HookTriggersOnPromotionEvent) {
  auto& manager = ReadCacheManager::getInstance();

  // Register hook
  manager.setupEpochHooks();

  // Get cache before promotion
  auto oldCache = manager.getBytesCacheRef();

  // Fire promotion event via global registry
  EpochPromotionEvent event{
      .oldEpochId_ = 0,
      .oldSnapshot_ =
          {.epochId_ = 0, .state_ = EpochState::SERVE, .timestampMs_ = 1000},
      .newEpochId_ = 1,
      .newSnapshot_ =
          {.epochId_ = 1, .state_ = EpochState::SERVE, .timestampMs_ = 2000},
      .promotionTimestampMs_ = 2000,
  };

  globalEpochCacheInvalidationRegistry.withWriteLock(
      [&event](auto& registry) { registry.fireOnEpochPromoted(event); });

  // Verify cache was swapped
  auto newCache = manager.getBytesCacheRef();
  EXPECT_NE(oldCache.get(), newCache.get());

  // Verify metrics updated
  auto metrics = manager.getMetrics();
  EXPECT_EQ(metrics.totalPromotions, 1);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(ReadCacheManagerTest, ConcurrentCacheAccess) {
  auto& manager = ReadCacheManager::getInstance();

  constexpr int kThreads = 10;
  constexpr int kIterations = 100;

  std::vector<std::thread> threads;

  // Multiple threads accessing caches concurrently
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&manager]() {
      for (int i = 0; i < kIterations; ++i) {
        auto bytesCache = manager.getBytesCacheRef();
        auto planCache = manager.getPlanCacheRef();
        auto negativeCache = manager.getNegativeCacheRef();

        // Access cache (should not crash)
        EXPECT_NE(bytesCache, nullptr);
        EXPECT_NE(planCache, nullptr);
        EXPECT_NE(negativeCache, nullptr);
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }
}

TEST_F(ReadCacheManagerTest, ConcurrentPromotionAndAccess) {
  auto& manager = ReadCacheManager::getInstance();

  std::atomic<bool> stopFlag{false};

  // Thread 1: Continuously promote epochs
  std::thread promoter([&manager, &stopFlag]() {
    EpochId epoch = 0;
    while (!stopFlag.load()) {
      manager.onEpochPromoted(epoch, epoch + 1);
      epoch++;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  });

  // Thread 2: Continuously access caches
  std::thread accessor([&manager, &stopFlag]() {
    while (!stopFlag.load()) {
      auto cache = manager.getBytesCacheRef();
      EXPECT_NE(cache, nullptr);
    }
  });

  // Run for a short time
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  stopFlag.store(true);

  promoter.join();
  accessor.join();

  // Should have done multiple promotions
  auto metrics = manager.getMetrics();
  EXPECT_GT(metrics.totalPromotions, 0);
}

// ============================================================================
// Integration Test
// ============================================================================

TEST_F(ReadCacheManagerTest, FullIntegrationWithEpochRegistry) {
  auto& manager = ReadCacheManager::getInstance();

  // Register hook
  manager.setupEpochHooks();

  // Collect cache references across epochs
  std::vector<std::shared_ptr<BytesCacheType>> cacheRefs;

  // Simulate 3 epochs
  for (EpochId epoch = 0; epoch < 3; ++epoch) {
    // Get current cache
    cacheRefs.push_back(manager.getBytesCacheRef());

    // Fire promotion event
    EpochPromotionEvent event{
        .oldEpochId_ = epoch,
        .oldSnapshot_ = {.epochId_ = epoch,
                         .state_ = EpochState::SERVE,
                         .timestampMs_ = static_cast<int64_t>(epoch * 1000)},
        .newEpochId_ = epoch + 1,
        .newSnapshot_ = {.epochId_ = epoch + 1,
                         .state_ = EpochState::SERVE,
                         .timestampMs_ = static_cast<int64_t>((epoch + 1) *
                                                               1000)},
        .promotionTimestampMs_ = static_cast<int64_t>((epoch + 1) * 1000),
    };

    fireEpochPromotionEvent(event);
  }

  // Get final cache
  cacheRefs.push_back(manager.getBytesCacheRef());

  // Verify all caches are different (0% cross-epoch reuse)
  for (size_t i = 0; i < cacheRefs.size(); ++i) {
    for (size_t j = i + 1; j < cacheRefs.size(); ++j) {
      EXPECT_NE(cacheRefs[i].get(), cacheRefs[j].get())
          << "Epoch " << i << " and " << j << " caches should be different";
    }
  }

  // Verify metrics
  auto metrics = manager.getMetrics();
  EXPECT_EQ(metrics.totalPromotions, 3);
}
