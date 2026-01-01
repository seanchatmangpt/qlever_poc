// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Observability signal types for Read-Plane Observability Plane
// (EPIC 4 Subsystem 6)
//
// All signals are structured (JSON-LD), hashable, and machine-readable.
// No human-readable-only logs. All signals indexable by digest.
//
// INVARIANT CONTRACT (from EPIC4_SHARED_INVARIANTS.md):
// - No human logs: All signals are structured (JSON-LD, not prose)
// - Machine-readable: Signals must be parseable and queryable
// - All signals hashable: Each signal has deterministic digest for indexing
// - Atomic visibility: Signal buffer is append-only, thread-safe
// - Failure = artifact: Unobservable failures are fatal (not silent)

#ifndef QLEVER_SRC_ENGINE_READPLANE_OBSERVABILITYSIGNAL_H
#define QLEVER_SRC_ENGINE_READPLANE_OBSERVABILITYSIGNAL_H

#include <chrono>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace readPlane {

// Forward declarations for artifact types from other subsystems
// These match the canonical artifacts from EPIC4_SHARED_INVARIANTS.md

// ============================================================================
// SignalType Enumeration
// ============================================================================

// Type of observability signal emitted by the system
enum class SignalType : uint32_t {
  CACHE_DECISION = 1,        // Cache admission/rejection decision
  EPOCH_TRANSITION = 2,      // Epoch state change (SEAL->SERVE, etc.)
  REPLAY_DIVERGENCE = 3,     // Workload replay detected divergence
  EXECUTION_FINGERPRINT = 4, // Deterministic execution digest computed
  RESOURCE_ENVELOPE = 5,     // Performance envelope update
  QUERY_ARTIFACT = 6         // Query workload record captured
};

// Convert SignalType to string for serialization
inline std::string signalTypeToString(SignalType type) {
  switch (type) {
    case SignalType::CACHE_DECISION:
      return "CACHE_DECISION";
    case SignalType::EPOCH_TRANSITION:
      return "EPOCH_TRANSITION";
    case SignalType::REPLAY_DIVERGENCE:
      return "REPLAY_DIVERGENCE";
    case SignalType::EXECUTION_FINGERPRINT:
      return "EXECUTION_FINGERPRINT";
    case SignalType::RESOURCE_ENVELOPE:
      return "RESOURCE_ENVELOPE";
    case SignalType::QUERY_ARTIFACT:
      return "QUERY_ARTIFACT";
    default:
      return "UNKNOWN";
  }
}

// ============================================================================
// Artifact Types (from Subsystems 1-5)
// These are the canonical artifact structures consumed by observability
// ============================================================================

// EpochKey for epoch binding (from shared invariants)
// Note: This is the observability plane's simplified view of EpochKey.
// Other subsystems may have their own versions with additional fields.
struct ObservabilityEpochKey {
  uint64_t epoch_id = 0;
  std::string manifest_sha256;

  ObservabilityEpochKey() = default;
  ObservabilityEpochKey(uint64_t id, std::string manifest)
      : epoch_id(id), manifest_sha256(std::move(manifest)) {}

  bool operator==(const ObservabilityEpochKey& other) const {
    return epoch_id == other.epoch_id &&
           manifest_sha256 == other.manifest_sha256;
  }
  bool operator!=(const ObservabilityEpochKey& other) const {
    return !(*this == other);
  }

  [[nodiscard]] bool isValid() const {
    return epoch_id > 0 && !manifest_sha256.empty();
  }

  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream oss;
    oss << "{\"@type\":\"EpochKey\",\"epoch_id\":" << epoch_id
        << ",\"manifest_sha256\":\"" << manifest_sha256 << "\"}";
    return oss.str();
  }
};

// Alias for backward compatibility within this file
using EpochKey = ObservabilityEpochKey;

// CacheDecision artifact (Subsystem 3: Cache Correctness Prover)
struct CacheDecision {
  uint64_t record_sequence_id = 0;
  EpochKey epoch_key;
  std::string decision_type;  // BYTES_HIT, BYTES_MISS, PLAN_HIT, etc.

  struct Factors {
    bool is_deterministic = false;
    bool meets_frequency_threshold = false;
    bool is_in_top_k_shapes = false;
    uint64_t observed_frequency = 0;
    std::string admission_reason;
  } factors;

