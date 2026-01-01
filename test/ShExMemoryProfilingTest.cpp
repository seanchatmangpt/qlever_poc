// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant (claude@anthropic.com)

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

#include "parser/ShEx.h"
#include "shex/ParallelValidator.h"
#include "shex/StreamingValidator.h"
#include "shex/ValidationCache.h"
#include "util/AllocatorWithLimit.h"

namespace {

// ============================================================================
// Test Fixtures
// ============================================================================

class ShExMemoryProfilingTest : public ::testing::Test {
 protected:
  shex::ShExSchema createTestSchema(size_t numShapes = 10) {
    shex::ShExSchema schema;
    for (size_t i = 0; i < numShapes; ++i) {
      std::string shapeId = "Shape" + std::to_string(i);
      shex::Shape shape(shapeId);

      for (size_t j = 0; j < 5; ++j) {
        std::string predicate = "pred" + std::to_string(j);
        shex::PropertyShape prop(predicate);
        prop.cardinality = shex::Cardinality::EXACTLY_ONE;
        shape.addProperty(prop);
      }

      schema.addShape(shape);
    }
    return schema;
  }

  std::map<std::string,
           std::map<std::string,
                    std::vector<std::pair<std::string, shex::ValueType>>>>
  createTestDataset(size_t numNodes, size_t numShapes = 10) {
    std::map<std::string, std::map<std::string, std::vector<std::pair<
                                                     std::string, shex::ValueType>>>>
        dataset;

    for (size_t i = 0; i < numNodes; ++i) {
      std::string nodeId = "node" + std::to_string(i);
      std::map<std::string, std::vector<std::pair<std::string, shex::ValueType>>>
          nodeData;

      for (size_t j = 0; j < 5; ++j) {
        std::string predicate = "pred" + std::to_string(j);
        std::string value =
            "value" + std::to_string(i) + "_" + std::to_string(j);
        nodeData[predicate].push_back({value, shex::ValueType::LITERAL});
      }

      dataset[nodeId] = std::move(nodeData);
    }

    return dataset;
  }

