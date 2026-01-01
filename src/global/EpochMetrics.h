#ifndef AD_UTILITY_EPOCH_METRICS_H
#define AD_UTILITY_EPOCH_METRICS_H

#include <chrono>
#include <cstdint>
#include <string>

#include "ad_utility/Synchronized.h"

namespace ad_utility {

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

}  // namespace ad_utility

#endif  // AD_UTILITY_EPOCH_METRICS_H
