// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.1 AGENT 10

#include "engine/regression/RegressionDetector.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>

namespace regression {

// =============================================================================
// RegressionDetector Implementation
// =============================================================================

RegressionDetector::RegressionDetector(RegressionBounds bounds)
    : bounds_(std::move(bounds)) {}

RegressionReport RegressionDetector::detectRegression(
    const PerformanceMetrics& baseline,
    const PerformanceMetrics& current) const {
  RegressionReport report;

  // Check latency regression
  double latency_variance_pct = 0.0;
  report.latency_regression =
      checkLatencyRegression(baseline, current, latency_variance_pct);
  report.latency_variance_pct = latency_variance_pct;
  report.latency_threshold_pct = bounds_.latency_variance_pct;
  report.baseline_mean_ns = baseline.mean_ns;
  report.current_mean_ns = current.mean_ns;

  // Check cache hit rate regression
  double bytes_change_pct = 0.0;
  double plan_change_pct = 0.0;
  report.cache_hit_rate_regression = checkCacheHitRateRegression(
      baseline, current, bytes_change_pct, plan_change_pct);
  report.bytes_hit_rate_change_pct = bytes_change_pct;
  report.plan_hit_rate_change_pct = plan_change_pct;
  report.baseline_bytes_hit_rate = baseline.bytes_hit_rate;
  report.current_bytes_hit_rate = current.bytes_hit_rate;
  report.baseline_plan_hit_rate = baseline.plan_hit_rate;
  report.current_plan_hit_rate = current.plan_hit_rate;

  // Fail-closed: regression if EITHER condition fails
  report.has_regression =
      report.latency_regression || report.cache_hit_rate_regression;

  // Generate summary
  std::ostringstream summary;
  if (report.has_regression) {
    summary << "REGRESSION DETECTED: ";
    if (report.latency_regression) {
      summary << "Latency variance " << latency_variance_pct
              << "% exceeds threshold " << bounds_.latency_variance_pct
              << "%. ";
    }
    if (report.cache_hit_rate_regression) {
      summary << "Cache hit rate change (bytes: " << bytes_change_pct
              << "%, plan: " << plan_change_pct << "%) exceeds threshold "
              << bounds_.cache_hit_rate_drift_pct << "%.";
    }
  } else {
    summary << "NO REGRESSION: All metrics within variance bounds.";
  }
  report.summary = summary.str();

  return report;
}

PerformanceMetrics RegressionDetector::computeMetrics(
    const std::vector<uint64_t>& latencies_ns, uint64_t bytes_hits,
    uint64_t bytes_misses, uint64_t plan_hits, uint64_t plan_misses,
    uint64_t neg_hits) {
  PerformanceMetrics metrics;

  if (latencies_ns.empty()) {
    return metrics;
  }

  // Sort for percentile computation
  std::vector<uint64_t> sorted = latencies_ns;
  std::sort(sorted.begin(), sorted.end());

  // Compute percentiles
  metrics.p50_ns = computePercentile(sorted, 0.50);
  metrics.p95_ns = computePercentile(sorted, 0.95);
  metrics.p99_ns = computePercentile(sorted, 0.99);
  metrics.max_ns = sorted.back();
  metrics.min_ns = sorted.front();

  // Compute mean
  uint64_t sum = std::accumulate(sorted.begin(), sorted.end(), 0ULL);
  metrics.mean_ns = sum / sorted.size();

  // Compute stddev
  double stddev = computeStdDev(sorted, static_cast<double>(metrics.mean_ns));
  metrics.stddev_ns = static_cast<uint64_t>(stddev);

  // Cache statistics
  metrics.bytes_hits = bytes_hits;
  metrics.bytes_misses = bytes_misses;
  metrics.plan_hits = plan_hits;
  metrics.plan_misses = plan_misses;
  metrics.neg_hits = neg_hits;

  uint64_t bytes_total = bytes_hits + bytes_misses;
  if (bytes_total > 0) {
    metrics.bytes_hit_rate =
        (static_cast<double>(bytes_hits) / bytes_total) * 100.0;
  }

  uint64_t plan_total = plan_hits + plan_misses;
  if (plan_total > 0) {
    metrics.plan_hit_rate =
        (static_cast<double>(plan_hits) / plan_total) * 100.0;
  }

  if (neg_hits > 0 && bytes_total > 0) {
    metrics.neg_hit_rate =
        (static_cast<double>(neg_hits) / bytes_total) * 100.0;
  }

  return metrics;
}

bool RegressionDetector::checkLatencyRegression(
    const PerformanceMetrics& baseline, const PerformanceMetrics& current,
    double& variance_pct) const {
  if (baseline.mean_ns == 0) {
    variance_pct = 0.0;
    return false;
  }

  // Compute variance: |current - baseline| / baseline * 100
  double diff = static_cast<double>(current.mean_ns) -
                static_cast<double>(baseline.mean_ns);
  variance_pct =
      (std::abs(diff) / static_cast<double>(baseline.mean_ns)) * 100.0;

  // Regression if variance exceeds threshold
  return variance_pct > bounds_.latency_variance_pct;
}

bool RegressionDetector::checkCacheHitRateRegression(
    const PerformanceMetrics& baseline, const PerformanceMetrics& current,
    double& bytes_change_pct, double& plan_change_pct) const {
  // Compute absolute change in cache hit rates
  bytes_change_pct = std::abs(current.bytes_hit_rate - baseline.bytes_hit_rate);
  plan_change_pct = std::abs(current.plan_hit_rate - baseline.plan_hit_rate);

  // Regression if either exceeds threshold
  return bytes_change_pct > bounds_.cache_hit_rate_drift_pct ||
         plan_change_pct > bounds_.cache_hit_rate_drift_pct;
}

uint64_t RegressionDetector::computePercentile(
    const std::vector<uint64_t>& sorted, double percentile) {
  if (sorted.empty()) {
    return 0;
  }

  size_t index = static_cast<size_t>(sorted.size() * percentile);
  if (index >= sorted.size()) {
    index = sorted.size() - 1;
  }

  return sorted[index];
}

double RegressionDetector::computeStdDev(const std::vector<uint64_t>& values,
                                         double mean) {
  if (values.empty()) {
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

}  // namespace regression
