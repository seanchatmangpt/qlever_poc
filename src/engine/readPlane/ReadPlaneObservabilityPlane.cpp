// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Implementation of Read-Plane Observability Plane (EPIC 4 Subsystem
// 6)
//
// All signals are structured (JSON-LD), hashable, and indexable.
// No human-only logs. Fail-closed on emission errors.

#include "engine/readPlane/ReadPlaneObservabilityPlane.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace readPlane {

// ============================================================================
// Global Singleton Instance
// ============================================================================

ad_utility::Synchronized<ReadPlaneObservabilityPlane> globalObservabilityPlane;

// ============================================================================
// Simple SHA256-like hash for signal digests
// Note: In production, use a proper crypto library. This is a placeholder
// that produces deterministic 64-char hex strings.
// ============================================================================

namespace {

// Simple hash function for digest computation
// This is a placeholder - in production use OpenSSL SHA256 or similar
std::string computeSimpleHash(const std::string& input) {
  // Use std::hash and combine with input length for uniqueness
  std::hash<std::string> hasher;
  size_t hash1 = hasher(input);
  size_t hash2 = hasher(input + std::to_string(input.length()));

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  oss << std::setw(16) << hash1;
  oss << std::setw(16) << hash2;
  // Pad to 64 chars (SHA256-like output length)
  std::string result = oss.str();
  while (result.length() < 64) {
    result += "0";
  }
  return result.substr(0, 64);
}

// Generate UUID-like identifier
std::string generateUUID() {
  static std::atomic<uint64_t> counter{0};
  auto now =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  uint64_t id = counter.fetch_add(1);

  std::ostringstream oss;
  oss << std::hex << std::setfill('0');
  oss << std::setw(8) << (now >> 32);
  oss << "-";
  oss << std::setw(4) << ((now >> 16) & 0xFFFF);
  oss << "-";
  oss << std::setw(4) << (now & 0xFFFF);
  oss << "-";
  oss << std::setw(4) << ((id >> 16) & 0xFFFF);
  oss << "-";
  oss << std::setw(12) << (id & 0xFFFFFFFFFFFF);
  return oss.str();
}

// Escape string for JSON
std::string escapeJson(const std::string& input) {
  std::ostringstream oss;
  for (char c : input) {
    switch (c) {
      case '"':
        oss << "\\\"";
        break;
      case '\\':
        oss << "\\\\";
        break;
      case '\n':
        oss << "\\n";
        break;
      case '\r':
        oss << "\\r";
        break;
      case '\t':
        oss << "\\t";
        break;
      default:
        oss << c;
    }
  }
  return oss.str();
}

}  // namespace

// ============================================================================
// Constructor and Lifecycle
// ============================================================================

ReadPlaneObservabilityPlane::ReadPlaneObservabilityPlane()
    : ReadPlaneObservabilityPlane(SignalBufferConfig{}) {}

ReadPlaneObservabilityPlane::ReadPlaneObservabilityPlane(
    SignalBufferConfig config)
    : config_(std::move(config)) {
  // Initialize epoch start timestamp to now if not set
  if (config_.epoch_start_timestamp_ns == 0) {
    epoch_start_timestamp_ns_ = static_cast<uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
  } else {
    epoch_start_timestamp_ns_ = config_.epoch_start_timestamp_ns;
  }
}

// ============================================================================
// Signal Emission Methods
// ============================================================================

void ReadPlaneObservabilityPlane::emitCacheDecision(
    const CacheDecision& decision) {
  ObservabilitySignal signal(SignalType::CACHE_DECISION);
  signal.source_artifact = decision;
  signal.outcome = decision.admitted_to_cache ? SignalOutcome::SUCCESS
                                              : SignalOutcome::FAILURE;
  signal.context = buildContext(signal.source_artifact, "CacheCorrectnessProver");
  emit(std::move(signal));
}

void ReadPlaneObservabilityPlane::emitEpochTransition(
    const EpochPromotionEvent& event) {
  ObservabilitySignal signal(SignalType::EPOCH_TRANSITION);
  signal.source_artifact = event;
  signal.outcome = event.success ? SignalOutcome::SUCCESS : SignalOutcome::FAILURE;
  signal.context = buildContext(signal.source_artifact, "EpochManager");
  emit(std::move(signal));
}

