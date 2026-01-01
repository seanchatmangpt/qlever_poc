#include "global/EpochMetrics.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace ad_utility {

// Global singleton instance
ad_utility::Synchronized<EpochMetricsCollector> globalEpochMetrics;

// Helper: get current timestamp in milliseconds
static int64_t getCurrentTimestampMs() {
  auto now = std::chrono::system_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch());
  return ms.count();
}

// Record successful query start
void EpochMetricsCollector::recordQueryStart() {
  auto lock = metrics_.acquire();
  lock->queriesInCurrentEpoch_++;
  lock->totalQueriesAllEpochs_++;
  lock->lastQueryTimestampMs_ = getCurrentTimestampMs();
}

// Record rejected write attempt
void EpochMetricsCollector::recordRejectedWrite() {
  auto lock = metrics_.acquire();
  lock->writeAttemptsRejected_++;
  lock->totalWriteRejectionsAllEpochs_++;
  lock->lastWriteRejectionTimestampMs_ = getCurrentTimestampMs();
}

// Record cache invalidation event
void EpochMetricsCollector::recordCacheInvalidation() {
  auto lock = metrics_.acquire();
  lock->cacheInvalidations_++;
}

// Record SEAL -> SERVE transition
void EpochMetricsCollector::recordEpochTransition() {
  auto lock = metrics_.acquire();
  lock->epochTransitions_++;
  lock->lastEpochTransitionTimestampMs_ = getCurrentTimestampMs();
  // Reset current epoch counters for new epoch
  lock->queriesInCurrentEpoch_ = 0;
  lock->writeAttemptsRejected_ = 0;
  lock->cacheInvalidations_ = 0;
}

// Record SERVE -> INIT restart
void EpochMetricsCollector::recordEpochRestart() {
  auto lock = metrics_.acquire();
  lock->epochRestarts_++;
}

// Get snapshot of current metrics
EpochMetrics EpochMetricsCollector::getMetrics() const {
  auto lock = metrics_.acquire();
  return *lock;
}

// Reset metrics (for testing)
void EpochMetricsCollector::reset() {
  auto lock = metrics_.acquire();
  *lock = EpochMetrics();
}

// Convert metrics to human-readable string
std::string EpochMetrics::toString() const {
  std::ostringstream oss;
  oss << "EpochMetrics{queries_current_epoch=" << queriesInCurrentEpoch_
      << ", writes_rejected=" << writeAttemptsRejected_
      << ", cache_invalidations=" << cacheInvalidations_
      << ", epoch_transitions=" << epochTransitions_
      << ", epoch_restarts=" << epochRestarts_
      << ", last_query_ms=" << lastQueryTimestampMs_
      << ", last_write_rejection_ms=" << lastWriteRejectionTimestampMs_
      << ", last_epoch_transition_ms=" << lastEpochTransitionTimestampMs_
      << ", total_queries_all_epochs=" << totalQueriesAllEpochs_
      << ", total_writes_rejected_all_epochs=" << totalWriteRejectionsAllEpochs_
      << "}";
  return oss.str();
}

// Convert metrics to CSV format
std::string EpochMetrics::toCSV() const {
  std::ostringstream oss;
  oss << queriesInCurrentEpoch_ << "," << writeAttemptsRejected_ << ","
      << cacheInvalidations_ << "," << epochTransitions_ << ","
      << epochRestarts_ << "," << lastQueryTimestampMs_ << ","
      << lastWriteRejectionTimestampMs_ << ","
      << lastEpochTransitionTimestampMs_ << "," << totalQueriesAllEpochs_ << ","
      << totalWriteRejectionsAllEpochs_;
  return oss.str();
}

// Get detailed human-readable report
std::string EpochMetricsCollector::getDetailedMetricsReport() const {
  auto lock = metrics_.acquire();
  std::ostringstream oss;
  oss << "\n=== EPOCH METRICS REPORT ===\n";
  oss << "Epoch State:\n";
  oss << "  Epoch Transitions (SEAL->SERVE): " << std::setw(10)
      << lock->epochTransitions_ << "\n";
  oss << "  Epoch Restarts (SERVE->INIT):    " << std::setw(10)
      << lock->epochRestarts_ << "\n";
  oss << "\nQueries:\n";
  oss << "  Queries in Current Epoch:        " << std::setw(10)
      << lock->queriesInCurrentEpoch_ << "\n";
  oss << "  Total Queries All Epochs:        " << std::setw(10)
      << lock->totalQueriesAllEpochs_ << "\n";
  if (lock->epochTransitions_ > 0) {
    double avg = static_cast<double>(lock->totalQueriesAllEpochs_) /
                 (lock->epochTransitions_ > 0 ? lock->epochTransitions_ : 1);
    oss << "  Average Queries per Epoch:       " << std::fixed
        << std::setprecision(2) << std::setw(10) << avg << "\n";
  }
  oss << "\nMutations:\n";
  oss << "  Write Attempts Rejected (Cur):   " << std::setw(10)
      << lock->writeAttemptsRejected_ << "\n";
  oss << "  Total Writes Rejected All Epochs:" << std::setw(10)
      << lock->totalWriteRejectionsAllEpochs_ << "\n";
  if (lock->epochTransitions_ > 0) {
    double avg = static_cast<double>(lock->totalWriteRejectionsAllEpochs_) /
                 (lock->epochTransitions_ > 0 ? lock->epochTransitions_ : 1);
    oss << "  Average Rejections per Epoch:    " << std::fixed
        << std::setprecision(2) << std::setw(10) << avg << "\n";
  }
  oss << "\nCache Management:\n";
  oss << "  Cache Invalidations (Cur Epoch): " << std::setw(10)
      << lock->cacheInvalidations_ << "\n";
  oss << "\nTimings:\n";
  oss << "  Last Epoch Transition (ms):      " << std::setw(10)
      << lock->lastEpochTransitionTimestampMs_ << "\n";
  oss << "=== END METRICS REPORT ===\n";
  return oss.str();
}

}  // namespace ad_utility
