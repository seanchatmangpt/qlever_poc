// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Tests for PlanCache with single-flight deduplication

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "engine/readCache/PlanCache.h"
#include "util/GTestHelpers.h"
#include "util/IndexTestHelpers.h"

using namespace readCache;
using namespace std::chrono_literals;

// Helper to create a dummy QueryFingerprint for testing
QueryFingerprint makeTestFingerprint(int id) {
  QueryFingerprint fp;
  fp.epoch_id = 1;
  fp.epoch_manifest_sha256 = "test_manifest_" + std::to_string(id);
  fp.raw_query_sha256 = "raw_" + std::to_string(id);
  fp.normalized_text_sha256 = "normalized_" + std::to_string(id);
  fp.shape_sha256 = "shape_" + std::to_string(id);
  fp.params_sha256 = "params_" + std::to_string(id);
  fp.shape_feature_vector_hash = "features_" + std::to_string(id);
  return fp;
}

// Helper to create a dummy CachedPlan
std::shared_ptr<CachedPlan> makeDummyPlan(int id, uint64_t bytes = 1024) {
  // Create a minimal QueryExecutionContext for testing
  // Note: This is a simplified approach; in real code we'd use proper test
  // fixtures
  auto plan = std::make_shared<CachedPlan>();
  plan->executionTree = nullptr;  // For now, we don't need a real tree
  plan->estimatedBytes = bytes;
  return plan;
}

// ============================================================================
// Basic Tests
// ============================================================================

TEST(PlanCacheTest, BasicInsertAndLookup) {
  PlanCache cache(1024 * 1024);  // 1 MB cache

  auto key1 = makeTestFingerprint(1);
  auto key2 = makeTestFingerprint(2);

  // Initially, both keys should miss
  EXPECT_EQ(cache.lookupHit(key1), std::nullopt);
  EXPECT_EQ(cache.lookupHit(key2), std::nullopt);

  // Insert key1
  std::atomic<int> computeCount{0};
  auto plan1 = cache.getOrCompile(key1, [&computeCount]() {
    computeCount++;
    return makeDummyPlan(1);
  });

  EXPECT_NE(plan1, nullptr);
  EXPECT_EQ(computeCount, 1);

  // Now key1 should hit
  auto hit = cache.lookupHit(key1);
  EXPECT_TRUE(hit.has_value());
  EXPECT_EQ(hit.value(), plan1);

  // key2 should still miss
  EXPECT_EQ(cache.lookupHit(key2), std::nullopt);

  // Check stats
  auto stats = cache.getStats();
  EXPECT_EQ(stats.hits, 1);
  EXPECT_EQ(stats.misses, 1);
  EXPECT_EQ(stats.numEntries, 1);
}

TEST(PlanCacheTest, GetOrCompileCachesResult) {
  PlanCache cache(1024 * 1024);
  auto key = makeTestFingerprint(1);

  std::atomic<int> computeCount{0};

  // First call should compute
  auto plan1 = cache.getOrCompile(key, [&computeCount]() {
    computeCount++;
    return makeDummyPlan(1);
  });

  EXPECT_EQ(computeCount, 1);

  // Second call should hit cache, not compute
  auto plan2 = cache.getOrCompile(key, [&computeCount]() {
    computeCount++;
    return makeDummyPlan(1);
  });

  EXPECT_EQ(computeCount, 1);  // Still 1, not called again
  EXPECT_EQ(plan1, plan2);     // Same pointer

  auto stats = cache.getStats();
  EXPECT_EQ(stats.hits, 1);
  EXPECT_EQ(stats.misses, 1);
}

// ============================================================================
// Single-Flight Tests (CRITICAL)
// ============================================================================

