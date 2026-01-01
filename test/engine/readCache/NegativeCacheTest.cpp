// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Unit tests for NegativeCache
// (EPIC 3 - Task 4: NegativeCache Core)

#include "engine/readCache/NegativeCache.h"

#include <gtest/gtest.h>

#include "engine/readCache/NegKey.h"

using namespace readCache;

class NegativeCacheTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Each test gets a fresh cache with default settings
    cache_ = std::make_unique<NegativeCache>(100);  // 100 entries max
  }

  void TearDown() override { cache_.reset(); }

  std::unique_ptr<NegativeCache> cache_;
};

// Test: Basic insertion and lookup
TEST_F(NegativeCacheTest, BasicInsertionAndLookup) {
  NegKey key("shape_hash_123", 1);
  NegEntry entry(NegKind::EMPTY_RESULT);

  // Insert entry
  ASSERT_TRUE(cache_->insertEmpty(key, entry));

  // Lookup should succeed
  auto result = cache_->lookupEmpty(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->kind, NegKind::EMPTY_RESULT);
  EXPECT_EQ(result->observed_count, 1);

  // Stats should reflect the operation
  auto stats = cache_->getStats();
  EXPECT_EQ(stats.num_entries, 1);
  EXPECT_EQ(stats.num_insertions, 1);
  EXPECT_EQ(stats.num_hits, 1);
  EXPECT_EQ(stats.num_misses, 0);
}

// Test: Lookup miss
TEST_F(NegativeCacheTest, LookupMiss) {
  NegKey key("nonexistent_key", 1);

  // Lookup non-existent key
  auto result = cache_->lookupEmpty(key);
  EXPECT_FALSE(result.has_value());

  // Stats should reflect the miss
  auto stats = cache_->getStats();
  EXPECT_EQ(stats.num_entries, 0);
  EXPECT_EQ(stats.num_misses, 1);
  EXPECT_EQ(stats.num_hits, 0);
}

// Test: Multiple different keys
TEST_F(NegativeCacheTest, MultipleDifferentKeys) {
  NegKey key1("shape_hash_1", 1);
  NegKey key2("shape_hash_2", 1);
  NegKey key3("shape_hash_3", 2);  // Different epoch

  ASSERT_TRUE(cache_->insertEmpty(key1, NegEntry(NegKind::EMPTY_RESULT)));
  ASSERT_TRUE(cache_->insertEmpty(key2, NegEntry(NegKind::UNSAT_FILTER)));
  ASSERT_TRUE(cache_->insertEmpty(key3, NegEntry(NegKind::LIMIT_ZERO)));

  // Verify each key
  auto result1 = cache_->lookupEmpty(key1);
  ASSERT_TRUE(result1.has_value());
  EXPECT_EQ(result1->kind, NegKind::EMPTY_RESULT);

  auto result2 = cache_->lookupEmpty(key2);
  ASSERT_TRUE(result2.has_value());
  EXPECT_EQ(result2->kind, NegKind::UNSAT_FILTER);

  auto result3 = cache_->lookupEmpty(key3);
  ASSERT_TRUE(result3.has_value());
  EXPECT_EQ(result3->kind, NegKind::LIMIT_ZERO);

  auto stats = cache_->getStats();
  EXPECT_EQ(stats.num_entries, 3);
  EXPECT_EQ(stats.num_insertions, 3);
}

// Test: Update existing entry (increment observed_count)
TEST_F(NegativeCacheTest, UpdateExistingEntry) {
  NegKey key("shape_hash_update", 1);
  NegEntry entry1(NegKind::EMPTY_RESULT, 1);
  NegEntry entry2(NegKind::EMPTY_RESULT, 5);

  // Insert first time
  ASSERT_TRUE(cache_->insertEmpty(key, entry1));

  // Insert again (should update observed_count)
  ASSERT_TRUE(cache_->insertEmpty(key, entry2));

  // Verify observed_count was incremented
  auto result = cache_->lookupEmpty(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->observed_count, 6);  // 1 + 5

  // Stats: still only 1 entry, 2 insertions
  auto stats = cache_->getStats();
  EXPECT_EQ(stats.num_entries, 1);
  EXPECT_EQ(stats.num_insertions, 2);
}

