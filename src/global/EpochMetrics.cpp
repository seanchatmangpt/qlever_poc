#include "global/EpochMetrics.h"

#include <algorithm>
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

// ============================================================================
// LatencyStats implementation
// ============================================================================

void LatencyStats::record(uint64_t latency_us) {
  count_++;
  sum_us_ += latency_us;
  min_us_ = std::min(min_us_, latency_us);
  max_us_ = std::max(max_us_, latency_us);
  samples_.push_back(latency_us);
}

uint64_t LatencyStats::p50() const {
  if (samples_.empty()) return 0;
  auto sorted = samples_;
  std::sort(sorted.begin(), sorted.end());
  return sorted[sorted.size() * 50 / 100];
}

uint64_t LatencyStats::p95() const {
  if (samples_.empty()) return 0;
  auto sorted = samples_;
  std::sort(sorted.begin(), sorted.end());
  return sorted[sorted.size() * 95 / 100];
}

uint64_t LatencyStats::p99() const {
  if (samples_.empty()) return 0;
  auto sorted = samples_;
  std::sort(sorted.begin(), sorted.end());
  return sorted[sorted.size() * 99 / 100];
}

uint64_t LatencyStats::avg() const {
  return count_ > 0 ? sum_us_ / count_ : 0;
}

// ============================================================================
// ReadCacheMetrics implementation
// ============================================================================

std::string ReadCacheMetrics::toString() const {
  std::ostringstream oss;
  oss << "ReadCacheMetrics{";
  oss << "bytes_hits=" << bytes_hits << ", bytes_misses=" << bytes_misses
      << ", bytes_inserts=" << bytes_inserts << ", bytes_evicts=" << bytes_evicts
      << ", bytes_served=" << bytes_served_from_cache
      << ", plan_hits=" << plan_hits << ", plan_misses=" << plan_misses
      << ", plan_inserts=" << plan_inserts << ", plan_evicts=" << plan_evicts
      << ", neg_hits=" << neg_hits << ", neg_inserts=" << neg_inserts
      << ", inflight_waiters=" << inflight_waiters
      << ", epoch_promotes=" << epoch_promote_count
      << ", prewarm_ms=" << prewarm_duration_ms
      << ", shape_count=" << shape_latency.size()
      << "}";
  return oss.str();
}

std::string ReadCacheMetrics::toJSON() const {
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"bytes\": {\n";
  oss << "    \"hits\": " << bytes_hits << ",\n";
  oss << "    \"misses\": " << bytes_misses << ",\n";
  oss << "    \"inserts\": " << bytes_inserts << ",\n";
  oss << "    \"evicts\": " << bytes_evicts << ",\n";
  oss << "    \"served_bytes\": " << bytes_served_from_cache << "\n";
  oss << "  },\n";
  oss << "  \"plan\": {\n";
  oss << "    \"hits\": " << plan_hits << ",\n";
  oss << "    \"misses\": " << plan_misses << ",\n";
  oss << "    \"inserts\": " << plan_inserts << ",\n";
  oss << "    \"evicts\": " << plan_evicts << "\n";
  oss << "  },\n";
  oss << "  \"negative\": {\n";
  oss << "    \"hits\": " << neg_hits << ",\n";
  oss << "    \"inserts\": " << neg_inserts << "\n";
  oss << "  },\n";
  oss << "  \"inflight_waiters\": " << inflight_waiters << ",\n";
  oss << "  \"epoch\": {\n";
  oss << "    \"promote_count\": " << epoch_promote_count << ",\n";
  oss << "    \"prewarm_duration_ms\": " << prewarm_duration_ms << "\n";
  oss << "  },\n";
  oss << "  \"shape_latency\": {\n";
  bool first = true;
  for (const auto& [shape_sha256, stats] : shape_latency) {
    if (!first) oss << ",\n";
    first = false;
    oss << "    \"" << shape_sha256 << "\": {\n";
    oss << "      \"count\": " << stats.count_ << ",\n";
    oss << "      \"avg_us\": " << stats.avg() << ",\n";
    oss << "      \"p50_us\": " << stats.p50() << ",\n";
    oss << "      \"p95_us\": " << stats.p95() << ",\n";
    oss << "      \"p99_us\": " << stats.p99() << ",\n";
    oss << "      \"min_us\": " << stats.min_us_ << ",\n";
    oss << "      \"max_us\": " << stats.max_us_ << "\n";
    oss << "    }";
  }
  oss << "\n  }\n";
  oss << "}\n";
  return oss.str();
}

// ============================================================================
// ReadCacheMetricsCollector implementation
// ============================================================================

// Global singleton instance
ad_utility::Synchronized<ReadCacheMetricsCollector> globalReadCacheMetrics;

void ReadCacheMetricsCollector::recordBytesHit(uint64_t bytes) {
  auto lock = metrics_.acquire();
  lock->bytes_hits++;
  lock->bytes_served_from_cache += bytes;
}

void ReadCacheMetricsCollector::recordBytesMiss() {
  auto lock = metrics_.acquire();
  lock->bytes_misses++;
}

void ReadCacheMetricsCollector::recordBytesInsert(uint64_t bytes) {
  auto lock = metrics_.acquire();
  lock->bytes_inserts++;
}

void ReadCacheMetricsCollector::recordBytesEvict(uint64_t bytes) {
  auto lock = metrics_.acquire();
  lock->bytes_evicts++;
}

void ReadCacheMetricsCollector::recordPlanHit() {
  auto lock = metrics_.acquire();
  lock->plan_hits++;
}

void ReadCacheMetricsCollector::recordPlanMiss() {
  auto lock = metrics_.acquire();
  lock->plan_misses++;
}

