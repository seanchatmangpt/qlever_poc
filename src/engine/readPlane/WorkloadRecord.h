// Copyright 2025, University of Freiburg,
//                 Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: WorkloadRecord data structure for workload capture (EPIC 4 - Subsystem 1)
// Captures canonicalized query workloads from the read plane for replay/analysis.

#ifndef QLEVER_SRC_ENGINE_READPLANE_WORKLOADRECORD_H
#define QLEVER_SRC_ENGINE_READPLANE_WORKLOADRECORD_H

#include <absl/strings/str_join.h>

#include <atomic>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>

#include "backports/three_way_comparison.h"
#include "engine/queryCanonical/QueryFingerprint.h"
#include "global/Epoch.h"
#include "util/CryptographicHashUtils.h"

namespace readPlane {

// EpochKey - Binding to specific epoch for reproducibility
// Every artifact MUST be keyed with EpochKey = (epochId, manifestSha256)
struct EpochKey {
  ad_utility::EpochId epoch_id = 0;
  std::string epoch_manifest_sha256;  // SHA256 hex string (64 chars)

  EpochKey() = default;
  EpochKey(ad_utility::EpochId id, std::string manifestHash)
      : epoch_id(id), epoch_manifest_sha256(std::move(manifestHash)) {}

  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(EpochKey, epoch_id,
                                               epoch_manifest_sha256)

  template <typename H>
  friend H AbslHashValue(H h, const EpochKey& key) {
    return H::combine(std::move(h), key.epoch_id, key.epoch_manifest_sha256);
  }

  [[nodiscard]] bool isValid() const {
    return epoch_id > 0 && !epoch_manifest_sha256.empty();
  }

  // Deterministic string representation
  [[nodiscard]] std::string toString() const {
    return std::to_string(epoch_id) + ":" + epoch_manifest_sha256;
  }
};

// Execution class classification for workload capture
// Matches cache behavior: CACHE_HIT_BYTES, CACHE_HIT_PLAN, CACHE_MISS, RECOMPILE
enum class ExecutionClass : uint8_t {
  CACHE_HIT_BYTES = 0,  // Full result served from bytes cache
  CACHE_HIT_PLAN = 1,   // Plan reused, result recomputed
  CACHE_MISS = 2,       // Full cache miss, plan + result computed
  RECOMPILE = 3         // Plan invalidated, forced recompile
};

// Convert ExecutionClass to deterministic string
inline std::string executionClassToString(ExecutionClass ec) {
  switch (ec) {
    case ExecutionClass::CACHE_HIT_BYTES:
      return "CACHE_HIT_BYTES";
    case ExecutionClass::CACHE_HIT_PLAN:
      return "CACHE_HIT_PLAN";
    case ExecutionClass::CACHE_MISS:
      return "CACHE_MISS";
    case ExecutionClass::RECOMPILE:
      return "RECOMPILE";
    default:
      return "UNKNOWN";
  }
}

// Parse ExecutionClass from string
inline ExecutionClass stringToExecutionClass(const std::string& s) {
  if (s == "CACHE_HIT_BYTES") return ExecutionClass::CACHE_HIT_BYTES;
  if (s == "CACHE_HIT_PLAN") return ExecutionClass::CACHE_HIT_PLAN;
  if (s == "CACHE_MISS") return ExecutionClass::CACHE_MISS;
  if (s == "RECOMPILE") return ExecutionClass::RECOMPILE;
  return ExecutionClass::CACHE_MISS;  // Default
}

// ============================================================================
// WorkloadRecord - Per-query capture for deterministic replay
// ============================================================================
//
// Captures all replay-relevant information for a single query execution.
// CONSTRAINTS from EPIC4_SHARED_INVARIANTS.md:
// - capture_timestamp_epoch must be deterministic (epoch counter, not wall-clock)
// - observed_latency_ns must NOT be used for correctness decisions
// - replay_params must be a subset of QueryFingerprint (already normalized)
// - All serialized to JSON-LD for portability
//
struct WorkloadRecord {
  // ============================================================================
  // Identity
  // ============================================================================

  // Global monotonic counter - deterministic sequence within epoch
  uint64_t sequence_id = 0;

  // Normalized to epoch counter, NOT wall-clock time
  // This is the epoch-local sequence number for deterministic ordering
  uint64_t capture_timestamp_epoch = 0;

  // ============================================================================
  // Execution Context
  // ============================================================================