void ReadPlaneObservabilityPlane::emitReplayDivergence(
    const ReplayResult& result, const std::string& expected_digest) {
  ObservabilitySignal signal(SignalType::REPLAY_DIVERGENCE);

  // Create a copy with expected_digest set
  ReplayResult resultWithExpected = result;
  resultWithExpected.expected_digest = expected_digest;
  resultWithExpected.digest_matches =
      (result.execution_digest == expected_digest);

  signal.source_artifact = resultWithExpected;
  signal.outcome = resultWithExpected.digest_matches ? SignalOutcome::SUCCESS
                                                     : SignalOutcome::DIVERGENCE;
  signal.context = buildContext(signal.source_artifact, "WorkloadReplayEngine");
  emit(std::move(signal));
}

void ReadPlaneObservabilityPlane::emitExecutionFingerprint(
    const ExecutionDigest& digest) {
  ObservabilitySignal signal(SignalType::EXECUTION_FINGERPRINT);
  signal.source_artifact = digest;
  signal.outcome =
      digest.digest_hash.empty() ? SignalOutcome::FAILURE : SignalOutcome::SUCCESS;
  signal.context = buildContext(signal.source_artifact, "ExecutionEnvelope");
  emit(std::move(signal));
}

void ReadPlaneObservabilityPlane::emitPerformanceChange(
    const PerformanceEnvelope& envelope, const PerformanceEnvelope& baseline) {
  ObservabilitySignal signal(SignalType::RESOURCE_ENVELOPE);
  signal.source_artifact = envelope;

  // Determine if this is a regression
  if (envelope.variance_bounds.is_regression) {
    signal.outcome = SignalOutcome::REGRESSION;
  } else {
    signal.outcome = SignalOutcome::SUCCESS;
  }

  signal.context = buildContext(signal.source_artifact, "BenchmarkRealizationEngine");

  // Add baseline comparison info to context tags
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(6);
  oss << baseline.cache_stats.bytes_hit_rate;
  signal.context.tags["baseline_bytes_hit_rate"] = oss.str();
  oss.str("");
  oss << baseline.latency_stats.p99_ns;
  signal.context.tags["baseline_p99_ns"] = oss.str();

  emit(std::move(signal));
}

void ReadPlaneObservabilityPlane::emitQueryArtifact(
    const WorkloadRecord& record) {
  ObservabilitySignal signal(SignalType::QUERY_ARTIFACT);
  signal.source_artifact = record;
  signal.outcome = SignalOutcome::SUCCESS;
  signal.context = buildContext(signal.source_artifact, "WorkloadCaptureAgent");
  emit(std::move(signal));
}

// ============================================================================
// Generic Signal Emission
// ============================================================================

void ReadPlaneObservabilityPlane::emit(ObservabilitySignal signal) {
  // Generate signal ID if not set
  if (signal.signal_id.empty()) {
    signal.signal_id = generateSignalId();
  }

  // Set timestamp if not set
  if (signal.timestamp_epoch_relative == 0) {
    signal.timestamp_epoch_relative = getCurrentEpochRelativeTimestamp();
  }

  // Compute digest if enabled
  if (config_.compute_digests && signal.signal_digest.empty()) {
    signal.signal_digest = computeSignalDigest(signal);
  }

  // Acquire write lock on buffer
  {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);

    // Check buffer capacity
    if (signal_buffer_.size() >= config_.max_signals) {
      if (config_.drop_oldest_on_overflow) {
        // Remove oldest signal
        if (!signal_buffer_.empty()) {
          // Remove from digest index
          {
            std::unique_lock<std::shared_mutex> idx_lock(index_mutex_);
            auto it = digest_index_.find(signal_buffer_.front().signal_digest);
            if (it != digest_index_.end()) {
              digest_index_.erase(it);
            }
          }
          signal_buffer_.pop_front();
          stats_.signals_dropped++;
        }
      } else {
        // Fail-closed: buffer full is a fatal error
        stats_.emission_errors++;
        throw std::runtime_error(
            "ObservabilityPlane: Signal buffer full and drop_oldest_on_overflow "
            "is disabled. This is a fatal error per fail-closed semantics.");
      }
    }

    // Add signal to buffer
    signal_buffer_.push_back(signal);

    // Update digest index
    if (!signal.signal_digest.empty()) {
      std::unique_lock<std::shared_mutex> idx_lock(index_mutex_);
      digest_index_[signal.signal_digest] = signal_buffer_.size() - 1;
    }
  }

  // Update statistics (atomic, no lock needed)
  updateStatistics(signal);
}