TEST(PlanCacheTest, SingleFlightPreventsDuplicateCompilation) {
  PlanCache cache(1024 * 1024);
  auto key = makeTestFingerprint(42);

  std::atomic<int> computeCount{0};
  std::atomic<int> startedCount{0};
  constexpr int NUM_THREADS = 10;

  auto computeFn = [&computeCount, &startedCount]() {
    startedCount++;
    // Simulate expensive compilation
    std::this_thread::sleep_for(50ms);
    computeCount++;
    return makeDummyPlan(42);
  };

  // Launch 10 threads simultaneously requesting the same key
  std::vector<std::thread> threads;
  std::vector<std::shared_ptr<CachedPlan>> results(NUM_THREADS);

  auto startTime = std::chrono::steady_clock::now();

  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.emplace_back([&, i]() { results[i] = cache.getOrCompile(key, computeFn); });
  }

  // Wait for all threads
  for (auto& t : threads) {
    t.join();
  }

  auto endTime = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                      endTime - startTime)
                      .count();

  // CRITICAL: computeFn should be called exactly once
  EXPECT_EQ(computeCount, 1)
      << "Single-flight FAILED: compute function called " << computeCount
      << " times instead of 1";

  // All threads should receive the same plan pointer
  for (int i = 0; i < NUM_THREADS; ++i) {
    EXPECT_NE(results[i], nullptr);
    EXPECT_EQ(results[i], results[0])
        << "Thread " << i << " received different plan pointer";
  }

  // Verify cache stats
  auto stats = cache.getStats();
  EXPECT_EQ(stats.misses, 1) << "Should have exactly 1 miss";
  EXPECT_EQ(stats.singleFlightSaves, NUM_THREADS - 1)
      << "Should have " << (NUM_THREADS - 1) << " single-flight saves";

  // Total time should be close to 50ms, not 500ms (10 * 50ms)
  // Allow some overhead for thread coordination
  EXPECT_LT(duration, 200)
      << "Single-flight should prevent sequential execution. Took " << duration
      << "ms";

  std::cout << "Single-flight test: " << NUM_THREADS << " threads, "
            << computeCount << " compilations, " << duration
            << "ms total time\n";
}

