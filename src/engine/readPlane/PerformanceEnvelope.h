// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: PerformanceEnvelope - Aggregated benchmark metrics for workload
// replay (EPIC 4 Subsystem 5: Production Benchmark Realization)
//
// INVARIANTS (from EPIC4_SHARED_INVARIANTS.md):
// - Epoch binding: All metrics tied to specific epoch via EpochKey
// - Determinism: Derived from captured deterministic workloads only
// - Replayability: Same workload on same machine -> same latency distribution
// - Variance bounds: Pre-defined, not human-interpreted
// - Regression detection: Automatic, no judgment calls

#ifndef QLEVER_SRC_ENGINE_READPLANE_PERFORMANCEENVELOPE_H
#define QLEVER_SRC_ENGINE_READPLANE_PERFORMANCEENVELOPE_H

#include <absl/strings/str_join.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

#include "engine/readPlane/CacheDecision.h"
#include "engine/readPlane/ExecutionTraceDigest.h"
#include "engine/readPlane/ReplayResult.h"
#include "util/CryptographicHashUtils.h"
#include "util/json.h"

namespace readPlane {

// =============================================================================
// Latency Distribution Statistics (nanoseconds)
// =============================================================================
struct LatencyDistribution {
  uint64_t p50_ns = 0;
  uint64_t p95_ns = 0;
  uint64_t p99_ns = 0;
  uint64_t max_ns = 0;

  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "LatencyDistribution";
    j["max_ns"] = max_ns;
    j["p50_ns"] = p50_ns;
    j["p95_ns"] = p95_ns;
    j["p99_ns"] = p99_ns;
    return j;
  }
};

// =============================================================================
// Cache Efficiency Statistics
// =============================================================================
struct CacheEfficiencyStats {
  uint64_t bytes_hits = 0;
  uint64_t bytes_misses = 0;
  uint64_t plan_hits = 0;
  uint64_t plan_misses = 0;
  uint64_t neg_hits = 0;
  double bytes_hit_rate = 0.0;  // bytes_hits / (bytes_hits + bytes_misses)
  double plan_hit_rate = 0.0;   // plan_hits / (plan_hits + plan_misses)

  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "CacheEfficiencyStats";
    j["bytes_hit_rate"] = bytes_hit_rate;
    j["bytes_hits"] = bytes_hits;
    j["bytes_misses"] = bytes_misses;
    j["neg_hits"] = neg_hits;
    j["plan_hit_rate"] = plan_hit_rate;
    j["plan_hits"] = plan_hits;
    j["plan_misses"] = plan_misses;
    return j;
  }
};

// =============================================================================
// Stability Metrics for Regression Detection
// =============================================================================
struct StabilityMetrics {
  double latency_coefficient_of_variation = 0.0;  // sigma / mu
  double plan_reuse_rate = 0.0;                   // plan_hits / total_queries
  double epoch_transition_overhead_pct = 0.0;     // (after - before) / before

  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "StabilityMetrics";
    j["epoch_transition_overhead_pct"] = epoch_transition_overhead_pct;
    j["latency_coefficient_of_variation"] = latency_coefficient_of_variation;
    j["plan_reuse_rate"] = plan_reuse_rate;
    return j;
  }
};

// =============================================================================
// Variance Bounds for CI Regression Detection
// =============================================================================
struct VarianceBounds {
  // Hardcoded thresholds (not configurable per spec)
  static constexpr uint64_t ACCEPTABLE_LATENCY_VARIANCE_PCT = 10;
  static constexpr uint64_t ACCEPTABLE_CACHE_HIT_RATE_CHANGE = 5;

  uint64_t acceptable_latency_variance_pct = ACCEPTABLE_LATENCY_VARIANCE_PCT;
  uint64_t acceptable_cache_hit_rate_change = ACCEPTABLE_CACHE_HIT_RATE_CHANGE;
  bool is_regression = false;

  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "VarianceBounds";
    j["acceptable_cache_hit_rate_change"] = acceptable_cache_hit_rate_change;
    j["acceptable_latency_variance_pct"] = acceptable_latency_variance_pct;
    j["is_regression"] = is_regression;
    return j;
  }
};

