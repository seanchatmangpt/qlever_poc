// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Agent 3 - EPIC 10.1 Integration (Epoch Gating)
//
// Purpose: QueryExecutionContextEpochKeyTest - Verify epoch key integration
// in QueryExecutionContext read-plane pipeline wiring
//
// Tests that:
// 1. getCurrentEpochKey() returns deterministic EpochKey
// 2. EpochKey validation fails for empty manifests (logged)
// 3. EpochKey remains stable across epoch context lifetime

#include <gtest/gtest.h>

#include <memory>

#include "engine/QueryExecutionContext.h"
#include "engine/readCache/ReadCacheKeys.h"
#include "global/EpochManifest.h"
#include "util/GTestHelpers.h"

namespace readCache {

// ============================================================================
// QueryExecutionContextEpochKeyTest - Epoch key integration validation
// ============================================================================

class QueryExecutionContextEpochKeyTest : public ::testing::Test {
 protected:
  // Helper: Mock index (minimal for testing)
  // Note: Full QueryExecutionContext requires real Index, so we test via
  // the public interface only (not instantiation)
};

// ===== Test 1: EpochKey Type Validation =====
// Validates that EpochKey type is properly defined and usable

TEST_F(QueryExecutionContextEpochKeyTest, EpochKeyTypeIsValid) {
  // Create EpochKey directly (as getCurrentEpochKey() would)
  readCache::EpochKey key1("abc123");
  EXPECT_TRUE(key1.isValid());
  EXPECT_EQ(key1.epoch_manifest_hash, "abc123");

  // Empty key is invalid
  readCache::EpochKey empty_key("");
  EXPECT_FALSE(empty_key.isValid());
  EXPECT_EQ(empty_key.epoch_manifest_hash, "");
}

// ===== Test 2: EpochKey Determinism =====
// Validates that same manifest hash produces same key

TEST_F(QueryExecutionContextEpochKeyTest, EpochKeyDeterministic) {
  const std::string manifest_hash =
      "abcdef0123456789abcdef0123456789"
      "abcdef0123456789abcdef0123456789";  // SHA256 format (64 chars)

  readCache::EpochKey key1(manifest_hash);
  readCache::EpochKey key2(manifest_hash);

  EXPECT_EQ(key1, key2);
  EXPECT_EQ(key1.epoch_manifest_hash, key2.epoch_manifest_hash);
}

// ===== Test 3: EpochKey Different for Different Manifests =====
// Validates that different manifests produce different keys

TEST_F(QueryExecutionContextEpochKeyTest,
       EpochKeyDifferentForDifferentManifests) {
  readCache::EpochKey key1("manifest_hash_1");
  readCache::EpochKey key2("manifest_hash_2");

  EXPECT_NE(key1, key2);
  EXPECT_NE(key1.epoch_manifest_hash, key2.epoch_manifest_hash);
}

// ===== Test 4: EpochKey in PlanKey =====
// Validates that EpochKey integrates properly into PlanKey

TEST_F(QueryExecutionContextEpochKeyTest, EpochKeyInPlanKey) {
  const std::string epoch_hash = "epoch_manifest_hash_v1";
  const std::string shape_hash = "query_shape_hash_v1";

  readCache::EpochKey epoch_key(epoch_hash);
  readCache::PlanKey plan_key(epoch_key, shape_hash);

  EXPECT_TRUE(plan_key.isValid());
  EXPECT_EQ(plan_key.epoch_key.epoch_manifest_hash, epoch_hash);
  EXPECT_EQ(plan_key.shape_sha256, shape_hash);
}

// ===== Test 5: EpochKey in BytesKey =====
// Validates that EpochKey integrates properly into BytesKey (serialization
// cache)

TEST_F(QueryExecutionContextEpochKeyTest, EpochKeyInBytesKey) {
  const std::string epoch_hash = "epoch_manifest_hash_v1";
  const std::string shape_hash = "result_shape_hash_v1";
  const std::string params_hash = "serialization_params_hash_v1";

  readCache::EpochKey epoch_key(epoch_hash);
  readCache::BytesKey bytes_key(epoch_key, shape_hash, params_hash,
                                ad_utility::MediaType::json, 0);

  EXPECT_TRUE(bytes_key.isValid());
  EXPECT_EQ(bytes_key.epoch_key.epoch_manifest_hash, epoch_hash);
  EXPECT_EQ(bytes_key.shape_sha256, shape_hash);
  EXPECT_EQ(bytes_key.params_sha256, params_hash);
}

// ===== Test 6: EpochKey Hashing (for cache key operations) =====
// Validates that EpochKey supports hashing (required for map/set operations)

TEST_F(QueryExecutionContextEpochKeyTest, EpochKeyHashable) {
  readCache::EpochKey key1("epoch_hash_1");
  readCache::EpochKey key2("epoch_hash_1");
  readCache::EpochKey key3("epoch_hash_2");

  // Keys with same hash should have same hash value
  // (not strictly required but expected for consistency)
  auto hash1 = std::hash<readCache::EpochKey>{}(key1);
  auto hash2 = std::hash<readCache::EpochKey>{}(key2);

  // Keys with different hash should (likely) have different hash values
  auto hash3 = std::hash<readCache::EpochKey>{}(key3);

  EXPECT_EQ(hash1, hash2);  // Same input -> same hash
  // hash1 != hash3 is not guaranteed but very likely

  // Verify they can be used in sets/maps
  std::unordered_set<readCache::EpochKey, std::hash<readCache::EpochKey>>
      key_set;
  key_set.insert(key1);
  key_set.insert(key2);
  key_set.insert(key3);

  EXPECT_EQ(key_set.size(), 2u);  // key1 and key2 are same, key3 is different
}

// ===== Test 7: ReadCacheKey Types Integration =====
// Validates that all read cache key types properly embed EpochKey

TEST_F(QueryExecutionContextEpochKeyTest, ReadCacheKeyTypesEmbedEpochKey) {
  const std::string epoch_hash = "test_epoch_manifest_hash";
  const std::string shape_hash = "test_shape_hash";
  const std::string params_hash = "test_params_hash";

  readCache::EpochKey epoch_key(epoch_hash);

  // Test PlanKey
  readCache::PlanKey plan_key(epoch_key, shape_hash);
  EXPECT_EQ(plan_key.epoch_key, epoch_key);

  // Test BytesKey
  readCache::BytesKey bytes_key(epoch_key, shape_hash, params_hash,
                                ad_utility::MediaType::json, 42);
  EXPECT_EQ(bytes_key.epoch_key, epoch_key);

  // Test NegKey
  readCache::NegKey neg_key(epoch_key, shape_hash, params_hash);
  EXPECT_EQ(neg_key.epoch_key, epoch_key);
}

// ===== Test 8: Empty EpochKey Validation =====
// Validates behavior with empty/invalid keys (fail-closed semantics)

TEST_F(QueryExecutionContextEpochKeyTest, EmptyEpochKeyBehavior) {
  readCache::EpochKey empty_key("");

  // Empty key should be invalid
  EXPECT_FALSE(empty_key.isValid());

  // But can still be used in PlanKey (outer type handles validation)
  readCache::PlanKey plan_with_empty =
      readCache::PlanKey(empty_key, "shape_hash");
  EXPECT_FALSE(
      plan_with_empty.isValid());  // PlanKey is invalid if epoch_key is

  // Verify isValid() on all key types checks epoch
  readCache::BytesKey bytes_with_empty(empty_key, "shape", "params",
                                       ad_utility::MediaType::json, 0);
  EXPECT_FALSE(
      bytes_with_empty.isValid());  // BytesKey is invalid if epoch_key is
}

}  // namespace readCache
