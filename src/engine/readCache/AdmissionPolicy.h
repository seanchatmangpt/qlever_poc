// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Admission policy for read-through caching (EPIC 3 Task 8)
// Implements intelligent admission decisions based on query shape popularity,
// determinism, and resource constraints.

#ifndef QLEVER_SRC_ENGINE_READCACHE_ADMISSIONPOLICY_H
#define QLEVER_SRC_ENGINE_READCACHE_ADMISSIONPOLICY_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/queryCanonical/QueryFingerprint.h"
#include "engine/readCache/CacheConfig.h"
#include "engine/readCache/ReadCacheKeys.h"
#include "util/Synchronized.h"

namespace readCache {

// QueryContext - Contextual information for admission decisions
// Provides all necessary information for the admission policy to make
// intelligent caching decisions.
struct QueryContext {
  // Query fingerprint from EPIC 2 (contains determinism info)
  queryCanonical::QueryFingerprint fingerprint;

  // Current cache sizes (for budget enforcement)
  size_t current_bytes_cache_size = 0;
  size_t current_plan_cache_size = 0;
  size_t current_negative_cache_entries = 0;

  // Result size (for per-query limit checks)
  size_t result_size_bytes = 0;

  // Query shape hash (for frequency tracking)
  std::string shape_sha256;

  QueryContext() = default;
};

// FrequencyTracker - Thread-safe tracking of query shape popularity
// Tracks how many times each query shape has been seen in the current epoch.
// Uses sharding to reduce contention on high-frequency workloads.
class FrequencyTracker {
 public:
  explicit FrequencyTracker(size_t num_shards = 16);

  // Record observation of a query shape
  void recordShape(const std::string& shape_sha256);

  // Get frequency count for a shape
  [[nodiscard]] size_t getFrequency(const std::string& shape_sha256) const;

  // Get top-K most frequent shapes
  [[nodiscard]] std::vector<std::string> getTopKShapes(size_t k) const;

  // Check if shape is in top-K
  [[nodiscard]] bool isInTopK(const std::string& shape_sha256, size_t k) const;

  // Reset all frequencies (for new epoch)
  void reset();

  // Get total number of unique shapes tracked
  [[nodiscard]] size_t numUniqueShapes() const;

  // Get total number of observations across all shapes
  [[nodiscard]] size_t totalObservations() const;

 private:
  // Sharded frequency maps to reduce contention
  struct Shard {
    mutable std::mutex mutex;
    std::unordered_map<std::string, std::atomic<size_t>> frequencies;
  };

  std::vector<std::unique_ptr<Shard>> shards_;
  size_t num_shards_;

  // Get shard index for a shape hash
  [[nodiscard]] size_t getShardIndex(const std::string& shape_sha256) const;

  // Get shard for a shape (const version)
  [[nodiscard]] const Shard& getShard(const std::string& shape_sha256) const;

  // Get shard for a shape (non-const version)
  [[nodiscard]] Shard& getShard(const std::string& shape_sha256);
};

// AdmissionPolicy - Intelligent cache admission decision engine
// Decides which queries and results should be cached based on:
// - Query determinism (from EPIC 2 QueryFingerprint)
// - Shape popularity (top-K tracking)
// - Frequency thresholds
// - Resource constraints
class AdmissionPolicy {
 public:
  explicit AdmissionPolicy(CacheConfig config);

  // ===== Admission Decisions =====

  // Should we cache serialized query results (BytesCache)?
  // Requirements:
  // - Query must be deterministic (if config requires)
  // - Result size must be within per-query limit
  // - Shape must be in top-K OR frequency >= threshold
  [[nodiscard]] bool shouldAdmitBytes(const QueryContext& ctx,
                                      const BytesKey& key,
                                      uint64_t size) const;

  // Should we cache query execution plan (PlanCache)?
  // Requirements (less strict than bytes):
  // - Determinism check skipped (plans are reusable)
  // - Shape must be in top-K OR frequency >= threshold
  [[nodiscard]] bool shouldAdmitPlan(const QueryContext& ctx,
                                     const PlanKey& key) const;

  // Should we cache empty/failed result indicator (NegativeCache)?
  // Requirements:
  // - Query must be deterministic (if config requires)
  // - Shape must be in top-K (don't cache obscure empty results)
  [[nodiscard]] bool shouldAdmitEmpty(const QueryContext& ctx,
                                      const NegKey& key) const;

  // ===== Shape Frequency Tracking =====

  // Record observation of a query shape
  void recordQueryShape(const std::string& shape_sha256);

  // Check if shape meets frequency threshold for bytes cache
  [[nodiscard]] bool meetsFrequencyThresholdBytes(
      const std::string& shape_sha256) const;

  // Check if shape meets frequency threshold for plan cache
  [[nodiscard]] bool meetsFrequencyThresholdPlan(
      const std::string& shape_sha256) const;

  // Check if shape is in top-K
  [[nodiscard]] bool isInTopK(const std::string& shape_sha256) const;

  // ===== Determinism Checking =====

  // Check if query is deterministic (from QueryFingerprint)
  [[nodiscard]] static bool isDeterministic(
      const queryCanonical::QueryFingerprint& fingerprint);

  // ===== Statistics & Monitoring =====

  // Get current admission statistics
  struct AdmissionStats {
    size_t bytes_admitted = 0;
    size_t bytes_rejected = 0;
    size_t plan_admitted = 0;
    size_t plan_rejected = 0;
    size_t negative_admitted = 0;
    size_t negative_rejected = 0;
    size_t unique_shapes = 0;
    size_t total_observations = 0;
  };

  [[nodiscard]] AdmissionStats getStats() const;

  // Reset statistics
  void resetStats();

  // Reset frequency tracking (new epoch)
  void resetFrequencies();

  // ===== Configuration Access =====

  [[nodiscard]] const CacheConfig& config() const { return config_; }

 private:
  // Configuration
  CacheConfig config_;

  // Frequency tracking
  std::unique_ptr<FrequencyTracker> frequency_tracker_;

  // Admission statistics (mutable for const methods)
  mutable ad_utility::Synchronized<AdmissionStats> stats_;

  // ===== Helper Methods =====

  // Check size constraints for bytes cache
  [[nodiscard]] bool checkSizeConstraints(const QueryContext& ctx,
                                          uint64_t size) const;

  // Update admission statistics
  void recordAdmission(bool admitted, bool is_bytes, bool is_plan,
                       bool is_negative) const;
};

}  // namespace readCache

#endif  // QLEVER_SRC_ENGINE_READCACHE_ADMISSIONPOLICY_H