  bool admitted_to_cache = false;

  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream oss;
    oss << "{\"@type\":\"CacheDecision\""
        << ",\"record_sequence_id\":" << record_sequence_id
        << ",\"epoch_key\":" << epoch_key.toJsonLD()
        << ",\"decision_type\":\"" << decision_type << "\""
        << ",\"factors\":{\"is_deterministic\":"
        << (factors.is_deterministic ? "true" : "false")
        << ",\"meets_frequency_threshold\":"
        << (factors.meets_frequency_threshold ? "true" : "false")
        << ",\"is_in_top_k_shapes\":"
        << (factors.is_in_top_k_shapes ? "true" : "false")
        << ",\"observed_frequency\":" << factors.observed_frequency
        << ",\"admission_reason\":\"" << factors.admission_reason << "\"}"
        << ",\"admitted_to_cache\":"
        << (admitted_to_cache ? "true" : "false") << "}";
    return oss.str();
  }
};

// EpochPromotionEvent artifact (Subsystem 1: Epoch Management)
struct EpochPromotionEvent {
  EpochKey old_epoch;
  EpochKey new_epoch;
  std::string transition_type;  // SEAL_TO_SERVE, SERVE_TO_INIT, etc.
  uint64_t transition_timestamp_epoch_relative = 0;
  bool success = false;
  std::string failure_reason;

  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream oss;
    oss << "{\"@type\":\"EpochPromotionEvent\""
        << ",\"old_epoch\":" << old_epoch.toJsonLD()
        << ",\"new_epoch\":" << new_epoch.toJsonLD()
        << ",\"transition_type\":\"" << transition_type << "\""
        << ",\"transition_timestamp_epoch_relative\":"
        << transition_timestamp_epoch_relative
        << ",\"success\":" << (success ? "true" : "false");
    if (!failure_reason.empty()) {
      oss << ",\"failure_reason\":\"" << failure_reason << "\"";
    }
    oss << "}";
    return oss.str();
  }
};

// ReplayResult artifact (Subsystem 2: Workload Replay Engine)
struct ReplayResult {
  uint64_t replay_run_id = 0;
  uint64_t workload_record_id = 0;
  EpochKey replayed_on_epoch;
  std::string replayed_on_hostname;
  std::string execution_status;  // SUCCESS, DIVERGENCE, ABORT
  std::string execution_digest;
  std::string expected_digest;
  bool digest_matches = false;

  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream oss;
    oss << "{\"@type\":\"ReplayResult\""
        << ",\"replay_run_id\":" << replay_run_id
        << ",\"workload_record_id\":" << workload_record_id
        << ",\"replayed_on_epoch\":" << replayed_on_epoch.toJsonLD()
        << ",\"replayed_on_hostname\":\"" << replayed_on_hostname << "\""
        << ",\"execution_status\":\"" << execution_status << "\""
        << ",\"execution_digest\":\"" << execution_digest << "\""
        << ",\"expected_digest\":\"" << expected_digest << "\""
        << ",\"digest_matches\":" << (digest_matches ? "true" : "false") << "}";
    return oss.str();
  }
};

// ExecutionDigest artifact (Subsystem 4: Execution Envelope)
struct ExecutionDigest {
  std::string query_fingerprint_sha256;
  std::string plan_hash;
  std::string resource_signature;
  std::string result_length_hash;
  std::string result_shape_hash;
  std::string digest_hash;  // Final combined digest

  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream oss;
    oss << "{\"@type\":\"ExecutionDigest\""
        << ",\"query_fingerprint_sha256\":\"" << query_fingerprint_sha256
        << "\""
        << ",\"plan_hash\":\"" << plan_hash << "\""
        << ",\"resource_signature\":\"" << resource_signature << "\""
        << ",\"result_length_hash\":\"" << result_length_hash << "\""
        << ",\"result_shape_hash\":\"" << result_shape_hash << "\""
        << ",\"digest_hash\":\"" << digest_hash << "\"}";
    return oss.str();
  }
};

// PerformanceEnvelope artifact (Subsystem 5: Benchmark Realization Engine)
struct PerformanceEnvelope {
  std::string workload_id;
  EpochKey epoch_key;

