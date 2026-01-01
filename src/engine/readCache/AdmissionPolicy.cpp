// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Implementation of admission policy for read-through caching
// (EPIC 3 Task 8)

#include "engine/readCache/AdmissionPolicy.h"

#include <absl/hash/hash.h>

#include <algorithm>
#include <mutex>
#include <vector>

namespace readCache {

// ============================================================================
// FrequencyTracker Implementation
// ============================================================================

FrequencyTracker::FrequencyTracker(size_t num_shards)
    : num_shards_(num_shards) {
  shards_.reserve(num_shards);
  for (size_t i = 0; i < num_shards; ++i) {
    shards_.push_back(std::make_unique<Shard>());
  }
}

// ____________________________________________________________________________
size_t FrequencyTracker::getShardIndex(const std::string& shape_sha256) const {
  // Use hash of the shape to distribute across shards
  size_t hash = absl::Hash<std::string>{}(shape_sha256);
  return hash % num_shards_;
}

// ____________________________________________________________________________
const FrequencyTracker::Shard& FrequencyTracker::getShard(
    const std::string& shape_sha256) const {
  return *shards_[getShardIndex(shape_sha256)];
}

// ____________________________________________________________________________
FrequencyTracker::Shard& FrequencyTracker::getShard(
    const std::string& shape_sha256) {
  return *shards_[getShardIndex(shape_sha256)];
}

// ____________________________________________________________________________
void FrequencyTracker::recordShape(const std::string& shape_sha256) {
  auto& shard = getShard(shape_sha256);
  std::lock_guard<std::mutex> lock(shard.mutex);

  // Find or create the frequency counter for this shape
  auto it = shard.frequencies.find(shape_sha256);
  if (it != shard.frequencies.end()) {
    // Increment existing counter
    it->second.fetch_add(1, std::memory_order_relaxed);
  } else {
    // Create new counter starting at 1
    shard.frequencies.emplace(shape_sha256, std::atomic<size_t>(1));
  }
}

// ____________________________________________________________________________
size_t FrequencyTracker::getFrequency(const std::string& shape_sha256) const {
  const auto& shard = getShard(shape_sha256);
  std::lock_guard<std::mutex> lock(shard.mutex);

  auto it = shard.frequencies.find(shape_sha256);
  if (it != shard.frequencies.end()) {
    return it->second.load(std::memory_order_relaxed);
  }
  return 0;
}

// ____________________________________________________________________________
std::vector<std::string> FrequencyTracker::getTopKShapes(size_t k) const {
  // Collect all shapes and their frequencies from all shards
  std::vector<std::pair<std::string, size_t>> all_shapes;

  for (const auto& shard_ptr : shards_) {
    std::lock_guard<std::mutex> lock(shard_ptr->mutex);
    for (const auto& [shape, freq] : shard_ptr->frequencies) {
      all_shapes.emplace_back(shape, freq.load(std::memory_order_relaxed));
    }
  }

  // Sort by frequency (descending)
  std::sort(all_shapes.begin(), all_shapes.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  // Take top K
  std::vector<std::string> top_k;
  top_k.reserve(std::min(k, all_shapes.size()));
  for (size_t i = 0; i < std::min(k, all_shapes.size()); ++i) {
    top_k.push_back(all_shapes[i].first);
  }

  return top_k;
}

// ____________________________________________________________________________
bool FrequencyTracker::isInTopK(const std::string& shape_sha256,
                                size_t k) const {
  // Get top-K shapes
  auto top_k = getTopKShapes(k);

  // Check if shape is in top-K
  return std::find(top_k.begin(), top_k.end(), shape_sha256) != top_k.end();
}

// ____________________________________________________________________________
void FrequencyTracker::reset() {
  for (auto& shard_ptr : shards_) {
    std::lock_guard<std::mutex> lock(shard_ptr->mutex);
    shard_ptr->frequencies.clear();
  }
}

// ____________________________________________________________________________
size_t FrequencyTracker::numUniqueShapes() const {
  size_t total = 0;
  for (const auto& shard_ptr : shards_) {
    std::lock_guard<std::mutex> lock(shard_ptr->mutex);
    total += shard_ptr->frequencies.size();
  }
  return total;
}

// ____________________________________________________________________________
size_t FrequencyTracker::totalObservations() const {
  size_t total = 0;
  for (const auto& shard_ptr : shards_) {
    std::lock_guard<std::mutex> lock(shard_ptr->mutex);
    for (const auto& [shape, freq] : shard_ptr->frequencies) {
      total += freq.load(std::memory_order_relaxed);
    }
  }
  return total;
}

// ============================================================================
// AdmissionPolicy Implementation
// ============================================================================

AdmissionPolicy::AdmissionPolicy(CacheConfig config)
    : config_(std::move(config)),
      frequency_tracker_(std::make_unique<FrequencyTracker>()) {
  // Validate configuration
  if (!config_.isValid()) {
    throw std::invalid_argument(
        "AdmissionPolicy: Invalid cache configuration");
  }
}

// ____________________________________________________________________________
bool AdmissionPolicy::isDeterministic(
    const queryCanonical::QueryFingerprint& fingerprint) {
  // Check if the NONDETERMINISTIC_RESULT flag is set
  return !queryCanonical::hasFeature(
      fingerprint.feature_flags,
      queryCanonical::QueryFeatureFlag::NONDETERMINISTIC_RESULT);
}

// ____________________________________________________________________________
bool AdmissionPolicy::checkSizeConstraints(const QueryContext& ctx,
                                           uint64_t size) const {
  // Check per-query size limit
  if (size > config_.max_bytes_per_query.getBytes()) {
    return false;
  }

  // Total cache budget check is handled by the cache itself via LRU eviction
  // We just provide a veto here for oversized individual queries
  return true;
}

// ____________________________________________________________________________
bool AdmissionPolicy::shouldAdmitBytes(const QueryContext& ctx,
                                       const BytesKey& key,
                                       uint64_t size) const {
  // Check 1: Determinism requirement
  if (config_.require_deterministic_bytes &&
      !isDeterministic(ctx.fingerprint)) {
    recordAdmission(false, true, false, false);
    return false;
  }

  // Check 2: Size constraints
  if (!checkSizeConstraints(ctx, size)) {
    recordAdmission(false, true, false, false);
    return false;
  }

  // Check 3: Shape popularity (top-K OR frequency threshold)
  bool is_popular = isInTopK(ctx.shape_sha256) ||
                    meetsFrequencyThresholdBytes(ctx.shape_sha256);

  if (!is_popular) {
    recordAdmission(false, true, false, false);
    return false;
  }

  // All checks passed - admit
  recordAdmission(true, true, false, false);
  return true;
}

// ____________________________________________________________________________
bool AdmissionPolicy::shouldAdmitPlan(const QueryContext& ctx,
                                      const PlanKey& key) const {
  // Plans are less strict than bytes:
  // - No determinism check (plans are reusable even for non-deterministic
  // queries)
  // - No size constraints (plans are small)

  // Check: Shape popularity (top-K OR frequency threshold)
  bool is_popular = isInTopK(ctx.shape_sha256) ||
                    meetsFrequencyThresholdPlan(ctx.shape_sha256);

  recordAdmission(is_popular, false, true, false);
  return is_popular;
}

// ____________________________________________________________________________
bool AdmissionPolicy::shouldAdmitEmpty(const QueryContext& ctx,
                                       const NegKey& key) const {
  // Check 1: Determinism requirement
  if (config_.require_deterministic_negative &&
      !isDeterministic(ctx.fingerprint)) {
    recordAdmission(false, false, false, true);
    return false;
  }

  // Check 2: Shape must be in top-K (stricter than bytes/plan)
  // We don't want to cache obscure empty results
  bool is_in_top_k = isInTopK(ctx.shape_sha256);

  recordAdmission(is_in_top_k, false, false, true);
  return is_in_top_k;
}

// ____________________________________________________________________________
void AdmissionPolicy::recordQueryShape(const std::string& shape_sha256) {
  frequency_tracker_->recordShape(shape_sha256);
}

// ____________________________________________________________________________
bool AdmissionPolicy::meetsFrequencyThresholdBytes(
    const std::string& shape_sha256) const {
  size_t freq = frequency_tracker_->getFrequency(shape_sha256);
  return freq >= config_.freq_threshold_bytes;
}

// ____________________________________________________________________________
bool AdmissionPolicy::meetsFrequencyThresholdPlan(
    const std::string& shape_sha256) const {
  size_t freq = frequency_tracker_->getFrequency(shape_sha256);
  return freq >= config_.freq_threshold_plan;
}

// ____________________________________________________________________________
bool AdmissionPolicy::isInTopK(const std::string& shape_sha256) const {
  return frequency_tracker_->isInTopK(shape_sha256, config_.top_k_shapes);
}

// ____________________________________________________________________________
void AdmissionPolicy::recordAdmission(bool admitted, bool is_bytes,
                                      bool is_plan, bool is_negative) const {
  if (!config_.enable_admission_stats) {
    return;
  }

  stats_.wlock()->visit([&](AdmissionStats& stats) {
    if (is_bytes) {
      if (admitted) {
        stats.bytes_admitted++;
      } else {
        stats.bytes_rejected++;
      }
    } else if (is_plan) {
      if (admitted) {
        stats.plan_admitted++;
      } else {
        stats.plan_rejected++;
      }
    } else if (is_negative) {
      if (admitted) {
        stats.negative_admitted++;
      } else {
        stats.negative_rejected++;
      }
    }
  });
}

// ____________________________________________________________________________
AdmissionPolicy::AdmissionStats AdmissionPolicy::getStats() const {
  return stats_.rlock()->visit([](const AdmissionStats& stats) {
    AdmissionStats copy = stats;
    return copy;
  });
}

// ____________________________________________________________________________
void AdmissionPolicy::resetStats() {
  stats_.wlock()->visit([](AdmissionStats& stats) {
    stats.bytes_admitted = 0;
    stats.bytes_rejected = 0;
    stats.plan_admitted = 0;
    stats.plan_rejected = 0;
    stats.negative_admitted = 0;
    stats.negative_rejected = 0;
    stats.unique_shapes = 0;
    stats.total_observations = 0;
  });
}

// ____________________________________________________________________________
void AdmissionPolicy::resetFrequencies() { frequency_tracker_->reset(); }

}  // namespace readCache
