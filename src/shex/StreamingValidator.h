// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant (claude@anthropic.com)

#ifndef QLEVER_SRC_SHEX_STREAMINGVALIDATOR_H
#define QLEVER_SRC_SHEX_STREAMINGVALIDATOR_H

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "parser/ShEx.h"
#include "shex/ShapeIndex.h"
#include "shex/ValidationCache.h"
#include "util/BatchedPipeline.h"
#include "util/Synchronized.h"

namespace shex {

// ============================================================================
// Validation Task - Unit of work for the streaming pipeline
// ============================================================================

struct ValidationTask {
  std::string nodeId;
  std::string shapeId;
  std::map<std::string, std::vector<std::pair<std::string, ValueType>>>
      nodeData;

  ValidationTask(std::string node, std::string shape,
                 std::map<std::string, std::vector<std::pair<std::string, ValueType>>> data)
      : nodeId(std::move(node)),
        shapeId(std::move(shape)),
        nodeData(std::move(data)) {}
};

struct ValidationResult {
  std::string nodeId;
  std::string shapeId;
  bool isValid;
  std::vector<std::string> errors;
  bool fromCache;

  ValidationResult(std::string node, std::string shape, bool valid,
                   std::vector<std::string> errs, bool cached = false)
      : nodeId(std::move(node)),
        shapeId(std::move(shape)),
        isValid(valid),
        errors(std::move(errs)),
        fromCache(cached) {}
};

// ============================================================================
// Pipeline Stage 1: Node Batcher
// Converts iterator of nodes into batches
// ============================================================================

template <typename NodeIterator>
class NodeBatcher {
 public:
  NodeBatcher(NodeIterator begin, NodeIterator end,
              const std::map<std::string, std::string>& nodeToShapeMapping)
      : current_(begin), end_(end), mapping_(nodeToShapeMapping) {}

  std::optional<ValidationTask> operator()() {
    if (current_ == end_) {
      return std::nullopt;
    }

    const auto& [nodeId, nodeData] = *current_;
    ++current_;

    // Find shape mapping
    auto it = mapping_.find(nodeId);
    if (it == mapping_.end()) {
      return std::nullopt;  // Skip nodes without shape mapping
    }

    return ValidationTask(nodeId, it->second, nodeData);
  }

 private:
  NodeIterator current_;
  NodeIterator end_;
  const std::map<std::string, std::string>& mapping_;
};

// ============================================================================
// Pipeline Stage 2: Cache Filter
// Checks cache and filters out already validated nodes
// ============================================================================

class CacheFilter {
 public:
  explicit CacheFilter(std::shared_ptr<NodeShapeValidationCache> cache)
      : cache_(std::move(cache)) {}

  std::optional<ValidationTask> operator()(ValidationTask task) {
    if (!cache_) {
      return task;  // No cache, pass through
    }

    auto cacheKey = ValidationCacheKey(task.nodeId, task.shapeId);
    auto cachedResult = cache_->get(cacheKey);

    if (cachedResult.has_value()) {
      // Store cached result for later retrieval
      auto lock = cachedResults_.wlock();
      (*lock)[task.nodeId] = ValidationResult(
          task.nodeId, task.shapeId, cachedResult->isValid,
          cachedResult->errors, true);
      return std::nullopt;  // Filtered out - already in cache
    }

    return task;  // Not in cache, needs validation
  }

  // Retrieve cached results
  std::vector<ValidationResult> getCachedResults() const {
    auto lock = cachedResults_.rlock();
    std::vector<ValidationResult> results;
    results.reserve(lock->size());
    for (const auto& [_, result] : *lock) {
      results.push_back(result);
    }
    return results;
  }

 private:
  std::shared_ptr<NodeShapeValidationCache> cache_;
  ad_utility::Synchronized<std::map<std::string, ValidationResult>>
      cachedResults_;
};

// ============================================================================
// Pipeline Stage 3: Validator
// Performs actual ShEx validation
// ============================================================================

class ValidatorStage {
 public:
  ValidatorStage(const ShExSchema& schema,
                 std::shared_ptr<NodeShapeValidationCache> cache)
      : schema_(schema), cache_(std::move(cache)) {}

  ValidationResult operator()(ValidationTask task) {
    const Shape* shape = schema_.getShape(task.shapeId);
    if (!shape) {
      return ValidationResult(task.nodeId, task.shapeId, false,
                              {"Shape not found: " + task.shapeId});
    }

    auto validationResult = shape->validate(task.nodeData);

    // Cache the result
    if (cache_) {
      auto cacheKey = ValidationCacheKey(task.nodeId, task.shapeId);
      cache_->put(cacheKey,
                  ValidationCacheValue(validationResult.isValid,
                                       validationResult.errors),
                  sizeof(ValidationCacheValue) +
                      validationResult.errors.size() * sizeof(std::string));
    }

    return ValidationResult(task.nodeId, task.shapeId,
                            validationResult.isValid, validationResult.errors);
  }