TEST(PlanCacheTest, SingleFlightWithDifferentKeys) {
  PlanCache cache(1024 * 1024);

  std::atomic<int> computeCount1{0};
  std::atomic<int> computeCount2{0};

  auto key1 = makeTestFingerprint(1);
  auto key2 = makeTestFingerprint(2);

  // Launch threads for two different keys simultaneously
  std::vector<std::thread> threads;

  // 5 threads for key1
  for (int i = 0; i < 5; ++i) {
    threads.emplace_back([&]() {
      cache.getOrCompile(key1, [&computeCount1]() {
        std::this_thread::sleep_for(30ms);
        computeCount1++;
        return makeDummyPlan(1);
      });
    });
  }

  // 5 threads for key2
  for (int i = 0; i < 5; ++i) {
    threads.emplace_back([&]() {
      cache.getOrCompile(key2, [&computeCount2]() {
        std::this_thread::sleep_for(30ms);
        computeCount2++;
        return makeDummyPlan(2);
      });
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // Each key should be compiled exactly once
  EXPECT_EQ(computeCount1, 1);
  EXPECT_EQ(computeCount2, 1);

  // Should have 2 cache entries
  auto stats = cache.getStats();
  EXPECT_EQ(stats.numEntries, 2);
  EXPECT_EQ(stats.misses, 2);
  EXPECT_GE(stats.singleFlightSaves, 8);  // At least 8 (4+4)
}

// ============================================================================
// Eviction Tests
// ============================================================================

TEST(PlanCacheTest, LRUEviction) {
  // Small cache: 3KB
  PlanCache cache(3 * 1024);

  // Insert 4 plans of 1KB each
  // Expected: first one gets evicted
  auto key1 = makeTestFingerprint(1);
  auto key2 = makeTestFingerprint(2);
  auto key3 = makeTestFingerprint(3);
  auto key4 = makeTestFingerprint(4);

  cache.getOrCompile(key1, []() { return makeDummyPlan(1, 1024); });
  cache.getOrCompile(key2, []() { return makeDummyPlan(2, 1024); });
  cache.getOrCompile(key3, []() { return makeDummyPlan(3, 1024); });

  // At this point, all 3 should be cached
  EXPECT_TRUE(cache.lookupHit(key1).has_value());
  EXPECT_TRUE(cache.lookupHit(key2).has_value());
  EXPECT_TRUE(cache.lookupHit(key3).has_value());

  // Insert 4th plan, should evict key1 (LRU)
  cache.getOrCompile(key4, []() { return makeDummyPlan(4, 1024); });

  // key1 should be evicted
  EXPECT_FALSE(cache.lookupHit(key1).has_value());
  // Others should still be present
  EXPECT_TRUE(cache.lookupHit(key2).has_value());
  EXPECT_TRUE(cache.lookupHit(key3).has_value());
  EXPECT_TRUE(cache.lookupHit(key4).has_value());

  auto stats = cache.getStats();
  EXPECT_GE(stats.evictions, 1);
}

TEST(PlanCacheTest, LRUTouchOnAccess) {
  PlanCache cache(3 * 1024);

  auto key1 = makeTestFingerprint(1);
  auto key2 = makeTestFingerprint(2);
  auto key3 = makeTestFingerprint(3);
  auto key4 = makeTestFingerprint(4);

  cache.getOrCompile(key1, []() { return makeDummyPlan(1, 1024); });
  cache.getOrCompile(key2, []() { return makeDummyPlan(2, 1024); });
  cache.getOrCompile(key3, []() { return makeDummyPlan(3, 1024); });

  // Access key1 to make it most recently used
  cache.lookupHit(key1);

  // Insert key4, should evict key2 (now LRU), not key1
  cache.getOrCompile(key4, []() { return makeDummyPlan(4, 1024); });

  // key1 should still be present (was touched)
  EXPECT_TRUE(cache.lookupHit(key1).has_value());
  // key2 should be evicted
  EXPECT_FALSE(cache.lookupHit(key2).has_value());
  // key3 and key4 should be present
  EXPECT_TRUE(cache.lookupHit(key3).has_value());
  EXPECT_TRUE(cache.lookupHit(key4).has_value());
}

TEST(PlanCacheTest, SetMaxBytesTriggersEviction) {
  PlanCache cache(10 * 1024);

  auto key1 = makeTestFingerprint(1);
  auto key2 = makeTestFingerprint(2);
  auto key3 = makeTestFingerprint(3);

  cache.getOrCompile(key1, []() { return makeDummyPlan(1, 2 * 1024); });
  cache.getOrCompile(key2, []() { return makeDummyPlan(2, 2 * 1024); });
  cache.getOrCompile(key3, []() { return makeDummyPlan(3, 2 * 1024); });

  EXPECT_EQ(cache.getStats().numEntries, 3);

  // Reduce max size to 3KB, should evict at least one
  cache.setMaxBytes(3 * 1024);

  auto stats = cache.getStats();
  EXPECT_LE(stats.numEntries, 2);
  EXPECT_LE(stats.totalBytes, 3 * 1024);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(PlanCacheTest, ComputeFunctionThrowsPropagates) {
  PlanCache cache(1024 * 1024);
  auto key = makeTestFingerprint(1);

  auto computeFn = []() -> std::shared_ptr<CachedPlan> {
    throw std::runtime_error("Compilation failed!");
  };

  EXPECT_THROW(cache.getOrCompile(key, computeFn), std::runtime_error);

  // Key should not be cached
  EXPECT_FALSE(cache.lookupHit(key).has_value());
}

TEST(PlanCacheTest, SingleFlightExceptionPropagation) {
  PlanCache cache(1024 * 1024);
  auto key = makeTestFingerprint(1);

  std::atomic<int> computeAttempts{0};

  auto computeFn = [&computeAttempts]() -> std::shared_ptr<CachedPlan> {
    computeAttempts++;
    std::this_thread::sleep_for(30ms);
    throw std::runtime_error("Deliberate failure");
  };

  constexpr int NUM_THREADS = 5;
  std::vector<std::thread> threads;
  std::atomic<int> exceptionsThrown{0};

  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.emplace_back([&]() {
      try {
        cache.getOrCompile(key, computeFn);
      } catch (const std::runtime_error&) {
        exceptionsThrown++;
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  // All threads should receive the exception
  EXPECT_EQ(exceptionsThrown, NUM_THREADS);

  // Compute should have been attempted only once (single-flight)
  EXPECT_EQ(computeAttempts, 1);

  // Key should not be cached
  EXPECT_FALSE(cache.lookupHit(key).has_value());
}

// ============================================================================
// Stats Tests
// ============================================================================

TEST(PlanCacheTest, StatsTracking) {
  PlanCache cache(10 * 1024);

  auto key1 = makeTestFingerprint(1);
  auto key2 = makeTestFingerprint(2);

  auto initialStats = cache.getStats();
  EXPECT_EQ(initialStats.hits, 0);
  EXPECT_EQ(initialStats.misses, 0);
  EXPECT_EQ(initialStats.numEntries, 0);

  // Miss + insert
  cache.getOrCompile(key1, []() { return makeDummyPlan(1, 1024); });

  auto stats1 = cache.getStats();
  EXPECT_EQ(stats1.misses, 1);
  EXPECT_EQ(stats1.numEntries, 1);
  EXPECT_EQ(stats1.totalBytes, 1024);

  // Hit
  cache.lookupHit(key1);

  auto stats2 = cache.getStats();
  EXPECT_EQ(stats2.hits, 1);
  EXPECT_EQ(stats2.misses, 1);

  // Another miss + insert
  cache.getOrCompile(key2, []() { return makeDummyPlan(2, 2048); });

  auto stats3 = cache.getStats();
  EXPECT_EQ(stats3.misses, 2);
  EXPECT_EQ(stats3.numEntries, 2);
  EXPECT_EQ(stats3.totalBytes, 3072);
}

// ============================================================================
// Clear Tests
// ============================================================================

TEST(PlanCacheTest, ClearAll) {
  PlanCache cache(10 * 1024);

  auto key1 = makeTestFingerprint(1);
  auto key2 = makeTestFingerprint(2);

  cache.getOrCompile(key1, []() { return makeDummyPlan(1, 1024); });
  cache.getOrCompile(key2, []() { return makeDummyPlan(2, 1024); });

  EXPECT_EQ(cache.getStats().numEntries, 2);

  cache.clearAll();

  auto stats = cache.getStats();
  EXPECT_EQ(stats.numEntries, 0);
  EXPECT_EQ(stats.totalBytes, 0);

  // Keys should not be found
  EXPECT_FALSE(cache.lookupHit(key1).has_value());
  EXPECT_FALSE(cache.lookupHit(key2).has_value());
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(PlanCacheTest, HighConcurrencyStressTest) {
  PlanCache cache(100 * 1024);

  constexpr int NUM_KEYS = 20;
  constexpr int NUM_THREADS_PER_KEY = 5;
  constexpr int TOTAL_THREADS = NUM_KEYS * NUM_THREADS_PER_KEY;

  std::vector<std::atomic<int>> computeCounts(NUM_KEYS);
  for (auto& c : computeCounts) {
    c = 0;
  }

  std::vector<std::thread> threads;

  for (int keyId = 0; keyId < NUM_KEYS; ++keyId) {
    for (int t = 0; t < NUM_THREADS_PER_KEY; ++t) {
      threads.emplace_back([&, keyId]() {
        auto key = makeTestFingerprint(keyId);
        cache.getOrCompile(key, [&, keyId]() {
          std::this_thread::sleep_for(5ms);
          computeCounts[keyId]++;
          return makeDummyPlan(keyId, 1024);
        });
      });
    }
  }

  for (auto& t : threads) {
    t.join();
  }

  // Each key should be compiled exactly once
  for (int i = 0; i < NUM_KEYS; ++i) {
    EXPECT_EQ(computeCounts[i], 1)
        << "Key " << i << " compiled " << computeCounts[i] << " times";
  }

  auto stats = cache.getStats();
  EXPECT_EQ(stats.numEntries, NUM_KEYS);
  EXPECT_EQ(stats.misses, NUM_KEYS);
  EXPECT_GE(stats.singleFlightSaves, TOTAL_THREADS - NUM_KEYS);
}