// ============================================================================
// Signal Querying
// ============================================================================

std::vector<ObservabilitySignal> ReadPlaneObservabilityPlane::querySignals(
    const SignalPredicate& predicate, const TimeRange& time_range) const {
  std::vector<ObservabilitySignal> result;

  std::shared_lock<std::shared_mutex> lock(buffer_mutex_);
  for (const auto& signal : signal_buffer_) {
    if (time_range.contains(signal.timestamp_epoch_relative) &&
        predicate(signal)) {
      result.push_back(signal);
    }
  }

  return result;
}

std::vector<ObservabilitySignal> ReadPlaneObservabilityPlane::queryByType(
    SignalType type, const TimeRange& time_range) const {
  return querySignals(ObservabilitySignal::byType(type), time_range);
}

std::vector<ObservabilitySignal> ReadPlaneObservabilityPlane::queryByOutcome(
    SignalOutcome outcome, const TimeRange& time_range) const {
  return querySignals(ObservabilitySignal::byOutcome(outcome), time_range);
}

std::vector<ObservabilitySignal> ReadPlaneObservabilityPlane::queryByEpoch(
    uint64_t epoch_id, const TimeRange& time_range) const {
  return querySignals(ObservabilitySignal::byEpochId(epoch_id), time_range);
}

std::vector<ObservabilitySignal> ReadPlaneObservabilityPlane::queryByFingerprint(
    const std::string& fingerprint, const TimeRange& time_range) const {
  return querySignals(ObservabilitySignal::byFingerprint(fingerprint),
                      time_range);
}

std::vector<ObservabilitySignal> ReadPlaneObservabilityPlane::getRecentSignals(
    size_t count) const {
  std::vector<ObservabilitySignal> result;

  std::shared_lock<std::shared_mutex> lock(buffer_mutex_);
  size_t start =
      signal_buffer_.size() > count ? signal_buffer_.size() - count : 0;
  for (size_t i = start; i < signal_buffer_.size(); ++i) {
    result.push_back(signal_buffer_[i]);
  }

  return result;
}

std::optional<ObservabilitySignal>
ReadPlaneObservabilityPlane::getSignalByDigest(const std::string& digest) const {
  std::shared_lock<std::shared_mutex> idx_lock(index_mutex_);
  auto it = digest_index_.find(digest);
  if (it == digest_index_.end()) {
    return std::nullopt;
  }

  std::shared_lock<std::shared_mutex> buf_lock(buffer_mutex_);
  if (it->second < signal_buffer_.size()) {
    return signal_buffer_[it->second];
  }

  return std::nullopt;
}

// ============================================================================
// Export Methods
// ============================================================================

std::string ReadPlaneObservabilityPlane::exportSignals(
    ExportFormat format) const {
  return exportSignals(format, ObservabilitySignal::all(), TimeRange::all());
}

std::string ReadPlaneObservabilityPlane::exportSignals(
    ExportFormat format, const SignalPredicate& predicate,
    const TimeRange& time_range) const {
  switch (format) {
    case ExportFormat::JSON_LD:
      return exportAsJsonLD(predicate, time_range);
    case ExportFormat::PROMETHEUS:
      return exportAsPrometheus();
    case ExportFormat::CSV:
      return exportAsCSV(predicate, time_range);
    default:
      throw std::runtime_error("Unknown export format");
  }
}

std::string ReadPlaneObservabilityPlane::exportAsJsonLD(
    const SignalPredicate& predicate, const TimeRange& time_range) const {
  auto signals = querySignals(predicate, time_range);

  std::ostringstream oss;
  oss << "{\"@context\":\"https://qlever.cs.uni-freiburg.de/ns/observability\""
      << ",\"@type\":\"SignalExport\""
      << ",\"format_version\":\"" << ObservabilitySignal::FORMAT_VERSION << "\""
      << ",\"export_timestamp\":" << getCurrentEpochRelativeTimestamp()
      << ",\"signal_count\":" << signals.size()
      << ",\"signals\":[";

  bool first = true;
  for (const auto& signal : signals) {
    if (!first) oss << ",";
    first = false;
    oss << signal.toJsonLD();
  }

  oss << "]}";
  return oss.str();
}

