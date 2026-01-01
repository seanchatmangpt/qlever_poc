#ifndef AD_UTILITY_EPOCH_METRICS_H
#define AD_UTILITY_EPOCH_METRICS_H

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "util/Synchronized.h"

namespace ad_utility {

// Latency statistics for tracking p50/p95/p99
struct LatencyStats {
  uint64_t count_ = 0;
  uint64_t sum_us_ = 0;
  uint64_t min_us_ = UINT64_MAX;
  uint64_t max_us_ = 0;
  std::vector<uint64_t> samples_;  // For percentile calculation

  void record(uint64_t latency_us);
  uint64_t p50() const;
  uint64_t p95() const;
  uint64_t p99() const;
  uint64_t avg() const;
};

// Comprehensive metrics for epoch system observability
struct EpochMetrics {
  // Current epoch query statistics
  uint64_t queriesInCurrentEpoch_ = 0;
  // Total queries across all epochs
  uint64_t totalQueriesAllEpochs_ = 0;

  // Current epoch write rejection statistics
  uint64_t writeAttemptsRejected_ = 0;
  // Total rejections across all epochs
  uint64_t totalWriteRejectionsAllEpochs_ = 0;

  // Cache invalidation tracking
  uint64_t cacheInvalidations_ = 0;

  // Epoch lifecycle tracking
  uint64_t epochTransitions_ = 0;  // SEAL -> SERVE transitions
  uint64_t epochRestarts_ = 0;     // SERVE -> INIT restarts

  // Timestamp tracking (milliseconds since epoch)
  int64_t lastQueryTimestampMs_ = 0;
  int64_t lastWriteRejectionTimestampMs_ = 0;
  int64_t lastEpochTransitionTimestampMs_ = 0;

  // Convert to human-readable string
  std::string toString() const;

  // Convert to CSV format
  std::string toCSV() const;
};

// Cache-specific metrics for read cache observability
struct ReadCacheMetrics {
  // Bytes cache metrics
  uint64_t bytes_hits = 0;
  uint64_t bytes_misses = 0;
  uint64_t bytes_inserts = 0;
  uint64_t bytes_evicts = 0;
  uint64_t bytes_served_from_cache = 0;  // cumulative bytes returned

  // Plan cache metrics
  uint64_t plan_hits = 0;
  uint64_t plan_misses = 0;
  uint64_t plan_inserts = 0;
  uint64_t plan_evicts = 0;

  // Negative cache metrics
  uint64_t neg_hits = 0;
  uint64_t neg_inserts = 0;

  // Inflight tracking
  uint64_t inflight_waiters = 0;  // current gauge

  // Per-shape latency tracking (shape_sha256 -> LatencyStats)
  std::map<std::string, LatencyStats> shape_latency;

  // Epoch management
  uint64_t epoch_promote_count = 0;
  uint64_t prewarm_duration_ms = 0;

  // Convert to human-readable string
  std::string toString() const;

  // Convert to JSON format
  std::string toJSON() const;
};

// Thread-safe metrics collector
class EpochMetricsCollector {
 private:
  Synchronized<EpochMetrics> metrics_;

 public:
  EpochMetricsCollector() = default;

  // Record successful query start
  void recordQueryStart();

  // Record rejected write attempt
  void recordRejectedWrite();

  // Record cache invalidation event
  void recordCacheInvalidation();

  // Record SEAL -> SERVE transition
  void recordEpochTransition();

  // Record SERVE -> INIT restart
  void recordEpochRestart();

  // Get snapshot of current metrics
  EpochMetrics getMetrics() const;

  // Reset metrics (for testing)
  void reset();

  // Get detailed human-readable report
  std::string getDetailedMetricsReport() const;
};

// Global singleton for epoch metrics
extern ad_utility::Synchronized<EpochMetricsCollector> globalEpochMetrics;

// Thread-safe read cache metrics collector
class ReadCacheMetricsCollector {
 private:
  Synchronized<ReadCacheMetrics> metrics_;

 public:
  ReadCacheMetricsCollector() = default;

  // Bytes cache operations
  void recordBytesHit(uint64_t bytes);
  void recordBytesMiss();
  void recordBytesInsert(uint64_t bytes);
  void recordBytesEvict(uint64_t bytes);

  // Plan cache operations
  void recordPlanHit();
  void recordPlanMiss();
  void recordPlanInsert();
  void recordPlanEvict();

  // Negative cache operations
  void recordNegativeHit();
  void recordNegativeInsert();

  // Inflight tracking
  void recordInflightWaiterAdd();
  void recordInflightWaiterRemove();

  // Per-shape latency tracking
  void recordShapeLatency(const std::string& shape_sha256, uint64_t latency_us);

  // Epoch operations
  void recordEpochPromote(uint64_t prewarm_ms);

  // Get snapshot of current metrics
  ReadCacheMetrics getMetrics() const;

  // Reset metrics (for testing)
  void reset();

  // Get detailed human-readable report
  std::string getDetailedMetricsReport() const;
};

// Global singleton for read cache metrics
extern ad_utility::Synchronized<ReadCacheMetricsCollector>
    globalReadCacheMetrics;

}  // namespace ad_utility

#endif  // AD_UTILITY_EPOCH_METRICS_H
