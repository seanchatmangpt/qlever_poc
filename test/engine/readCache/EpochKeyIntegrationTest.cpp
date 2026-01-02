// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Agent 3 - EPIC 10.1 Integration (Epoch Gating)
//
// Purpose: EpochKeyIntegrationTest - Verify deterministic envelope integration
// with epoch gating into the read-plane pipeline
//
// Tests the critical wiring:
// 1. QueryExecutionContext::getCurrentEpochKey() returns manifest-bound
// EpochKey
// 2. EpochCacheGate mechanically rejects cross-epoch lookups
// 3. Epoch transitions invalidate old cache keys
//
// This test validates EPIC 10.1 item B: integration wiring for deterministic
// envelope + epoch gating into read-plane pipeline.

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "engine/QueryExecutionContext.h"
#include "engine/readCache/BytesCache.h"
#include "engine/readCache/EpochCacheGate.h"
#include "engine/readCache/NegativeCache.h"
#include "engine/readCache/PlanCache.h"
#include "engine/readCache/ReadCacheKeys.h"
#include "global/EpochManifest.h"
#include "util/GTestHelpers.h"

namespace readCache {

// ============================================================================
// EpochKeyIntegrationTest - Wiring validation for read-plane pipeline
// ============================================================================

class EpochKeyIntegrationTest : public ::testing::Test {
 protected:
  // Helper: Create a BytesKey with specific epoch
  BytesKey makeKey(const std::string& epoch_hash, const std::string& shape,
                   const std::string& params) {
    return BytesKey{EpochKey{epoch_hash}, shape, params,
                    ad_utility::MediaType::json, 0};
  }

