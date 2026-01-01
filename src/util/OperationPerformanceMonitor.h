#ifndef QLEVER_SRC_UTIL_OPERATION_PERFORMANCE_MONITOR_H
#define QLEVER_SRC_UTIL_OPERATION_PERFORMANCE_MONITOR_H

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "util/CopyableSynchronization.h"

namespace ad_utility {

/**
 * Generic performance monitor for tracking operation metrics
 *
 * This is a lightweight, opt-in performance monitoring system that tracks:
 * - Execution count (attempted and successful)
 * - Cache statistics (hits and misses)
 * - Execution time (average)
 *
 * Follows the 80/20 principle: provides 80% of monitoring value with 20% of
 * complexity compared to comprehensive profiling systems.
 *
 * Thread-safe with minimal contention using atomic operations.
 *
 * Usage:
 *   auto& monitor = OperationPerformanceMonitor::instance();
 *   monitor.recordExecution("JoinOperation", true, 5.2);  // cache hit in 5.2ms
 *   auto metrics = monitor.getMetrics("JoinOperation");
 *   std::cout << "Hit rate: " << (metrics.getCacheHitRate() * 100) << "%\n";
 */
class OperationPerformanceMonitor {
 public:
  struct OperationMetrics {
    // Basic execution statistics
    std::atomic<size_t> executionsAttempted{0};
    std::atomic<size_t> executionsSucceeded{0};

    // Cache statistics
    std::atomic<size_t> cacheHits{0};
    std::atomic<size_t> cacheMisses{0};

    // Timing information
    std::atomic<double> totalExecutionTimeMs{0.0};
    std::atomic<size_t> executionCount{0};

    /**
     * Get cache hit rate (hits / (hits + misses))
     * Returns 0.0 if no cache operations recorded
     */
    double getCacheHitRate() const {
      size_t total = cacheHits.load() + cacheMisses.load();
      if (total == 0) return 0.0;
      return static_cast<double>(cacheHits.load()) / total;
    }

    /**
     * Get average execution time in milliseconds
     */
    double getAverageExecutionTimeMs() const {
      size_t count = executionCount.load();
      if (count == 0) return 0.0;
      return totalExecutionTimeMs.load() / count;
    }

    /**
     * Get success rate (succeeded / attempted)
     */
    double getSuccessRate() const {
      size_t total = executionsAttempted.load();
      if (total == 0) return 0.0;
      return static_cast<double>(executionsSucceeded.load()) / total;
    }

    /**
     * Reset all metrics to zero
     */
    void reset() {
      executionsAttempted.store(0);
      executionsSucceeded.store(0);
      cacheHits.store(0);
      cacheMisses.store(0);
      totalExecutionTimeMs.store(0.0);
      executionCount.store(0);
    }
  };

  /**
   * Get the global singleton instance
   */
  static OperationPerformanceMonitor& instance();

  /**
   * Record a cache access (hit or miss)
   */
  void recordCacheAccess(const std::string& operationName, bool hit) {
    auto& metrics = getOrCreateMetrics(operationName);
    if (hit) {
      metrics.cacheHits++;
    } else {
      metrics.cacheMisses++;
    }
  }

  /**
   * Record operation execution with timing
   *
   * @param operationName Name of the operation
   * @param succeeded Whether the operation succeeded
   * @param executionTimeMs Time taken in milliseconds
   */
  void recordExecution(const std::string& operationName, bool succeeded,
                      double executionTimeMs) {
    auto& metrics = getOrCreateMetrics(operationName);
    metrics.executionsAttempted++;
    if (succeeded) {
      metrics.executionsSucceeded++;
    }
    metrics.totalExecutionTimeMs += executionTimeMs;
    metrics.executionCount++;
  }

  /**
   * Get metrics for a specific operation
   * Returns a snapshot of current metrics
   */
  OperationMetrics getMetrics(const std::string& operationName) const;

  /**
   * Get metrics for all operations
   */
  absl::flat_hash_map<std::string, OperationMetrics> getAllMetrics() const;

  /**
   * Reset metrics for a specific operation
   */
  void resetMetrics(const std::string& operationName) {
    auto it = metrics_.find(operationName);
    if (it != metrics_.end()) {
      it->second->reset();
    }
  }

  /**
   * Reset all metrics
   */
  void resetAllMetrics() {
    for (auto& [name, metrics] : metrics_) {
      metrics->reset();
    }
  }

  /**
   * Clear all metrics (removes entries entirely)
   */
  void clearAllMetrics() { metrics_.clear(); }

  /**
   * Get count of monitored operations
   */
  size_t getMonitoredOperationCount() const { return metrics_.size(); }

  /**
   * Check if a specific operation is being monitored
   */
  bool isMonitored(const std::string& operationName) const {
    return metrics_.find(operationName) != metrics_.end();
  }

 private:
  OperationPerformanceMonitor() = default;

  /**
   * Get or create metrics for an operation
   * Thread-safe: creates entry if missing
   */
  OperationMetrics& getOrCreateMetrics(const std::string& operationName);

  // Thread-safe storage of operation metrics
  // Using Synchronized for safety, though individual metrics use atomics
  Synchronized<absl::flat_hash_map<std::string, std::shared_ptr<OperationMetrics>>>
      metrics_;
};

/**
 * RAII helper for automatic operation execution recording
 *
 * Usage:
 *   {
 *     OperationExecutionTimer timer("JoinOperation");
 *     // Perform operation...
 *   }  // Automatically records execution with timing
 */
class OperationExecutionTimer {
 public:
  explicit OperationExecutionTimer(const std::string& operationName)
      : operationName_(operationName),
        startTime_(std::chrono::steady_clock::now()),
        succeeded_(false) {}

  ~OperationExecutionTimer() {
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime_);
    double timeMs = duration.count() / 1000.0;

    OperationPerformanceMonitor::instance().recordExecution(operationName_,
                                                            succeeded_, timeMs);
  }

  /**
   * Mark operation as succeeded (default is false)
   */
  void markSucceeded() { succeeded_ = true; }

  /**
   * Mark operation as failed
   */
  void markFailed() { succeeded_ = false; }

 private:
  std::string operationName_;
  std::chrono::steady_clock::time_point startTime_;
  bool succeeded_;
};

}  // namespace ad_utility

#endif  // QLEVER_SRC_UTIL_OPERATION_PERFORMANCE_MONITOR_H