  struct LatencyStats {
    uint64_t p50_ns = 0;
    uint64_t p95_ns = 0;
    uint64_t p99_ns = 0;
    uint64_t max_ns = 0;
  } latency_stats;

  struct CacheStats {
    uint64_t bytes_hits = 0;
    uint64_t bytes_misses = 0;
    uint64_t plan_hits = 0;
    uint64_t plan_misses = 0;
    uint64_t neg_hits = 0;
    float bytes_hit_rate = 0.0f;
    float plan_hit_rate = 0.0f;
  } cache_stats;

  struct Stability {
    float latency_coefficient_of_variation = 0.0f;
    float plan_reuse_rate = 0.0f;
    float epoch_transition_overhead_pct = 0.0f;
  } stability;

  struct VarianceBounds {
    uint64_t acceptable_latency_variance_pct = 10;
    uint64_t acceptable_cache_hit_rate_change = 5;
    bool is_regression = false;
  } variance_bounds;

  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream oss;
    oss << std::fixed;
    oss.precision(6);
    oss << "{\"@type\":\"PerformanceEnvelope\""
        << ",\"workload_id\":\"" << workload_id << "\""
        << ",\"epoch_key\":" << epoch_key.toJsonLD() << ",\"latency_stats\":{"
        << "\"p50_ns\":" << latency_stats.p50_ns
        << ",\"p95_ns\":" << latency_stats.p95_ns
        << ",\"p99_ns\":" << latency_stats.p99_ns
        << ",\"max_ns\":" << latency_stats.max_ns << "}"
        << ",\"cache_stats\":{"
        << "\"bytes_hits\":" << cache_stats.bytes_hits
        << ",\"bytes_misses\":" << cache_stats.bytes_misses
        << ",\"plan_hits\":" << cache_stats.plan_hits
        << ",\"plan_misses\":" << cache_stats.plan_misses
        << ",\"neg_hits\":" << cache_stats.neg_hits
        << ",\"bytes_hit_rate\":" << cache_stats.bytes_hit_rate
        << ",\"plan_hit_rate\":" << cache_stats.plan_hit_rate << "}"
        << ",\"stability\":{"
        << "\"latency_coefficient_of_variation\":"
        << stability.latency_coefficient_of_variation
        << ",\"plan_reuse_rate\":" << stability.plan_reuse_rate
        << ",\"epoch_transition_overhead_pct\":"
        << stability.epoch_transition_overhead_pct << "}"
        << ",\"variance_bounds\":{"
        << "\"acceptable_latency_variance_pct\":"
        << variance_bounds.acceptable_latency_variance_pct
        << ",\"acceptable_cache_hit_rate_change\":"
        << variance_bounds.acceptable_cache_hit_rate_change
        << ",\"is_regression\":"
        << (variance_bounds.is_regression ? "true" : "false") << "}}";
    return oss.str();
  }
};

// QueryFingerprint for query identity (simplified for observability)
struct QueryFingerprint {
  std::string shape_sha256;
  std::string params_sha256;
  std::string raw_query_sha256;
  EpochKey epoch_key;

  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream oss;
    oss << "{\"@type\":\"QueryFingerprint\""
        << ",\"shape_sha256\":\"" << shape_sha256 << "\""
        << ",\"params_sha256\":\"" << params_sha256 << "\""
        << ",\"raw_query_sha256\":\"" << raw_query_sha256 << "\""
        << ",\"epoch_key\":" << epoch_key.toJsonLD() << "}";
    return oss.str();
  }
};

