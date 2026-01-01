// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: ReplayResult and ReplayRun artifacts for workload replay.
// Part of EPIC 4 Subsystem 2: Workload Replay Subsystem.
//
// INVARIANT CONTRACT (from EPIC4_SHARED_INVARIANTS.md):
// - Comparison must be digest-based, not result-based
// - If `digest_matches == false` -> execution ABORTS (fail-closed)
// - Trace events may contain wall-clock time (purely informational)

#ifndef QLEVER_SRC_ENGINE_READPLANE_REPLAYRESULT_H
#define QLEVER_SRC_ENGINE_READPLANE_REPLAYRESULT_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "engine/readPlane/ExecutionDigest.h"
#include "engine/readPlane/ExecutionTraceDigest.h"
#include "engine/readPlane/WorkloadRecord.h"
#include "util/json.h"

namespace readPlane {

// Forward declaration - WorkloadManifest is in WorkloadManifest.h
class WorkloadManifest;

// =============================================================================
// ReplayStatus - Execution outcome for a single replay
// =============================================================================
enum class ReplayStatus {
  SUCCESS,     // Digest matched
  DIVERGENCE,  // Digest mismatch (fail-closed ABORT)
  ABORT,       // Execution aborted (due to previous divergence or error)
  ERROR        // Execution error (not a divergence, e.g., query parse error)
};

inline std::string replayStatusToString(ReplayStatus status) {
  switch (status) {
    case ReplayStatus::SUCCESS:
      return "SUCCESS";
    case ReplayStatus::DIVERGENCE:
      return "DIVERGENCE";
    case ReplayStatus::ABORT:
      return "ABORT";
    case ReplayStatus::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

// =============================================================================
// DivergenceArtifact - Detailed divergence information for fail-closed abort
// =============================================================================
struct DivergenceArtifact {
  // Identity
  std::string query_fingerprint_sha256;
  uint64_t workload_record_sequence_id = 0;

  // Expected vs Actual
  std::string expected_digest;
  std::string actual_digest;

  // Detailed breakdown
  std::string expected_plan_hash;
  std::string actual_plan_hash;
  std::string expected_result_shape_hash;
  std::string actual_result_shape_hash;
  std::string expected_resource_signature;
  std::string actual_resource_signature;

  // Trace for debugging
  std::vector<ExecutionTraceEvent> trace_events;

  // Timestamp (informational only)
  uint64_t detection_timestamp_ns = 0;

  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "DivergenceArtifact";
    j["@severity"] = "FAIL_CLOSED";
    j["actual_digest"] = actual_digest;
    j["actual_plan_hash"] = actual_plan_hash;
    j["actual_resource_signature"] = actual_resource_signature;
    j["actual_result_shape_hash"] = actual_result_shape_hash;
    j["detection_timestamp_ns"] = detection_timestamp_ns;
    j["expected_digest"] = expected_digest;
    j["expected_plan_hash"] = expected_plan_hash;
    j["expected_resource_signature"] = expected_resource_signature;
    j["expected_result_shape_hash"] = expected_result_shape_hash;
    j["query_fingerprint_sha256"] = query_fingerprint_sha256;

    nlohmann::json trace_json = nlohmann::json::array();
    for (const auto& evt : trace_events) {
      trace_json.push_back(evt.toJsonLD());
    }
    j["trace_events"] = trace_json;
    j["workload_record_sequence_id"] = workload_record_sequence_id;
    return j;
  }
};

// =============================================================================
// ReplayResult - Per Execution Result (from EPIC4_SHARED_INVARIANTS.md)
// =============================================================================
//
// Constraints:
// - Comparison must be digest-based, not result-based
// - If `digest_matches == false` -> execution ABORTS (fail-closed)
// - Trace events may contain wall-clock time (purely informational)
//
struct ReplayResult {
  // Identity
  uint64_t replay_run_id = 0;       // UUID for this replay execution
  uint64_t workload_record_id = 0;  // Which WorkloadRecord was replayed

  // Replay Context
  EpochKey replayed_on_epoch;  // Epoch on replay machine (may differ from
                               // capture epoch)
  std::string replayed_on_hostname;

  // Execution Outcome
  ReplayStatus execution_status = ReplayStatus::ERROR;
  ExecutionDigest execution_digest;  // Full digest computed during replay

  // Comparison to Expected
  std::string expected_digest;  // From capture machine (for comparison)
  bool digest_matches = false;  // True iff hashes equal

  // Cross-epoch note
  bool is_cross_epoch_reproducible =
      false;  // True if digest matches but epochs differ

  // Trace (Optional, for Debugging)
  std::vector<ExecutionTraceEvent>
      trace_events;  // Timestamped events (wall-clock, for inspection only)

  // Divergence artifact (populated if status is DIVERGENCE)
  std::optional<DivergenceArtifact> divergence_artifact;

  // Execution timing (informational only)
  uint64_t execution_duration_ns = 0;

  // Check if expected digest matches computed digest
  [[nodiscard]] bool matches(const std::string& expected) const {
    return execution_digest.digest_hash == expected;
  }

  [[nodiscard]] bool matches(const ExecutionDigest& expected) const {
    return execution_digest.matches(expected);
  }

  // JSON-LD serialization (deterministic, canonical ordering)
  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "ReplayResult";
    j["@version"] = "v1.0";
    j["digest_matches"] = digest_matches;
    j["execution_digest"] = execution_digest.toJsonLD();
    j["execution_duration_ns"] = execution_duration_ns;
    j["execution_duration_ns@excluded_from_hash"] = true;
    j["execution_status"] = replayStatusToString(execution_status);
    j["expected_digest"] = expected_digest;
    j["is_cross_epoch_reproducible"] = is_cross_epoch_reproducible;
    j["replay_run_id"] = replay_run_id;

    // Replayed on epoch
    nlohmann::ordered_json ep;
    ep["epoch_id"] = replayed_on_epoch.epoch_id;
    ep["epoch_manifest_sha256"] = replayed_on_epoch.epoch_manifest_sha256;
    j["replayed_on_epoch"] = ep;
    j["replayed_on_hostname"] = replayed_on_hostname;

    nlohmann::json trace_json = nlohmann::json::array();
    for (const auto& evt : trace_events) {
      trace_json.push_back(evt.toJsonLD());
    }
    j["trace_events"] = trace_json;
    j["workload_record_id"] = workload_record_id;

    if (divergence_artifact.has_value()) {
      j["divergence_artifact"] = divergence_artifact->toJsonLD();
    }

    return j;
  }

  static ReplayResult fromJsonLD(const nlohmann::json& j) {
    ReplayResult rr;
    rr.replay_run_id = j.value("replay_run_id", uint64_t{0});
    rr.workload_record_id = j.value("workload_record_id", uint64_t{0});
    rr.replayed_on_hostname = j.value("replayed_on_hostname", std::string{});
    rr.expected_digest = j.value("expected_digest", std::string{});
    rr.digest_matches = j.value("digest_matches", false);
    rr.is_cross_epoch_reproducible =
        j.value("is_cross_epoch_reproducible", false);
    rr.execution_duration_ns = j.value("execution_duration_ns", uint64_t{0});

    std::string status_str = j.value("execution_status", std::string{"ERROR"});
    if (status_str == "SUCCESS") {
      rr.execution_status = ReplayStatus::SUCCESS;
    } else if (status_str == "DIVERGENCE") {
      rr.execution_status = ReplayStatus::DIVERGENCE;
    } else if (status_str == "ABORT") {
      rr.execution_status = ReplayStatus::ABORT;
    } else {
      rr.execution_status = ReplayStatus::ERROR;
    }

    if (j.contains("replayed_on_epoch")) {
      auto ep = j["replayed_on_epoch"];
      rr.replayed_on_epoch.epoch_id = ep.value("epoch_id", uint64_t{0});
      rr.replayed_on_epoch.epoch_manifest_sha256 =
          ep.value("epoch_manifest_sha256", std::string{});
    }

    if (j.contains("execution_digest")) {
      rr.execution_digest = ExecutionDigest::fromJsonLD(j["execution_digest"]);
    }

    return rr;
  }
};

// =============================================================================
// ReplayRun - Metadata for entire replay session
// =============================================================================
struct ReplayRun {
  // Identity
  uint64_t run_id = 0;  // Unique ID for this replay run
  std::string format_version = "v1.0";