  // Epoch binding - (epochId, manifestSha256)
  EpochKey epoch_key;

  // Complete query identity from EPIC 3 QueryFingerprint
  // IMMUTABLE: Do not modify QueryFingerprint fields
  queryCanonical::QueryFingerprint query_fp;

  // ============================================================================
  // Execution Metadata
  // ============================================================================

  // Cache behavior classification
  ExecutionClass execution_class = ExecutionClass::CACHE_MISS;

  // Wall-clock latency (informational ONLY, NOT for correctness)
  // Marked as excluded from digest computation
  uint64_t observed_latency_ns = 0;

  // ============================================================================
  // Replay-Relevant Parameters
  // ============================================================================

  // Subset of QueryFingerprint fields that affect execution path
  // Keys are alphabetically sorted for determinism
  std::map<std::string, std::string> replay_params;

  // ============================================================================
  // Digest for Reproducibility
  // ============================================================================

  // SHA256 of serialized query_fp (computed at capture time)
  std::string fingerprint_sha256;

  // ============================================================================
  // Methods
  // ============================================================================

  WorkloadRecord() = default;

  // Compute SHA256 fingerprint of this record (deterministic)
  // Excludes: observed_latency_ns (wall-clock, informational only)
  [[nodiscard]] std::string computeFingerprint() const {
    std::ostringstream ss;

    // Deterministic field ordering (alphabetical)
    ss << "capture_timestamp_epoch:" << capture_timestamp_epoch << ";";
    ss << "epoch_id:" << epoch_key.epoch_id << ";";
    ss << "epoch_manifest_sha256:" << epoch_key.epoch_manifest_sha256 << ";";
    ss << "execution_class:" << executionClassToString(execution_class) << ";";

    // Query fingerprint fields
    ss << "params_sha256:" << query_fp.params_sha256 << ";";
    ss << "shape_sha256:" << query_fp.shape_sha256 << ";";

    // Replay params (already sorted by std::map)
    ss << "replay_params:{";
    bool first = true;
    for (const auto& [key, value] : replay_params) {
      if (!first) ss << ",";
      ss << key << ":" << value;
      first = false;
    }
    ss << "};";

    ss << "sequence_id:" << sequence_id << ";";

    // Compute SHA256
    std::string data = ss.str();
    auto hash = ad_utility::HashSha256{}(data);
    return absl::StrJoin(hash, "", ad_utility::hexFormatter);
  }

  // Serialize to deterministic JSON-LD format
  // Fields ordered alphabetically, no pointers, no thread IDs
  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << R"(  "@context": "https://qlever.cs.uni-freiburg.de/workload/v1",)" << "\n";
    ss << R"(  "@type": "WorkloadRecord",)" << "\n";
    ss << R"(  "capture_timestamp_epoch": )" << capture_timestamp_epoch << ",\n";
    ss << R"(  "epoch_key": {)" << "\n";
    ss << R"(    "epoch_id": )" << epoch_key.epoch_id << ",\n";
    ss << R"(    "epoch_manifest_sha256": ")" << epoch_key.epoch_manifest_sha256 << "\"\n";
    ss << "  },\n";
    ss << R"(  "execution_class": ")" << executionClassToString(execution_class) << "\",\n";
    ss << R"(  "fingerprint_sha256": ")" << fingerprint_sha256 << "\",\n";

    // Query fingerprint (subset of fields for portability)
    ss << R"(  "query_fingerprint": {)" << "\n";
    ss << R"(    "epoch_id": )" << query_fp.epoch_id << ",\n";
    ss << R"(    "epoch_manifest_sha256": ")" << query_fp.epoch_manifest_sha256 << "\",\n";
    ss << R"(    "normalized_text_sha256": ")" << query_fp.normalized_text_sha256 << "\",\n";
    ss << R"(    "params_sha256": ")" << query_fp.params_sha256 << "\",\n";
    ss << R"(    "shape_feature_vector_hash": ")" << query_fp.shape_feature_vector_hash << "\",\n";
    ss << R"(    "shape_sha256": ")" << query_fp.shape_sha256 << "\"\n";
    ss << "  },\n";

    // Replay params (sorted map ensures deterministic order)
    ss << R"(  "replay_params": {)" << "\n";
    bool first = true;
    for (const auto& [key, value] : replay_params) {
      if (!first) ss << ",\n";
      ss << "    \"" << key << "\": \"" << value << "\"";
      first = false;
    }
    ss << "\n  },\n";

    ss << R"(  "sequence_id": )" << sequence_id << "\n";
    // NOTE: observed_latency_ns is intentionally EXCLUDED from JSON-LD
    // It is wall-clock time, informational only, not for correctness
    ss << "}";
    return ss.str();
  }

  // Validate record completeness
  [[nodiscard]] bool isValid() const {
    return epoch_key.isValid() && query_fp.isValid() &&
           !fingerprint_sha256.empty();
  }

  // Equality for testing
  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(
      WorkloadRecord, sequence_id, capture_timestamp_epoch, epoch_key,
      execution_class, fingerprint_sha256)
};