// WorkloadRecord artifact (Subsystem 1: Workload Capture Agent)
struct WorkloadRecord {
  uint64_t sequence_id = 0;
  uint64_t capture_timestamp_epoch = 0;
  EpochKey epoch_key;
  QueryFingerprint query_fp;
  std::string execution_class;  // CACHE_HIT_BYTES, CACHE_MISS, etc.
  uint64_t observed_latency_ns = 0;
  std::map<std::string, std::string> replay_params;
  std::string fingerprint_sha256;

  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream oss;
    oss << "{\"@type\":\"WorkloadRecord\""
        << ",\"sequence_id\":" << sequence_id
        << ",\"capture_timestamp_epoch\":" << capture_timestamp_epoch
        << ",\"epoch_key\":" << epoch_key.toJsonLD()
        << ",\"query_fp\":" << query_fp.toJsonLD() << ",\"execution_class\":\""
        << execution_class << "\""
        << ",\"observed_latency_ns\":" << observed_latency_ns
        << ",\"fingerprint_sha256\":\"" << fingerprint_sha256 << "\""
        << ",\"replay_params\":{";
    bool first = true;
    for (const auto& [k, v] : replay_params) {
      if (!first) oss << ",";
      first = false;
      oss << "\"" << k << "\":\"" << v << "\"";
    }
    oss << "}}";
    return oss.str();
  }
};

// ============================================================================
// SourceArtifact Variant
// ============================================================================

// Union type for all source artifacts
using SourceArtifact =
    std::variant<CacheDecision, EpochPromotionEvent, ReplayResult,
                 ExecutionDigest, PerformanceEnvelope, WorkloadRecord>;

// Get JSON-LD representation of any source artifact
inline std::string sourceArtifactToJsonLD(const SourceArtifact& artifact) {
  return std::visit([](const auto& a) { return a.toJsonLD(); }, artifact);
}

// ============================================================================
// SignalContext - Additional metadata for signals
// ============================================================================

struct SignalContext {
  EpochKey epoch_key;
  std::string query_fingerprint_sha256;
  std::string source_subsystem;  // Which subsystem emitted this signal
  std::map<std::string, std::string> tags;

  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream oss;
    oss << "{\"@type\":\"SignalContext\""
        << ",\"epoch_key\":" << epoch_key.toJsonLD()
        << ",\"query_fingerprint_sha256\":\"" << query_fingerprint_sha256
        << "\""
        << ",\"source_subsystem\":\"" << source_subsystem << "\""
        << ",\"tags\":{";
    bool first = true;
    for (const auto& [k, v] : tags) {
      if (!first) oss << ",";
      first = false;
      oss << "\"" << k << "\":\"" << v << "\"";
    }
    oss << "}}";
    return oss.str();
  }
};

// ============================================================================
// SignalOutcome - Result classification
// ============================================================================

enum class SignalOutcome : uint32_t {
  SUCCESS = 1,
  FAILURE = 2,
  DIVERGENCE = 3,
  REGRESSION = 4,
  EPOCH_MISMATCH = 5,
  NON_DETERMINISTIC = 6,
  UNKNOWN = 99
};

inline std::string outcomeToString(SignalOutcome outcome) {
  switch (outcome) {
    case SignalOutcome::SUCCESS:
      return "SUCCESS";
    case SignalOutcome::FAILURE:
      return "FAILURE";
    case SignalOutcome::DIVERGENCE:
      return "DIVERGENCE";
    case SignalOutcome::REGRESSION:
      return "REGRESSION";
    case SignalOutcome::EPOCH_MISMATCH:
      return "EPOCH_MISMATCH";
    case SignalOutcome::NON_DETERMINISTIC:
      return "NON_DETERMINISTIC";
    default:
      return "UNKNOWN";
  }
}

// ============================================================================
// SignalPredicate - For filtering/querying signals
// ============================================================================

// Forward declaration
struct ObservabilitySignal;

// Predicate function type for filtering signals
using SignalPredicate = std::function<bool(const ObservabilitySignal&)>;

// ============================================================================
// ObservabilitySignal - The main signal container
// ============================================================================

struct ObservabilitySignal {
  // Signal identity
  std::string signal_id;  // UUID unique identifier

  // Signal classification
  SignalType signal_type;
  SignalOutcome outcome = SignalOutcome::UNKNOWN;

  // Timing (epoch-relative, not wall-clock)
  uint64_t timestamp_epoch_relative = 0;

  // The actual artifact that triggered this signal
  SourceArtifact source_artifact;

  // Additional context
  SignalContext context;

  // Deterministic digest for indexing (SHA256 of serialized form)
  std::string signal_digest;

  // Format version for compatibility
  static constexpr const char* FORMAT_VERSION = "v1.0";

  // Default constructor
  ObservabilitySignal() = default;