std::string ReadPlaneObservabilityPlane::exportAsPrometheus() const {
  std::ostringstream oss;

  // Signal type counters
  oss << "# HELP observability_signals_total Total signals by type\n";
  oss << "# TYPE observability_signals_total counter\n";
  oss << "observability_signals_total{type=\"CACHE_DECISION\"} "
      << stats_.cache_decision_count.load() << "\n";
  oss << "observability_signals_total{type=\"EPOCH_TRANSITION\"} "
      << stats_.epoch_transition_count.load() << "\n";
  oss << "observability_signals_total{type=\"REPLAY_DIVERGENCE\"} "
      << stats_.replay_divergence_count.load() << "\n";
  oss << "observability_signals_total{type=\"EXECUTION_FINGERPRINT\"} "
      << stats_.execution_fingerprint_count.load() << "\n";
  oss << "observability_signals_total{type=\"RESOURCE_ENVELOPE\"} "
      << stats_.resource_envelope_count.load() << "\n";
  oss << "observability_signals_total{type=\"QUERY_ARTIFACT\"} "
      << stats_.query_artifact_count.load() << "\n";

  // Outcome counters
  oss << "\n# HELP observability_outcomes_total Total signals by outcome\n";
  oss << "# TYPE observability_outcomes_total counter\n";
  oss << "observability_outcomes_total{outcome=\"SUCCESS\"} "
      << stats_.success_count.load() << "\n";
  oss << "observability_outcomes_total{outcome=\"FAILURE\"} "
      << stats_.failure_count.load() << "\n";
  oss << "observability_outcomes_total{outcome=\"DIVERGENCE\"} "
      << stats_.divergence_count.load() << "\n";
  oss << "observability_outcomes_total{outcome=\"REGRESSION\"} "
      << stats_.regression_count.load() << "\n";

  // Buffer health gauges
  oss << "\n# HELP observability_buffer_size Current buffer size\n";
  oss << "# TYPE observability_buffer_size gauge\n";
  oss << "observability_buffer_size " << getBufferSize() << "\n";

  oss << "\n# HELP observability_buffer_capacity Maximum buffer capacity\n";
  oss << "# TYPE observability_buffer_capacity gauge\n";
  oss << "observability_buffer_capacity " << getBufferCapacity() << "\n";

  oss << "\n# HELP observability_signals_emitted_total Total signals emitted\n";
  oss << "# TYPE observability_signals_emitted_total counter\n";
  oss << "observability_signals_emitted_total "
      << stats_.total_signals_emitted.load() << "\n";

  oss << "\n# HELP observability_signals_dropped_total Signals dropped due to "
         "overflow\n";
  oss << "# TYPE observability_signals_dropped_total counter\n";
  oss << "observability_signals_dropped_total " << stats_.signals_dropped.load()
      << "\n";

  oss << "\n# HELP observability_emission_errors_total Signal emission errors\n";
  oss << "# TYPE observability_emission_errors_total counter\n";
  oss << "observability_emission_errors_total "
      << stats_.emission_errors.load() << "\n";

  return oss.str();
}

std::string ReadPlaneObservabilityPlane::exportAsCSV(
    const SignalPredicate& predicate, const TimeRange& time_range) const {
  auto signals = querySignals(predicate, time_range);

  std::ostringstream oss;

  // CSV header
  oss << "signal_id,signal_type,outcome,timestamp_epoch_relative,"
      << "epoch_id,query_fingerprint_sha256,source_subsystem,signal_digest\n";

  // CSV rows
  for (const auto& signal : signals) {
    oss << escapeJson(signal.signal_id) << ","
        << signalTypeToString(signal.signal_type) << ","
        << outcomeToString(signal.outcome) << ","
        << signal.timestamp_epoch_relative << ","
        << signal.context.epoch_key.epoch_id << ","
        << escapeJson(signal.context.query_fingerprint_sha256) << ","
        << escapeJson(signal.context.source_subsystem) << ","
        << escapeJson(signal.signal_digest) << "\n";
  }

  return oss.str();
}

// ============================================================================
// Statistics and Health
// ============================================================================

