// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: WorkloadReplayEngine for deterministic replay of captured workloads.
// Part of EPIC 4 Subsystem 2: Workload Replay Subsystem.
//
// INVARIANT CONTRACT (from EPIC4_SHARED_INVARIANTS.md):
// - Epoch Binding: Workload has single epoch; replay can happen on different
//   epoch
// - Determinism: Replay ONLY deterministic queries (inherit from capture
//   exclusions)
// - Query fingerprints: Use as-is from WorkloadRecord
// - Atomic visibility: ReplayResult writes are atomic/locked
// - Failure = abort: Replay divergence (digest mismatch) -> fail-closed ABORT
// - Cross-epoch reproducibility: Same ExecutionDigest on different epoch =
//   SUCCESS with note

#ifndef QLEVER_SRC_ENGINE_READPLANE_WORKLOADREPLAYENGINE_H
#define QLEVER_SRC_ENGINE_READPLANE_WORKLOADREPLAYENGINE_H

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "engine/readPlane/ExecutionDigest.h"
#include "engine/readPlane/ExecutionTraceDigest.h"
#include "engine/readPlane/ReplayResult.h"
#include "engine/readPlane/WorkloadManifest.h"
#include "engine/readPlane/WorkloadRecord.h"
#include "global/Epoch.h"
#include "util/Synchronized.h"

// Forward declarations
class Index;
class Server;
class QueryResultCache;

namespace readPlane {

// =============================================================================
// ReplayConfiguration - Settings for replay execution
// =============================================================================
struct ReplayConfiguration {
  // Abort on first divergence (fail-closed behavior)
  bool abort_on_divergence = true;

  // Enable trace event collection (informational, not for correctness)
  bool collect_trace_events = true;

  // Maximum number of records to replay (0 = no limit)
  uint64_t max_records = 0;

  // Hostname for this replay machine
  std::string hostname;

  // Read-only mode: do NOT modify caches during replay
  bool read_only_caches = true;

  // Timeout per query (milliseconds, 0 = no timeout)
  uint64_t query_timeout_ms = 300000;  // 5 minutes default

  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "ReplayConfiguration";
    j["abort_on_divergence"] = abort_on_divergence;
    j["collect_trace_events"] = collect_trace_events;
    j["hostname"] = hostname;
    j["max_records"] = max_records;
    j["query_timeout_ms"] = query_timeout_ms;
    j["read_only_caches"] = read_only_caches;
    return j;
  }
};

// =============================================================================
// ReplayProgress - Progress tracking for replay
// =============================================================================
struct ReplayProgress {
  uint64_t total_records = 0;
  uint64_t completed_records = 0;
  uint64_t successful_records = 0;
  uint64_t divergent_records = 0;
  bool is_aborted = false;
  uint64_t aborted_at_sequence_id = 0;

  [[nodiscard]] double getCompletionPercentage() const {
    if (total_records == 0) return 0.0;
    return static_cast<double>(completed_records) /
           static_cast<double>(total_records) * 100.0;
  }

  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "ReplayProgress";
    j["aborted_at_sequence_id"] = aborted_at_sequence_id;
    j["completed_records"] = completed_records;
    j["divergent_records"] = divergent_records;
    j["is_aborted"] = is_aborted;
    j["successful_records"] = successful_records;
    j["total_records"] = total_records;
    return j;
  }
};

// =============================================================================
// ReplayCallbacks - Callbacks for replay events
// =============================================================================
struct ReplayCallbacks {
  // Called when a single record is completed
  std::function<void(const ReplayResult&)> onRecordCompleted;

  // Called on progress update (after each record)
  std::function<void(const ReplayProgress&)> onProgressUpdate;

  // Called when replay is aborted due to divergence
  std::function<void(const DivergenceArtifact&)> onDivergenceDetected;

  // Called when replay completes (success or abort)
  std::function<void(const ReplayRun&)> onReplayCompleted;
};

// =============================================================================
// WorkloadReplayEngine - Deterministic replay of captured workloads
// =============================================================================
//
// Scheduling: Single-threaded deterministic ordering (process records in
// sequence_id order) Result comparison: ExecutionDigest comparison (not byte
// content) Divergence handling: If digest != expected -> ABORT with artifact,
// do NOT continue
//
class WorkloadReplayEngine {
 public:
  // Constructor with manifest pointer (manifest must outlive engine)
  explicit WorkloadReplayEngine(const WorkloadManifest* manifest);

  // Constructor with manifest and configuration
  WorkloadReplayEngine(const WorkloadManifest* manifest,
                       ReplayConfiguration config);

  // Destructor
  ~WorkloadReplayEngine();