// =============================================================================
// Regression Detection Report
// =============================================================================
struct RegressionReport {
  bool is_regression = false;
  bool latency_variance_exceeded = false;
  bool cache_hit_rate_exceeded = false;
  double latency_cv_current = 0.0;
  double latency_cv_baseline = 0.0;
  double bytes_hit_rate_change_pct = 0.0;
  double plan_hit_rate_change_pct = 0.0;
  std::string summary;

  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "RegressionReport";
    j["bytes_hit_rate_change_pct"] = bytes_hit_rate_change_pct;
    j["cache_hit_rate_exceeded"] = cache_hit_rate_exceeded;
    j["is_regression"] = is_regression;
    j["latency_cv_baseline"] = latency_cv_baseline;
    j["latency_cv_current"] = latency_cv_current;
    j["latency_variance_exceeded"] = latency_variance_exceeded;
    j["plan_hit_rate_change_pct"] = plan_hit_rate_change_pct;
    j["summary"] = summary;
    return j;
  }
};

// =============================================================================
// PerformanceEnvelope: Main artifact for benchmark realization
// =============================================================================
struct PerformanceEnvelope {
  // Identity
  std::string workload_id;    // Which WorkloadManifest
  EpochKey epoch_key;

  // Latency distribution (deterministic from replay)
  LatencyDistribution latency_stats;

  // Cache efficiency (aggregated from CacheDecision)
  CacheEfficiencyStats cache_stats;

  // Stability metrics (for regression detection)
  StabilityMetrics stability;

  // Variance bounds (for CI)
  VarianceBounds variance_bounds;

  // Computed epoch transition detected during benchmark
  bool epoch_transition_detected = false;

  // Envelope creation timestamp (informational, excluded from digest)
  uint64_t created_timestamp_ms = 0;

