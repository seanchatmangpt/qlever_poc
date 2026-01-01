// Copyright 2025, University of Freiburg,
//                 Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: WorkloadCaptureAgent - Hook interface for capturing query workloads
// (EPIC 4 - Subsystem 1)
// Integrates with read execution pipeline to capture deterministic queries.

#ifndef QLEVER_SRC_ENGINE_READPLANE_WORKLOADCAPTUREAGENT_H
#define QLEVER_SRC_ENGINE_READPLANE_WORKLOADCAPTUREAGENT_H

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include "engine/queryCanonical/DeterminismClassifier.h"
#include "engine/queryCanonical/QueryFingerprint.h"
#include "engine/readPlane/WorkloadManifest.h"
#include "engine/readPlane/WorkloadRecord.h"
#include "global/Epoch.h"
#include "global/EpochCacheInvalidationHook.h"
#include "parser/ParsedQuery.h"
#include "util/Synchronized.h"

namespace readPlane {

// ============================================================================
// QueryContext - Execution context passed to capture hook
// ============================================================================

struct QueryContext {
  // Parsed query for determinism analysis
  const ParsedQuery* parsed_query = nullptr;

  // Query fingerprint from canonicalization pipeline
  queryCanonical::QueryFingerprint fingerprint;

  // Epoch binding at query execution time
  EpochKey epoch_key;

  // Request parameters that affect execution
  std::map<std::string, std::string> request_params;
};

// ============================================================================
// ExecutionResult - Execution outcome passed to capture hook
// ============================================================================

struct ExecutionResult {
  // Execution classification
  ExecutionClass execution_class = ExecutionClass::CACHE_MISS;

  // Observed latency (wall-clock, informational only)
  std::chrono::nanoseconds latency{0};

  // Result size (for metrics)
  size_t result_rows = 0;
  size_t result_bytes = 0;

  // Execution success
  bool success = true;

  // Error message if failed
  std::string error_message;
};

// ============================================================================
// WorkloadCaptureConfig - Configuration for capture behavior
// ============================================================================

struct WorkloadCaptureConfig {
  // Enable/disable capture globally
  bool enabled = true;

  // Maximum records per manifest (0 = unlimited)
  size_t max_records_per_manifest = 0;

  // Capture excluded queries for observability
  bool log_excluded_queries = true;

  // Fail-closed on serialization errors
  bool fail_on_serialization_error = true;

  // Hostname for manifest metadata
  std::string hostname;
};

// ============================================================================
// WorkloadCaptureAgent - Singleton agent for capturing query workloads
// ============================================================================
//
// CONSTRAINTS from EPIC4_SHARED_INVARIANTS.md:
// - Check determinism: call DeterminismClassifier::analyze() for every captured
// query
// - Non-deterministic queries go to excluded section (NOT fail-closed here)
// - All manifest updates atomic via std::atomic<shared_ptr<WorkloadManifest>>
// - Fail-closed ONLY on serialization errors
// - No dependencies on other subsystems (Replay, ExecutionDigest, CacheDecision,
// PerformanceEnvelope)
//
class WorkloadCaptureAgent {
 private:
  // Current manifest (atomic swap on epoch promotion)
  std::atomic<std::shared_ptr<WorkloadManifest>> current_manifest_;

  // Determinism classifier (thread-safe, stateless)
  queryCanonical::DeterminismClassifier determinism_classifier_;

  // Configuration
  WorkloadCaptureConfig config_;

  // Metrics
  struct Metrics {
    uint64_t queries_captured = 0;
    uint64_t queries_excluded = 0;
    uint64_t serialization_errors = 0;
    uint64_t epoch_transitions = 0;
  };
  ad_utility::Synchronized<Metrics> metrics_;

  // Private constructor - singleton
  WorkloadCaptureAgent();

 public:
  // Singleton access
  static WorkloadCaptureAgent& getInstance();

  // Prevent copying
  WorkloadCaptureAgent(const WorkloadCaptureAgent&) = delete;
  WorkloadCaptureAgent& operator=(const WorkloadCaptureAgent&) = delete;
  WorkloadCaptureAgent(WorkloadCaptureAgent&&) = delete;
  WorkloadCaptureAgent& operator=(WorkloadCaptureAgent&&) = delete;

  // ============================================================================
  // Hook Interface - Called by read execution pipeline
  // ============================================================================

