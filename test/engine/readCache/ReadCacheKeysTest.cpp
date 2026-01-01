// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Unit tests for read cache key types (EPIC 3)

#include <gtest/gtest.h>

#include <absl/container/flat_hash_map.h>
#include <unordered_map>

#include "engine/readCache/ReadCacheKeys.h"
#include "util/http/MediaTypes.h"

using namespace readCache;

// _____________________________________________________________________________
// Test fixture for ReadCacheKeys tests
class ReadCacheKeysTest : public ::testing::Test {
 protected:
  // Sample data for testing
  const std::string sampleEpochHash1_ =
      "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef";
  const std::string sampleEpochHash2_ =
      "fedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321";
  const std::string sampleShapeHash_ =
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  const std::string sampleParamsHash_ =
      "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
};

// _____________________________________________________________________________
// EpochKey Tests
// _____________________________________________________________________________

TEST_F(ReadCacheKeysTest, EpochKeyConstruction) {
  EpochKey key1;
  EXPECT_FALSE(key1.isValid());
  EXPECT_TRUE(key1.epoch_manifest_hash.empty());

  EpochKey key2(sampleEpochHash1_);
  EXPECT_TRUE(key2.isValid());
  EXPECT_EQ(key2.epoch_manifest_hash, sampleEpochHash1_);
}

TEST_F(ReadCacheKeysTest, EpochKeyEquality) {
  EpochKey key1(sampleEpochHash1_);
  EpochKey key2(sampleEpochHash1_);
  EpochKey key3(sampleEpochHash2_);

  EXPECT_EQ(key1, key2);
  EXPECT_NE(key1, key3);
}

TEST_F(ReadCacheKeysTest, EpochKeyHashing) {
  EpochKey key1(sampleEpochHash1_);
  EpochKey key2(sampleEpochHash1_);
  EpochKey key3(sampleEpochHash2_);

  // Same keys should have same hash
  EXPECT_EQ(absl::Hash<EpochKey>{}(key1), absl::Hash<EpochKey>{}(key2));

  // Different keys should (probably) have different hashes
  EXPECT_NE(absl::Hash<EpochKey>{}(key1), absl::Hash<EpochKey>{}(key3));
}

TEST_F(ReadCacheKeysTest, EpochKeyHashMapUsage) {
  absl::flat_hash_map<EpochKey, int> map;

  EpochKey key1(sampleEpochHash1_);
  EpochKey key2(sampleEpochHash2_);

  map[key1] = 42;
  map[key2] = 100;

  EXPECT_EQ(map[key1], 42);
  EXPECT_EQ(map[key2], 100);
  EXPECT_EQ(map.size(), 2);
}

// _____________________________________________________________________________
// PlanKey Tests
// _____________________________________________________________________________

TEST_F(ReadCacheKeysTest, PlanKeyConstruction) {
  PlanKey key1;
  EXPECT_FALSE(key1.isValid());

  EpochKey epochKey(sampleEpochHash1_);
  PlanKey key2(epochKey, sampleShapeHash_);
  EXPECT_TRUE(key2.isValid());
  EXPECT_EQ(key2.epoch_key, epochKey);
  EXPECT_EQ(key2.shape_sha256, sampleShapeHash_);
}

TEST_F(ReadCacheKeysTest, PlanKeyEqualityAndHashing) {
  EpochKey epoch1(sampleEpochHash1_);
  EpochKey epoch2(sampleEpochHash2_);

  PlanKey key1(epoch1, sampleShapeHash_);
  PlanKey key2(epoch1, sampleShapeHash_);
  PlanKey key3(epoch2, sampleShapeHash_);

  // Same epoch and shape should be equal
  EXPECT_EQ(key1, key2);
  EXPECT_NE(key1, key3);

  // Same keys should have same hash
  EXPECT_EQ(absl::Hash<PlanKey>{}(key1), absl::Hash<PlanKey>{}(key2));
}

