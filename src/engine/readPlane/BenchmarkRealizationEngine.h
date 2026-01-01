// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: BenchmarkRealizationEngine - Production benchmark realization from
// workload replay (EPIC 4 Subsystem 5)
//
// INVARIANTS (from EPIC4_SHARED_INVARIANTS.md):
// - Benchmarks derived from WorkloadManifest (NOT synthetic microbenchmarks)
// - Latency distribution from ReplayResult.trace_events
// - Cache efficiency from CacheDecision artifacts
// - Variance bounds pre-defined (10% latency, 5% cache hit rate)
// - Regression detection automatic (boolean, no judgment calls)
// - Output: PerformanceEnvelope (immutable, portable, CI-parseable)

#ifndef QLEVER_SRC_ENGINE_READPLANE_BENCHMARKREALIZATIONENGINE_H
#define QLEVER_SRC_ENGINE_READPLANE_BENCHMARKREALIZATIONENGINE_H

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "engine/readPlane/CacheDecision.h"
#include "engine/readPlane/PerformanceEnvelope.h"
#include "engine/readPlane/ReplayResult.h"

namespace readPlane {

// =============================================================================
// BenchmarkRealizationEngine Configuration
// =============================================================================
struct BenchmarkConfig {
  // Variance bounds (hardcoded, not configurable per EPIC4 spec)
  static constexpr double LATENCY_CV_THRESHOLD = 0.10;      // 10%
  static constexpr double CACHE_HIT_RATE_THRESHOLD = 0.05;  // 5%

  // Minimum samples required for valid benchmark
  uint64_t min_samples_required = 10;

  // Whether to abort on non-determinism detection
  bool fail_closed_on_nondeterminism = true;

  // Whether to include epoch transition overhead in regression check
  bool track_epoch_transitions = true;
};

// =============================================================================
// Benchmark execution status
// =============================================================================
enum class BenchmarkStatus {
  SUCCESS,              // Benchmark completed successfully
  INSUFFICIENT_SAMPLES, // Not enough replay results for valid benchmark
  DIVERGENCE_DETECTED,  // Execution divergence detected during replay
  ABORT_NONDETERMINISM, // Non-determinism detected, fail-closed
  EPOCH_MISMATCH,       // Workload epoch does not match replay epoch
  INTERNAL_ERROR        // Unexpected error during benchmark
};

inline std::string benchmarkStatusToString(BenchmarkStatus status) {
  switch (status) {
    case BenchmarkStatus::SUCCESS: return "SUCCESS";
    case BenchmarkStatus::INSUFFICIENT_SAMPLES: return "INSUFFICIENT_SAMPLES";
    case BenchmarkStatus::DIVERGENCE_DETECTED: return "DIVERGENCE_DETECTED";
    case BenchmarkStatus::ABORT_NONDETERMINISM: return "ABORT_NONDETERMINISM";
    case BenchmarkStatus::EPOCH_MISMATCH: return "EPOCH_MISMATCH";
    case BenchmarkStatus::INTERNAL_ERROR: return "INTERNAL_ERROR";
    default: return "UNKNOWN";
  }
}

// =============================================================================
// Benchmark execution result
// =============================================================================
struct BenchmarkResult {
  BenchmarkStatus status = BenchmarkStatus::INTERNAL_ERROR;
  std::optional<PerformanceEnvelope> envelope;
  std::string error_message;
  uint64_t samples_processed = 0;
  uint64_t samples_skipped = 0;

  // Artifact for fail-closed scenarios
  std::string abort_artifact;

  bool isSuccess() const { return status == BenchmarkStatus::SUCCESS; }

  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "BenchmarkResult";
    j["abort_artifact"] = abort_artifact;
    j["error_message"] = error_message;
    if (envelope.has_value()) {
      j["envelope"] = envelope->toJsonLD();
    }
    j["samples_processed"] = samples_processed;
    j["samples_skipped"] = samples_skipped;
    j["status"] = benchmarkStatusToString(status);
    return j;
  }
};

// =============================================================================
// BenchmarkRealizationEngine: Main engine for production benchmarking
// =============================================================================
class BenchmarkRealizationEngine {
 public:
  // =========================================================================
  // Constructor
  // =========================================================================
  // Takes pointer to WorkloadManifest for benchmark execution.
  // The manifest must remain valid for the lifetime of the engine.
  explicit BenchmarkRealizationEngine(const WorkloadManifest* manifest,
                                       BenchmarkConfig config = BenchmarkConfig{});

