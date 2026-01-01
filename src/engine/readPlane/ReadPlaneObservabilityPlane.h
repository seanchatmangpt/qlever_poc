// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Read-Plane Observability Plane (EPIC 4 Subsystem 6)
//
// This is the convergence point for all 6 subsystems. Exposes everything
// the system knows about itself as machine-readable signals:
// - Cache decisions
// - Epoch transitions
// - Replay deviations
// - Execution fingerprints
// - Resource envelopes
// - Query artifacts
//
// All signals are structured (JSON-LD), hashable, and indexable.
// No human-only logs. Fail-closed on emission errors.

#ifndef QLEVER_SRC_ENGINE_READPLANE_READPLANEOBSERVABILITYPLANE_H
#define QLEVER_SRC_ENGINE_READPLANE_READPLANEOBSERVABILITYPLANE_H

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/readPlane/ObservabilitySignal.h"
#include "util/Synchronized.h"

namespace readPlane {

// ============================================================================
// Export Format Enumeration
// ============================================================================

enum class ExportFormat : uint32_t {
  JSON_LD = 1,    // Full structured JSON-LD (debugging, archival)
  PROMETHEUS = 2, // Prometheus metrics format (scraping)
  CSV = 3         // Tabular CSV (ML/analytics)
};

inline std::string exportFormatToString(ExportFormat format) {
  switch (format) {
    case ExportFormat::JSON_LD:
      return "JSON_LD";
    case ExportFormat::PROMETHEUS:
      return "PROMETHEUS";
    case ExportFormat::CSV:
      return "CSV";
    default:
      return "UNKNOWN";
  }
}

// ============================================================================
// Signal Buffer Configuration
// ============================================================================

struct SignalBufferConfig {
  // Maximum number of signals to retain in memory
  size_t max_signals = 100000;

  // When buffer is full, drop oldest signals (true) or fail (false)
  bool drop_oldest_on_overflow = true;

  // Enable digest computation for each signal
  bool compute_digests = true;

  // Epoch reference point for relative timestamps
  uint64_t epoch_start_timestamp_ns = 0;
};

// ============================================================================
// Signal Statistics (Counters for Prometheus export)
// ============================================================================

struct SignalStatistics {
  // Per-type counters
  std::atomic<uint64_t> cache_decision_count{0};
  std::atomic<uint64_t> epoch_transition_count{0};
  std::atomic<uint64_t> replay_divergence_count{0};
  std::atomic<uint64_t> execution_fingerprint_count{0};
  std::atomic<uint64_t> resource_envelope_count{0};
  std::atomic<uint64_t> query_artifact_count{0};

  // Outcome counters
  std::atomic<uint64_t> success_count{0};
  std::atomic<uint64_t> failure_count{0};
  std::atomic<uint64_t> divergence_count{0};
  std::atomic<uint64_t> regression_count{0};

  // Buffer health
  std::atomic<uint64_t> total_signals_emitted{0};
  std::atomic<uint64_t> signals_dropped{0};
  std::atomic<uint64_t> emission_errors{0};

  // Reset all counters
  void reset() {
    cache_decision_count = 0;
    epoch_transition_count = 0;
    replay_divergence_count = 0;
    execution_fingerprint_count = 0;
    resource_envelope_count = 0;
    query_artifact_count = 0;
    success_count = 0;
    failure_count = 0;
    divergence_count = 0;
    regression_count = 0;
    total_signals_emitted = 0;
    signals_dropped = 0;
    emission_errors = 0;
  }
};

// ============================================================================
// ReadPlaneObservabilityPlane Class
// ============================================================================

class ReadPlaneObservabilityPlane {
 public:
  // ========================================================================
  // Construction and Lifecycle
  // ========================================================================

  // Default constructor with default config
  ReadPlaneObservabilityPlane();

  // Constructor with custom config
  explicit ReadPlaneObservabilityPlane(SignalBufferConfig config);

  // Destructor
  ~ReadPlaneObservabilityPlane() = default;

  // Non-copyable, non-movable (singleton pattern)
  ReadPlaneObservabilityPlane(const ReadPlaneObservabilityPlane&) = delete;
  ReadPlaneObservabilityPlane& operator=(const ReadPlaneObservabilityPlane&) =
      delete;
  ReadPlaneObservabilityPlane(ReadPlaneObservabilityPlane&&) = delete;
  ReadPlaneObservabilityPlane& operator=(ReadPlaneObservabilityPlane&&) =
      delete;

