// Copyright 2026, University of Freiburg
// Chair of Algorithms and Data Structures.
// Author: Claude Code Assistant (claude@anthropic.com)

#ifndef QLEVER_SRC_SHEX_PARALLELVALIDATOR_H
#define QLEVER_SRC_SHEX_PARALLELVALIDATOR_H

#include <atomic>
#include <future>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

#include "parser/ShEx.h"
#include "shex/ShapeIndex.h"
#include "shex/StreamingValidator.h"
#include "shex/ValidationCache.h"
#include "util/AllocatorWithLimit.h"
#include "util/Synchronized.h"

namespace shex {

// ============================================================================
// Work Stealing Queue for Load Balancing
// ============================================================================

template <typename T>
class WorkStealingQueue {
 public:
  void push(T item) {
    auto lock = queue_.wlock();
    lock->push(std::move(item));
  }

  std::optional<T> tryPop() {
    auto lock = queue_.wlock();
    if (lock->empty()) {
      return std::nullopt;
    }
    T item = std::move(lock->front());
    lock->pop();
    return item;
  }

  bool empty() const { return queue_.rlock()->empty(); }

  size_t size() const { return queue_.rlock()->size(); }

 private:
  ad_utility::Synchronized<std::queue<T>> queue_;
};

// ============================================================================
// Parallel Validation Metrics
// ============================================================================

struct ParallelValidationMetrics {
  std::atomic<uint64_t> tasksProcessed{0};
  std::atomic<uint64_t> tasksStolen{0};
  std::atomic<uint64_t> totalValidationTimeMs{0};
  std::atomic<uint64_t> cacheHits{0};
  std::atomic<uint64_t> cacheMisses{0};
  std::atomic<size_t> peakMemoryBytes{0};

  double averageValidationTimeMs() const {
    uint64_t tasks = tasksProcessed.load();
    return tasks > 0 ? static_cast<double>(totalValidationTimeMs.load()) / tasks
                     : 0.0;
  }

  std::string summary() const {
    std::ostringstream oss;
    oss << "ParallelValidationMetrics:\n";
    oss << "  Tasks Processed: " << tasksProcessed << "\n";
    oss << "  Tasks Stolen: " << tasksStolen << "\n";
    oss << "  Avg Validation Time: " << averageValidationTimeMs() << " ms\n";
    oss << "  Cache Hits: " << cacheHits << "\n";
    oss << "  Cache Misses: " << cacheMisses << "\n";
    oss << "  Peak Memory: " << peakMemoryBytes << " bytes\n";
    return oss.str();
  }
};

// ============================================================================
// Worker Thread for Parallel Validation
// ============================================================================

class ValidationWorker {
 public:
  ValidationWorker(const ShExSchema& schema,
                   std::shared_ptr<NodeShapeValidationCache> cache,
                   std::shared_ptr<WorkStealingQueue<ValidationTask>> localQueue,
                   std::vector<std::shared_ptr<WorkStealingQueue<ValidationTask>>>*
                       allQueues,
                   size_t workerId, ParallelValidationMetrics* metrics)
      : schema_(schema),
        cache_(std::move(cache)),
        localQueue_(std::move(localQueue)),
        allQueues_(allQueues),
        workerId_(workerId),
        metrics_(metrics),
        running_(true) {}

  void run() {
    while (running_) {
      auto task = getTask();
      if (!task.has_value()) {
        // No more work, exit
        break;
      }

      processTask(std::move(task.value()));
    }
  }

  void stop() { running_ = false; }

  std::vector<ValidationResult> getResults() const {
    auto lock = results_.rlock();
    return *lock;
  }

 private:
  std::optional<ValidationTask> getTask() {
    // Try local queue first
    auto task = localQueue_->tryPop();
    if (task.has_value()) {
      return task;
    }

    // Try stealing from other queues
    for (size_t i = 0; i < allQueues_->size(); ++i) {
      if (i == workerId_) continue;  // Skip own queue

      task = (*allQueues_)[i]->tryPop();
      if (task.has_value()) {
        metrics_->tasksStolen++;
        return task;
      }
    }

    return std::nullopt;  // No work available
  }