  // Destructor
  ~BenchmarkRealizationEngine() = default;

  // No copy (stateful)
  BenchmarkRealizationEngine(const BenchmarkRealizationEngine&) = delete;
  BenchmarkRealizationEngine& operator=(const BenchmarkRealizationEngine&) = delete;

  // Move allowed
  BenchmarkRealizationEngine(BenchmarkRealizationEngine&&) = default;
  BenchmarkRealizationEngine& operator=(BenchmarkRealizationEngine&&) = default;

  // =========================================================================
  // Core Methods
  // =========================================================================

  // Generate benchmark from workload manifest and replay results.
  //
  // Steps:
  // 1. Derive latency samples from ReplayResult.trace_events
  // 2. Compute latency distribution (p50, p95, p99, max)
  // 3. Aggregate cache hit/miss counts from CacheDecision artifacts
  // 4. Compute stability metrics (CV, plan reuse rate)
  // 5. Check variance bounds (predefined thresholds)
  // 6. If any metric exceeds bound: is_regression = true
  //
  // Returns: BenchmarkResult containing PerformanceEnvelope or error
  BenchmarkResult generateBenchmark(
      const std::vector<ReplayResult>& replay_results,
      const std::vector<CacheDecision>& cache_decisions);

  // Convenience overload: generate benchmark from accumulated data
  BenchmarkResult generateBenchmark();

  // Compare current benchmark to baseline envelope.
  //
  // Automatic regression detection (NO human judgment):
  // - is_regression = (cv > 0.10) OR (hit_rate_change > 5%)
  //
  // Returns: RegressionReport with boolean is_regression flag
  RegressionReport compareToBaseline(const PerformanceEnvelope& baseline_envelope);

  // =========================================================================
  // Accessors
  // =========================================================================

  // Get the workload manifest this engine operates on
  const WorkloadManifest* getManifest() const { return manifest_; }

  // Get the most recent benchmark envelope (if generated)
  std::optional<PerformanceEnvelope> getLastEnvelope() const { return last_envelope_; }

  // Get configuration
  const BenchmarkConfig& getConfig() const { return config_; }

  // =========================================================================
  // Replay Result Injection
  // =========================================================================

  // Add replay results for benchmark computation.
  void addReplayResults(const std::vector<ReplayResult>& results);

  // Add cache decisions for benchmark computation.
  void addCacheDecisions(const std::vector<CacheDecision>& decisions);

  // Clear accumulated replay results and cache decisions
  void clearAccumulatedData();

  // =========================================================================
  // CI Integration
  // =========================================================================

  // Export last envelope as JSON-LD for CI consumption.
  std::string exportJsonLD() const;

  // Compare two envelopes and return CI-friendly diff.
  static std::string diffEnvelopes(const PerformanceEnvelope& current,
                                   const PerformanceEnvelope& baseline);

 private:
  // =========================================================================
  // Internal Methods
  // =========================================================================

  // Validate that all replay results have consistent epochs
  bool validateEpochConsistency(const std::vector<ReplayResult>& results) const;

  // Check for divergence in replay results (fail-closed on mismatch)
  bool checkForDivergence(const std::vector<ReplayResult>& results) const;

  // Extract latencies from trace events
  std::vector<uint64_t> extractLatencies(const std::vector<ReplayResult>& results) const;

  // Compute percentile from sorted vector
  static uint64_t computePercentile(const std::vector<uint64_t>& sorted, double percentile);

  // Compute standard deviation
  static double computeStdDev(const std::vector<uint64_t>& values, double mean);

  // Generate abort artifact for fail-closed scenarios
  std::string generateAbortArtifact(const std::string& reason,
                                    const std::string& details) const;

  // =========================================================================
  // Member Variables
  // =========================================================================
  const WorkloadManifest* manifest_;  // Not owned
  BenchmarkConfig config_;
  std::optional<PerformanceEnvelope> last_envelope_;

  // Accumulated data for incremental benchmark generation
  std::vector<ReplayResult> accumulated_replay_results_;
  std::vector<CacheDecision> accumulated_cache_decisions_;
};

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_BENCHMARKREALIZATIONENGINE_H