  // ========================================================================
  // Signal Emission Methods (from all 6 subsystems)
  // ========================================================================

  // Emit cache decision signal (Subsystem 3: Cache Correctness Prover)
  void emitCacheDecision(const CacheDecision& decision);

  // Emit epoch transition signal (Subsystem 1: Epoch Management)
  void emitEpochTransition(const EpochPromotionEvent& event);

  // Emit replay divergence signal (Subsystem 2: Workload Replay Engine)
  // expected_digest is the digest from capture, for comparison
  void emitReplayDivergence(const ReplayResult& result,
                            const std::string& expected_digest);

  // Emit execution fingerprint signal (Subsystem 4: Execution Envelope)
  void emitExecutionFingerprint(const ExecutionDigest& digest);

  // Emit performance envelope change signal (Subsystem 5: Benchmark Engine)
  // baseline is the previous envelope for comparison
  void emitPerformanceChange(const PerformanceEnvelope& envelope,
                             const PerformanceEnvelope& baseline);

  // Emit query artifact signal (Subsystem 1: Workload Capture Agent)
  void emitQueryArtifact(const WorkloadRecord& record);

  // ========================================================================
  // Generic Signal Emission (for advanced use cases)
  // ========================================================================

  // Emit a pre-constructed signal
  void emit(ObservabilitySignal signal);

  // ========================================================================
  // Signal Querying
  // ========================================================================

  // Query signals matching predicate within time range
  [[nodiscard]] std::vector<ObservabilitySignal> querySignals(
      const SignalPredicate& predicate,
      const TimeRange& time_range = TimeRange::all()) const;

  // Query signals by type
  [[nodiscard]] std::vector<ObservabilitySignal> queryByType(
      SignalType type, const TimeRange& time_range = TimeRange::all()) const;

  // Query signals by outcome
  [[nodiscard]] std::vector<ObservabilitySignal> queryByOutcome(
      SignalOutcome outcome,
      const TimeRange& time_range = TimeRange::all()) const;

  // Query signals by epoch
  [[nodiscard]] std::vector<ObservabilitySignal> queryByEpoch(
      uint64_t epoch_id,
      const TimeRange& time_range = TimeRange::all()) const;

  // Query signals by query fingerprint
  [[nodiscard]] std::vector<ObservabilitySignal> queryByFingerprint(
      const std::string& fingerprint,
      const TimeRange& time_range = TimeRange::all()) const;

  // Get the most recent N signals
  [[nodiscard]] std::vector<ObservabilitySignal> getRecentSignals(
      size_t count) const;

  // Get signal by digest (for deduplication/lookup)
  [[nodiscard]] std::optional<ObservabilitySignal> getSignalByDigest(
      const std::string& digest) const;

  // ========================================================================
  // Export Methods
  // ========================================================================

  // Export all signals in specified format
  [[nodiscard]] std::string exportSignals(ExportFormat format) const;

  // Export filtered signals in specified format
  [[nodiscard]] std::string exportSignals(
      ExportFormat format, const SignalPredicate& predicate,
      const TimeRange& time_range = TimeRange::all()) const;

  // Export as JSON-LD array (structured, queryable)
  [[nodiscard]] std::string exportAsJsonLD(
      const SignalPredicate& predicate = ObservabilitySignal::all(),
      const TimeRange& time_range = TimeRange::all()) const;

  // Export as Prometheus metrics (gauge/counter)
  [[nodiscard]] std::string exportAsPrometheus() const;

  // Export as CSV (for ML/analytics)
  [[nodiscard]] std::string exportAsCSV(
      const SignalPredicate& predicate = ObservabilitySignal::all(),
      const TimeRange& time_range = TimeRange::all()) const;

  // ========================================================================
  // Statistics and Health
  // ========================================================================

  // Get current statistics (atomic snapshot)
  [[nodiscard]] SignalStatistics getStatistics() const;

  // Get current buffer size
  [[nodiscard]] size_t getBufferSize() const;

  // Get buffer capacity
  [[nodiscard]] size_t getBufferCapacity() const;

  // Check if buffer is at capacity
  [[nodiscard]] bool isBufferFull() const;

  // ========================================================================
  // Configuration
  // ========================================================================