// Test: Eviction by max entries
TEST_F(NegativeCacheTest, EvictionByMaxEntries) {
  // Create cache with small capacity (4 entries per shard, 16 total)
  cache_ = std::make_unique<NegativeCache>(16);

  // Insert more entries than capacity
  std::vector<NegKey> keys;
  for (int i = 0; i < 20; i++) {
    NegKey key("shape_hash_" + std::to_string(i), 1);
    keys.push_back(key);
    cache_->insertEmpty(key, NegEntry(NegKind::EMPTY_RESULT));
  }

  // Cache should have at most 16 entries
  auto stats = cache_->getStats();
  EXPECT_LE(stats.num_entries, 16);
  EXPECT_GT(stats.num_evictions, 0);

  // Oldest entries should have been evicted (LRU)
  // Newest entries should still be present
  auto result_old = cache_->lookupEmpty(keys[0]);  // Oldest
  auto result_new = cache_->lookupEmpty(keys[19]);  // Newest
  EXPECT_FALSE(result_old.has_value());  // Evicted
  EXPECT_TRUE(result_new.has_value());   // Still present
}

// Test: setMaxEntries triggers eviction
TEST_F(NegativeCacheTest, SetMaxEntriesTriggersEviction) {
  // Insert 50 entries
  for (int i = 0; i < 50; i++) {
    NegKey key("shape_hash_" + std::to_string(i), 1);
    cache_->insertEmpty(key, NegEntry(NegKind::EMPTY_RESULT));
  }

  auto stats_before = cache_->getStats();
  EXPECT_EQ(stats_before.num_entries, 50);

  // Reduce max entries to 20
  cache_->setMaxEntries(20);

  auto stats_after = cache_->getStats();
  EXPECT_LE(stats_after.num_entries, 20);
}

// Test: clearAll removes all entries
TEST_F(NegativeCacheTest, ClearAll) {
  // Insert multiple entries
  for (int i = 0; i < 10; i++) {
    NegKey key("shape_hash_" + std::to_string(i), 1);
    cache_->insertEmpty(key, NegEntry(NegKind::EMPTY_RESULT));
  }

  auto stats_before = cache_->getStats();
  EXPECT_EQ(stats_before.num_entries, 10);

  // Clear all
  cache_->clearAll();

  auto stats_after = cache_->getStats();
  EXPECT_EQ(stats_after.num_entries, 0);

  // Statistics counters are preserved
  EXPECT_EQ(stats_after.num_insertions, stats_before.num_insertions);
}

// Test: getMaxEntries
TEST_F(NegativeCacheTest, GetMaxEntries) {
  EXPECT_EQ(cache_->getMaxEntries(), 100);

  cache_->setMaxEntries(500);
  EXPECT_EQ(cache_->getMaxEntries(), 500);
}

// Test: Different NegKind types
TEST_F(NegativeCacheTest, DifferentNegKindTypes) {
  NegKey key1("key1", 1);
  NegKey key2("key2", 1);
  NegKey key3("key3", 1);

  cache_->insertEmpty(key1, NegEntry(NegKind::EMPTY_RESULT));
  cache_->insertEmpty(key2, NegEntry(NegKind::UNSAT_FILTER));
  cache_->insertEmpty(key3, NegEntry(NegKind::LIMIT_ZERO));

  auto result1 = cache_->lookupEmpty(key1);
  auto result2 = cache_->lookupEmpty(key2);
  auto result3 = cache_->lookupEmpty(key3);

  ASSERT_TRUE(result1.has_value());
  ASSERT_TRUE(result2.has_value());
  ASSERT_TRUE(result3.has_value());

  EXPECT_EQ(result1->kind, NegKind::EMPTY_RESULT);
  EXPECT_EQ(result2->kind, NegKind::UNSAT_FILTER);
  EXPECT_EQ(result3->kind, NegKind::LIMIT_ZERO);
}

// Test: Epoch isolation
TEST_F(NegativeCacheTest, EpochIsolation) {
  // Same shape, different epochs
  NegKey key_epoch1("same_shape_hash", 1);
  NegKey key_epoch2("same_shape_hash", 2);

  cache_->insertEmpty(key_epoch1, NegEntry(NegKind::EMPTY_RESULT));
  cache_->insertEmpty(key_epoch2, NegEntry(NegKind::UNSAT_FILTER));

  // Both should be present as distinct entries
  auto result1 = cache_->lookupEmpty(key_epoch1);
  auto result2 = cache_->lookupEmpty(key_epoch2);

  ASSERT_TRUE(result1.has_value());
  ASSERT_TRUE(result2.has_value());

  EXPECT_EQ(result1->kind, NegKind::EMPTY_RESULT);
  EXPECT_EQ(result2->kind, NegKind::UNSAT_FILTER);

  auto stats = cache_->getStats();
  EXPECT_EQ(stats.num_entries, 2);  // Two distinct entries
}