  // Called after query execution completes
  // DECISION LOGIC:
  // 1. Check determinism via DeterminismClassifier
  // 2. If deterministic: extract fingerprint, create WorkloadRecord, append to
  // manifest
  // 3. If non-deterministic: log to excluded section (NOT fail-closed)
  // 4. If serialization error: fail-closed (abort, emit error artifact)
  //
  // Thread-safe: uses atomic manifest pointer
  void onQueryExecuted(const QueryContext& context,
                       const ExecutionResult& result);

  // ============================================================================
  // Manifest Access
  // ============================================================================

  // Get current manifest (lock-free snapshot)
  std::shared_ptr<WorkloadManifest> getCurrentManifest() const;

  // Get manifest records snapshot (lock-free)
  std::shared_ptr<std::vector<WorkloadRecord>> getRecordsSnapshot() const;

  // ============================================================================
  // Configuration
  // ============================================================================

  // Update configuration
  void setConfig(const WorkloadCaptureConfig& config);

  // Get current configuration
  WorkloadCaptureConfig getConfig() const;

  // Enable/disable capture
  void setEnabled(bool enabled);
  bool isEnabled() const;

  // ============================================================================
  // Epoch Integration
  // ============================================================================

  // Called on epoch promotion - creates new manifest for new epoch
  // Atomically swaps current manifest (old manifest remains valid for readers)
  void onEpochPromoted(ad_utility::EpochId oldEpochId,
                       ad_utility::EpochId newEpochId,
                       const std::string& newEpochManifestSha256);

  // Register with global epoch hook registry
  void setupEpochHooks();

  // ============================================================================
  // Metrics & Observability
  // ============================================================================

  struct CaptureMetrics {
    uint64_t queries_captured;
    uint64_t queries_excluded;
    uint64_t serialization_errors;
    uint64_t epoch_transitions;
    size_t current_manifest_size;
  };

  CaptureMetrics getMetrics() const;

  // Reset metrics (for testing)
  void resetMetrics();

  // ============================================================================
  // Testing Support
  // ============================================================================

  // Create new empty manifest (for testing)
  void resetManifestForTesting(ad_utility::EpochId epochId,
                               const std::string& epochManifestSha256);
};

// ============================================================================
// WorkloadCaptureEpochHandler - EpochCacheInvalidationHandler implementation
// ============================================================================
//
// Connects WorkloadCaptureAgent to epoch promotion events.
// Registered with globalEpochCacheInvalidationRegistry at startup.
//
class WorkloadCaptureEpochHandler : public ad_utility::EpochCacheInvalidationHandler {
 private:
  WorkloadCaptureAgent& agent_;

 public:
  explicit WorkloadCaptureEpochHandler(WorkloadCaptureAgent& agent)
      : agent_(agent) {}

  void onEpochPromoted(
      const ad_utility::EpochPromotionEvent& event) override;

  std::string getName() const override {
    return "WorkloadCaptureEpochHandler";
  }
};

// ============================================================================
// WorkloadCaptureHook - Integration point for read execution pipeline
// ============================================================================
//
// Provides static methods for easy integration into Server::processQuery
// without requiring instance access.
//
class WorkloadCaptureHook {
 public:
  // Called after query execution - dispatches to singleton agent
  static void afterQueryExecuted(const QueryContext& context,
                                 const ExecutionResult& result) {
    WorkloadCaptureAgent::getInstance().onQueryExecuted(context, result);
  }

  // Create QueryContext from execution parameters
  static QueryContext createContext(
      const ParsedQuery& parsedQuery,
      const queryCanonical::QueryFingerprint& fingerprint,
      ad_utility::EpochId epochId,
      const std::string& epochManifestSha256,
      const std::map<std::string, std::string>& params = {});

  // Create ExecutionResult from execution outcome
  static ExecutionResult createResult(ExecutionClass executionClass,
                                      std::chrono::nanoseconds latency,
                                      size_t resultRows = 0,
                                      size_t resultBytes = 0,
                                      bool success = true,
                                      const std::string& errorMessage = "");

  // Check if capture is enabled
  static bool isEnabled() {
    return WorkloadCaptureAgent::getInstance().isEnabled();
  }
};

// ============================================================================
// Global Access
// ============================================================================

// Global singleton for workload capture agent
// Usage pattern matches globalEpochCacheInvalidationRegistry
inline WorkloadCaptureAgent& getWorkloadCaptureAgent() {
  return WorkloadCaptureAgent::getInstance();
}

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_WORKLOADCAPTUREAGENT_H