  // Update epoch start timestamp (for relative timing)
  void setEpochStartTimestamp(uint64_t timestamp_ns);

  // Get current epoch start timestamp
  [[nodiscard]] uint64_t getEpochStartTimestamp() const;

  // Update buffer configuration
  void setConfig(SignalBufferConfig config);

  // Get current configuration
  [[nodiscard]] SignalBufferConfig getConfig() const;

  // ========================================================================
  // Buffer Management
  // ========================================================================

  // Clear all signals (for testing or epoch transitions)
  void clear();

  // Compact buffer (remove signals older than timestamp)
  void compactBefore(uint64_t timestamp_epoch_relative);

  // ========================================================================
  // Singleton Access (Optional)
  // ========================================================================

  // Get global singleton instance
  static ReadPlaneObservabilityPlane& instance();

 private:
  // ========================================================================
  // Internal Implementation
  // ========================================================================

  // Generate unique signal ID (UUID-like)
  [[nodiscard]] std::string generateSignalId() const;

  // Compute signal digest (SHA256 of serialized form)
  [[nodiscard]] std::string computeSignalDigest(
      const ObservabilitySignal& signal) const;

  // Get current epoch-relative timestamp
  [[nodiscard]] uint64_t getCurrentEpochRelativeTimestamp() const;

  // Update statistics based on signal
  void updateStatistics(const ObservabilitySignal& signal);

  // Determine outcome from source artifact
  [[nodiscard]] SignalOutcome determineOutcome(
      const SourceArtifact& artifact) const;

  // Build context for signal
  [[nodiscard]] SignalContext buildContext(const SourceArtifact& artifact,
                                           const std::string& subsystem) const;

  // ========================================================================
  // Data Members
  // ========================================================================

  // Configuration
  mutable std::shared_mutex config_mutex_;
  SignalBufferConfig config_;

  // Signal buffer (append-only, protected by shared_mutex)
  // Using deque for efficient front removal during compaction
  mutable std::shared_mutex buffer_mutex_;
  std::deque<ObservabilitySignal> signal_buffer_;

  // Digest index for fast lookup (digest -> buffer index)
  mutable std::shared_mutex index_mutex_;
  std::unordered_map<std::string, size_t> digest_index_;

  // Statistics (atomic, no locking needed)
  mutable SignalStatistics stats_;

  // Signal ID counter (atomic)
  mutable std::atomic<uint64_t> signal_id_counter_{0};

  // Epoch start timestamp for relative timing
  std::atomic<uint64_t> epoch_start_timestamp_ns_{0};
};

// ============================================================================
// Global Singleton Instance
// ============================================================================

// Thread-safe global observability plane
extern ad_utility::Synchronized<ReadPlaneObservabilityPlane>
    globalObservabilityPlane;

// Convenience macros for signal emission
#define OBSERVABILITY_EMIT_CACHE_DECISION(decision)          \
  do {                                                       \
    readPlane::globalObservabilityPlane.wlock()              \
        ->emitCacheDecision(decision);                       \
  } while (0)

#define OBSERVABILITY_EMIT_EPOCH_TRANSITION(event)           \
  do {                                                       \
    readPlane::globalObservabilityPlane.wlock()              \
        ->emitEpochTransition(event);                        \
  } while (0)

#define OBSERVABILITY_EMIT_REPLAY_DIVERGENCE(result, expected) \
  do {                                                         \
    readPlane::globalObservabilityPlane.wlock()                \
        ->emitReplayDivergence(result, expected);              \
  } while (0)

#define OBSERVABILITY_EMIT_EXECUTION_FINGERPRINT(digest)     \
  do {                                                       \
    readPlane::globalObservabilityPlane.wlock()              \
        ->emitExecutionFingerprint(digest);                  \
  } while (0)

#define OBSERVABILITY_EMIT_PERFORMANCE_CHANGE(envelope, baseline) \
  do {                                                            \
    readPlane::globalObservabilityPlane.wlock()                   \
        ->emitPerformanceChange(envelope, baseline);              \
  } while (0)

#define OBSERVABILITY_EMIT_QUERY_ARTIFACT(record)            \
  do {                                                       \
    readPlane::globalObservabilityPlane.wlock()              \
        ->emitQueryArtifact(record);                         \
  } while (0)

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_READPLANEOBSERVABILITYPLANE_H