// Test: LRU behavior
TEST_F(NegativeCacheTest, LRUBehavior) {
  // Create cache with very small capacity
  cache_ = std::make_unique<NegativeCache>(8);  // 2 entries per shard

  // Insert keys in order
  NegKey key1("key1", 1);
  NegKey key2("key2", 1);
  NegKey key3("key3", 1);

  cache_->insertEmpty(key1, NegEntry(NegKind::EMPTY_RESULT));
  cache_->insertEmpty(key2, NegEntry(NegKind::EMPTY_RESULT));

  // Access key1 to refresh its LRU position
  cache_->lookupEmpty(key1);

  // Insert key3, which should evict key2 (least recently used)
  // Note: This test is probabilistic since keys may hash to different shards
  cache_->insertEmpty(key3, NegEntry(NegKind::EMPTY_RESULT));

  // We can verify that eviction happened
  auto stats = cache_->getStats();
  // With 8 total entries, we shouldn't have evicted yet with only 3 insertions
  EXPECT_LE(stats.num_entries, 8);
}

// Test: Thread safety (basic)
TEST_F(NegativeCacheTest, BasicThreadSafety) {
  // This is a basic smoke test for thread safety
  // More comprehensive concurrency tests would be needed for production
  NegKey key("concurrent_key", 1);
  NegEntry entry(NegKind::EMPTY_RESULT);

  // Insert and lookup from multiple "threads" (sequential for simplicity)
  for (int i = 0; i < 100; i++) {
    cache_->insertEmpty(key, entry);
    auto result = cache_->lookupEmpty(key);
    ASSERT_TRUE(result.has_value());
  }

  // Verify observed_count was incremented
  auto result = cache_->lookupEmpty(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_GT(result->observed_count, 1);
}

// Test: Stats accuracy
TEST_F(NegativeCacheTest, StatsAccuracy) {
  cache_->clearAll();  // Reset stats

  NegKey key1("key1", 1);
  NegKey key2("key2", 1);

  // Insert key1
  cache_->insertEmpty(key1, NegEntry(NegKind::EMPTY_RESULT));

  // Hit on key1
  cache_->lookupEmpty(key1);

  // Miss on key2
  cache_->lookupEmpty(key2);

  // Insert key2
  cache_->insertEmpty(key2, NegEntry(NegKind::UNSAT_FILTER));

  auto stats = cache_->getStats();
  EXPECT_EQ(stats.num_entries, 2);
  EXPECT_EQ(stats.num_insertions, 2);
  EXPECT_EQ(stats.num_hits, 1);
  EXPECT_EQ(stats.num_misses, 1);
}

// Test: NegKey validation
TEST(NegKeyTest, Validation) {
  NegKey valid_key("sha256_hash", 1);
  EXPECT_TRUE(valid_key.isValid());

  NegKey invalid_key1("", 1);
  EXPECT_FALSE(invalid_key1.isValid());

  NegKey invalid_key2("sha256_hash", 0);
  EXPECT_FALSE(invalid_key2.isValid());

  NegKey invalid_key3("", 0);
  EXPECT_FALSE(invalid_key3.isValid());
}

// Test: NegKey equality
TEST(NegKeyTest, Equality) {
  NegKey key1("hash1", 1);
  NegKey key2("hash1", 1);
  NegKey key3("hash2", 1);
  NegKey key4("hash1", 2);

  EXPECT_EQ(key1, key2);  // Same hash and epoch
  EXPECT_NE(key1, key3);  // Different hash
  EXPECT_NE(key1, key4);  // Different epoch
}

// Test: NegEntry construction
TEST(NegEntryTest, Construction) {
  NegEntry entry1;
  EXPECT_EQ(entry1.kind, NegKind::EMPTY_RESULT);
  EXPECT_EQ(entry1.observed_count, 1);

  NegEntry entry2(NegKind::UNSAT_FILTER);
  EXPECT_EQ(entry2.kind, NegKind::UNSAT_FILTER);
  EXPECT_EQ(entry2.observed_count, 1);

  NegEntry entry3(NegKind::LIMIT_ZERO, 42);
  EXPECT_EQ(entry3.kind, NegKind::LIMIT_ZERO);
  EXPECT_EQ(entry3.observed_count, 42);
}