  // Source manifest
  std::string source_manifest_id;
  std::string source_manifest_digest;

  // Replay context
  EpochKey replay_epoch;  // Epoch on replay machine
  std::string replay_hostname;

  // Results
  std::vector<ReplayResult> results;

  // Aggregate status
  ReplayStatus overall_status = ReplayStatus::SUCCESS;
  uint64_t aborted_at_sequence_id =
      0;  // If aborted, which sequence_id caused it

  // Statistics
  struct Stats {
    uint64_t total_records = 0;
    uint64_t successful_replays = 0;
    uint64_t divergent_replays = 0;
    uint64_t aborted_replays = 0;
    uint64_t error_replays = 0;
    uint64_t cross_epoch_reproducible = 0;
    uint64_t total_execution_time_ns = 0;

    nlohmann::ordered_json toJsonLD() const {
      nlohmann::ordered_json j;
      j["@type"] = "ReplayRunStats";
      j["aborted_replays"] = aborted_replays;
      j["cross_epoch_reproducible"] = cross_epoch_reproducible;
      j["divergent_replays"] = divergent_replays;
      j["error_replays"] = error_replays;
      j["successful_replays"] = successful_replays;
      j["total_execution_time_ns"] = total_execution_time_ns;
      j["total_records"] = total_records;
      return j;
    }
  } stats;

