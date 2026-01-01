// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: BenchmarkRealizationEngine implementation
// (EPIC 4 Subsystem 5: Production Benchmark Realization)

#include "engine/readPlane/BenchmarkRealizationEngine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace readPlane {

// =============================================================================
// Constructor
// =============================================================================
BenchmarkRealizationEngine::BenchmarkRealizationEngine(
    const WorkloadManifest* manifest,
    BenchmarkConfig config)
    : manifest_(manifest), config_(config) {
  // Invariant: manifest must not be null
  if (manifest_ == nullptr) {
    throw std::invalid_argument(
        "BenchmarkRealizationEngine: manifest cannot be null");
  }
}

// =============================================================================
// generateBenchmark (with explicit results)
// =============================================================================
BenchmarkResult BenchmarkRealizationEngine::generateBenchmark(
    const std::vector<ReplayResult>& replay_results,
    const std::vector<CacheDecision>& cache_decisions) {

  BenchmarkResult result;
  result.status = BenchmarkStatus::SUCCESS;

  // =========================================================================
  // Step 0: Validate inputs
  // =========================================================================

  // Check minimum samples
  if (replay_results.size() < config_.min_samples_required) {
    result.status = BenchmarkStatus::INSUFFICIENT_SAMPLES;
    result.error_message = "Insufficient samples: got " +
                           std::to_string(replay_results.size()) +
                           ", need " + std::to_string(config_.min_samples_required);
    result.samples_processed = 0;
    result.samples_skipped = replay_results.size();
    result.abort_artifact = generateAbortArtifact(
        "INSUFFICIENT_SAMPLES", result.error_message);
    return result;
  }

  // Validate epoch consistency
  if (!validateEpochConsistency(replay_results)) {
    result.status = BenchmarkStatus::EPOCH_MISMATCH;
    result.error_message = "Epoch mismatch: replay results from different epochs";
    result.abort_artifact = generateAbortArtifact(
        "EPOCH_MISMATCH", result.error_message);
    return result;
  }

  // Check for divergence (fail-closed)
  if (checkForDivergence(replay_results)) {
    result.status = BenchmarkStatus::DIVERGENCE_DETECTED;
    result.error_message = "Execution divergence detected in replay results";
    result.abort_artifact = generateAbortArtifact(
        "DIVERGENCE_DETECTED", result.error_message);
    return result;
  }

  // =========================================================================
  // Step 1: Compute PerformanceEnvelope
  // =========================================================================

  try {
    PerformanceEnvelope envelope = PerformanceEnvelope::compute(
        *manifest_, replay_results, cache_decisions);

    // Track statistics
    result.samples_processed = 0;
    result.samples_skipped = 0;

    for (const auto& replay : replay_results) {
      if (replay.execution_status == ReplayStatus::SUCCESS) {
        result.samples_processed++;
      } else {
        result.samples_skipped++;
      }
    }

    // =========================================================================
    // Step 2: Final validation - check if we have enough successful samples
    // =========================================================================
    if (result.samples_processed < config_.min_samples_required) {
      result.status = BenchmarkStatus::INSUFFICIENT_SAMPLES;
      result.error_message = "Insufficient successful samples after filtering: " +
                             std::to_string(result.samples_processed);
      result.abort_artifact = generateAbortArtifact(
          "INSUFFICIENT_SAMPLES", result.error_message);
      return result;
    }

    // =========================================================================
    // Step 3: Store envelope and return success
    // =========================================================================
    result.envelope = envelope;
    last_envelope_ = envelope;
    result.status = BenchmarkStatus::SUCCESS;

  } catch (const std::exception& e) {
    result.status = BenchmarkStatus::INTERNAL_ERROR;
    result.error_message = std::string("Exception during benchmark: ") + e.what();
    result.abort_artifact = generateAbortArtifact("INTERNAL_ERROR", e.what());
  }

  return result;
}

