// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant (claude@anthropic.com)

#include <benchmark/benchmark.h>

#include <chrono>
#include <random>

#include "parser/ShEx.h"
#include "shex/ParallelValidator.h"
#include "shex/StreamingValidator.h"
#include "shex/ValidationCache.h"
#include "util/AllocatorWithLimit.h"

namespace {

// ============================================================================
// Test Data Generators
// ============================================================================

shex::ShExSchema createTestSchema(size_t numShapes = 10) {
  shex::ShExSchema schema;

  for (size_t i = 0; i < numShapes; ++i) {
    std::string shapeId = "Shape" + std::to_string(i);
    shex::Shape shape(shapeId);

    // Add 3-5 properties per shape
    for (size_t j = 0; j < 3 + (i % 3); ++j) {
      std::string predicate = "pred" + std::to_string(j);
      shex::PropertyShape prop(predicate);

      // Alternate cardinalities
      if (j % 4 == 0) {
        prop.cardinality = shex::Cardinality::EXACTLY_ONE;
      } else if (j % 4 == 1) {
        prop.cardinality = shex::Cardinality::ZERO_OR_ONE;
      } else if (j % 4 == 2) {
        prop.cardinality = shex::Cardinality::ONE_OR_MORE;
      } else {
        prop.cardinality = shex::Cardinality::ZERO_OR_MORE;
      }

      shape.addProperty(prop);
    }

    schema.addShape(shape);
  }

  return schema;
}

std::map<std::string, std::map<std::string, std::vector<std::pair<std::string, shex::ValueType>>>>
createTestDataset(size_t numNodes, size_t numShapes = 10) {
  std::map<std::string,
           std::map<std::string, std::vector<std::pair<std::string, shex::ValueType>>>>
      dataset;

  std::mt19937 rng(42);
  std::uniform_int_distribution<size_t> shapeDist(0, numShapes - 1);
  std::uniform_int_distribution<size_t> predCountDist(1, 5);

  for (size_t i = 0; i < numNodes; ++i) {
    std::string nodeId = "node" + std::to_string(i);
    std::map<std::string, std::vector<std::pair<std::string, shex::ValueType>>>
        nodeData;

    // Add random predicates
    size_t numPreds = predCountDist(rng);
    for (size_t j = 0; j < numPreds; ++j) {
      std::string predicate = "pred" + std::to_string(j);
      std::string value = "value" + std::to_string(i) + "_" + std::to_string(j);
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

// ============================================================================
// Benchmark: Baseline Sequential Validation (No Optimizations)
// ============================================================================

static void BM_SequentialValidation_1K(benchmark::State& state) {
  auto schema = createTestSchema(10);
  auto dataset = createTestDataset(1000, 10);
  auto mapping = createNodeToShapeMapping(1000, 10);

  shex::ShExValidator validator(schema);

  for (auto _ : state) {
    auto report = validator.validateDataset(dataset, mapping);
    benchmark::DoNotOptimize(report);
  }

  state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_SequentialValidation_1K);

static void BM_SequentialValidation_10K(benchmark::State& state) {
  auto schema = createTestSchema(20);
  auto dataset = createTestDataset(10000, 20);
  auto mapping = createNodeToShapeMapping(10000, 20);

  shex::ShExValidator validator(schema);

  for (auto _ : state) {
    auto report = validator.validateDataset(dataset, mapping);
    benchmark::DoNotOptimize(report);
  }

  state.SetItemsProcessed(state.iterations() * 10000);
}
BENCHMARK(BM_SequentialValidation_10K);

// ============================================================================
// Benchmark: Caching (Cold vs Warm)
// ============================================================================

static void BM_CachingValidation_ColdCache_1K(benchmark::State& state) {
  auto schema = createTestSchema(10);
  auto dataset = createTestDataset(1000, 10);
  auto mapping = createNodeToShapeMapping(1000, 10);

  for (auto _ : state) {
    auto cache =
        std::make_shared<shex::NodeShapeValidationCache>(
            shex::createDefaultValidationCache());
    shex::BatchValidator validator(schema, cache);

    std::vector<shex::ValidationTask> tasks;
    for (const auto& [nodeId, nodeData] : dataset) {
      auto shapeId = mapping.at(nodeId);
      tasks.emplace_back(nodeId, shapeId, nodeData);
    }

    auto results = validator.validateBatch(tasks);
    benchmark::DoNotOptimize(results);
  }

  state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_CachingValidation_ColdCache_1K);

static void BM_CachingValidation_WarmCache_1K(benchmark::State& state) {
  auto schema = createTestSchema(10);
  auto dataset = createTestDataset(1000, 10);
  auto mapping = createNodeToShapeMapping(1000, 10);

  auto cache = std::make_shared<shex::NodeShapeValidationCache>(
      shex::createDefaultValidationCache());
  shex::BatchValidator validator(schema, cache);

  // Warmup cache
  std::vector<shex::ValidationTask> tasks;
  for (const auto& [nodeId, nodeData] : dataset) {
    auto shapeId = mapping.at(nodeId);
    tasks.emplace_back(nodeId, shapeId, nodeData);
  }
  validator.validateBatch(tasks);

  for (auto _ : state) {
    auto results = validator.validateBatch(tasks);
    benchmark::DoNotOptimize(results);
  }

  state.SetItemsProcessed(state.iterations() * 1000);
  state.counters["CacheHitRate"] = cache->metrics().hitRate() * 100.0;
}
BENCHMARK(BM_CachingValidation_WarmCache_1K);

// ============================================================================
// Benchmark: Streaming Validation
// ============================================================================

static void BM_StreamingValidation_1K(benchmark::State& state) {
  auto schema = createTestSchema(10);
  auto dataset = createTestDataset(1000, 10);
  auto mapping = createNodeToShapeMapping(1000, 10);

  shex::StreamingValidator::Config config;
  config.batchSize = 100;
  config.validatorParallelism = 4;

  for (auto _ : state) {
    shex::StreamingValidator validator(schema, config);
    auto results =
        validator.validateDataset(dataset.begin(), dataset.end(), mapping);
    benchmark::DoNotOptimize(results);
  }

  state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_StreamingValidation_1K);

static void BM_StreamingValidation_100K(benchmark::State& state) {
  auto schema = createTestSchema(50);
  auto dataset = createTestDataset(100000, 50);
  auto mapping = createNodeToShapeMapping(100000, 50);

  shex::StreamingValidator::Config config;
  config.batchSize = 1000;
  config.validatorParallelism = 8;

  for (auto _ : state) {
    shex::StreamingValidator validator(schema, config);
    auto results =
        validator.validateDataset(dataset.begin(), dataset.end(), mapping);
    benchmark::DoNotOptimize(results);
  }

  state.SetItemsProcessed(state.iterations() * 100000);
}
BENCHMARK(BM_StreamingValidation_100K);

// ============================================================================
// Benchmark: Parallel Validation (Dataset-Level)
// ============================================================================

static void BM_ParallelValidation_1K_Workers(benchmark::State& state) {
  auto schema = createTestSchema(10);
  auto dataset = createTestDataset(1000, 10);
  auto mapping = createNodeToShapeMapping(1000, 10);

  shex::ParallelValidator::Config config;
  config.numWorkers = state.range(0);
  config.enableCaching = true;
  config.enableWorkStealing = true;

  for (auto _ : state) {
    shex::ParallelValidator validator(schema, config);
    auto results = validator.validateDatasetParallel(dataset, mapping);
    benchmark::DoNotOptimize(results);
  }

  state.SetItemsProcessed(state.iterations() * 1000);
  state.counters["Workers"] = state.range(0);
}
BENCHMARK(BM_ParallelValidation_1K_Workers)->DenseRange(1, 8, 1);

static void BM_ParallelValidation_100K_Workers(benchmark::State& state) {
  auto schema = createTestSchema(50);
  auto dataset = createTestDataset(100000, 50);
  auto mapping = createNodeToShapeMapping(100000, 50);

  shex::ParallelValidator::Config config;
  config.numWorkers = state.range(0);
  config.enableCaching = true;
  config.enableWorkStealing = true;

  for (auto _ : state) {
    shex::ParallelValidator validator(schema, config);
    auto results = validator.validateDatasetParallel(dataset, mapping);
    benchmark::DoNotOptimize(results);
  }

  state.SetItemsProcessed(state.iterations() * 100000);
  state.counters["Workers"] = state.range(0);
  state.counters["Speedup"] =
      static_cast<double>(state.iterations() * 100000) /
      state.iterations();
}
BENCHMARK(BM_ParallelValidation_100K_Workers)->DenseRange(1, 8, 1);

// ============================================================================
// Benchmark: Cache Hit Rate Analysis
// ============================================================================

static void BM_CacheHitRate_VaryingDatasetSize(benchmark::State& state) {
  auto schema = createTestSchema(20);
  size_t datasetSize = state.range(0);
  auto dataset = createTestDataset(datasetSize, 20);
  auto mapping = createNodeToShapeMapping(datasetSize, 20);

  auto cache = std::make_shared<shex::NodeShapeValidationCache>(
      shex::createHighPerformanceCache());
  shex::BatchValidator validator(schema, cache);

  std::vector<shex::ValidationTask> tasks;
  for (const auto& [nodeId, nodeData] : dataset) {
    auto shapeId = mapping.at(nodeId);
    tasks.emplace_back(nodeId, shapeId, nodeData);
  }

  // Warmup
  validator.validateBatch(tasks);

  for (auto _ : state) {
    auto results = validator.validateBatch(tasks);
    benchmark::DoNotOptimize(results);
  }

  state.SetItemsProcessed(state.iterations() * datasetSize);
  state.counters["DatasetSize"] = datasetSize;
  state.counters["CacheHitRate"] = cache->metrics().hitRate() * 100.0;
  state.counters["L1HitRate"] = cache->metrics().l1HitRate() * 100.0;
}
BENCHMARK(BM_CacheHitRate_VaryingDatasetSize)
    ->Arg(100)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000);

// ============================================================================
// Benchmark: Memory Pressure Handling
// ============================================================================

static void BM_MemoryLimitedValidation_1K(benchmark::State& state) {
  auto schema = createTestSchema(10);
  auto dataset = createTestDataset(1000, 10);
  auto mapping = createNodeToShapeMapping(1000, 10);

  shex::ParallelValidator::Config config;
  config.numWorkers = 4;
  config.memoryLimit = ad_utility::MemorySize::megabytes(50);

  for (auto _ : state) {
    shex::ParallelValidator validator(schema, config);
    auto results = validator.validateDatasetParallel(dataset, mapping);
    benchmark::DoNotOptimize(results);
  }

  state.SetItemsProcessed(state.iterations() * 1000);
}
BENCHMARK(BM_MemoryLimitedValidation_1K);

// ============================================================================
// Benchmark: Scalability Analysis (1K to 1M nodes)
// ============================================================================

static void BM_Scalability_VaryingSize(benchmark::State& state) {
  size_t numNodes = state.range(0);
  size_t numShapes = std::min(numNodes / 100, size_t(100));

  auto schema = createTestSchema(numShapes);
  auto dataset = createTestDataset(numNodes, numShapes);
  auto mapping = createNodeToShapeMapping(numNodes, numShapes);

  shex::ParallelValidator::Config config;
  config.numWorkers = 8;
  config.enableCaching = true;

  for (auto _ : state) {
    shex::ParallelValidator validator(schema, config);
    auto results = validator.validateDatasetParallel(dataset, mapping);
    benchmark::DoNotOptimize(results);
  }

  state.SetItemsProcessed(state.iterations() * numNodes);
  state.counters["Nodes"] = numNodes;
  state.counters["ThroughputNodesPerSec"] =
      benchmark::Counter(state.iterations() * numNodes,
                         benchmark::Counter::kIsRate);
}
BENCHMARK(BM_Scalability_VaryingSize)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

// ============================================================================
// Benchmark: Cache Effectiveness (Multiple Runs)
// ============================================================================

static void BM_CacheEffectiveness_MultipleRuns(benchmark::State& state) {
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

  size_t runCount = 0;
  for (auto _ : state) {
    auto results = validator.validateBatch(tasks);
    benchmark::DoNotOptimize(results);
    ++runCount;

    if (runCount % 10 == 0) {
      state.counters["HitRate_Run" + std::to_string(runCount)] =
          cache->metrics().hitRate() * 100.0;
    }
  }

  state.SetItemsProcessed(state.iterations() * 1000);
  state.counters["FinalHitRate"] = cache->metrics().hitRate() * 100.0;
}
BENCHMARK(BM_CacheEffectiveness_MultipleRuns)->Iterations(100);

}  // namespace

BENCHMARK_MAIN();
