// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.1 AGENT 10
//
// Purpose: Regression Detection & Baseline Validation Gates
//
// SPEC-LOCK CONSTRAINTS (EPIC 10.1):
// - Section 3.5: Regression bounds = ±10% latency, ±5% cache hit rate
// - Section 6.7: Baselines as committed artifacts, drift mechanically
// enforceable
// - Section 6.1: Validation artifact proves regression-free equivalence
// - Section 6.2-6.3: Determinism + SIMD equivalence must pass regression gates
//
// INVARIANTS:
// - Fail-closed on regression (threshold crossing = test failure)
// - No soft regression acceptance (boolean pass/fail)
// - Variance calculation mandatory (stddev required for bound assessment)
// - Cache hit rate drift enforced (±5%)

#ifndef QLEVER_SRC_ENGINE_REGRESSION_REGRESSIONDETECTOR_H
#define QLEVER_SRC_ENGINE_REGRESSION_REGRESSIONDETECTOR_H

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "util/json.h"

namespace regression {

// =============================================================================
// Performance Metrics Structure
// =============================================================================
struct PerformanceMetrics {
  // Latency statistics (nanoseconds)
  uint64_t p50_ns = 0;
  uint64_t p95_ns = 0;
  uint64_t p99_ns = 0;
  uint64_t mean_ns = 0;
  uint64_t stddev_ns = 0;
  uint64_t max_ns = 0;
  uint64_t min_ns = 0;

  // Cache efficiency (percentages 0-100)
  double bytes_hit_rate = 0.0;
  double plan_hit_rate = 0.0;
  double neg_hit_rate = 0.0;

  // Raw counts
  uint64_t bytes_hits = 0;
  uint64_t bytes_misses = 0;
  uint64_t plan_hits = 0;
  uint64_t plan_misses = 0;
  uint64_t neg_hits = 0;

  // Build metadata
  std::string compiler;
  std::string build_flags;
  std::string machine_class;
  std::string timestamp;

  // JSON serialization (deterministic order)
  nlohmann::ordered_json toJson() const {
    nlohmann::ordered_json j;
    j["bytes_hit_rate"] = bytes_hit_rate;
    j["bytes_hits"] = bytes_hits;
    j["bytes_misses"] = bytes_misses;
    j["build_flags"] = build_flags;
    j["compiler"] = compiler;
    j["machine_class"] = machine_class;
    j["max_ns"] = max_ns;
    j["mean_ns"] = mean_ns;
    j["min_ns"] = min_ns;
    j["neg_hit_rate"] = neg_hit_rate;
    j["neg_hits"] = neg_hits;
    j["p50_ns"] = p50_ns;
    j["p95_ns"] = p95_ns;
    j["p99_ns"] = p99_ns;
    j["plan_hit_rate"] = plan_hit_rate;
    j["plan_hits"] = plan_hits;
    j["plan_misses"] = plan_misses;
    j["stddev_ns"] = stddev_ns;
    j["timestamp"] = timestamp;
    return j;
  }

  static PerformanceMetrics fromJson(const nlohmann::json& j) {
    PerformanceMetrics m;
    m.bytes_hit_rate = j.value("bytes_hit_rate", 0.0);
    m.bytes_hits = j.value("bytes_hits", 0);
    m.bytes_misses = j.value("bytes_misses", 0);
    m.build_flags = j.value("build_flags", "");
    m.compiler = j.value("compiler", "");
    m.machine_class = j.value("machine_class", "");
    m.max_ns = j.value("max_ns", 0);
    m.mean_ns = j.value("mean_ns", 0);
    m.min_ns = j.value("min_ns", 0);
    m.neg_hit_rate = j.value("neg_hit_rate", 0.0);
    m.neg_hits = j.value("neg_hits", 0);
    m.p50_ns = j.value("p50_ns", 0);
    m.p95_ns = j.value("p95_ns", 0);
    m.p99_ns = j.value("p99_ns", 0);
    m.plan_hit_rate = j.value("plan_hit_rate", 0.0);
    m.plan_hits = j.value("plan_hits", 0);
    m.plan_misses = j.value("plan_misses", 0);
    m.stddev_ns = j.value("stddev_ns", 0);
    m.timestamp = j.value("timestamp", "");
    return m;
  }
};

// =============================================================================
// Regression Bounds (SPEC-LOCKED: Section 3.5)
// =============================================================================
struct RegressionBounds {
  // ±10% latency variance (within same build/machine)
  static constexpr double LATENCY_VARIANCE_PCT = 10.0;

