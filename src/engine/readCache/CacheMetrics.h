// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Lightweight cache metrics facade for read-through caching (EPIC 3)
// This facade provides a simple interface for cache operations to record
// metrics without pulling in the entire EpochMetrics infrastructure.

#ifndef QLEVER_SRC_ENGINE_READCACHE_CACHEMETRICS_H
#define QLEVER_SRC_ENGINE_READCACHE_CACHEMETRICS_H

#include <atomic>
#include <cstdint>
#include <random>
#include <string>

#include "util/Log.h"

namespace readCache {

// Sample rate for debug logging (1 in 100 operations)
inline constexpr int LOG_SAMPLE_RATE = 100;

// Lightweight metrics facade for cache operations
// This class provides thread-safe metric recording with minimal overhead
class CacheMetrics {
 public:
  CacheMetrics() = default;

  // Bytes cache operations
  static void recordBytesHit(uint64_t bytes);
  static void recordBytesMiss();
  static void recordBytesInsert(uint64_t bytes);
  static void recordBytesEvict(uint64_t bytes);

  // Plan cache operations
  static void recordPlanHit();
  static void recordPlanMiss();
  static void recordPlanInsert();
  static void recordPlanEvict();

  // Negative cache operations
  static void recordNegativeHit();
  static void recordNegativeInsert();

  // Inflight tracking
  static void recordInflightWaiterAdd();
  static void recordInflightWaiterRemove();

  // Per-shape latency tracking
  static void recordShapeLatency(const std::string& shape_sha256,
                                 uint64_t latency_us);

  // Epoch operations
  static void recordEpochPromote(uint64_t prewarm_ms);

  // Logging helpers with sampling
  // These methods log cache decisions at appropriate levels
  // with 1-in-100 sampling to avoid log spam

  // Log a cache hit (DEBUG level, sampled)
  static void logCacheHit(const std::string& cache_type,
                          const std::string& key_preview);

  // Log a cache miss (DEBUG level, sampled)
  static void logCacheMiss(const std::string& cache_type,
                           const std::string& key_preview);

  // Log an admission decision (DEBUG level, sampled)
  static void logAdmissionDecision(const std::string& cache_type,
                                   bool admitted, const std::string& reason);

  // Log an epoch promote (INFO level, always logged)
  static void logEpochPromote(uint64_t prewarm_ms);

  // Log prewarm completion (INFO level, always logged)
  static void logPrewarmComplete(uint64_t duration_ms, size_t items_warmed);

 private:
  // Thread-local random number generator for sampling
  static thread_local std::mt19937 rng_;
  static thread_local std::uniform_int_distribution<int> dist_;

  // Helper: should we log this operation? (1 in LOG_SAMPLE_RATE chance)
  static bool shouldLog();
};

}  // namespace readCache

#endif  // QLEVER_SRC_ENGINE_READCACHE_CACHEMETRICS_H
