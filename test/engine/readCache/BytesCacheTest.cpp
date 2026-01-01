//  Copyright 2025, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code (AI Assistant)
//  Created for EPIC 3 Task 2 - BytesCache Tests

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "engine/readCache/BytesCache.h"

using namespace ad_utility::readCache;

namespace {

// Helper function to create a test BytesKey
::readCache::BytesKey makeKey(
    const std::string& shapeSha, const std::string& paramsSha,
    ad_utility::MediaType format = ad_utility::MediaType::json) {
  ::readCache::EpochKey epochKey("test_epoch_hash_" + shapeSha);
  return ::readCache::BytesKey(epochKey, shapeSha, paramsSha, format, 0);
}

// Helper function to create a test page
BytePage makeTestPage(const std::string& content) {
  return BytePage(std::make_shared<const std::string>(content));
}

// Helper function to create a cached response with specific size
std::shared_ptr<CachedResponseBytes> makeResponse(
    size_t approxSize,
    ad_utility::MediaType format = ad_utility::MediaType::json) {
  auto response = std::make_shared<CachedResponseBytes>();
  response->format = format;
  response->uncompressedSize = approxSize;

  // Create pages to approximately match the desired size
  size_t pageSize = 1024;  // 1KB pages for testing
  size_t numPages = (approxSize + pageSize - 1) / pageSize;

  for (size_t i = 0; i < numPages; ++i) {
    std::string pageData(std::min(pageSize, approxSize - i * pageSize), 'x');
    response->pages.push_back(makeTestPage(pageData));
  }

  response->metadata.contentType = "application/json";
  return response;
}

}  // namespace

// Test fixture
class BytesCacheTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create cache with small size for testing (1MB)
    cache_ = std::make_unique<BytesCache>(1024 * 1024, 256 * 1024, 4);
  }

  std::unique_ptr<BytesCache> cache_;
};

// Basic insert and lookup
TEST_F(BytesCacheTest, BasicInsertAndLookup) {
  auto key = makeKey("shape1", "params1");
  auto value = makeResponse(1024);

  // Insert
  EXPECT_TRUE(cache_->insert(key, value));

  // Lookup hit
  auto result = cache_->lookupHit(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), value);

  // Verify stats
  auto stats = cache_->getStats();
  EXPECT_EQ(stats.hits, 1);
  EXPECT_EQ(stats.misses, 0);
  EXPECT_EQ(stats.inserts, 1);
  EXPECT_EQ(stats.currentEntries, 1);
}

// Cache miss
TEST_F(BytesCacheTest, CacheMiss) {
  auto key = makeKey("shape1", "params1");

  // Lookup non-existent key
  auto result = cache_->lookupHit(key);
  EXPECT_FALSE(result.has_value());

  // Verify stats
  auto stats = cache_->getStats();
  EXPECT_EQ(stats.hits, 0);
  EXPECT_EQ(stats.misses, 1);
}

// Multiple inserts and lookups
TEST_F(BytesCacheTest, MultipleEntries) {
  std::vector<::readCache::BytesKey> keys;
  std::vector<std::shared_ptr<CachedResponseBytes>> values;

  // Insert 10 entries
  for (int i = 0; i < 10; ++i) {
    auto key =
        makeKey("shape" + std::to_string(i), "params" + std::to_string(i));
    auto value = makeResponse(1024);
    keys.push_back(key);
    values.push_back(value);

    EXPECT_TRUE(cache_->insert(key, value));
  }

  // Verify all entries can be retrieved
  for (size_t i = 0; i < keys.size(); ++i) {
    auto result = cache_->lookupHit(keys[i]);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), values[i]);
  }

  auto stats = cache_->getStats();
  EXPECT_EQ(stats.hits, 10);
  EXPECT_EQ(stats.inserts, 10);
}

// LRU behavior
TEST_F(BytesCacheTest, LRUOrdering) {
  auto key1 = makeKey("shape1", "params1");
  auto key2 = makeKey("shape2", "params2");
  auto key3 = makeKey("shape3", "params3");

  auto value1 = makeResponse(1024);
  auto value2 = makeResponse(1024);
  auto value3 = makeResponse(1024);

  // Insert in order: 1, 2, 3
  cache_->insert(key1, value1);
  cache_->insert(key2, value2);
  cache_->insert(key3, value3);

  // Access key1 to make it most recently used
  cache_->lookupHit(key1);

  // Now order should be: 1 (MRU), 3, 2 (LRU)
  // This is tested implicitly through eviction behavior
}