  // ±5% cache hit rate drift
  static constexpr double CACHE_HIT_RATE_DRIFT_PCT = 5.0;

  double latency_variance_pct = LATENCY_VARIANCE_PCT;
  double cache_hit_rate_drift_pct = CACHE_HIT_RATE_DRIFT_PCT;

  nlohmann::ordered_json toJson() const {
    nlohmann::ordered_json j;
    j["cache_hit_rate_drift_pct"] = cache_hit_rate_drift_pct;
    j["latency_variance_pct"] = latency_variance_pct;
    return j;
  }
};

// =============================================================================
// Regression Report (Fail-Closed)
// =============================================================================
struct RegressionReport {
  bool has_regression = false;
  bool latency_regression = false;
  bool cache_hit_rate_regression = false;

  // Latency variance details
  double latency_variance_pct = 0.0;
  double latency_threshold_pct = 0.0;
  uint64_t baseline_mean_ns = 0;
  uint64_t current_mean_ns = 0;

  // Cache hit rate details
  double bytes_hit_rate_change_pct = 0.0;
  double plan_hit_rate_change_pct = 0.0;
  double baseline_bytes_hit_rate = 0.0;
  double current_bytes_hit_rate = 0.0;
  double baseline_plan_hit_rate = 0.0;
  double current_plan_hit_rate = 0.0;

  std::string summary;

  nlohmann::ordered_json toJson() const {
    nlohmann::ordered_json j;
    j["baseline_bytes_hit_rate"] = baseline_bytes_hit_rate;
    j["baseline_mean_ns"] = baseline_mean_ns;
    j["baseline_plan_hit_rate"] = baseline_plan_hit_rate;
    j["bytes_hit_rate_change_pct"] = bytes_hit_rate_change_pct;
    j["cache_hit_rate_regression"] = cache_hit_rate_regression;
    j["current_bytes_hit_rate"] = current_bytes_hit_rate;
    j["current_mean_ns"] = current_mean_ns;
    j["current_plan_hit_rate"] = current_plan_hit_rate;
    j["has_regression"] = has_regression;
    j["latency_regression"] = latency_regression;
    j["latency_threshold_pct"] = latency_threshold_pct;
    j["latency_variance_pct"] = latency_variance_pct;
    j["plan_hit_rate_change_pct"] = plan_hit_rate_change_pct;
    j["summary"] = summary;
    return j;
  }
};

// =============================================================================
// RegressionDetector: Core regression detection engine
// =============================================================================
class RegressionDetector {
 public:
  // Constructor with custom bounds (default: SPEC-LOCKED values)
  explicit RegressionDetector(RegressionBounds bounds = RegressionBounds{});

  // Compare current metrics against baseline
  // Returns: RegressionReport with boolean pass/fail
  RegressionReport detectRegression(const PerformanceMetrics& baseline,
                                    const PerformanceMetrics& current) const;

  // Compute performance metrics from raw latency samples
  static PerformanceMetrics computeMetrics(
      const std::vector<uint64_t>& latencies_ns, uint64_t bytes_hits = 0,
      uint64_t bytes_misses = 0, uint64_t plan_hits = 0,
      uint64_t plan_misses = 0, uint64_t neg_hits = 0);

  // Get configured bounds
  const RegressionBounds& getBounds() const { return bounds_; }

 private:
  RegressionBounds bounds_;

  // Check latency regression: |current_mean - baseline_mean| / baseline_mean >
  // threshold
  bool checkLatencyRegression(const PerformanceMetrics& baseline,
                              const PerformanceMetrics& current,
                              double& variance_pct) const;

  // Check cache hit rate regression: |current_rate - baseline_rate| > threshold
  bool checkCacheHitRateRegression(const PerformanceMetrics& baseline,
                                   const PerformanceMetrics& current,
                                   double& bytes_change_pct,
                                   double& plan_change_pct) const;

  // Compute percentile from sorted vector
  static uint64_t computePercentile(const std::vector<uint64_t>& sorted,
                                    double percentile);

  // Compute standard deviation
  static double computeStdDev(const std::vector<uint64_t>& values, double mean);
};

}  // namespace regression

#endif  // QLEVER_SRC_ENGINE_REGRESSION_REGRESSIONDETECTOR_H