TEST_F(ReadCacheKeysTest, PlanKeyDifferentEpochsDifferentKeys) {
  // This test verifies the critical requirement: same query shape but
  // different epochs must produce different cache keys
  EpochKey epoch1(sampleEpochHash1_);
  EpochKey epoch2(sampleEpochHash2_);

  PlanKey key1(epoch1, sampleShapeHash_);
  PlanKey key2(epoch2, sampleShapeHash_);

  EXPECT_NE(key1, key2);
  EXPECT_NE(absl::Hash<PlanKey>{}(key1), absl::Hash<PlanKey>{}(key2));
}

// _____________________________________________________________________________
// BytesKey Tests
// _____________________________________________________________________________

TEST_F(ReadCacheKeysTest, BytesKeyConstruction) {
  BytesKey key1;
  EXPECT_FALSE(key1.isValid());

  EpochKey epochKey(sampleEpochHash1_);
  BytesKey key2(epochKey, sampleShapeHash_, sampleParamsHash_,
                ad_utility::MediaType::json, 12345);
  EXPECT_TRUE(key2.isValid());
  EXPECT_EQ(key2.epoch_key, epochKey);
  EXPECT_EQ(key2.shape_sha256, sampleShapeHash_);
  EXPECT_EQ(key2.params_sha256, sampleParamsHash_);
  EXPECT_EQ(key2.output_format, ad_utility::MediaType::json);
  EXPECT_EQ(key2.options_hash, 12345);
}

TEST_F(ReadCacheKeysTest, BytesKeyDifferentFormats) {
  EpochKey epochKey(sampleEpochHash1_);

  BytesKey keyJson(epochKey, sampleShapeHash_, sampleParamsHash_,
                   ad_utility::MediaType::json, 0);
  BytesKey keyTsv(epochKey, sampleShapeHash_, sampleParamsHash_,
                  ad_utility::MediaType::tsv, 0);

  // Same query but different output formats should have different keys
  EXPECT_NE(keyJson, keyTsv);
  EXPECT_NE(absl::Hash<BytesKey>{}(keyJson), absl::Hash<BytesKey>{}(keyTsv));
}

TEST_F(ReadCacheKeysTest, BytesKeyDifferentOptions) {
  EpochKey epochKey(sampleEpochHash1_);

  BytesKey key1(epochKey, sampleShapeHash_, sampleParamsHash_,
                ad_utility::MediaType::json, 0);
  BytesKey key2(epochKey, sampleShapeHash_, sampleParamsHash_,
                ad_utility::MediaType::json, 12345);

  // Same query and format but different options should have different keys
  EXPECT_NE(key1, key2);
  EXPECT_NE(absl::Hash<BytesKey>{}(key1), absl::Hash<BytesKey>{}(key2));
}

TEST_F(ReadCacheKeysTest, BytesKeyEpochDependence) {
  // Critical test: same query, format, options but different epochs
  EpochKey epoch1(sampleEpochHash1_);
  EpochKey epoch2(sampleEpochHash2_);

  BytesKey key1(epoch1, sampleShapeHash_, sampleParamsHash_,
                ad_utility::MediaType::json, 0);
  BytesKey key2(epoch2, sampleShapeHash_, sampleParamsHash_,
                ad_utility::MediaType::json, 0);

  EXPECT_NE(key1, key2);
  EXPECT_NE(absl::Hash<BytesKey>{}(key1), absl::Hash<BytesKey>{}(key2));
}

TEST_F(ReadCacheKeysTest, BytesKeyHashMapUsage) {
  absl::flat_hash_map<BytesKey, std::string> cache;

  EpochKey epochKey(sampleEpochHash1_);
  BytesKey key1(epochKey, sampleShapeHash_, sampleParamsHash_,
                ad_utility::MediaType::json, 0);
  BytesKey key2(epochKey, sampleShapeHash_, sampleParamsHash_,
                ad_utility::MediaType::tsv, 0);

  cache[key1] = "json_data";
  cache[key2] = "tsv_data";

  EXPECT_EQ(cache[key1], "json_data");
  EXPECT_EQ(cache[key2], "tsv_data");
  EXPECT_EQ(cache.size(), 2);
}

