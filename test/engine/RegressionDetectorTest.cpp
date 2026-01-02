// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.1 AGENT 10
//
// Regression Detector Unit Tests
// Validates regression detection logic against SPEC-LOCKED bounds

#include <gtest/gtest.h>

#include <vector>

#include "engine/regression/RegressionDetector.h"

namespace regression {

class RegressionDetectorTest : public ::testing::Test {
 protected:
  RegressionDetector detector_;

  // Helper: Create sample latency data
  std::vector<uint64_t> createLatencies(uint64_t mean_ns, size_t count = 100) {
    std::vector<uint64_t> latencies;
    latencies.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      // Simple: all values = mean (zero variance for predictable tests)
      latencies.push_back(mean_ns);
    }
    return latencies;
  }

  // Helper: Create latencies with variance
  std::vector<uint64_t> createVariedLatencies(uint64_t mean_ns,
                                              double stddev_pct,
                                              size_t count = 100) {
    std::vector<uint64_t> latencies;
    latencies.reserve(count);
    uint64_t stddev = static_cast<uint64_t>(mean_ns * stddev_pct / 100.0);
    for (size_t i = 0; i < count; ++i) {
      // Simple linear distribution: mean ± i * (stddev / count)
      int64_t offset =
          static_cast<int64_t>(i) - static_cast<int64_t>(count / 2);
      int64_t value =
          static_cast<int64_t>(mean_ns) +
          (offset * static_cast<int64_t>(stddev) / static_cast<int64_t>(count));
      latencies.push_back(static_cast<uint64_t>(std::max(int64_t(1), value)));
    }
    return latencies;
  }
};

// =============================================================================
// Metric Computation Tests
// =============================================================================

TEST_F(RegressionDetectorTest, ComputeMetricsBasic) {
  std::vector<uint64_t> latencies = {100, 200, 300, 400, 500};

  auto metrics = RegressionDetector::computeMetrics(latencies);

  EXPECT_EQ(metrics.min_ns, 100);
  EXPECT_EQ(metrics.max_ns, 500);
  EXPECT_EQ(metrics.mean_ns, 300);
  EXPECT_EQ(metrics.p50_ns, 300);
}

TEST_F(RegressionDetectorTest, ComputeMetricsWithCache) {
  std::vector<uint64_t> latencies = createLatencies(1000000, 50);  // 1ms mean

  auto metrics = RegressionDetector::computeMetrics(latencies,
                                                    /* bytes_hits */ 80,
                                                    /* bytes_misses */ 20,
                                                    /* plan_hits */ 90,
                                                    /* plan_misses */ 10,
                                                    /* neg_hits */ 5);

  EXPECT_EQ(metrics.bytes_hits, 80);
  EXPECT_EQ(metrics.bytes_misses, 20);
  EXPECT_EQ(metrics.plan_hits, 90);
  EXPECT_EQ(metrics.plan_misses, 10);
  EXPECT_DOUBLE_EQ(metrics.bytes_hit_rate, 80.0);  // 80/(80+20) = 80%
  EXPECT_DOUBLE_EQ(metrics.plan_hit_rate, 90.0);   // 90/(90+10) = 90%
}

TEST_F(RegressionDetectorTest, ComputeMetricsPercentiles) {
  std::vector<uint64_t> latencies;
  for (uint64_t i = 1; i <= 100; ++i) {
    latencies.push_back(i * 1000);  // 1ms, 2ms, ..., 100ms
  }

  auto metrics = RegressionDetector::computeMetrics(latencies);

  // p50 should be around 50th element (50ms = 50000ns)
  EXPECT_EQ(metrics.p50_ns, 50000);
  // p95 should be around 95th element (95ms = 95000ns)
  EXPECT_EQ(metrics.p95_ns, 95000);
  // p99 should be around 99th element (99ms = 99000ns)
  EXPECT_EQ(metrics.p99_ns, 99000);
}

// =============================================================================
// Latency Regression Tests
// =============================================================================

TEST_F(RegressionDetectorTest, NoRegressionWithinBounds) {
  // Baseline: 1ms mean latency
  auto baseline_latencies = createLatencies(1000000, 100);
  auto baseline = RegressionDetector::computeMetrics(baseline_latencies);

  // Current: 1.05ms mean (5% increase, within ±10% threshold)
  auto current_latencies = createLatencies(1050000, 100);
  auto current = RegressionDetector::computeMetrics(current_latencies);

  auto report = detector_.detectRegression(baseline, current);

  EXPECT_FALSE(report.has_regression);
  EXPECT_FALSE(report.latency_regression);
  EXPECT_LT(report.latency_variance_pct, 10.0);
}