// =============================================================================
// generateBenchmark (using accumulated data)
// =============================================================================
BenchmarkResult BenchmarkRealizationEngine::generateBenchmark() {
  return generateBenchmark(accumulated_replay_results_, accumulated_cache_decisions_);
}

// =============================================================================
// compareToBaseline
// =============================================================================
RegressionReport BenchmarkRealizationEngine::compareToBaseline(
    const PerformanceEnvelope& baseline_envelope) {

  if (!last_envelope_.has_value()) {
    RegressionReport report;
    report.is_regression = false;
    report.summary = "NO_ENVELOPE: Cannot compare without generated envelope";
    return report;
  }

  // Use PerformanceEnvelope's built-in regression detection
  return last_envelope_->detectRegression(baseline_envelope);
}

// =============================================================================
// addReplayResults
// =============================================================================
void BenchmarkRealizationEngine::addReplayResults(
    const std::vector<ReplayResult>& results) {
  accumulated_replay_results_.insert(
      accumulated_replay_results_.end(), results.begin(), results.end());
}

// =============================================================================
// addCacheDecisions
// =============================================================================
void BenchmarkRealizationEngine::addCacheDecisions(
    const std::vector<CacheDecision>& decisions) {
  accumulated_cache_decisions_.insert(
      accumulated_cache_decisions_.end(), decisions.begin(), decisions.end());
}

// =============================================================================
// clearAccumulatedData
// =============================================================================
void BenchmarkRealizationEngine::clearAccumulatedData() {
  accumulated_replay_results_.clear();
  accumulated_cache_decisions_.clear();
}

// =============================================================================
// exportJsonLD
// =============================================================================
std::string BenchmarkRealizationEngine::exportJsonLD() const {
  if (!last_envelope_.has_value()) {
    return "";
  }
  return last_envelope_->toJsonLD().dump(2);
}

// =============================================================================
// diffEnvelopes (static)
// =============================================================================
std::string BenchmarkRealizationEngine::diffEnvelopes(
    const PerformanceEnvelope& current,
    const PerformanceEnvelope& baseline) {

  nlohmann::ordered_json j;
  j["@type"] = "EnvelopeDiff";

  // Latency diff
  nlohmann::ordered_json latency_diff;
  latency_diff["max_ns_delta"] = static_cast<int64_t>(current.latency_stats.max_ns) -
                                  static_cast<int64_t>(baseline.latency_stats.max_ns);
  latency_diff["p50_ns_delta"] = static_cast<int64_t>(current.latency_stats.p50_ns) -
                                  static_cast<int64_t>(baseline.latency_stats.p50_ns);
  latency_diff["p95_ns_delta"] = static_cast<int64_t>(current.latency_stats.p95_ns) -
                                  static_cast<int64_t>(baseline.latency_stats.p95_ns);
  latency_diff["p99_ns_delta"] = static_cast<int64_t>(current.latency_stats.p99_ns) -
                                  static_cast<int64_t>(baseline.latency_stats.p99_ns);
  j["latency_diff"] = latency_diff;

  // Cache stats diff
  nlohmann::ordered_json cache_diff;
  cache_diff["bytes_hit_rate_delta"] =
      current.cache_stats.bytes_hit_rate - baseline.cache_stats.bytes_hit_rate;
  cache_diff["plan_hit_rate_delta"] =
      current.cache_stats.plan_hit_rate - baseline.cache_stats.plan_hit_rate;
  j["cache_stats_diff"] = cache_diff;

  // Stability diff
  nlohmann::ordered_json stability_diff;
  stability_diff["cv_delta"] =
      current.stability.latency_coefficient_of_variation -
      baseline.stability.latency_coefficient_of_variation;
  stability_diff["plan_reuse_rate_delta"] =
      current.stability.plan_reuse_rate - baseline.stability.plan_reuse_rate;
  j["stability_diff"] = stability_diff;

  // Regression check
  RegressionReport report = current.detectRegression(baseline);
  j["regression_report"] = report.toJsonLD();

  return j.dump(2);
}