  void processTask(ValidationTask task) {
    auto startTime = std::chrono::steady_clock::now();

    // Check cache
    if (cache_) {
      auto cacheKey = ValidationCacheKey(task.nodeId, task.shapeId);
      auto cachedResult = cache_->get(cacheKey);
      if (cachedResult.has_value()) {
        auto lock = results_.wlock();
        lock->emplace_back(task.nodeId, task.shapeId, cachedResult->isValid,
                           cachedResult->errors, true);
        metrics_->cacheHits++;
        metrics_->tasksProcessed++;
        return;
      }
      metrics_->cacheMisses++;
    }

    // Validate
    const Shape* shape = schema_.getShape(task.shapeId);
    if (!shape) {
      auto lock = results_.wlock();
      lock->emplace_back(task.nodeId, task.shapeId, false,
                         std::vector<std::string>{
                             "Shape not found: " + task.shapeId});
      metrics_->tasksProcessed++;
      return;
    }

    auto validationResult = shape->validate(task.nodeData);

    // Cache result
    if (cache_) {
      auto cacheKey = ValidationCacheKey(task.nodeId, task.shapeId);
      cache_->put(cacheKey,
                  ValidationCacheValue(validationResult.isValid,
                                       validationResult.errors));
    }

    // Store result
    {
      auto lock = results_.wlock();
      lock->emplace_back(task.nodeId, task.shapeId, validationResult.isValid,
                         validationResult.errors, false);
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        endTime - startTime)
                        .count();
    metrics_->totalValidationTimeMs += duration;
    metrics_->tasksProcessed++;
  }

  const ShExSchema& schema_;
  std::shared_ptr<NodeShapeValidationCache> cache_;
  std::shared_ptr<WorkStealingQueue<ValidationTask>> localQueue_;
  std::vector<std::shared_ptr<WorkStealingQueue<ValidationTask>>>* allQueues_;
  size_t workerId_;
  ParallelValidationMetrics* metrics_;
  std::atomic<bool> running_;
  ad_utility::Synchronized<std::vector<ValidationResult>> results_;
};

// ============================================================================
// Multi-Level Parallel Validator
// ============================================================================

class ParallelValidator {
 public:
  struct Config {
    size_t numWorkers = std::thread::hardware_concurrency();
    size_t batchSize = 100;
    bool enableCaching = true;
    bool enableWorkStealing = true;
    std::optional<ad_utility::MemorySize> memoryLimit = std::nullopt;
  };

  ParallelValidator(const ShExSchema& schema, const Config& config = Config())
      : schema_(schema), config_(config) {
    if (config_.enableCaching) {
      cache_ = std::make_shared<NodeShapeValidationCache>(
          createHighPerformanceCache());
    }

    // Initialize worker queues
    for (size_t i = 0; i < config_.numWorkers; ++i) {
      workerQueues_.push_back(
          std::make_shared<WorkStealingQueue<ValidationTask>>());
    }
  }

  // Dataset-level parallelization
  std::vector<ValidationResult> validateDatasetParallel(
      const std::map<std::string,
                     std::map<std::string, std::vector<std::pair<std::string, ValueType>>>>& dataset,
      const std::map<std::string, std::string>& nodeToShapeMapping) {
    metrics_ = ParallelValidationMetrics{};

    // Distribute tasks to worker queues (round-robin)
    size_t queueIdx = 0;
    for (const auto& [nodeId, nodeData] : dataset) {
      auto shapeIt = nodeToShapeMapping.find(nodeId);
      if (shapeIt == nodeToShapeMapping.end()) {
        continue;
      }

      ValidationTask task(nodeId, shapeIt->second, nodeData);
      workerQueues_[queueIdx]->push(std::move(task));
      queueIdx = (queueIdx + 1) % config_.numWorkers;
    }

    // Launch workers
    std::vector<std::unique_ptr<ValidationWorker>> workers;
    std::vector<std::future<void>> futures;

    for (size_t i = 0; i < config_.numWorkers; ++i) {
      auto worker = std::make_unique<ValidationWorker>(
          schema_, cache_, workerQueues_[i], &workerQueues_, i, &metrics_);

      futures.push_back(std::async(std::launch::async,
                                    [&worker]() { worker->run(); }));
      workers.push_back(std::move(worker));
    }

    // Wait for all workers to complete
    for (auto& future : futures) {
      future.wait();
    }

    // Collect results from all workers
    std::vector<ValidationResult> allResults;
    for (const auto& worker : workers) {
      auto workerResults = worker->getResults();
      allResults.insert(allResults.end(), workerResults.begin(),
                        workerResults.end());
    }

    return allResults;
  }