  // ==========================================================================
  // Static Factory Method: compute from WorkloadManifest and ReplayResults
  // ==========================================================================
  static PerformanceEnvelope compute(
      const WorkloadManifest& manifest,
      const std::vector<ReplayResult>& replay_results,
      const std::vector<CacheDecision>& cache_decisions) {

    PerformanceEnvelope envelope;
    envelope.workload_id = manifest.id;
    envelope.epoch_key = EpochKey(manifest.epoch_id, manifest.epoch_manifest_sha256);

    // ========================================================================
    // Step 1: Extract latencies from trace events
    // ========================================================================
    std::vector<uint64_t> latencies_ns;
    latencies_ns.reserve(replay_results.size());

    for (const auto& result : replay_results) {
      if (result.execution_status != ReplayStatus::SUCCESS) {
        continue;  // Skip failed replays
      }

      uint64_t query_start_ns = 0;
      uint64_t query_complete_ns = 0;

      for (const auto& event : result.trace_events) {
        if (event.event_type == TraceEventType::QUERY_START) {
          query_start_ns = event.wall_clock_ns;
        } else if (event.event_type == TraceEventType::QUERY_COMPLETE) {
          query_complete_ns = event.wall_clock_ns;
        }
      }

      if (query_start_ns > 0 && query_complete_ns > query_start_ns) {
        uint64_t latency_ns = query_complete_ns - query_start_ns;
        // Round to microsecond precision (divide by 1000, round, multiply by 1000)
        latency_ns = (latency_ns / 1000) * 1000;
        latencies_ns.push_back(latency_ns);
      }
    }

    // ========================================================================
    // Step 2: Compute latency distribution (p50, p95, p99, max)
    // ========================================================================
    if (!latencies_ns.empty()) {
      std::vector<uint64_t> sorted = latencies_ns;
      std::sort(sorted.begin(), sorted.end());

      size_t n = sorted.size();
      envelope.latency_stats.p50_ns = sorted[n * 50 / 100];
      envelope.latency_stats.p95_ns = sorted[n * 95 / 100];
      envelope.latency_stats.p99_ns = sorted[n * 99 / 100];
      envelope.latency_stats.max_ns = sorted.back();
    }

    // ========================================================================
    // Step 3: Aggregate cache efficiency from CacheDecision artifacts
    // ========================================================================
    for (const auto& decision : cache_decisions) {
      switch (decision.decision_type) {
        case DecisionType::BYTES_HIT:
          envelope.cache_stats.bytes_hits++;
          break;
        case DecisionType::BYTES_MISS:
          envelope.cache_stats.bytes_misses++;
          break;
        case DecisionType::PLAN_HIT:
          envelope.cache_stats.plan_hits++;
          break;
        case DecisionType::PLAN_MISS:
          envelope.cache_stats.plan_misses++;
          break;
        case DecisionType::NEG_HIT:
          envelope.cache_stats.neg_hits++;
          break;
        default:
          break;  // Ignore inserts/evicts for hit rate calculation
      }
    }

    // Compute hit rates
    uint64_t bytes_total = envelope.cache_stats.bytes_hits + envelope.cache_stats.bytes_misses;
    if (bytes_total > 0) {
      envelope.cache_stats.bytes_hit_rate =
          static_cast<double>(envelope.cache_stats.bytes_hits) / bytes_total;
    }

    uint64_t plan_total = envelope.cache_stats.plan_hits + envelope.cache_stats.plan_misses;
    if (plan_total > 0) {
      envelope.cache_stats.plan_hit_rate =
          static_cast<double>(envelope.cache_stats.plan_hits) / plan_total;
    }

    // ========================================================================
    // Step 4: Compute stability metrics
    // ========================================================================

    // Coefficient of variation = stdev / mean
    if (!latencies_ns.empty()) {
      double sum = std::accumulate(latencies_ns.begin(), latencies_ns.end(), 0.0);
      double mean = sum / latencies_ns.size();

      if (mean > 0) {
        double sq_sum = 0.0;
        for (uint64_t lat : latencies_ns) {
          double diff = static_cast<double>(lat) - mean;
          sq_sum += diff * diff;
        }
        double variance = sq_sum / latencies_ns.size();
        double stdev = std::sqrt(variance);
        envelope.stability.latency_coefficient_of_variation = stdev / mean;
      }
    }

    // Plan reuse rate = plan_hits / total_queries
    if (!replay_results.empty()) {
      envelope.stability.plan_reuse_rate =
          static_cast<double>(envelope.cache_stats.plan_hits) / replay_results.size();
    }

    // Epoch transition overhead detection
    EpochKey first_epoch;
    uint64_t pre_transition_sum_ns = 0;
    uint64_t post_transition_sum_ns = 0;
    size_t pre_count = 0;
    size_t post_count = 0;
    bool transition_found = false;

    for (size_t i = 0; i < replay_results.size(); ++i) {
      const auto& result = replay_results[i];
      if (i == 0) {
        first_epoch = result.replayed_on_epoch;
      }
      if (result.replayed_on_epoch.epoch_id != first_epoch.epoch_id && !transition_found) {
        transition_found = true;
        envelope.epoch_transition_detected = true;
      }

      // Extract latency from this result
      uint64_t result_latency_ns = result.execution_duration_ns;

      if (!transition_found) {
        pre_transition_sum_ns += result_latency_ns;
        pre_count++;
      } else {
        post_transition_sum_ns += result_latency_ns;
        post_count++;
      }
    }

    if (transition_found && pre_count > 0 && post_count > 0) {
      double pre_avg = static_cast<double>(pre_transition_sum_ns) / pre_count;
      double post_avg = static_cast<double>(post_transition_sum_ns) / post_count;
      if (pre_avg > 0) {
        envelope.stability.epoch_transition_overhead_pct =
            ((post_avg / pre_avg) - 1.0) * 100.0;
      }
    }

    // ========================================================================
    // Step 5: Check variance bounds and set regression flag
    // ========================================================================
    // Automatic regression detection: NO human judgment
    // is_regression = (cv > 0.10) OR (any hit_rate_change > 5%)

    bool latency_exceeds = (envelope.stability.latency_coefficient_of_variation > 0.10);
    envelope.variance_bounds.is_regression = latency_exceeds;

    return envelope;
  }