// =============================================================================
// validateEpochConsistency (private)
// =============================================================================
bool BenchmarkRealizationEngine::validateEpochConsistency(
    const std::vector<ReplayResult>& results) const {

  if (results.empty()) {
    return true;  // No results to validate
  }

  // Per EPIC4 spec, replay can happen on different epoch
  // We just need consistency within the replay set
  EpochKey reference_epoch = results[0].replayed_on_epoch;

  for (const auto& result : results) {
    if (result.replayed_on_epoch.epoch_id != reference_epoch.epoch_id) {
      return false;
    }
  }

  return true;
}

// =============================================================================
// checkForDivergence (private)
// =============================================================================
bool BenchmarkRealizationEngine::checkForDivergence(
    const std::vector<ReplayResult>& results) const {

  // Per EPIC4 spec: If digest_matches == false, execution ABORTS
  for (const auto& result : results) {
    if (result.execution_status == ReplayStatus::DIVERGENCE) {
      return true;  // Divergence detected
    }
    if (!result.digest_matches && result.execution_status != ReplayStatus::ABORT) {
      // Digest mismatch without explicit abort is also divergence
      return true;
    }
  }

  return false;
}

// =============================================================================
// extractLatencies (private)
// =============================================================================
std::vector<uint64_t> BenchmarkRealizationEngine::extractLatencies(
    const std::vector<ReplayResult>& results) const {

  std::vector<uint64_t> latencies;
  latencies.reserve(results.size());

  for (const auto& result : results) {
    if (result.execution_status != ReplayStatus::SUCCESS) {
      continue;
    }

    uint64_t start_ns = 0;
    uint64_t complete_ns = 0;

    for (const auto& event : result.trace_events) {
      if (event.event_type == TraceEventType::QUERY_START) {
        start_ns = event.wall_clock_ns;
      } else if (event.event_type == TraceEventType::QUERY_COMPLETE) {
        complete_ns = event.wall_clock_ns;
      }
    }

    if (start_ns > 0 && complete_ns > start_ns) {
      uint64_t latency_ns = complete_ns - start_ns;
      // Round to microsecond precision
      latency_ns = (latency_ns / 1000) * 1000;
      latencies.push_back(latency_ns);
    }
  }

  return latencies;
}

// =============================================================================
// computePercentile (private static)
// =============================================================================
uint64_t BenchmarkRealizationEngine::computePercentile(
    const std::vector<uint64_t>& sorted, double percentile) {

  if (sorted.empty()) {
    return 0;
  }

  size_t index = static_cast<size_t>(sorted.size() * percentile / 100.0);
  if (index >= sorted.size()) {
    index = sorted.size() - 1;
  }

  return sorted[index];
}

// =============================================================================
// computeStdDev (private static)
// =============================================================================
double BenchmarkRealizationEngine::computeStdDev(
    const std::vector<uint64_t>& values, double mean) {

  if (values.empty() || mean <= 0) {
    return 0.0;
  }

  double sq_sum = 0.0;
  for (uint64_t val : values) {
    double diff = static_cast<double>(val) - mean;
    sq_sum += diff * diff;
  }

  double variance = sq_sum / values.size();
  return std::sqrt(variance);
}

// =============================================================================
// generateAbortArtifact (private)
// =============================================================================
std::string BenchmarkRealizationEngine::generateAbortArtifact(
    const std::string& reason,
    const std::string& details) const {

  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()).count();

  nlohmann::ordered_json j;
  j["@type"] = "BenchmarkAbortArtifact";
  j["details"] = details;
  j["epoch_id"] = manifest_->epoch_id;
  j["reason"] = reason;
  j["timestamp_ms"] = timestamp;
  j["workload_id"] = manifest_->id;

  return j.dump(2);
}

}  // namespace readPlane