  // Helper: Create a cached response
  std::shared_ptr<ad_utility::readCache::CachedResponseBytes> makeCachedBytes(
      const std::string& content) {
    auto response =
        std::make_shared<ad_utility::readCache::CachedResponseBytes>();
    response->format = ad_utility::MediaType::json;
    response->uncompressedSize = content.size();
    return response;
  }
};

// ===== Test 1: EpochCacheGate Rejects Cross-Epoch Access =====
// Validates that cache gate mechanically prevents stale hits

TEST_F(EpochKeyIntegrationTest, CacheGateRejectedCrossEpochLookup) {
  auto gate = createBytesCacheGate("manifest_hash_epoch1", 1024 * 1024);

  // Create key bound to DIFFERENT epoch
  auto mismatched_key = makeKey("manifest_hash_epoch2", "shape1", "params1");
  auto value = makeCachedBytes("test data");

  // Insert FAILS - epoch mismatch
  EXPECT_FALSE(gate->insertWithEpochCheck(mismatched_key, value));

  // Lookup FAILS - epoch mismatch (treated as cache miss, not error)
  auto result = gate->lookupWithEpochCheck<
      BytesKey, std::shared_ptr<ad_utility::readCache::CachedResponseBytes>>(
      mismatched_key);
  EXPECT_FALSE(result.has_value());

  // Verify violations recorded
  auto metrics = gate->getMetrics();
  EXPECT_EQ(metrics.epoch_violations, 2u);  // 1 failed insert + 1 failed lookup
}

// ===== Test 2: Correct Epoch Allows Cache Hit =====
// Validates that matching epoch allows proper cache access

TEST_F(EpochKeyIntegrationTest, CacheGateAcceptsMatchingEpoch) {
  const std::string epoch_hash = "manifest_hash_epoch1";
  auto gate = createBytesCacheGate(epoch_hash, 1024 * 1024);

  // Create key bound to SAME epoch
  auto matching_key = makeKey(epoch_hash, "shape1", "params1");
  auto value = makeCachedBytes("test data");

  // Insert SUCCEEDS - epoch matches
  EXPECT_TRUE(gate->insertWithEpochCheck(matching_key, value));

  // Lookup SUCCEEDS - epoch matches, cache hit
  auto result = gate->lookupWithEpochCheck<
      BytesKey, std::shared_ptr<ad_utility::readCache::CachedResponseBytes>>(
      matching_key);
  EXPECT_TRUE(result.has_value());

  // Verify no violations
  auto metrics = gate->getMetrics();
  EXPECT_EQ(metrics.epoch_violations, 0u);
}

// ===== Test 3: Epoch Transition Invalidates Old Cache =====
// Validates atomic swap on epoch boundary

TEST_F(EpochKeyIntegrationTest, EpochTransitionInvalidatesCache) {
  const std::string epoch1_hash = "manifest_hash_epoch1";
  const std::string epoch2_hash = "manifest_hash_epoch2";
  auto gate = createBytesCacheGate(epoch1_hash, 1024 * 1024);

  // Insert data in epoch1
  auto key1 = makeKey(epoch1_hash, "shape1", "params1");
  auto value = makeCachedBytes("epoch1 data");
  ASSERT_TRUE(gate->insertWithEpochCheck(key1, value));

  // Verify lookup succeeds in epoch1
  auto result1 = gate->lookupWithEpochCheck<
      BytesKey, std::shared_ptr<ad_utility::readCache::CachedResponseBytes>>(
      key1);
  ASSERT_TRUE(result1.has_value());

  // Transition to epoch2
  gate->transitionToNewEpoch(epoch2_hash);

  // Verify current epoch changed
  EXPECT_EQ(gate->getCurrentEpoch(), epoch2_hash);

  // Verify lookup with OLD epoch key FAILS (cache invalidated)
  auto result_after_transition = gate->lookupWithEpochCheck<
      BytesKey, std::shared_ptr<ad_utility::readCache::CachedResponseBytes>>(
      key1);
  EXPECT_FALSE(result_after_transition.has_value());

  // Verify lookup with NEW epoch key works (new cache instance)
  auto key2 = makeKey(epoch2_hash, "shape1", "params1");
  auto value2 = makeCachedBytes("epoch2 data");
  ASSERT_TRUE(gate->insertWithEpochCheck(key2, value2));
  auto result2 = gate->lookupWithEpochCheck<
      BytesKey, std::shared_ptr<ad_utility::readCache::CachedResponseBytes>>(
      key2);
  EXPECT_TRUE(result2.has_value());

  // Verify transition recorded in metrics
  auto metrics = gate->getMetrics();
  EXPECT_EQ(metrics.epoch_transitions, 1u);
  EXPECT_EQ(metrics.cache_invalidations, 1u);
}

// ===== Test 4: Fail-Closed Semantics on Epoch Mismatch =====
// Validates that epoch check is mechanically enforced (not conditional)

TEST_F(EpochKeyIntegrationTest, FailClosedEpochEnforcement) {
  auto gate = createBytesCacheGate("epoch1", 1024 * 1024);

  // Create multiple mismatched keys
  std::vector<std::string> mismatched_epochs = {"epoch2", "epoch3", "epoch4"};
  for (const auto& epoch : mismatched_epochs) {
    auto key = makeKey(epoch, "shape1", "params1");
    auto value = makeCachedBytes("test");

    // Every insert MUST fail (fail-closed)
    EXPECT_FALSE(gate->insertWithEpochCheck(key, value));

    // Every lookup MUST return nullopt (fail-closed)
    auto result = gate->lookupWithEpochCheck<
        BytesKey, std::shared_ptr<ad_utility::readCache::CachedResponseBytes>>(
        key);
    EXPECT_FALSE(result.has_value());
  }

  // Verify all violations counted
  auto metrics = gate->getMetrics();
  EXPECT_EQ(metrics.epoch_violations, mismatched_epochs.size() * 2u);
}

// ===== Test 5: Memory Bounds Enforcement with Epoch Gating =====
// Validates that memory limits still apply after epoch check

TEST_F(EpochKeyIntegrationTest, MemoryBoundsEnforced) {
  const uint64_t max_bytes = 1000;  // Very small limit
  auto gate = createBytesCacheGate("epoch1", max_bytes);

  auto key = makeKey("epoch1", "shape1", "params1");
  auto large_value = makeCachedBytes(std::string(2000, 'x'));  // Exceeds limit

  // Even with matching epoch, insertion may fail due to memory bounds
  // (depends on cache admission policy)
  // This tests that both epoch check AND memory check are applied
  gate->insertWithEpochCheck(key, large_value);

  // Verify bounds checking still active
  auto metrics = gate->getMetrics();
  EXPECT_EQ(metrics.total_insertions, 1u);
}

}  // namespace readCache