SignalStatistics ReadPlaneObservabilityPlane::getStatistics() const {
  // Return a copy of atomic statistics
  SignalStatistics copy;
  copy.cache_decision_count = stats_.cache_decision_count.load();
  copy.epoch_transition_count = stats_.epoch_transition_count.load();
  copy.replay_divergence_count = stats_.replay_divergence_count.load();
  copy.execution_fingerprint_count = stats_.execution_fingerprint_count.load();
  copy.resource_envelope_count = stats_.resource_envelope_count.load();
  copy.query_artifact_count = stats_.query_artifact_count.load();
  copy.success_count = stats_.success_count.load();
  copy.failure_count = stats_.failure_count.load();
  copy.divergence_count = stats_.divergence_count.load();
  copy.regression_count = stats_.regression_count.load();
  copy.total_signals_emitted = stats_.total_signals_emitted.load();
  copy.signals_dropped = stats_.signals_dropped.load();
  copy.emission_errors = stats_.emission_errors.load();
  return copy;
}

size_t ReadPlaneObservabilityPlane::getBufferSize() const {
  std::shared_lock<std::shared_mutex> lock(buffer_mutex_);
  return signal_buffer_.size();
}

size_t ReadPlaneObservabilityPlane::getBufferCapacity() const {
  std::shared_lock<std::shared_mutex> lock(config_mutex_);
  return config_.max_signals;
}

bool ReadPlaneObservabilityPlane::isBufferFull() const {
  return getBufferSize() >= getBufferCapacity();
}

// ============================================================================
// Configuration
// ============================================================================

void ReadPlaneObservabilityPlane::setEpochStartTimestamp(
    uint64_t timestamp_ns) {
  epoch_start_timestamp_ns_ = timestamp_ns;
}

uint64_t ReadPlaneObservabilityPlane::getEpochStartTimestamp() const {
  return epoch_start_timestamp_ns_.load();
}

void ReadPlaneObservabilityPlane::setConfig(SignalBufferConfig config) {
  std::unique_lock<std::shared_mutex> lock(config_mutex_);
  config_ = std::move(config);
}

SignalBufferConfig ReadPlaneObservabilityPlane::getConfig() const {
  std::shared_lock<std::shared_mutex> lock(config_mutex_);
  return config_;
}

// ============================================================================
// Buffer Management
// ============================================================================

void ReadPlaneObservabilityPlane::clear() {
  {
    std::unique_lock<std::shared_mutex> lock(buffer_mutex_);
    signal_buffer_.clear();
  }
  {
    std::unique_lock<std::shared_mutex> lock(index_mutex_);
    digest_index_.clear();
  }
  stats_.reset();
}

void ReadPlaneObservabilityPlane::compactBefore(
    uint64_t timestamp_epoch_relative) {
  std::unique_lock<std::shared_mutex> buf_lock(buffer_mutex_);
  std::unique_lock<std::shared_mutex> idx_lock(index_mutex_);

  // Remove signals older than timestamp
  while (!signal_buffer_.empty() &&
         signal_buffer_.front().timestamp_epoch_relative <
             timestamp_epoch_relative) {
    // Remove from index
    auto it = digest_index_.find(signal_buffer_.front().signal_digest);
    if (it != digest_index_.end()) {
      digest_index_.erase(it);
    }
    signal_buffer_.pop_front();
    stats_.signals_dropped++;
  }

  // Rebuild index (indices have shifted)
  digest_index_.clear();
  for (size_t i = 0; i < signal_buffer_.size(); ++i) {
    if (!signal_buffer_[i].signal_digest.empty()) {
      digest_index_[signal_buffer_[i].signal_digest] = i;
    }
  }
}

// ============================================================================
// Singleton Access
// ============================================================================

ReadPlaneObservabilityPlane& ReadPlaneObservabilityPlane::instance() {
  static ReadPlaneObservabilityPlane instance_;
  return instance_;
}

// ============================================================================
// Internal Implementation
// ============================================================================

std::string ReadPlaneObservabilityPlane::generateSignalId() const {
  return generateUUID();
}

std::string ReadPlaneObservabilityPlane::computeSignalDigest(
    const ObservabilitySignal& signal) const {
  // Serialize signal without the digest field for deterministic hashing
  std::ostringstream oss;
  oss << signal.signal_id << "|"
      << signalTypeToString(signal.signal_type) << "|"
      << outcomeToString(signal.outcome) << "|"
      << signal.timestamp_epoch_relative << "|"
      << sourceArtifactToJsonLD(signal.source_artifact) << "|"
      << signal.context.toJsonLD();

  return computeSimpleHash(oss.str());
}