 private:
  const ShExSchema& schema_;
  std::shared_ptr<NodeShapeValidationCache> cache_;
};

// ============================================================================
// Pipeline Stage 4: Result Aggregator
// Collects and aggregates validation results
// ============================================================================

class ResultAggregator {
 public:
  ResultAggregator() = default;

  ValidationResult operator()(ValidationResult result) {
    auto lock = results_.wlock();
    lock->push_back(result);
    return result;
  }

  std::vector<ValidationResult> getResults() const {
    auto lock = results_.rlock();
    return *lock;
  }

  void clear() { results_.wlock()->clear(); }

 private:
  ad_utility::Synchronized<std::vector<ValidationResult>> results_;
};

// ============================================================================
// Streaming Validator - Orchestrates the pipeline
// ============================================================================

class StreamingValidator {
 public:
  struct Config {
    size_t batchSize = 100;
    size_t validatorParallelism = 4;
    bool enableCaching = true;
    bool enableIndexing = true;
  };

  StreamingValidator(const ShExSchema& schema, const Config& config = Config())
      : schema_(schema), config_(config) {
    if (config_.enableCaching) {
      cache_ = std::make_shared<NodeShapeValidationCache>(
          createDefaultValidationCache());
    }

    if (config_.enableIndexing) {
      indexManager_ = std::make_unique<ShapeIndexManager>();
      indexManager_->buildAllIndexes(schema);
    }
  }

  // Validate dataset with streaming pipeline
  template <typename DatasetIterator>
  std::vector<ValidationResult> validateDataset(
      DatasetIterator begin, DatasetIterator end,
      const std::map<std::string, std::string>& nodeToShapeMapping) {
    // Create pipeline stages
    NodeBatcher<DatasetIterator> batcher(begin, end, nodeToShapeMapping);
    CacheFilter cacheFilter(cache_);
    ValidatorStage validator(schema_, cache_);
    ResultAggregator aggregator;

    // Setup 4-stage pipeline with configurable parallelism
    auto pipeline = ad_pipeline::setupParallelPipeline<1, 1, 4, 1>(
        config_.batchSize,
        std::move(batcher),    // Stage 1: Batch nodes
        cacheFilter,           // Stage 2: Filter cached
        validator,             // Stage 3: Validate (parallel)
        aggregator             // Stage 4: Aggregate
    );

    // Process entire pipeline
    std::vector<ValidationResult> results;
    while (auto result = pipeline.getNextValue()) {
      results.push_back(std::move(*result));
    }

    // Merge cached results
    auto cachedResults = cacheFilter.getCachedResults();
    results.insert(results.end(), cachedResults.begin(), cachedResults.end());

    return results;
  }

  // Get cache metrics
  const CacheMetrics& cacheMetrics() const {
    return cache_ ? cache_->metrics() : defaultMetrics_;
  }

  // Get index statistics
  std::optional<ShapeIndexManager::IndexStats> indexStats() const {
    return indexManager_ ? std::optional(indexManager_->stats()) : std::nullopt;
  }

 private:
  const ShExSchema& schema_;
  Config config_;
  std::shared_ptr<NodeShapeValidationCache> cache_;
  std::unique_ptr<ShapeIndexManager> indexManager_;
  static inline CacheMetrics defaultMetrics_{};
};

// ============================================================================
// Batch Validator (for non-streaming use cases)
// ============================================================================

class BatchValidator {
 public:
  BatchValidator(const ShExSchema& schema,
                 std::shared_ptr<NodeShapeValidationCache> cache = nullptr)
      : schema_(schema), cache_(std::move(cache)) {}

  std::vector<ValidationResult> validateBatch(
      const std::vector<ValidationTask>& tasks) {
    std::vector<ValidationResult> results;
    results.reserve(tasks.size());

    for (const auto& task : tasks) {
      // Check cache first
      if (cache_) {
        auto cacheKey = ValidationCacheKey(task.nodeId, task.shapeId);
        auto cachedResult = cache_->get(cacheKey);
        if (cachedResult.has_value()) {
          results.emplace_back(task.nodeId, task.shapeId,
                               cachedResult->isValid, cachedResult->errors,
                               true);
          continue;
        }
      }

      // Validate
      const Shape* shape = schema_.getShape(task.shapeId);
      if (!shape) {
        results.emplace_back(task.nodeId, task.shapeId, false,
                             std::vector<std::string>{
                                 "Shape not found: " + task.shapeId});
        continue;
      }

      auto validationResult = shape->validate(task.nodeData);

      // Cache result
      if (cache_) {
        auto cacheKey = ValidationCacheKey(task.nodeId, task.shapeId);
        cache_->put(cacheKey,
                    ValidationCacheValue(validationResult.isValid,
                                         validationResult.errors));
      }

      results.emplace_back(task.nodeId, task.shapeId, validationResult.isValid,
                           validationResult.errors, false);
    }

    return results;
  }

 private:
  const ShExSchema& schema_;
  std::shared_ptr<NodeShapeValidationCache> cache_;
};

}  // namespace shex

#endif  // QLEVER_SRC_SHEX_STREAMINGVALIDATOR_H
