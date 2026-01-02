// Copyright 2026, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.2 AGENT 9 (Variance Bounding)
//
// Query Latency Distribution Benchmark - EPIC 10.2 Performance Seal
//
// Purpose: Measure query execution latency with P99 distribution tracking
//
// Metrics captured:
// - P99 latency (ns)
// - P95 latency (ns)
// - P50 latency (ns)
// - Mean latency (ns)
// - Standard deviation (ns)
// - Min/Max latency (ns)
//
// Output: JSON format compatible with PerformanceMetrics

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "../benchmark/infrastructure/Benchmark.h"
#include "engine/regression/RegressionDetector.h"
#include "util/json.h"

using namespace ad_benchmark;
using namespace regression;

namespace {

// Synthetic query workload patterns
enum class QueryPattern { SIMPLE_LOOKUP, JOIN_HEAVY, FILTER_HEAVY, AGGREGATION };

// Simulate query execution (placeholder for actual implementation)
void executeQuery(QueryPattern pattern, size_t complexity) {
  // Simulate query processing work
  volatile uint64_t result = 0;

  // Different patterns have different computational costs
  size_t iterations = 0;
  switch (pattern) {
    case QueryPattern::SIMPLE_LOOKUP:
      iterations = complexity * 100;
      break;
    case QueryPattern::JOIN_HEAVY:
      iterations = complexity * 1000;
      break;
    case QueryPattern::FILTER_HEAVY:
      iterations = complexity * 500;
      break;
    case QueryPattern::AGGREGATION:
      iterations = complexity * 750;
      break;
  }

  for (size_t i = 0; i < iterations; ++i) {
    result += i * i;
  }

  // Prevent optimization
  (void)result;
}

const char* patternName(QueryPattern p) {
  switch (p) {
    case QueryPattern::SIMPLE_LOOKUP:
      return "Simple Lookup";
    case QueryPattern::JOIN_HEAVY:
      return "Join Heavy";
    case QueryPattern::FILTER_HEAVY:
      return "Filter Heavy";
    case QueryPattern::AGGREGATION:
      return "Aggregation";
    default:
      return "Unknown";
  }
}

}  // namespace

class QueryLatencyDistributionBenchmark : public BenchmarkInterface {
 public:
  std::string name() const final {
    return "EPIC 10.2: Query Latency Distribution with P99";
  }

  BenchmarkResults runAllBenchmarks() final {
    BenchmarkResults results{};

    // Test configurations
    const std::vector<QueryPattern> patterns = {
        QueryPattern::SIMPLE_LOOKUP, QueryPattern::JOIN_HEAVY,
        QueryPattern::FILTER_HEAVY, QueryPattern::AGGREGATION};

    const std::vector<size_t> complexities = {10, 50, 100};

    std::vector<uint64_t> all_latencies;

    for (auto pattern : patterns) {
      for (auto complexity : complexities) {
        // Warmup run (not measured)
        executeQuery(pattern, complexity);

        // Measure 10 runs for this configuration
        std::vector<uint64_t> latencies;
        for (int run = 0; run < 10; ++run) {
          auto start = std::chrono::high_resolution_clock::now();
          executeQuery(pattern, complexity);
          auto end = std::chrono::high_resolution_clock::now();

          auto duration_ns =
              std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                  .count();
          latencies.push_back(duration_ns);
          all_latencies.push_back(duration_ns);
        }

        // Compute metrics for this configuration
        auto metrics = RegressionDetector::computeMetrics(latencies);

        // Add as benchmark measurement
        std::string name =
            std::string(patternName(pattern)) + " (complexity=" +
            std::to_string(complexity) + ")";
        results.addMeasurement(name,
                               [pattern, complexity]() {
                                 executeQuery(pattern, complexity);
                               })
            .metadata()
            .addKeyValuePair("pattern", patternName(pattern))
            .addKeyValuePair("complexity", complexity)
            .addKeyValuePair("p99_ns", metrics.p99_ns)
            .addKeyValuePair("p95_ns", metrics.p95_ns)
            .addKeyValuePair("p50_ns", metrics.p50_ns)
            .addKeyValuePair("mean_ns", metrics.mean_ns)
            .addKeyValuePair("stddev_ns", metrics.stddev_ns)
            .addKeyValuePair("max_ns", metrics.max_ns)
            .addKeyValuePair("min_ns", metrics.min_ns);
      }
    }

    // Compute overall metrics
    auto overall_metrics = RegressionDetector::computeMetrics(all_latencies);

    // Output metrics as JSON to stdout (for variance gate)
    std::cout << "\n=== QUERY LATENCY DISTRIBUTION METRICS ===\n";
    std::cout << overall_metrics.toJson().dump(2) << "\n\n";

    return results;
  }
};

AD_REGISTER_BENCHMARK(QueryLatencyDistributionBenchmark);