  // Shape-level parallelization (validate multiple shapes in parallel)
  std::vector<ValidationResult> validateNodeWithMultipleShapes(
      const std::string& nodeId,
      const std::vector<std::string>& shapeIds,
      const std::map<std::string, std::vector<std::pair<std::string, ValueType>>>& nodeData) {
    std::vector<std::future<ValidationResult>> futures;

    for (const auto& shapeId : shapeIds) {
      futures.push_back(std::async(
          std::launch::async,
          [this, &nodeId, &shapeId, &nodeData]() -> ValidationResult {
            // Check cache
            if (cache_) {
              auto cacheKey = ValidationCacheKey(nodeId, shapeId);
              auto cachedResult = cache_->get(cacheKey);
              if (cachedResult.has_value()) {
                return ValidationResult(nodeId, shapeId, cachedResult->isValid,
                                        cachedResult->errors, true);
              }
            }

            // Validate
            const Shape* shape = schema_.getShape(shapeId);
            if (!shape) {
              return ValidationResult(nodeId, shapeId, false,
                                      std::vector<std::string>{
                                          "Shape not found: " + shapeId});
            }

            auto validationResult = shape->validate(nodeData);

            // Cache result
            if (cache_) {
              auto cacheKey = ValidationCacheKey(nodeId, shapeId);
              cache_->put(cacheKey,
                          ValidationCacheValue(validationResult.isValid,
                                               validationResult.errors));
            }

            return ValidationResult(nodeId, shapeId, validationResult.isValid,
                                    validationResult.errors, false);
          }));
    }

    // Collect results
    std::vector<ValidationResult> results;
    for (auto& future : futures) {
      results.push_back(future.get());
    }

    return results;
  }

  // Constraint-level parallelization (validate shape constraints in parallel)
  ValidationResult validateNodeConstraintsParallel(
      const std::string& nodeId, const std::string& shapeId,
      const std::map<std::string, std::vector<std::pair<std::string, ValueType>>>& nodeData) {
    // Check cache first
    if (cache_) {
      auto cacheKey = ValidationCacheKey(nodeId, shapeId);
      auto cachedResult = cache_->get(cacheKey);
      if (cachedResult.has_value()) {
        return ValidationResult(nodeId, shapeId, cachedResult->isValid,
                                cachedResult->errors, true);
      }
    }

    const Shape* shape = schema_.getShape(shapeId);
    if (!shape) {
      return ValidationResult(nodeId, shapeId, false,
                              std::vector<std::string>{
                                  "Shape not found: " + shapeId});
    }

    // Validate each property constraint in parallel
    std::vector<std::future<std::pair<bool, std::vector<std::string>>>> futures;

    for (const auto& prop : shape->properties) {
      futures.push_back(std::async(
          std::launch::async,
          [&prop, &nodeData]() -> std::pair<bool, std::vector<std::string>> {
            std::vector<std::string> errors;
            auto it = nodeData.find(prop.predicate);
            size_t count = (it != nodeData.end()) ? it->second.size() : 0;

            // Check cardinality
            bool valid = true;
            switch (prop.cardinality) {
              case Cardinality::EXACTLY_ONE:
                if (count != 1) {
                  valid = false;
                  errors.push_back("Property " + prop.predicate +
                                   " must appear exactly once (found " +
                                   std::to_string(count) + ")");
                }
                break;
              case Cardinality::ZERO_OR_ONE:
                if (count > 1) {
                  valid = false;
                  errors.push_back("Property " + prop.predicate +
                                   " must appear at most once (found " +
                                   std::to_string(count) + ")");
                }
                break;
              case Cardinality::ZERO_OR_MORE:
                break;
              case Cardinality::ONE_OR_MORE:
                if (count < 1) {
                  valid = false;
                  errors.push_back("Property " + prop.predicate +
                                   " must appear at least once");
                }
                break;
            }

            // Validate values
            if (it != nodeData.end()) {
              for (const auto& [value, type] : it->second) {
                if (!prop.validate(value, type)) {
                  valid = false;
                  errors.push_back(
                      "Property " + prop.predicate + " value '" + value +
                      "' does not match constraints");
                }
              }
            }

            return {valid, errors};
          }));
    }

    // Aggregate results
    bool overallValid = true;
    std::vector<std::string> allErrors;

    for (auto& future : futures) {
      auto [valid, errors] = future.get();
      if (!valid) {
        overallValid = false;
        allErrors.insert(allErrors.end(), errors.begin(), errors.end());
      }
    }

    // Cache result
    if (cache_) {
      auto cacheKey = ValidationCacheKey(nodeId, shapeId);
      cache_->put(cacheKey,
                  ValidationCacheValue(overallValid, allErrors));
    }

    return ValidationResult(nodeId, shapeId, overallValid, allErrors, false);
  }

  const ParallelValidationMetrics& metrics() const { return metrics_; }

  const CacheMetrics* cacheMetrics() const {
    return cache_ ? &cache_->metrics() : nullptr;
  }

 private:
  const ShExSchema& schema_;
  Config config_;
  std::shared_ptr<NodeShapeValidationCache> cache_;
  std::vector<std::shared_ptr<WorkStealingQueue<ValidationTask>>> workerQueues_;
  ParallelValidationMetrics metrics_;
};

}  // namespace shex

#endif  // QLEVER_SRC_SHEX_PARALLELVALIDATOR_H
