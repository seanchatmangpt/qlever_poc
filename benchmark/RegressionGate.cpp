// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: EPIC 10.1 AGENT 10
//
// Regression Gate: Standalone tool for regression detection
//
// Usage:
//   RegressionGate --baseline baseline_performance.json
//                  --current current_performance.json
//                  [--strict]
//
// Exit codes:
//   0 = No regression detected
//   1 = Regression detected (fail-closed)
//   2 = Invalid input or error

#include <fstream>
#include <iostream>
#include <string>

#include "../benchmark/infrastructure/Benchmark.h"
#include "engine/regression/RegressionDetector.h"
#include "util/json.h"

using namespace regression;
using namespace ad_benchmark;

namespace {

struct Args {
  std::string baseline_file;
  std::string current_file;
  bool strict = false;
  bool verbose = false;
};

Args parseArgs(int argc, char** argv) {
  Args args;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--baseline" && i + 1 < argc) {
      args.baseline_file = argv[++i];
    } else if (arg == "--current" && i + 1 < argc) {
      args.current_file = argv[++i];
    } else if (arg == "--strict") {
      args.strict = true;
    } else if (arg == "--verbose" || arg == "-v") {
      args.verbose = true;
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: RegressionGate [OPTIONS]\n"
                << "\n"
                << "Options:\n"
                << "  --baseline FILE    Baseline performance JSON file\n"
                << "  --current FILE     Current performance JSON file\n"
                << "  --strict           Use stricter bounds (±5% latency, ±2% "
                   "cache)\n"
                << "  --verbose, -v      Verbose output\n"
                << "  --help, -h         Show this help message\n"
                << "\n"
                << "Exit codes:\n"
                << "  0 = No regression detected\n"
                << "  1 = Regression detected (fail-closed)\n"
                << "  2 = Invalid input or error\n";
      std::exit(0);
    }
  }

  return args;
}

nlohmann::json loadJsonFile(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + filename);
  }

  nlohmann::json j;
  file >> j;
  return j;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    auto args = parseArgs(argc, argv);

    if (args.baseline_file.empty() || args.current_file.empty()) {
      std::cerr << "Error: Both --baseline and --current are required\n";
      std::cerr << "Run with --help for usage information\n";
      return 2;
    }

    // Load baseline and current metrics
    if (args.verbose) {
      std::cout << "Loading baseline from: " << args.baseline_file << "\n";
    }
    auto baseline_json = loadJsonFile(args.baseline_file);
    auto baseline = PerformanceMetrics::fromJson(baseline_json);

    if (args.verbose) {
      std::cout << "Loading current from: " << args.current_file << "\n";
    }
    auto current_json = loadJsonFile(args.current_file);
    auto current = PerformanceMetrics::fromJson(current_json);

    // Create detector with appropriate bounds
    RegressionBounds bounds;
    if (args.strict) {
      bounds.latency_variance_pct = 5.0;
      bounds.cache_hit_rate_drift_pct = 2.0;
      if (args.verbose) {
        std::cout << "Using strict bounds: ±5% latency, ±2% cache hit rate\n";
      }
    } else {
      if (args.verbose) {
        std::cout << "Using default bounds: ±10% latency, ±5% cache hit rate\n";
      }
    }

    RegressionDetector detector(bounds);

    // Detect regression
    auto report = detector.detectRegression(baseline, current);

    // Display results
    std::cout << "\n=== REGRESSION GATE REPORT ===\n\n";

    std::cout << "Baseline metrics:\n";
    std::cout << "  Mean latency: " << baseline.mean_ns << " ns\n";
    std::cout << "  Bytes hit rate: " << baseline.bytes_hit_rate << "%\n";
    std::cout << "  Plan hit rate: " << baseline.plan_hit_rate << "%\n\n";

    std::cout << "Current metrics:\n";
    std::cout << "  Mean latency: " << current.mean_ns << " ns\n";
    std::cout << "  Bytes hit rate: " << current.bytes_hit_rate << "%\n";
    std::cout << "  Plan hit rate: " << current.plan_hit_rate << "%\n\n";

    std::cout << "Variance analysis:\n";
    std::cout << "  Latency variance: " << report.latency_variance_pct << "% ";
    if (report.latency_regression) {
      std::cout << "(EXCEEDS threshold " << report.latency_threshold_pct
                << "%)\n";
    } else {
      std::cout << "(within threshold " << report.latency_threshold_pct
                << "%)\n";
    }

    std::cout << "  Bytes hit rate change: " << report.bytes_hit_rate_change_pct
              << "% ";
    if (report.cache_hit_rate_regression) {
      std::cout << "(EXCEEDS threshold " << bounds.cache_hit_rate_drift_pct
                << "%)\n";
    } else {
      std::cout << "(within threshold " << bounds.cache_hit_rate_drift_pct
                << "%)\n";
    }

    std::cout << "  Plan hit rate change: " << report.plan_hit_rate_change_pct
              << "% ";
    if (report.cache_hit_rate_regression) {
      std::cout << "(EXCEEDS threshold " << bounds.cache_hit_rate_drift_pct
                << "%)\n";
    } else {
      std::cout << "(within threshold " << bounds.cache_hit_rate_drift_pct
                << "%)\n";
    }

    std::cout << "\n";
    std::cout << "Result: " << report.summary << "\n\n";

    // Output JSON report if verbose
    if (args.verbose) {
      std::cout << "JSON Report:\n";
      std::cout << report.toJson().dump(2) << "\n\n";
    }

    // Exit with appropriate code
    if (report.has_regression) {
      std::cout << "EXIT CODE: 1 (REGRESSION DETECTED - FAIL)\n";
      return 1;
    } else {
      std::cout << "EXIT CODE: 0 (NO REGRESSION - PASS)\n";
      return 0;
    }

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 2;
  }
}