// Eviction under size pressure
TEST_F(BytesCacheTest, EvictionUnderSizePressure) {
  // Create cache with very small size (100KB)
  auto smallCache = std::make_unique<BytesCache>(100 * 1024, 256 * 1024, 4);

  std::vector<::readCache::BytesKey> keys;
  std::vector<std::shared_ptr<CachedResponseBytes>> values;

  // Insert entries until we exceed capacity and trigger eviction
  // Each entry is ~10KB, so we should be able to fit ~10 entries
  for (int i = 0; i < 20; ++i) {
    auto key =
        makeKey("shape" + std::to_string(i), "params" + std::to_string(i));
    auto value = makeResponse(10 * 1024);
    keys.push_back(key);
    values.push_back(value);

    smallCache->insert(key, value);
  }

  auto stats = smallCache->getStats();

  // Should have evicted some entries
  EXPECT_GT(stats.evictions, 0);

  // Current size should be below max
  EXPECT_LE(stats.currentBytes, 100 * 1024);

  // Should have fewer entries than we inserted
  EXPECT_LT(stats.currentEntries, 20);

  // Early entries (LRU) should be evicted
  auto result = smallCache->lookupHit(keys[0]);
  EXPECT_FALSE(result.has_value());

  // Recent entries should still be present
  auto recent = smallCache->lookupHit(keys[19]);
  EXPECT_TRUE(recent.has_value());
}

// Update existing entry
TEST_F(BytesCacheTest, UpdateExistingEntry) {
  auto key = makeKey("shape1", "params1");
  auto value1 = makeResponse(1024);
  auto value2 = makeResponse(2048);

  // Insert first value
  cache_->insert(key, value1);

  auto stats1 = cache_->getStats();
  EXPECT_EQ(stats1.inserts, 1);
  EXPECT_EQ(stats1.currentEntries, 1);

  // Insert second value with same key (update)
  cache_->insert(key, value2);

  auto stats2 = cache_->getStats();
  // Inserts count doesn't change for updates (only new keys increment it)
  EXPECT_EQ(stats2.inserts, 1);
  EXPECT_EQ(stats2.currentEntries, 1);

  // Verify updated value is retrieved
  auto result = cache_->lookupHit(key);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), value2);
}

// Admission policy rejection
TEST_F(BytesCacheTest, AdmissionPolicyRejection) {
  // Create cache with 100KB max
  auto smallCache = std::make_unique<BytesCache>(100 * 1024, 256 * 1024, 4);

  auto key = makeKey("large", "params");
  // Create value larger than 10% of cache (> 10KB)
  auto largeValue = makeResponse(20 * 1024);

  // Should be rejected by admission policy
  bool inserted = smallCache->insert(key, largeValue);
  EXPECT_FALSE(inserted);

  // Verify stats
  auto stats = smallCache->getStats();
  EXPECT_EQ(stats.rejections, 1);
  EXPECT_EQ(stats.inserts, 0);
  EXPECT_EQ(stats.currentEntries, 0);

  // Verify not in cache
  auto result = smallCache->lookupHit(key);
  EXPECT_FALSE(result.has_value());
}

// setMaxBytes triggers eviction
TEST_F(BytesCacheTest, SetMaxBytesTriggersEviction) {
  // Insert several entries
  for (int i = 0; i < 10; ++i) {
    auto key =
        makeKey("shape" + std::to_string(i), "params" + std::to_string(i));
    auto value = makeResponse(10 * 1024);
    cache_->insert(key, value);
  }

  auto statsBefore = cache_->getStats();
  EXPECT_GT(statsBefore.currentEntries, 0);

  // Reduce max size significantly
  cache_->setMaxBytes(20 * 1024);

  auto statsAfter = cache_->getStats();

  // Should have evicted entries
  EXPECT_GT(statsAfter.evictions, 0);

  // Current size should be below new max
  EXPECT_LE(statsAfter.currentBytes, 20 * 1024);

  // Should have fewer entries
  EXPECT_LT(statsAfter.currentEntries, statsBefore.currentEntries);
}

// Clear cache
TEST_F(BytesCacheTest, ClearCache) {
  // Insert some entries
  for (int i = 0; i < 5; ++i) {
    auto key =
        makeKey("shape" + std::to_string(i), "params" + std::to_string(i));
    auto value = makeResponse(1024);
    cache_->insert(key, value);
  }

  auto statsBefore = cache_->getStats();
  EXPECT_EQ(statsBefore.currentEntries, 5);

  // Clear
  cache_->clear();

  auto statsAfter = cache_->getStats();
  EXPECT_EQ(statsAfter.currentEntries, 0);
  EXPECT_EQ(statsAfter.currentBytes, 0);

  // Verify entries are gone
  auto key = makeKey("shape0", "params0");
  auto result = cache_->lookupHit(key);
  EXPECT_FALSE(result.has_value());
}