// ============================================================================
// WorkloadExcludedRecord - Non-deterministic query log entry
// ============================================================================
//
// Logged when a query is excluded from capture due to non-determinism.
// These are NOT captured as WorkloadRecords but logged for observability.
// NOT fail-closed: queries continue to execute, just not captured.
//
struct WorkloadExcludedRecord {
  // Sequence ID for ordering
  uint64_t sequence_id = 0;

  // Epoch context
  EpochKey epoch_key;

  // Reason for exclusion
  enum class ExclusionReason : uint8_t {
    NON_DETERMINISTIC_FUNCTION = 0,  // NOW(), RAND(), UUID(), BNODE()
    SERVICE_CLAUSE = 1,               // Federated query (SERVICE)
    AMBIGUOUS_PLAN = 2,               // Multiple valid execution plans
    SERIALIZATION_ERROR = 3,          // Failed to serialize to JSON-LD
    INTERNAL_ERROR = 4                // Other internal error
  };

  ExclusionReason reason = ExclusionReason::NON_DETERMINISTIC_FUNCTION;

  // Details about why excluded (e.g., which function was non-deterministic)
  std::string reason_details;

  // Query shape hash for analysis (even excluded queries have shapes)
  std::string shape_sha256;

  // Convert reason to string
  [[nodiscard]] static std::string reasonToString(ExclusionReason r) {
    switch (r) {
      case ExclusionReason::NON_DETERMINISTIC_FUNCTION:
        return "NON_DETERMINISTIC_FUNCTION";
      case ExclusionReason::SERVICE_CLAUSE:
        return "SERVICE_CLAUSE";
      case ExclusionReason::AMBIGUOUS_PLAN:
        return "AMBIGUOUS_PLAN";
      case ExclusionReason::SERIALIZATION_ERROR:
        return "SERIALIZATION_ERROR";
      case ExclusionReason::INTERNAL_ERROR:
        return "INTERNAL_ERROR";
      default:
        return "UNKNOWN";
    }
  }

  // Serialize to JSON-LD for observability logging
  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << R"(  "@context": "https://qlever.cs.uni-freiburg.de/workload/v1",)" << "\n";
    ss << R"(  "@type": "WorkloadExcludedRecord",)" << "\n";
    ss << R"(  "epoch_key": {)" << "\n";
    ss << R"(    "epoch_id": )" << epoch_key.epoch_id << ",\n";
    ss << R"(    "epoch_manifest_sha256": ")" << epoch_key.epoch_manifest_sha256 << "\"\n";
    ss << "  },\n";
    ss << R"(  "reason": ")" << reasonToString(reason) << "\",\n";
    ss << R"(  "reason_details": ")" << reason_details << "\",\n";
    ss << R"(  "sequence_id": )" << sequence_id << ",\n";
    ss << R"(  "shape_sha256": ")" << shape_sha256 << "\"\n";
    ss << "}";
    return ss.str();
  }
};

// ============================================================================
// Global Sequence Counter
// ============================================================================

// Thread-safe global monotonic sequence counter for WorkloadRecord ordering
// Ensures deterministic ordering across concurrent query captures
class WorkloadSequenceCounter {
 private:
  std::atomic<uint64_t> counter_{0};

 public:
  WorkloadSequenceCounter() = default;

  // Get next sequence ID (thread-safe, monotonic)
  uint64_t next() { return counter_.fetch_add(1, std::memory_order_relaxed); }

  // Get current value (for metrics/debugging)
  uint64_t current() const { return counter_.load(std::memory_order_relaxed); }

  // Reset counter (for testing only)
  void reset() { counter_.store(0, std::memory_order_relaxed); }
};

// Global singleton sequence counter
inline WorkloadSequenceCounter globalWorkloadSequenceCounter;

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_WORKLOADRECORD_H