uint64_t ReadPlaneObservabilityPlane::getCurrentEpochRelativeTimestamp() const {
  auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
  uint64_t now_ns = static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
  uint64_t epoch_start = epoch_start_timestamp_ns_.load();

  // Return relative time in nanoseconds
  return now_ns > epoch_start ? now_ns - epoch_start : 0;
}

void ReadPlaneObservabilityPlane::updateStatistics(
    const ObservabilitySignal& signal) {
  stats_.total_signals_emitted++;

  // Update type-specific counter
  switch (signal.signal_type) {
    case SignalType::CACHE_DECISION:
      stats_.cache_decision_count++;
      break;
    case SignalType::EPOCH_TRANSITION:
      stats_.epoch_transition_count++;
      break;
    case SignalType::REPLAY_DIVERGENCE:
      stats_.replay_divergence_count++;
      break;
    case SignalType::EXECUTION_FINGERPRINT:
      stats_.execution_fingerprint_count++;
      break;
    case SignalType::RESOURCE_ENVELOPE:
      stats_.resource_envelope_count++;
      break;
    case SignalType::QUERY_ARTIFACT:
      stats_.query_artifact_count++;
      break;
  }

  // Update outcome counter
  switch (signal.outcome) {
    case SignalOutcome::SUCCESS:
      stats_.success_count++;
      break;
    case SignalOutcome::FAILURE:
      stats_.failure_count++;
      break;
    case SignalOutcome::DIVERGENCE:
      stats_.divergence_count++;
      break;
    case SignalOutcome::REGRESSION:
      stats_.regression_count++;
      break;
    default:
      break;
  }
}

SignalOutcome ReadPlaneObservabilityPlane::determineOutcome(
    const SourceArtifact& artifact) const {
  return std::visit(
      [](const auto& a) -> SignalOutcome {
        using T = std::decay_t<decltype(a)>;
        if constexpr (std::is_same_v<T, CacheDecision>) {
          return a.admitted_to_cache ? SignalOutcome::SUCCESS
                                     : SignalOutcome::FAILURE;
        } else if constexpr (std::is_same_v<T, EpochPromotionEvent>) {
          return a.success ? SignalOutcome::SUCCESS : SignalOutcome::FAILURE;
        } else if constexpr (std::is_same_v<T, ReplayResult>) {
          return a.digest_matches ? SignalOutcome::SUCCESS
                                  : SignalOutcome::DIVERGENCE;
        } else if constexpr (std::is_same_v<T, ExecutionDigest>) {
          return a.digest_hash.empty() ? SignalOutcome::FAILURE
                                       : SignalOutcome::SUCCESS;
        } else if constexpr (std::is_same_v<T, PerformanceEnvelope>) {
          return a.variance_bounds.is_regression ? SignalOutcome::REGRESSION
                                                 : SignalOutcome::SUCCESS;
        } else if constexpr (std::is_same_v<T, WorkloadRecord>) {
          return SignalOutcome::SUCCESS;
        } else {
          return SignalOutcome::UNKNOWN;
        }
      },
      artifact);
}

SignalContext ReadPlaneObservabilityPlane::buildContext(
    const SourceArtifact& artifact, const std::string& subsystem) const {
  SignalContext ctx;
  ctx.source_subsystem = subsystem;

  // Extract epoch key and fingerprint from artifact
  std::visit(
      [&ctx](const auto& a) {
        using T = std::decay_t<decltype(a)>;
        if constexpr (std::is_same_v<T, CacheDecision>) {
          ctx.epoch_key = a.epoch_key;
        } else if constexpr (std::is_same_v<T, EpochPromotionEvent>) {
          ctx.epoch_key = a.new_epoch;
        } else if constexpr (std::is_same_v<T, ReplayResult>) {
          ctx.epoch_key = a.replayed_on_epoch;
        } else if constexpr (std::is_same_v<T, ExecutionDigest>) {
          ctx.query_fingerprint_sha256 = a.query_fingerprint_sha256;
        } else if constexpr (std::is_same_v<T, PerformanceEnvelope>) {
          ctx.epoch_key = a.epoch_key;
        } else if constexpr (std::is_same_v<T, WorkloadRecord>) {
          ctx.epoch_key = a.epoch_key;
          ctx.query_fingerprint_sha256 = a.fingerprint_sha256;
        }
      },
      artifact);

  return ctx;
}

}  // namespace readPlane