  // Timing (informational only)
  uint64_t start_timestamp_ns = 0;
  uint64_t end_timestamp_ns = 0;

  // Check if run completed successfully
  [[nodiscard]] bool isSuccess() const {
    return overall_status == ReplayStatus::SUCCESS;
  }

  // Check if run was aborted due to divergence
  [[nodiscard]] bool wasAborted() const {
    return overall_status == ReplayStatus::DIVERGENCE ||
           overall_status == ReplayStatus::ABORT;
  }

  // Get first divergence artifact (if any)
  [[nodiscard]] std::optional<DivergenceArtifact> getFirstDivergence() const {
    for (const auto& result : results) {
      if (result.divergence_artifact.has_value()) {
        return result.divergence_artifact;
      }
    }
    return std::nullopt;
  }

  // JSON-LD serialization
  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "ReplayRun";
    j["@version"] = format_version;
    j["aborted_at_sequence_id"] = aborted_at_sequence_id;
    j["end_timestamp_ns"] = end_timestamp_ns;
    j["end_timestamp_ns@excluded_from_hash"] = true;
    j["overall_status"] = replayStatusToString(overall_status);

    // Replay epoch
    nlohmann::ordered_json ep;
    ep["epoch_id"] = replay_epoch.epoch_id;
    ep["epoch_manifest_sha256"] = replay_epoch.epoch_manifest_sha256;
    j["replay_epoch"] = ep;
    j["replay_hostname"] = replay_hostname;

    nlohmann::json results_json = nlohmann::json::array();
    for (const auto& res : results) {
      results_json.push_back(res.toJsonLD());
    }
    j["results"] = results_json;
    j["run_id"] = run_id;
    j["source_manifest_digest"] = source_manifest_digest;
    j["source_manifest_id"] = source_manifest_id;
    j["start_timestamp_ns"] = start_timestamp_ns;
    j["start_timestamp_ns@excluded_from_hash"] = true;
    j["stats"] = stats.toJsonLD();
    return j;
  }
};

// =============================================================================
// Thread-safe result writer (atomic visibility)
// =============================================================================
class ReplayResultWriter {
 private:
  mutable std::mutex mutex_;
  ReplayRun run_;

 public:
  explicit ReplayResultWriter(uint64_t run_id) { run_.run_id = run_id; }

  // Add a result atomically
  void addResult(ReplayResult result) {
    std::lock_guard<std::mutex> lock(mutex_);
    run_.results.push_back(std::move(result));
  }

  // Get current run state (thread-safe copy)
  ReplayRun getRun() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return run_;
  }

  // Set overall status atomically
  void setOverallStatus(ReplayStatus status) {
    std::lock_guard<std::mutex> lock(mutex_);
    run_.overall_status = status;
  }

  // Mark as aborted at specific sequence ID
  void markAborted(uint64_t sequence_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    run_.overall_status = ReplayStatus::ABORT;
    run_.aborted_at_sequence_id = sequence_id;
  }

  // Finalize the run with computed stats
  ReplayRun finalize() {
    std::lock_guard<std::mutex> lock(mutex_);
    run_.stats.total_records = run_.results.size();
    for (const auto& res : run_.results) {
      switch (res.execution_status) {
        case ReplayStatus::SUCCESS:
          run_.stats.successful_replays++;
          break;
        case ReplayStatus::DIVERGENCE:
          run_.stats.divergent_replays++;
          break;
        case ReplayStatus::ABORT:
          run_.stats.aborted_replays++;
          break;
        case ReplayStatus::ERROR:
          run_.stats.error_replays++;
          break;
      }
      if (res.is_cross_epoch_reproducible) {
        run_.stats.cross_epoch_reproducible++;
      }
      run_.stats.total_execution_time_ns += res.execution_duration_ns;
    }
    return run_;
  }
};

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_REPLAYRESULT_H