  // ==========================================================================
  // toJsonLD: Deterministic JSON-LD output for CI integration
  // ==========================================================================
  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;

    // JSON-LD context
    j["@context"] = "https://qlever.cs.uni-freiburg.de/epic4/v1.0";
    j["@type"] = "PerformanceEnvelope";

    // Canonical field ordering (alphabetical)
    j["cache_stats"] = cache_stats.toJsonLD();
    j["epoch_key"] = epoch_key.toJsonLD();
    j["epoch_transition_detected"] = epoch_transition_detected;
    j["latency_stats"] = latency_stats.toJsonLD();
    j["stability"] = stability.toJsonLD();
    j["variance_bounds"] = variance_bounds.toJsonLD();
    j["workload_id"] = workload_id;

    return j;
  }

  // ==========================================================================
  // detectRegression: Compare against baseline envelope
  // ==========================================================================
  RegressionReport detectRegression(const PerformanceEnvelope& baseline) const {
    RegressionReport report;

    // Automatic regression detection - NO human judgment calls

    // Check 1: Latency coefficient of variation exceeds bound
    double cv_threshold = 0.10;  // 10%
    if (stability.latency_coefficient_of_variation > cv_threshold) {
      report.latency_variance_exceeded = true;
    }

    // Check 2: Cache hit rate degradation exceeds bound
    double hit_rate_threshold = 0.05;  // 5%

    double bytes_hit_rate_change = std::abs(
        cache_stats.bytes_hit_rate - baseline.cache_stats.bytes_hit_rate);
    double plan_hit_rate_change = std::abs(
        cache_stats.plan_hit_rate - baseline.cache_stats.plan_hit_rate);

    report.bytes_hit_rate_change_pct = bytes_hit_rate_change * 100.0;
    report.plan_hit_rate_change_pct = plan_hit_rate_change * 100.0;

    if (bytes_hit_rate_change > hit_rate_threshold ||
        plan_hit_rate_change > hit_rate_threshold) {
      report.cache_hit_rate_exceeded = true;
    }

    // Final regression determination: OR logic
    // is_regression = (cv > 0.10) OR (hit_rate_change > 5%)
    report.is_regression = report.latency_variance_exceeded || report.cache_hit_rate_exceeded;

    report.latency_cv_current = stability.latency_coefficient_of_variation;
    report.latency_cv_baseline = baseline.stability.latency_coefficient_of_variation;

    // Generate summary
    std::ostringstream summary;
    if (report.is_regression) {
      summary << "REGRESSION_DETECTED: ";
      if (report.latency_variance_exceeded) {
        summary << "CV=" << std::fixed << std::setprecision(3)
                << report.latency_cv_current << " exceeds 10% threshold. ";
      }
      if (report.cache_hit_rate_exceeded) {
        summary << "Hit rate change: bytes=" << std::fixed << std::setprecision(2)
                << report.bytes_hit_rate_change_pct << "%, plan="
                << report.plan_hit_rate_change_pct << "% exceeds 5% threshold.";
      }
    } else {
      summary << "NO_REGRESSION: All metrics within variance bounds.";
    }
    report.summary = summary.str();

    return report;
  }

  // ==========================================================================
  // Utility: Compute SHA-256 digest of envelope for integrity checking
  // ==========================================================================
  std::string computeDigest() const {
    // Serialize to deterministic JSON
    std::string serialized = toJsonLD().dump();

    // Compute SHA-256
    ad_utility::HashSha256 hasher;
    auto hash_bytes = hasher(serialized);

    // Convert to hex string
    return absl::StrJoin(hash_bytes, "", ad_utility::hexFormatter);
  }
};

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_PERFORMANCEENVELOPE_H