  // Prevent copying
  WorkloadReplayEngine(const WorkloadReplayEngine&) = delete;
  WorkloadReplayEngine& operator=(const WorkloadReplayEngine&) = delete;

  // Allow moving
  WorkloadReplayEngine(WorkloadReplayEngine&&) noexcept;
  WorkloadReplayEngine& operator=(WorkloadReplayEngine&&) noexcept;

  // ==========================================================================
  // Main replay methods
  // ==========================================================================

  // Replay all records in the manifest
  // Returns ReplayRun with vector<ReplayResult>
  // Processes records in sequence_id order (deterministic)
  // Aborts on first divergence if configured (fail-closed)
  ReplayRun replayAll();

  // Replay a single WorkloadRecord
  // Returns ReplayResult for this specific record
  ReplayResult replaySingle(const WorkloadRecord& record);

  // ==========================================================================
  // Configuration
  // ==========================================================================

  // Set configuration
  void setConfiguration(ReplayConfiguration config);

  // Get current configuration
  [[nodiscard]] const ReplayConfiguration& getConfiguration() const;

  // Set callbacks
  void setCallbacks(ReplayCallbacks callbacks);

  // ==========================================================================
  // Context setup (required before replay)
  // ==========================================================================

  // Set the query execution context for replay
  void setQueryExecutionContext(QueryExecutionContext* qec);

  // Set the server for query parsing and execution
  void setServer(Server* server);

  // Set the index for query execution
  void setIndex(const Index* index);

  // Set the cache for cache observation (read-only)
  void setCache(QueryResultCache* cache);

  // ==========================================================================
  // Progress and status
  // ==========================================================================

  // Get current progress (thread-safe)
  [[nodiscard]] ReplayProgress getProgress() const;

  // Check if replay is currently running
  [[nodiscard]] bool isRunning() const;

  // Request abort (graceful stop after current record)
  void requestAbort();

  // ==========================================================================
  // Epoch management
  // ==========================================================================

  // Set the epoch for replay (can differ from capture epoch)
  void setReplayEpoch(const EpochKey& epoch);

  // Get the current replay epoch
  [[nodiscard]] const EpochKey& getReplayEpoch() const;

  // Check if replay is cross-epoch (replay epoch != capture epoch)
  [[nodiscard]] bool isCrossEpoch() const;

 private:
  // ==========================================================================
  // Internal state
  // ==========================================================================

  struct Impl;
  std::unique_ptr<Impl> impl_;

  // ==========================================================================
  // Internal methods
  // ==========================================================================

  // Initialize replay run
  ReplayRun initializeReplayRun();

  // Execute a single query and compute digest
  ExecutionDigest executeQueryAndComputeDigest(
      const WorkloadRecord& record, ResourceMetrics& resources,
      std::vector<ExecutionTraceEvent>& trace);

  // Compare digests and determine outcome
  DigestComparisonResult compareDigests(const std::string& expected_digest,
                                        const ExecutionDigest& actual_digest,
                                        ad_utility::EpochId capture_epoch,
                                        ad_utility::EpochId replay_epoch);

  // Build divergence artifact
  DivergenceArtifact buildDivergenceArtifact(
      const WorkloadRecord& record, const std::string& expected_digest,
      const ExecutionDigest& actual_digest,
      const std::vector<ExecutionTraceEvent>& trace);

  // Reconstruct query from WorkloadRecord
  std::string reconstructQuery(const WorkloadRecord& record);

  // Parse and plan query
  std::optional<std::shared_ptr<QueryExecutionTree>> parseAndPlanQuery(
      const std::string& query_text, const WorkloadRecord& record);

  // Execute query and get result
  std::optional<std::shared_ptr<const Result>> executeQuery(
      const std::shared_ptr<QueryExecutionTree>& qet);

  // Compute plan hash from QueryExecutionTree
  std::string computePlanHash(const QueryExecutionTree& qet);

  // Track resource usage during execution
  ResourceMetrics trackResourceUsage();

  // Update progress atomically
  void updateProgress(const ReplayResult& result);

  // Generate unique run ID
  static uint64_t generateRunId();

  // Get current timestamp in nanoseconds
  static uint64_t getCurrentTimestampNs();
};

// =============================================================================
// Factory functions
// =============================================================================

// Create a WorkloadReplayEngine from a manifest file path
std::unique_ptr<WorkloadReplayEngine> createReplayEngineFromFile(
    const std::string& manifest_path);

// Create a WorkloadReplayEngine from JSON-LD manifest
std::unique_ptr<WorkloadReplayEngine> createReplayEngineFromJson(
    const nlohmann::json& manifest_json);

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_WORKLOADREPLAYENGINE_H