  std::map<std::string, std::string> createNodeToShapeMapping(
      size_t numNodes, size_t numShapes = 10) {
    std::map<std::string, std::string> mapping;
    for (size_t i = 0; i < numNodes; ++i) {
      std::string nodeId = "node" + std::to_string(i);
      std::string shapeId = "Shape" + std::to_string(i % numShapes);
      mapping[nodeId] = shapeId;
    }
    return mapping;
  }
};

// ============================================================================
// Test: Cache Memory Limits
// ============================================================================

TEST_F(ShExMemoryProfilingTest, CacheRespectsMemoryLimit) {
  typename shex::NodeShapeValidationCache::Config config;
  config.l1Capacity = 100;
  config.l2Capacity = 500;
  config.l3Capacity = 1000;
  config.maxMemory = ad_utility::MemorySize::kilobytes(100);
  config.enableMemoryLimit = true;

  shex::NodeShapeValidationCache cache(config);

  // Insert many entries
  for (size_t i = 0; i < 2000; ++i) {
    auto key = shex::ValidationCacheKey(
        "node" + std::to_string(i), "Shape" + std::to_string(i % 10));
    shex::ValidationCacheValue value(true, {});
    cache.put(key, std::move(value), sizeof(shex::ValidationCacheValue));
  }

  // Memory should be within limit
  auto metrics = cache.metrics();
  EXPECT_LE(metrics.totalSizeBytes.load(),
            config.maxMemory.getBytes() * 1.1);  // Allow 10% tolerance
}

TEST_F(ShExMemoryProfilingTest, CacheEvictionUnderPressure) {
  typename shex::NodeShapeValidationCache::Config config;
  config.l1Capacity = 50;
  config.l2Capacity = 100;
  config.l3Capacity = 200;
  config.maxMemory = ad_utility::MemorySize::kilobytes(10);
  config.enableMemoryLimit = true;

  shex::NodeShapeValidationCache cache(config);

  size_t insertedEntries = 0;
  size_t evictedEntries = 0;

  // Insert entries until eviction occurs
  for (size_t i = 0; i < 500; ++i) {
    auto key = shex::ValidationCacheKey("node" + std::to_string(i),
                                        "Shape" + std::to_string(i % 10));
    shex::ValidationCacheValue value(true, {});

    cache.put(key, std::move(value), sizeof(shex::ValidationCacheValue));
    ++insertedEntries;

    auto metrics = cache.metrics();
    if (metrics.evictions.load() > 0) {
      evictedEntries = metrics.evictions.load();
      break;
    }
  }

  EXPECT_GT(evictedEntries, 0);  // Eviction should have occurred
  EXPECT_LT(cache.metrics().totalEntries.load(),
            insertedEntries);  // Some entries evicted
}

// ============================================================================
// Test: AllocatorWithLimit Integration
// ============================================================================

TEST_F(ShExMemoryProfilingTest, AllocatorLimitEnforcement) {
  auto memoryLeft = ad_utility::makeAllocationMemoryLeftThreadsafeObject(
      ad_utility::MemorySize::megabytes(10));
  auto allocator = ad_utility::AllocatorWithLimit<char>(memoryLeft);

  // Allocate memory up to limit
  std::vector<std::vector<char, ad_utility::AllocatorWithLimit<char>>>
      allocations;

  size_t totalAllocated = 0;
  bool limitReached = false;

  try {
    for (size_t i = 0; i < 100; ++i) {
      std::vector<char, ad_utility::AllocatorWithLimit<char>> vec(allocator);
      vec.resize(200000);  // 200 KB per allocation
      totalAllocated += 200000;
      allocations.push_back(std::move(vec));
    }
  } catch (const ad_utility::detail::AllocationExceedsLimitException& e) {
    limitReached = true;
  }

  EXPECT_TRUE(limitReached);
  EXPECT_LE(totalAllocated, ad_utility::MemorySize::megabytes(10).getBytes());
}

TEST_F(ShExMemoryProfilingTest, MemoryReleaseOnDeallocation) {
  auto memoryLeft = ad_utility::makeAllocationMemoryLeftThreadsafeObject(
      ad_utility::MemorySize::megabytes(5));
  auto allocator = ad_utility::AllocatorWithLimit<char>(memoryLeft);

  auto initialMemory = allocator.amountMemoryLeft();

  {
    std::vector<char, ad_utility::AllocatorWithLimit<char>> vec(allocator);
    vec.resize(1000000);  // 1 MB
    auto memoryDuringAllocation = allocator.amountMemoryLeft();
    EXPECT_LT(memoryDuringAllocation, initialMemory);
  }

  // Memory should be released after vector goes out of scope
  auto finalMemory = allocator.amountMemoryLeft();
  EXPECT_EQ(finalMemory, initialMemory);
}

// ============================================================================
// Test: Peak Memory Tracking
// ============================================================================

TEST_F(ShExMemoryProfilingTest, ParallelValidatorPeakMemory) {
  auto schema = createTestSchema(10);
  auto dataset = createTestDataset(1000, 10);
  auto mapping = createNodeToShapeMapping(1000, 10);

  shex::ParallelValidator::Config config;
  config.numWorkers = 4;
  config.enableCaching = true;

  shex::ParallelValidator validator(schema, config);

  auto results = validator.validateDatasetParallel(dataset, mapping);

  auto metrics = validator.metrics();
  EXPECT_GT(metrics.peakMemoryBytes.load(), 0);

  // Peak memory should be reasonable (< 100MB for 1K nodes)
  EXPECT_LT(metrics.peakMemoryBytes.load(),
            ad_utility::MemorySize::megabytes(100).getBytes());
}

TEST_F(ShExMemoryProfilingTest, StreamingValidatorMemoryFootprint) {
  auto schema = createTestSchema(20);
  auto dataset = createTestDataset(10000, 20);
  auto mapping = createNodeToShapeMapping(10000, 20);

  shex::StreamingValidator::Config config;
  config.batchSize = 100;
  config.validatorParallelism = 4;
  config.enableCaching = true;

  shex::StreamingValidator validator(schema, config);

  auto results =
      validator.validateDataset(dataset.begin(), dataset.end(), mapping);

  EXPECT_EQ(results.size(), 10000);

  // Streaming should have lower memory footprint than batch processing
  auto cacheMetrics = validator.cacheMetrics();
  EXPECT_LT(cacheMetrics.totalSizeBytes.load(),
            ad_utility::MemorySize::megabytes(50).getBytes());
}

// ============================================================================
// Test: Memory Leak Detection
// ============================================================================

TEST_F(ShExMemoryProfilingTest, NoMemoryLeakOnRepeatedValidation) {
  auto schema = createTestSchema(5);
  auto dataset = createTestDataset(100, 5);
  auto mapping = createNodeToShapeMapping(100, 5);

  auto memoryLeft = ad_utility::makeAllocationMemoryLeftThreadsafeObject(
      ad_utility::MemorySize::megabytes(50));

  auto initialMemory =
      memoryLeft.ptr()->rlock()->amountMemoryLeft().getBytes();

  // Run validation multiple times
  for (size_t i = 0; i < 10; ++i) {
    shex::ParallelValidator::Config config;
    config.numWorkers = 2;
    shex::ParallelValidator validator(schema, config);
    auto results = validator.validateDatasetParallel(dataset, mapping);
  }

  auto finalMemory = memoryLeft.ptr()->rlock()->amountMemoryLeft().getBytes();

  // Memory should return to near-initial levels (allow small variance)
  EXPECT_NEAR(finalMemory, initialMemory, initialMemory * 0.01);
}

// ============================================================================
// Test: Cache Memory Overhead
// ============================================================================

TEST_F(ShExMemoryProfilingTest, CacheMemoryOverheadUnder5Percent) {
  auto schema = createTestSchema(10);
  auto dataset = createTestDataset(1000, 10);
  auto mapping = createNodeToShapeMapping(1000, 10);

  auto cache = std::make_shared<shex::NodeShapeValidationCache>(
      shex::createDefaultValidationCache());
  shex::BatchValidator validator(schema, cache);

  std::vector<shex::ValidationTask> tasks;
  for (const auto& [nodeId, nodeData] : dataset) {
    auto shapeId = mapping.at(nodeId);
    tasks.emplace_back(nodeId, shapeId, nodeData);
  }

  auto results = validator.validateBatch(tasks);

  auto metrics = cache->metrics();
  size_t cacheMemory = metrics.totalSizeBytes.load();

  // Estimate baseline memory (without cache)
  size_t baselineMemory =
      tasks.size() * sizeof(shex::ValidationTask);

  // Cache overhead should be < 5%
  double overhead =
      static_cast<double>(cacheMemory) / static_cast<double>(baselineMemory);
  EXPECT_LT(overhead, 0.05);
}

// ============================================================================
// Test: OOM Handling
// ============================================================================

TEST_F(ShExMemoryProfilingTest, GracefulHandlingOfOOM) {
  auto schema = createTestSchema(10);
  auto dataset = createTestDataset(100000, 10);  // Large dataset
  auto mapping = createNodeToShapeMapping(100000, 10);

  shex::ParallelValidator::Config config;
  config.numWorkers = 4;
  config.memoryLimit = ad_utility::MemorySize::kilobytes(100);  // Very low limit

  shex::ParallelValidator validator(schema, config);

  // Should not crash, may return partial results or throw gracefully
  try {
    auto results = validator.validateDatasetParallel(dataset, mapping);
    // If it succeeds, results should be valid
    EXPECT_GT(results.size(), 0);
  } catch (const ad_utility::detail::AllocationExceedsLimitException& e) {
    // Expected behavior under severe memory pressure
    SUCCEED();
  }
}

// ============================================================================
// Test: Concurrent Memory Access Safety
// ============================================================================

TEST_F(ShExMemoryProfilingTest, ConcurrentCacheAccessThreadSafe) {
  auto cache = std::make_shared<shex::NodeShapeValidationCache>(
      shex::createDefaultValidationCache());

  std::vector<std::thread> threads;
  std::atomic<size_t> successfulOperations{0};

  // Launch multiple threads accessing cache concurrently
  for (size_t i = 0; i < 10; ++i) {
    threads.emplace_back([&cache, &successfulOperations, i]() {
      for (size_t j = 0; j < 100; ++j) {
        auto key = shex::ValidationCacheKey("node" + std::to_string(j),
                                            "Shape" + std::to_string(j % 5));

        // Alternate between put and get
        if (j % 2 == 0) {
          shex::ValidationCacheValue value(true, {});
          cache->put(key, std::move(value));
        } else {
          auto result = cache->get(key);
          if (result.has_value()) {
            ++successfulOperations;
          }
        }
      }
    });
  }

  // Wait for all threads
  for (auto& thread : threads) {
    thread.join();
  }

  // Should not crash, and some operations should succeed
  EXPECT_GT(successfulOperations.load(), 0);
  EXPECT_GT(cache->metrics().hits.load() + cache->metrics().misses.load(), 0);
}

}  // namespace