// _____________________________________________________________________________
// NegKey Tests
// _____________________________________________________________________________

TEST_F(ReadCacheKeysTest, NegKeyConstruction) {
  NegKey key1;
  EXPECT_FALSE(key1.isValid());

  EpochKey epochKey(sampleEpochHash1_);
  NegKey key2(epochKey, sampleShapeHash_, sampleParamsHash_);
  EXPECT_TRUE(key2.isValid());
  EXPECT_EQ(key2.epoch_key, epochKey);
  EXPECT_EQ(key2.shape_sha256, sampleShapeHash_);
  EXPECT_EQ(key2.params_sha256, sampleParamsHash_);
}

TEST_F(ReadCacheKeysTest, NegKeyEquality) {
  EpochKey epoch1(sampleEpochHash1_);
  EpochKey epoch2(sampleEpochHash2_);

  NegKey key1(epoch1, sampleShapeHash_, sampleParamsHash_);
  NegKey key2(epoch1, sampleShapeHash_, sampleParamsHash_);
  NegKey key3(epoch2, sampleShapeHash_, sampleParamsHash_);

  EXPECT_EQ(key1, key2);
  EXPECT_NE(key1, key3);
}

TEST_F(ReadCacheKeysTest, NegKeyDifferentEpochs) {
  // Negative cache must also prevent cross-epoch hits
  EpochKey epoch1(sampleEpochHash1_);
  EpochKey epoch2(sampleEpochHash2_);

  NegKey key1(epoch1, sampleShapeHash_, sampleParamsHash_);
  NegKey key2(epoch2, sampleShapeHash_, sampleParamsHash_);

  EXPECT_NE(key1, key2);
  EXPECT_NE(absl::Hash<NegKey>{}(key1), absl::Hash<NegKey>{}(key2));
}

TEST_F(ReadCacheKeysTest, NegKeyHashMapUsage) {
  absl::flat_hash_map<NegKey, bool> negativeCache;

  EpochKey epochKey(sampleEpochHash1_);
  NegKey key1(epochKey, sampleShapeHash_, sampleParamsHash_);
  NegKey key2(epochKey, "different_shape_hash", sampleParamsHash_);

  negativeCache[key1] = true;
  negativeCache[key2] = true;

  EXPECT_TRUE(negativeCache[key1]);
  EXPECT_TRUE(negativeCache[key2]);
  EXPECT_EQ(negativeCache.size(), 2);
}

// _____________________________________________________________________________
// Cross-type comparison tests
// _____________________________________________________________________________

TEST_F(ReadCacheKeysTest, AllKeyTypesIncludeEpoch) {
  // Verify that all key types include epoch in their hash computation
  // by checking that different epochs produce different hashes
  EpochKey epoch1(sampleEpochHash1_);
  EpochKey epoch2(sampleEpochHash2_);

  // PlanKey
  PlanKey plan1(epoch1, sampleShapeHash_);
  PlanKey plan2(epoch2, sampleShapeHash_);
  EXPECT_NE(absl::Hash<PlanKey>{}(plan1), absl::Hash<PlanKey>{}(plan2));

  // BytesKey
  BytesKey bytes1(epoch1, sampleShapeHash_, sampleParamsHash_,
                  ad_utility::MediaType::json, 0);
  BytesKey bytes2(epoch2, sampleShapeHash_, sampleParamsHash_,
                  ad_utility::MediaType::json, 0);
  EXPECT_NE(absl::Hash<BytesKey>{}(bytes1), absl::Hash<BytesKey>{}(bytes2));

  // NegKey
  NegKey neg1(epoch1, sampleShapeHash_, sampleParamsHash_);
  NegKey neg2(epoch2, sampleShapeHash_, sampleParamsHash_);
  EXPECT_NE(absl::Hash<NegKey>{}(neg1), absl::Hash<NegKey>{}(neg2));
}