TEST_F(RegressionDetectorTest, LatencyRegressionExceedsBounds) {
  // Baseline: 1ms mean latency
  auto baseline_latencies = createLatencies(1000000, 100);
  auto baseline = RegressionDetector::computeMetrics(baseline_latencies);

  // Current: 1.15ms mean (15% increase, exceeds ±10% threshold)
  auto current_latencies = createLatencies(1150000, 100);
  auto current = RegressionDetector::computeMetrics(current_latencies);

  auto report = detector_.detectRegression(baseline, current);

  EXPECT_TRUE(report.has_regression);
  EXPECT_TRUE(report.latency_regression);
  EXPECT_GT(report.latency_variance_pct, 10.0);
  EXPECT_THAT(report.summary, ::testing::HasSubstr("REGRESSION DETECTED"));
}

TEST_F(RegressionDetectorTest, LatencyRegressionNegativeChange) {
  // Baseline: 1ms mean latency
  auto baseline_latencies = createLatencies(1000000, 100);
  auto baseline = RegressionDetector::computeMetrics(baseline_latencies);

  // Current: 0.85ms mean (15% decrease, exceeds ±10% threshold)
  auto current_latencies = createLatencies(850000, 100);
  auto current = RegressionDetector::computeMetrics(current_latencies);

  auto report = detector_.detectRegression(baseline, current);

  // Even improvement counts as regression if it exceeds bounds
  // (indicates instability, not true performance improvement)
  EXPECT_TRUE(report.has_regression);
  EXPECT_TRUE(report.latency_regression);
}

TEST_F(RegressionDetectorTest, LatencyExactlyAtThreshold) {
  // Baseline: 1ms mean latency
  auto baseline_latencies = createLatencies(1000000, 100);
  auto baseline = RegressionDetector::computeMetrics(baseline_latencies);

  // Current: 1.10ms mean (exactly 10% increase)
  auto current_latencies = createLatencies(1100000, 100);
  auto current = RegressionDetector::computeMetrics(current_latencies);

  auto report = detector_.detectRegression(baseline, current);

  // Exactly at threshold should NOT trigger regression (≤ threshold)
  EXPECT_FALSE(report.has_regression);
  EXPECT_FALSE(report.latency_regression);
}

// =============================================================================
// Cache Hit Rate Regression Tests
// =============================================================================

TEST_F(RegressionDetectorTest, CacheHitRateWithinBounds) {
  auto latencies = createLatencies(1000000, 100);

  // Baseline: 80% bytes hit rate, 90% plan hit rate
  auto baseline =
      RegressionDetector::computeMetrics(latencies, 80, 20, 90, 10, 0);

  // Current: 82% bytes hit rate, 91% plan hit rate (within ±5%)
  auto current =
      RegressionDetector::computeMetrics(latencies, 82, 18, 91, 9, 0);

  auto report = detector_.detectRegression(baseline, current);

  EXPECT_FALSE(report.has_regression);
  EXPECT_FALSE(report.cache_hit_rate_regression);
}

TEST_F(RegressionDetectorTest, BytesCacheHitRateExceedsBounds) {
  auto latencies = createLatencies(1000000, 100);

  // Baseline: 80% bytes hit rate
  auto baseline =
      RegressionDetector::computeMetrics(latencies, 80, 20, 90, 10, 0);

  // Current: 73% bytes hit rate (7% decrease, exceeds ±5% threshold)
  auto current =
      RegressionDetector::computeMetrics(latencies, 73, 27, 90, 10, 0);

  auto report = detector_.detectRegression(baseline, current);

  EXPECT_TRUE(report.has_regression);
  EXPECT_TRUE(report.cache_hit_rate_regression);
  EXPECT_GT(report.bytes_hit_rate_change_pct, 5.0);
  EXPECT_THAT(report.summary, ::testing::HasSubstr("Cache hit rate"));
}

TEST_F(RegressionDetectorTest, PlanCacheHitRateExceedsBounds) {
  auto latencies = createLatencies(1000000, 100);

  // Baseline: 90% plan hit rate
  auto baseline =
      RegressionDetector::computeMetrics(latencies, 80, 20, 90, 10, 0);

  // Current: 83% plan hit rate (7% decrease, exceeds ±5% threshold)
  auto current =
      RegressionDetector::computeMetrics(latencies, 80, 20, 83, 17, 0);

  auto report = detector_.detectRegression(baseline, current);

  EXPECT_TRUE(report.has_regression);
  EXPECT_TRUE(report.cache_hit_rate_regression);
  EXPECT_GT(report.plan_hit_rate_change_pct, 5.0);
}

// =============================================================================
// Combined Regression Tests
// =============================================================================

TEST_F(RegressionDetectorTest, BothLatencyAndCacheRegression) {
  // Baseline: 1ms latency, 80% bytes hit rate
  auto baseline_latencies = createLatencies(1000000, 100);
  auto baseline =
      RegressionDetector::computeMetrics(baseline_latencies, 80, 20, 90, 10, 0);

  // Current: 1.2ms latency (20% increase), 70% bytes hit rate (10% decrease)
  auto current_latencies = createLatencies(1200000, 100);
  auto current =
      RegressionDetector::computeMetrics(current_latencies, 70, 30, 90, 10, 0);

  auto report = detector_.detectRegression(baseline, current);

  EXPECT_TRUE(report.has_regression);
  EXPECT_TRUE(report.latency_regression);
  EXPECT_TRUE(report.cache_hit_rate_regression);
  EXPECT_THAT(report.summary, ::testing::HasSubstr("REGRESSION DETECTED"));
  EXPECT_THAT(report.summary, ::testing::HasSubstr("Latency"));
  EXPECT_THAT(report.summary, ::testing::HasSubstr("Cache hit rate"));
}