void ReadCacheMetricsCollector::recordPlanInsert() {
  auto lock = metrics_.acquire();
  lock->plan_inserts++;
}

void ReadCacheMetricsCollector::recordPlanEvict() {
  auto lock = metrics_.acquire();
  lock->plan_evicts++;
}

void ReadCacheMetricsCollector::recordNegativeHit() {
  auto lock = metrics_.acquire();
  lock->neg_hits++;
}

void ReadCacheMetricsCollector::recordNegativeInsert() {
  auto lock = metrics_.acquire();
  lock->neg_inserts++;
}

void ReadCacheMetricsCollector::recordInflightWaiterAdd() {
  auto lock = metrics_.acquire();
  lock->inflight_waiters++;
}

void ReadCacheMetricsCollector::recordInflightWaiterRemove() {
  auto lock = metrics_.acquire();
  if (lock->inflight_waiters > 0) {
    lock->inflight_waiters--;
  }
}

void ReadCacheMetricsCollector::recordShapeLatency(
    const std::string& shape_sha256, uint64_t latency_us) {
  auto lock = metrics_.acquire();
  lock->shape_latency[shape_sha256].record(latency_us);
}

void ReadCacheMetricsCollector::recordEpochPromote(uint64_t prewarm_ms) {
  auto lock = metrics_.acquire();
  lock->epoch_promote_count++;
  lock->prewarm_duration_ms = prewarm_ms;
}

ReadCacheMetrics ReadCacheMetricsCollector::getMetrics() const {
  auto lock = metrics_.acquire();
  return *lock;
}

void ReadCacheMetricsCollector::reset() {
  auto lock = metrics_.acquire();
  *lock = ReadCacheMetrics();
}

std::string ReadCacheMetricsCollector::getDetailedMetricsReport() const {
  auto lock = metrics_.acquire();
  std::ostringstream oss;
  oss << "\n=== READ CACHE METRICS REPORT ===\n";

  oss << "\nBytes Cache:\n";
  oss << "  Hits:                            " << std::setw(10)
      << lock->bytes_hits << "\n";
  oss << "  Misses:                          " << std::setw(10)
      << lock->bytes_misses << "\n";
  oss << "  Inserts:                         " << std::setw(10)
      << lock->bytes_inserts << "\n";
  oss << "  Evicts:                          " << std::setw(10)
      << lock->bytes_evicts << "\n";
  oss << "  Total Bytes Served:              " << std::setw(10)
      << lock->bytes_served_from_cache << "\n";
  if (lock->bytes_hits + lock->bytes_misses > 0) {
    double hit_rate = 100.0 * lock->bytes_hits /
                      (lock->bytes_hits + lock->bytes_misses);
    oss << "  Hit Rate:                        " << std::fixed
        << std::setprecision(2) << std::setw(10) << hit_rate << "%\n";
  }

  oss << "\nPlan Cache:\n";
  oss << "  Hits:                            " << std::setw(10)
      << lock->plan_hits << "\n";
  oss << "  Misses:                          " << std::setw(10)
      << lock->plan_misses << "\n";
  oss << "  Inserts:                         " << std::setw(10)
      << lock->plan_inserts << "\n";
  oss << "  Evicts:                          " << std::setw(10)
      << lock->plan_evicts << "\n";
  if (lock->plan_hits + lock->plan_misses > 0) {
    double hit_rate =
        100.0 * lock->plan_hits / (lock->plan_hits + lock->plan_misses);
    oss << "  Hit Rate:                        " << std::fixed
        << std::setprecision(2) << std::setw(10) << hit_rate << "%\n";
  }

  oss << "\nNegative Cache:\n";
  oss << "  Hits:                            " << std::setw(10)
      << lock->neg_hits << "\n";
  oss << "  Inserts:                         " << std::setw(10)
      << lock->neg_inserts << "\n";

  oss << "\nInflight:\n";
  oss << "  Current Waiters:                 " << std::setw(10)
      << lock->inflight_waiters << "\n";

  oss << "\nEpoch Management:\n";
  oss << "  Promotes:                        " << std::setw(10)
      << lock->epoch_promote_count << "\n";
  oss << "  Last Prewarm Duration (ms):      " << std::setw(10)
      << lock->prewarm_duration_ms << "\n";

  oss << "\nPer-Shape Latency (top 10):\n";
  if (lock->shape_latency.empty()) {
    oss << "  (no data)\n";
  } else {
    // Sort by count (most frequently executed shapes)
    std::vector<std::pair<std::string, LatencyStats>> sorted;
    for (const auto& [shape_sha256, stats] : lock->shape_latency) {
      sorted.emplace_back(shape_sha256, stats);
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) {
                return a.second.count_ > b.second.count_;
              });

    int shown = 0;
    for (const auto& [shape_sha256, stats] : sorted) {
      if (shown++ >= 10) break;
      // Show first 16 chars of the hash
      std::string short_hash = shape_sha256.length() > 16
                                   ? shape_sha256.substr(0, 16) + "..."
                                   : shape_sha256;
      oss << "  " << short_hash << ":\n";
      oss << "    Count:  " << std::setw(8) << stats.count_
          << "  Avg: " << std::setw(8) << stats.avg() << "us"
          << "  P50: " << std::setw(8) << stats.p50() << "us"
          << "  P95: " << std::setw(8) << stats.p95() << "us"
          << "  P99: " << std::setw(8) << stats.p99() << "us\n";
    }
  }

  oss << "=== END READ CACHE METRICS REPORT ===\n";
  return oss.str();
}

}  // namespace ad_utility