  // Constructor with signal type
  explicit ObservabilitySignal(SignalType type) : signal_type(type) {}

  // ========================================================================
  // JSON-LD Serialization (Deterministic)
  // ========================================================================

  [[nodiscard]] std::string toJsonLD() const {
    std::ostringstream oss;
    oss << "{\"@context\":\"https://qlever.cs.uni-freiburg.de/ns/observability\""
        << ",\"@type\":\"ObservabilitySignal\""
        << ",\"format_version\":\"" << FORMAT_VERSION << "\""
        << ",\"signal_id\":\"" << signal_id << "\""
        << ",\"signal_type\":\"" << signalTypeToString(signal_type) << "\""
        << ",\"outcome\":\"" << outcomeToString(outcome) << "\""
        << ",\"timestamp_epoch_relative\":" << timestamp_epoch_relative
        << ",\"source_artifact\":" << sourceArtifactToJsonLD(source_artifact)
        << ",\"context\":" << context.toJsonLD()
        << ",\"signal_digest\":\"" << signal_digest << "\"}";
    return oss.str();
  }

  // ========================================================================
  // Predicate matching for filtering
  // ========================================================================

  [[nodiscard]] bool matches(const SignalPredicate& predicate) const {
    return predicate(*this);
  }

  // ========================================================================
  // Convenience predicates (static factory methods)
  // ========================================================================

  // Match by signal type
  static SignalPredicate byType(SignalType type) {
    return [type](const ObservabilitySignal& s) {
      return s.signal_type == type;
    };
  }

  // Match by outcome
  static SignalPredicate byOutcome(SignalOutcome outcome) {
    return [outcome](const ObservabilitySignal& s) {
      return s.outcome == outcome;
    };
  }

  // Match by epoch ID
  static SignalPredicate byEpochId(uint64_t epochId) {
    return [epochId](const ObservabilitySignal& s) {
      return s.context.epoch_key.epoch_id == epochId;
    };
  }

  // Match by query fingerprint
  static SignalPredicate byFingerprint(const std::string& fingerprint) {
    return [fingerprint](const ObservabilitySignal& s) {
      return s.context.query_fingerprint_sha256 == fingerprint;
    };
  }

  // Match by time range (epoch-relative)
  static SignalPredicate byTimeRange(uint64_t start, uint64_t end) {
    return [start, end](const ObservabilitySignal& s) {
      return s.timestamp_epoch_relative >= start &&
             s.timestamp_epoch_relative <= end;
    };
  }

  // Match by source subsystem
  static SignalPredicate bySubsystem(const std::string& subsystem) {
    return [subsystem](const ObservabilitySignal& s) {
      return s.context.source_subsystem == subsystem;
    };
  }

  // Combine predicates with AND
  static SignalPredicate andPredicate(const SignalPredicate& a,
                                      const SignalPredicate& b) {
    return [a, b](const ObservabilitySignal& s) { return a(s) && b(s); };
  }

  // Combine predicates with OR
  static SignalPredicate orPredicate(const SignalPredicate& a,
                                     const SignalPredicate& b) {
    return [a, b](const ObservabilitySignal& s) { return a(s) || b(s); };
  }

  // Match all (no filter)
  static SignalPredicate all() {
    return [](const ObservabilitySignal&) { return true; };
  }
};

// ============================================================================
// TimeRange helper for queries
// ============================================================================

struct TimeRange {
  uint64_t start_epoch_relative = 0;
  uint64_t end_epoch_relative = UINT64_MAX;

  TimeRange() = default;
  TimeRange(uint64_t start, uint64_t end)
      : start_epoch_relative(start), end_epoch_relative(end) {}

  [[nodiscard]] bool contains(uint64_t timestamp) const {
    return timestamp >= start_epoch_relative &&
           timestamp <= end_epoch_relative;
  }

  // Convenience factory methods
  static TimeRange all() { return TimeRange(); }

  static TimeRange since(uint64_t start) {
    return TimeRange(start, UINT64_MAX);
  }

  static TimeRange until(uint64_t end) { return TimeRange(0, end); }

  static TimeRange lastN(uint64_t current, uint64_t duration) {
    return TimeRange(current > duration ? current - duration : 0, current);
  }
};

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_OBSERVABILITYSIGNAL_H