TEST_F(RegressionDetectorTest, OnlyLatencyRegression) {
  // Baseline: 1ms latency, 80% bytes hit rate
  auto baseline_latencies = createLatencies(1000000, 100);
  auto baseline =
      RegressionDetector::computeMetrics(baseline_latencies, 80, 20, 90, 10, 0);

  // Current: 1.2ms latency (20% increase), cache unchanged
  auto current_latencies = createLatencies(1200000, 100);
  auto current =
      RegressionDetector::computeMetrics(current_latencies, 80, 20, 90, 10, 0);

  auto report = detector_.detectRegression(baseline, current);

  EXPECT_TRUE(report.has_regression);
  EXPECT_TRUE(report.latency_regression);
  EXPECT_FALSE(report.cache_hit_rate_regression);
}

TEST_F(RegressionDetectorTest, OnlyCacheRegression) {
  // Baseline: 1ms latency, 80% bytes hit rate
  auto baseline_latencies = createLatencies(1000000, 100);
  auto baseline =
      RegressionDetector::computeMetrics(baseline_latencies, 80, 20, 90, 10, 0);

  // Current: latency unchanged, 70% bytes hit rate (10% decrease)
  auto current =
      RegressionDetector::computeMetrics(baseline_latencies, 70, 30, 90, 10, 0);

  auto report = detector_.detectRegression(baseline, current);

  EXPECT_TRUE(report.has_regression);
  EXPECT_FALSE(report.latency_regression);
  EXPECT_TRUE(report.cache_hit_rate_regression);
}

// =============================================================================
// JSON Serialization Tests
// =============================================================================

TEST_F(RegressionDetectorTest, MetricsJsonRoundTrip) {
  auto latencies = createLatencies(1000000, 50);
  auto original =
      RegressionDetector::computeMetrics(latencies, 80, 20, 90, 10, 5);

  // Serialize to JSON
  auto json = original.toJson();

  // Deserialize back
  auto reconstructed = PerformanceMetrics::fromJson(json);

  // Verify all fields match
  EXPECT_EQ(reconstructed.p50_ns, original.p50_ns);
  EXPECT_EQ(reconstructed.p95_ns, original.p95_ns);
  EXPECT_EQ(reconstructed.p99_ns, original.p99_ns);
  EXPECT_EQ(reconstructed.mean_ns, original.mean_ns);
  EXPECT_EQ(reconstructed.stddev_ns, original.stddev_ns);
  EXPECT_EQ(reconstructed.bytes_hits, original.bytes_hits);
  EXPECT_EQ(reconstructed.bytes_misses, original.bytes_misses);
  EXPECT_DOUBLE_EQ(reconstructed.bytes_hit_rate, original.bytes_hit_rate);
}

TEST_F(RegressionDetectorTest, RegressionReportJson) {
  auto baseline_latencies = createLatencies(1000000, 100);
  auto baseline =
      RegressionDetector::computeMetrics(baseline_latencies, 80, 20, 90, 10, 0);

  auto current_latencies = createLatencies(1200000, 100);
  auto current =
      RegressionDetector::computeMetrics(current_latencies, 70, 30, 90, 10, 0);

  auto report = detector_.detectRegression(baseline, current);
  auto json = report.toJson();

  EXPECT_TRUE(json["has_regression"].get<bool>());
  EXPECT_TRUE(json["latency_regression"].get<bool>());
  EXPECT_TRUE(json["cache_hit_rate_regression"].get<bool>());
  EXPECT_TRUE(json.contains("summary"));
}

// =============================================================================
// Custom Bounds Tests
// =============================================================================

TEST_F(RegressionDetectorTest, CustomBounds) {
  RegressionBounds custom_bounds;
  custom_bounds.latency_variance_pct = 5.0;  // Stricter: ±5% instead of ±10%
  custom_bounds.cache_hit_rate_drift_pct = 2.0;  // Stricter: ±2% instead of ±5%

  RegressionDetector strict_detector(custom_bounds);

  auto baseline_latencies = createLatencies(1000000, 100);
  auto baseline =
      RegressionDetector::computeMetrics(baseline_latencies, 80, 20, 90, 10, 0);

  // 7% latency increase - would pass default bounds, fails strict bounds
  auto current_latencies = createLatencies(1070000, 100);
  auto current =
      RegressionDetector::computeMetrics(current_latencies, 80, 20, 90, 10, 0);

  auto report = strict_detector.detectRegression(baseline, current);

  EXPECT_TRUE(report.has_regression);
  EXPECT_TRUE(report.latency_regression);
  EXPECT_DOUBLE_EQ(report.latency_threshold_pct, 5.0);
}

}  // namespace regression