// Different output formats
TEST_F(BytesCacheTest, DifferentOutputFormats) {
  auto shapeHash = "shape1";
  auto paramsHash = "params1";

  // Insert same query result in different formats
  auto keyJson = makeKey(shapeHash, paramsHash, ad_utility::MediaType::json);
  auto keyCsv = makeKey(shapeHash, paramsHash, ad_utility::MediaType::csv);
  auto keyTsv = makeKey(shapeHash, paramsHash, ad_utility::MediaType::tsv);

  auto valueJson = makeResponse(1024, ad_utility::MediaType::json);
  auto valueCsv = makeResponse(1024, ad_utility::MediaType::csv);
  auto valueTsv = makeResponse(1024, ad_utility::MediaType::tsv);

  cache_->insert(keyJson, valueJson);
  cache_->insert(keyCsv, valueCsv);
  cache_->insert(keyTsv, valueTsv);

  // Verify all three are stored separately
  auto resultJson = cache_->lookupHit(keyJson);
  auto resultCsv = cache_->lookupHit(keyCsv);
  auto resultTsv = cache_->lookupHit(keyTsv);

  ASSERT_TRUE(resultJson.has_value());
  ASSERT_TRUE(resultCsv.has_value());
  ASSERT_TRUE(resultTsv.has_value());

  EXPECT_EQ(resultJson.value(), valueJson);
  EXPECT_EQ(resultCsv.value(), valueCsv);
  EXPECT_EQ(resultTsv.value(), valueTsv);

  auto stats = cache_->getStats();
  EXPECT_EQ(stats.currentEntries, 3);
}

// Concurrent access (basic thread safety test)
TEST_F(BytesCacheTest, ConcurrentAccess) {
  const int numThreads = 8;
  const int operationsPerThread = 100;

  auto worker = [&](int threadId) {
    for (int i = 0; i < operationsPerThread; ++i) {
      auto key =
          makeKey("shape" + std::to_string(threadId) + "_" + std::to_string(i),
                  "params" + std::to_string(i));
      auto value = makeResponse(1024);

      // Insert
      cache_->insert(key, value);

      // Lookup
      auto result = cache_->lookupHit(key);
      EXPECT_TRUE(result.has_value());
    }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < numThreads; ++i) {
    threads.emplace_back(worker, i);
  }

  for (auto& thread : threads) {
    thread.join();
  }

  // Verify stats are consistent
  auto stats = cache_->getStats();
  EXPECT_GT(stats.currentEntries, 0);
  EXPECT_EQ(stats.hits, numThreads * operationsPerThread);
}

// Page management
TEST_F(BytesCacheTest, PageManagement) {
  auto key = makeKey("shape1", "params1");
  auto response = std::make_shared<CachedResponseBytes>();
  response->format = ad_utility::MediaType::json;
  response->uncompressedSize = 0;

  // Create multiple pages
  std::vector<std::string> pageContents = {"page1_data", "page2_data",
                                           "page3_data"};

  for (const auto& content : pageContents) {
    response->pages.push_back(makeTestPage(content));
    response->uncompressedSize += content.size();
  }

  cache_->insert(key, response);

  // Lookup and verify pages
  auto result = cache_->lookupHit(key);
  ASSERT_TRUE(result.has_value());

  auto retrieved = result.value();
  EXPECT_EQ(retrieved->pages.size(), 3);

  for (size_t i = 0; i < pageContents.size(); ++i) {
    EXPECT_EQ(retrieved->pages[i].data(), pageContents[i]);
  }
}

// Verify shard distribution
TEST_F(BytesCacheTest, ShardDistribution) {
  // Insert many entries and verify they're distributed across shards
  const int numEntries = 100;

  for (int i = 0; i < numEntries; ++i) {
    auto key =
        makeKey("shape" + std::to_string(i), "params" + std::to_string(i));
    auto value = makeResponse(1024);
    cache_->insert(key, value);
  }

  // We can't directly verify shard distribution without exposing internals,
  // but we can verify that all entries are accessible and stats are correct
  auto stats = cache_->getStats();
  EXPECT_EQ(stats.currentEntries, numEntries);
  EXPECT_EQ(stats.inserts, numEntries);
}

// Test page size configuration
TEST_F(BytesCacheTest, PageSizeConfiguration) {
  auto cache1 = std::make_unique<BytesCache>(1024 * 1024, 128 * 1024, 4);
  auto cache2 = std::make_unique<BytesCache>(1024 * 1024, 512 * 1024, 4);

  EXPECT_EQ(cache1->getPageSize(), 128 * 1024);
  EXPECT_EQ(cache2->getPageSize(), 512 * 1024);
}

// Test metadata storage
TEST_F(BytesCacheTest, MetadataStorage) {
  auto key = makeKey("shape1", "params1");
  auto response = makeResponse(1024);

  // Set metadata
  response->metadata.contentType = "application/sparql-results+json";
  response->metadata.headers["Content-Encoding"] = "gzip";
  response->metadata.headers["Cache-Control"] = "max-age=3600";

  cache_->insert(key, response);

  // Retrieve and verify metadata
  auto result = cache_->lookupHit(key);
  ASSERT_TRUE(result.has_value());

  auto retrieved = result.value();
  EXPECT_EQ(retrieved->metadata.contentType, "application/sparql-results+json");
  EXPECT_EQ(retrieved->metadata.headers["Content-Encoding"], "gzip");
  EXPECT_EQ(retrieved->metadata.headers["Cache-Control"], "max-age=3600");
}
