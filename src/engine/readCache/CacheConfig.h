// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Configuration parameters for read-through caching (EPIC 3 Task 8)
// Defines tunable parameters for cache sizes, admission policies, and
// behavioral thresholds.

#ifndef QLEVER_SRC_ENGINE_READCACHE_CACHECONFIG_H
#define QLEVER_SRC_ENGINE_READCACHE_CACHECONFIG_H

#include <cstddef>
#include <cstdint>

#include "util/MemorySize.h"

namespace readCache {

// Configuration for read-through caching system
// All parameters are tunable via runtime configuration or command-line flags
struct CacheConfig {
  // ===== Cache Size Limits =====

  // Maximum total size for BytesCache (serialized query results)
  // Default: 2GB - balances memory usage with cache hit rate for typical
  // DBLP/Wikidata workloads
  ad_utility::MemorySize max_bytes_cache = ad_utility::MemorySize::gigabytes(2);

  // Maximum total size for PlanCache (query execution plans)
  // Default: 256MB - plans are much smaller than results, but still valuable
  ad_utility::MemorySize max_bytes_plan_cache =
      ad_utility::MemorySize::megabytes(256);

  // Maximum number of entries in NegativeCache (empty/failed queries)
  // Default: 64K entries - negative entries are tiny (just keys), so we can
  // afford many
  size_t max_entries_negative_cache = 64 * 1024;

  // ===== Admission Policy Parameters =====

  // Top-K most frequent query shapes to always admit
  // Default: 1000 - captures common queries without overwhelming the cache
  size_t top_k_shapes = 1000;

  // Minimum frequency threshold for bytes cache admission (per epoch)
  // Default: 2 - query must be seen at least twice OR be in top-K
  size_t freq_threshold_bytes = 2;

  // Minimum frequency threshold for plan cache admission (per epoch)
  // Default: 2 - same as bytes, plans benefit from similar admission logic
  size_t freq_threshold_plan = 2;

  // Minimum frequency threshold for negative cache admission (per epoch)
  // Default: 2 - avoid caching one-off empty results
  size_t freq_threshold_negative = 2;

  // ===== Per-Query Limits =====

  // Maximum size for a single query result to be cached
  // Default: 256MB - prevents a single huge result from evicting many smaller
  // ones
  ad_utility::MemorySize max_bytes_per_query =
      ad_utility::MemorySize::megabytes(256);

  // ===== Determinism Requirements =====

  // Require queries to be deterministic for bytes cache admission
  // Default: true - non-deterministic results (NOW(), RAND(), etc.) should not
  // be cached as they can produce incorrect results on cache hits
  bool require_deterministic_bytes = true;

  // Require queries to be deterministic for plan cache admission
  // Default: false - plans are reusable even if the query uses non-deterministic
  // functions (only the result varies, not the execution strategy)
  bool require_deterministic_plan = false;

  // Require queries to be deterministic for negative cache admission
  // Default: true - empty results should be consistent across executions
  bool require_deterministic_negative = true;

  // ===== Paging Parameters =====

  // Page size for BytePage chunking
  // Default: 256KB - balances granularity with overhead
  ad_utility::MemorySize page_size = ad_utility::MemorySize::kilobytes(256);

  // ===== Statistics and Monitoring =====

  // Enable detailed admission statistics logging
  // Default: false - can be enabled for debugging admission policy behavior
  bool enable_admission_stats = false;

  // Epoch duration for frequency tracking (seconds)
  // Default: 3600 (1 hour) - balance between responsiveness and stability
  uint64_t frequency_epoch_duration_seconds = 3600;

  // ===== Default Constructor =====
  CacheConfig() = default;

  // Validation - ensure configuration is sensible
  [[nodiscard]] bool isValid() const {
    return max_bytes_cache.getBytes() > 0 &&
           max_bytes_plan_cache.getBytes() > 0 &&
           max_entries_negative_cache > 0 && top_k_shapes > 0 &&
           freq_threshold_bytes > 0 && freq_threshold_plan > 0 &&
           freq_threshold_negative > 0 &&
           max_bytes_per_query.getBytes() > 0 &&
           max_bytes_per_query <= max_bytes_cache && page_size.getBytes() > 0 &&
           frequency_epoch_duration_seconds > 0;
  }
};

}  // namespace readCache

#endif  // QLEVER_SRC_ENGINE_READCACHE_CACHECONFIG_H
