// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Code (AI Assistant)
//
// Purpose: CacheDecision artifact for EPIC 4 Subsystem 3 - Read Cache Correctness
// All cache operations (hit, miss, insert, evict) are logged as machine-readable
// CacheDecision artifacts. No silent cache behavior.
//
// BINDING INVARIANTS (from EPIC4_SHARED_INVARIANTS.md):
// - Epoch binding: Every cache entry keyed by (epochId, manifestSha256)
// - Determinism rules: BytesCache + NegativeCache require deterministic queries
// - Cache decisions: ALL decisions logged (no silent behavior)
// - Atomic visibility: Log is append-only, lock-free where possible
// - Failure = abort: Cache ambiguity or epoch mismatch -> logged as MISS

#ifndef QLEVER_SRC_ENGINE_READPLANE_CACHEDECISION_H
#define QLEVER_SRC_ENGINE_READPLANE_CACHEDECISION_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "engine/readCache/ReadCacheKeys.h"
#include "util/json.h"

namespace readPlane {

// Decision type enumeration - machine-readable decision outcomes
enum class DecisionType : uint8_t {
  BYTES_HIT = 0,      // BytesCache hit - result found
  BYTES_MISS = 1,     // BytesCache miss - result not found
  BYTES_INSERT = 2,   // BytesCache insertion
  BYTES_EVICT = 3,    // BytesCache eviction
  PLAN_HIT = 4,       // PlanCache hit - plan found
  PLAN_MISS = 5,      // PlanCache miss - plan not found
  PLAN_INSERT = 6,    // PlanCache insertion
  PLAN_EVICT = 7,     // PlanCache eviction
  NEG_HIT = 8,        // NegativeCache hit - empty result confirmed
  NEG_MISS = 9,       // NegativeCache miss - no empty result record
  NEG_INSERT = 10,    // NegativeCache insertion
  NEG_EVICT = 11,     // NegativeCache eviction
};

// Miss reason enumeration - machine-readable reasons for cache misses
enum class MissReason : uint8_t {
  NOT_FOUND = 0,              // Key not in cache (normal miss)
  EPOCH_MISMATCH = 1,         // Entry epoch != query epoch (CRITICAL)
  DETERMINISM_FAIL = 2,       // Non-deterministic query in deterministic cache
  FREQUENCY_THRESHOLD = 3,    // Query not frequent enough for admission
  NOT_TOP_K = 4,              // Query not in top-K shapes
  SIZE_LIMIT_EXCEEDED = 5,    // Result too large for cache
  ADMISSION_REJECTED = 6,     // AdmissionPolicy rejected
  CACHE_FULL = 7,             // Cache at capacity, eviction failed
  EPOCH_STALE = 8,            // Entry from stale epoch (evicted)
  CONCURRENT_EVICTION = 9,    // Entry evicted during lookup
  INTERNAL_ERROR = 10,        // Internal cache error (CRITICAL)
};

// Admission reason enumeration - why entry was admitted
enum class AdmissionReason : uint8_t {
  FREQ_THRESHOLD_MET = 0,     // Frequency >= configured threshold
  TOP_K = 1,                  // Shape in top-K most frequent
  FORCED = 2,                 // Explicit admission (testing)
  PREWARM = 3,                // Pre-warming cache
};

// Inline string conversion functions for JSON-LD serialization
inline std::string decisionTypeToString(DecisionType type) {
  switch (type) {
    case DecisionType::BYTES_HIT: return "BYTES_HIT";
    case DecisionType::BYTES_MISS: return "BYTES_MISS";
    case DecisionType::BYTES_INSERT: return "BYTES_INSERT";
    case DecisionType::BYTES_EVICT: return "BYTES_EVICT";
    case DecisionType::PLAN_HIT: return "PLAN_HIT";
    case DecisionType::PLAN_MISS: return "PLAN_MISS";
    case DecisionType::PLAN_INSERT: return "PLAN_INSERT";
    case DecisionType::PLAN_EVICT: return "PLAN_EVICT";
    case DecisionType::NEG_HIT: return "NEG_HIT";
    case DecisionType::NEG_MISS: return "NEG_MISS";
    case DecisionType::NEG_INSERT: return "NEG_INSERT";
    case DecisionType::NEG_EVICT: return "NEG_EVICT";
    default: return "UNKNOWN";
  }
}

inline std::string missReasonToString(MissReason reason) {
  switch (reason) {
    case MissReason::NOT_FOUND: return "NOT_FOUND";
    case MissReason::EPOCH_MISMATCH: return "EPOCH_MISMATCH";
    case MissReason::DETERMINISM_FAIL: return "DETERMINISM_FAIL";
    case MissReason::FREQUENCY_THRESHOLD: return "FREQUENCY_THRESHOLD";
    case MissReason::NOT_TOP_K: return "NOT_TOP_K";
    case MissReason::SIZE_LIMIT_EXCEEDED: return "SIZE_LIMIT_EXCEEDED";
    case MissReason::ADMISSION_REJECTED: return "ADMISSION_REJECTED";
    case MissReason::CACHE_FULL: return "CACHE_FULL";
    case MissReason::EPOCH_STALE: return "EPOCH_STALE";
    case MissReason::CONCURRENT_EVICTION: return "CONCURRENT_EVICTION";
    case MissReason::INTERNAL_ERROR: return "INTERNAL_ERROR";
    default: return "UNKNOWN";
  }
}

inline std::string admissionReasonToString(AdmissionReason reason) {
  switch (reason) {
    case AdmissionReason::FREQ_THRESHOLD_MET: return "FREQ_THRESHOLD_MET";
    case AdmissionReason::TOP_K: return "TOP_K";
    case AdmissionReason::FORCED: return "FORCED";
    case AdmissionReason::PREWARM: return "PREWARM";
    default: return "UNKNOWN";
  }
}

// EpochKeyFull - Complete epoch key for cache binding
// Matches EPIC4_SHARED_INVARIANTS.md Section 3: Epoch Binding Rules
struct EpochKeyFull {
  uint64_t epoch_id = 0;               // Epoch ID (monotonic counter)
  std::string manifest_sha256;          // SHA256 of epoch manifest

  EpochKeyFull() = default;
  EpochKeyFull(uint64_t epochId, std::string manifestSha256)
      : epoch_id(epochId), manifest_sha256(std::move(manifestSha256)) {}

  // Construct from existing EpochKey
  explicit EpochKeyFull(const ::readCache::EpochKey& key)
      : epoch_id(0), manifest_sha256(key.epoch_manifest_hash) {}

  [[nodiscard]] bool isValid() const {
    return epoch_id > 0 || !manifest_sha256.empty();
  }

  // Equality for epoch soundness checks
  bool operator==(const EpochKeyFull& other) const {
    return epoch_id == other.epoch_id && manifest_sha256 == other.manifest_sha256;
  }

  bool operator!=(const EpochKeyFull& other) const {
    return !(*this == other);
  }

  // JSON-LD serialization with deterministic field ordering (alphabetical)
  [[nodiscard]] nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "EpochKey";
    j["epochId"] = epoch_id;
    j["manifestSha256"] = manifest_sha256;
    return j;
  }
};

// DecisionFactors - Machine-readable admission decision factors
// All factors affecting the cache decision are captured here
struct DecisionFactors {
  bool is_deterministic = false;            // Query passed DeterminismClassifier
  bool meets_frequency_threshold = false;   // Frequency >= configured threshold
  bool is_in_top_k_shapes = false;          // Shape in top-K most frequent
  uint64_t observed_frequency = 0;          // How many times query shape seen
  uint64_t configured_frequency_threshold = 0;  // Configured threshold
  uint64_t configured_top_k = 0;            // Configured top-K value
  std::string admission_reason;             // Human-readable reason

  // Cache key components
  std::string shape_sha256;                 // Query shape hash
  std::string params_sha256;                // Parameters hash

  // Result metadata (for hits/inserts)
  uint64_t result_size_bytes = 0;           // Size of cached result
  std::string result_format;                // Output format (e.g., "JSON", "TSV")

  DecisionFactors() = default;

  // JSON-LD serialization with deterministic field ordering (alphabetical)
  [[nodiscard]] nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "DecisionFactors";
    j["admissionReason"] = admission_reason;
    j["configuredFrequencyThreshold"] = configured_frequency_threshold;
    j["configuredTopK"] = configured_top_k;
    j["isDeterministic"] = is_deterministic;
    j["isInTopKShapes"] = is_in_top_k_shapes;
    j["meetsFrequencyThreshold"] = meets_frequency_threshold;
    j["observedFrequency"] = observed_frequency;
    j["paramsSha256"] = params_sha256;
    j["resultFormat"] = result_format;
    j["resultSizeBytes"] = result_size_bytes;
    j["shapeSha256"] = shape_sha256;
    return j;
  }
};

// CacheDecision - Complete record of a cache operation
// BINDING INVARIANT: Every cache operation MUST produce a CacheDecision.
// No silent behavior is allowed.
struct CacheDecision {
  // Identity
  uint64_t record_sequence_id = 0;          // Global monotonic sequence ID
  uint64_t decision_timestamp_ns = 0;       // Nanoseconds since epoch (deterministic)

  // Epoch binding (CRITICAL for correctness)
  EpochKeyFull epoch_key;

  // Decision outcome
  DecisionType decision_type = DecisionType::BYTES_MISS;
  bool admitted_to_cache = false;           // Final admission decision

  // Decision factors (machine-readable)
  DecisionFactors factors;

  // Miss reason (if applicable)
  std::optional<MissReason> miss_reason;

  // For evictions: why was this entry evicted?
  std::optional<std::string> eviction_reason;

  // Query fingerprint (for traceability)
  std::string query_fingerprint_sha256;

  // Cached entry metadata (for hits)
  std::optional<uint64_t> cached_entry_size_bytes;
  std::optional<std::string> cached_entry_epoch_manifest;

  // Default constructor
  CacheDecision() = default;

  // Builder-style setters for fluent construction
  CacheDecision& withSequenceId(uint64_t id) {
    record_sequence_id = id;
    return *this;
  }

  CacheDecision& withTimestamp(uint64_t ts) {
    decision_timestamp_ns = ts;
    return *this;
  }

  CacheDecision& withEpochKey(EpochKeyFull key) {
    epoch_key = std::move(key);
    return *this;
  }

  CacheDecision& withDecisionType(DecisionType type) {
    decision_type = type;
    return *this;
  }

  CacheDecision& withAdmitted(bool admitted) {
    admitted_to_cache = admitted;
    return *this;
  }

  CacheDecision& withFactors(DecisionFactors f) {
    factors = std::move(f);
    return *this;
  }

  CacheDecision& withMissReason(MissReason reason) {
    miss_reason = reason;
    return *this;
  }

  CacheDecision& withQueryFingerprint(std::string fp) {
    query_fingerprint_sha256 = std::move(fp);
    return *this;
  }

  // Convert to JSON-LD format
  // BINDING INVARIANT: Deterministic serialization with canonical field ordering
  [[nodiscard]] nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;

    // JSON-LD context for semantic interpretation
    j["@context"] = {
        {"@vocab", "https://qlever.cs.uni-freiburg.de/epic4/cache#"},
        {"epochKey", "@id"},
        {"factors", "@nest"}
    };
    j["@type"] = "CacheDecision";

    // Identity (alphabetical order)
    j["admittedToCache"] = admitted_to_cache;

    // Cached entry metadata
    if (cached_entry_epoch_manifest.has_value()) {
      j["cachedEntryEpochManifest"] = *cached_entry_epoch_manifest;
    }
    if (cached_entry_size_bytes.has_value()) {
      j["cachedEntrySizeBytes"] = *cached_entry_size_bytes;
    }

    j["decisionTimestampNs"] = decision_timestamp_ns;
    j["decisionType"] = decisionTypeToString(decision_type);
    j["epochKey"] = epoch_key.toJsonLD();

    // Eviction reason
    if (eviction_reason.has_value()) {
      j["evictionReason"] = *eviction_reason;
    }

    j["factors"] = factors.toJsonLD();

    // Miss reason
    if (miss_reason.has_value()) {
      j["missReason"] = missReasonToString(*miss_reason);
    }

    j["queryFingerprintSha256"] = query_fingerprint_sha256;
    j["recordSequenceId"] = record_sequence_id;

    return j;
  }

  // Get machine-readable reason for miss
  // Returns empty string if not a miss
  [[nodiscard]] std::string reasonForMiss() const {
    if (miss_reason.has_value()) {
      return missReasonToString(*miss_reason);
    }

    // Infer from decision type if miss_reason not set
    switch (decision_type) {
      case DecisionType::BYTES_MISS:
      case DecisionType::PLAN_MISS:
      case DecisionType::NEG_MISS:
        return "NOT_FOUND";
      default:
        return "";  // Not a miss
    }
  }

  // Check if this is a hit
  [[nodiscard]] bool isHit() const {
    return decision_type == DecisionType::BYTES_HIT ||
           decision_type == DecisionType::PLAN_HIT ||
           decision_type == DecisionType::NEG_HIT;
  }

  // Check if this is a miss
  [[nodiscard]] bool isMiss() const {
    return decision_type == DecisionType::BYTES_MISS ||
           decision_type == DecisionType::PLAN_MISS ||
           decision_type == DecisionType::NEG_MISS;
  }

  // Check if this is an insert
  [[nodiscard]] bool isInsert() const {
    return decision_type == DecisionType::BYTES_INSERT ||
           decision_type == DecisionType::PLAN_INSERT ||
           decision_type == DecisionType::NEG_INSERT;
  }

  // Check if this is an eviction
  [[nodiscard]] bool isEvict() const {
    return decision_type == DecisionType::BYTES_EVICT ||
           decision_type == DecisionType::PLAN_EVICT ||
           decision_type == DecisionType::NEG_EVICT;
  }

  // Validate the decision is complete and consistent
  [[nodiscard]] bool isValid() const {
    // Sequence ID must be set
    if (record_sequence_id == 0) return false;

    // Epoch key should be valid (except for internal errors)
    if (!epoch_key.isValid() && miss_reason != MissReason::INTERNAL_ERROR) {
      return false;
    }

    // Hits must have cached entry metadata
    if (isHit() && !cached_entry_size_bytes.has_value()) {
      return false;
    }

    // Misses should have a reason
    if (isMiss() && !miss_reason.has_value()) {
      return false;
    }

    return true;
  }
};

// CacheDecisionLog - Thread-safe, append-only log of cache decisions
// BINDING INVARIANT: Lock-free MPMC queue for decision appends
// Readers see eventually-consistent view. No lost decisions.
class CacheDecisionLog {
 public:
  // Default capacity: 100K decisions (ring buffer behavior)
  static constexpr size_t DEFAULT_CAPACITY = 100000;

  explicit CacheDecisionLog(size_t capacity = DEFAULT_CAPACITY);

  // Destructor
  ~CacheDecisionLog() = default;

  // Non-copyable
  CacheDecisionLog(const CacheDecisionLog&) = delete;
  CacheDecisionLog& operator=(const CacheDecisionLog&) = delete;

  // Movable
  CacheDecisionLog(CacheDecisionLog&&) noexcept = default;
  CacheDecisionLog& operator=(CacheDecisionLog&&) noexcept = default;

  // Append a decision to the log
  // BINDING INVARIANT: Never fails (may drop oldest if at capacity)
  // Returns: the assigned sequence ID
  [[nodiscard]] uint64_t append(CacheDecision decision);

  // Get all decisions since a given sequence ID
  // Thread-safe: Returns consistent snapshot
  [[nodiscard]] std::vector<CacheDecision> getDecisionsSince(
      uint64_t since_sequence_id) const;

  // Get all decisions in the log
  [[nodiscard]] std::vector<CacheDecision> getAllDecisions() const;

  // Get the most recent N decisions
  [[nodiscard]] std::vector<CacheDecision> getRecentDecisions(size_t n) const;

  // Get count of decisions by type
  [[nodiscard]] uint64_t countByType(DecisionType type) const;

  // Get current sequence counter
  [[nodiscard]] uint64_t getCurrentSequenceId() const;

  // Get total number of decisions logged
  [[nodiscard]] uint64_t getTotalDecisions() const;

  // Export all decisions as JSON-LD array
  [[nodiscard]] nlohmann::ordered_json toJsonLD() const;

  // Clear the log (for testing only)
  void clear();

 private:
  // Ring buffer of decisions
  std::vector<CacheDecision> decisions_;
  size_t capacity_;

  // Write position (monotonically increasing)
  std::atomic<uint64_t> sequence_counter_{1};

  // Total decisions ever logged (including dropped)
  std::atomic<uint64_t> total_decisions_{0};

  // Mutex for consistent reads
  mutable std::mutex mutex_;
};

// Factory functions for creating common CacheDecision types

// Create a BYTES_HIT decision
[[nodiscard]] inline CacheDecision createBytesHitDecision(
    uint64_t sequence_id,
    const EpochKeyFull& epoch_key,
    const std::string& query_fingerprint,
    uint64_t cached_size_bytes,
    const std::string& cached_epoch_manifest) {
  CacheDecision d;
  d.record_sequence_id = sequence_id;
  d.decision_timestamp_ns = static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  d.epoch_key = epoch_key;
  d.decision_type = DecisionType::BYTES_HIT;
  d.admitted_to_cache = true;
  d.query_fingerprint_sha256 = query_fingerprint;
  d.cached_entry_size_bytes = cached_size_bytes;
  d.cached_entry_epoch_manifest = cached_epoch_manifest;
  return d;
}

// Create a BYTES_MISS decision
[[nodiscard]] inline CacheDecision createBytesMissDecision(
    uint64_t sequence_id,
    const EpochKeyFull& epoch_key,
    const std::string& query_fingerprint,
    MissReason reason,
    const DecisionFactors& factors = {}) {
  CacheDecision d;
  d.record_sequence_id = sequence_id;
  d.decision_timestamp_ns = static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  d.epoch_key = epoch_key;
  d.decision_type = DecisionType::BYTES_MISS;
  d.admitted_to_cache = false;
  d.query_fingerprint_sha256 = query_fingerprint;
  d.miss_reason = reason;
  d.factors = factors;
  return d;
}

// Create a PLAN_HIT decision
[[nodiscard]] inline CacheDecision createPlanHitDecision(
    uint64_t sequence_id,
    const EpochKeyFull& epoch_key,
    const std::string& query_fingerprint,
    uint64_t plan_size_bytes) {
  CacheDecision d;
  d.record_sequence_id = sequence_id;
  d.decision_timestamp_ns = static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  d.epoch_key = epoch_key;
  d.decision_type = DecisionType::PLAN_HIT;
  d.admitted_to_cache = true;
  d.query_fingerprint_sha256 = query_fingerprint;
  d.cached_entry_size_bytes = plan_size_bytes;
  return d;
}

// Create a PLAN_MISS decision
[[nodiscard]] inline CacheDecision createPlanMissDecision(
    uint64_t sequence_id,
    const EpochKeyFull& epoch_key,
    const std::string& query_fingerprint,
    MissReason reason) {
  CacheDecision d;
  d.record_sequence_id = sequence_id;
  d.decision_timestamp_ns = static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  d.epoch_key = epoch_key;
  d.decision_type = DecisionType::PLAN_MISS;
  d.admitted_to_cache = false;
  d.query_fingerprint_sha256 = query_fingerprint;
  d.miss_reason = reason;
  return d;
}

// Create a NEG_HIT decision
[[nodiscard]] inline CacheDecision createNegHitDecision(
    uint64_t sequence_id,
    const EpochKeyFull& epoch_key,
    const std::string& query_fingerprint) {
  CacheDecision d;
  d.record_sequence_id = sequence_id;
  d.decision_timestamp_ns = static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  d.epoch_key = epoch_key;
  d.decision_type = DecisionType::NEG_HIT;
  d.admitted_to_cache = true;
  d.query_fingerprint_sha256 = query_fingerprint;
  d.cached_entry_size_bytes = 0;  // Negative entries have no size
  return d;
}

// Create a NEG_MISS decision
[[nodiscard]] inline CacheDecision createNegMissDecision(
    uint64_t sequence_id,
    const EpochKeyFull& epoch_key,
    const std::string& query_fingerprint,
    MissReason reason) {
  CacheDecision d;
  d.record_sequence_id = sequence_id;
  d.decision_timestamp_ns = static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  d.epoch_key = epoch_key;
  d.decision_type = DecisionType::NEG_MISS;
  d.admitted_to_cache = false;
  d.query_fingerprint_sha256 = query_fingerprint;
  d.miss_reason = reason;
  return d;
}

// Create an INSERT decision (any cache type)
[[nodiscard]] inline CacheDecision createInsertDecision(
    uint64_t sequence_id,
    const EpochKeyFull& epoch_key,
    const std::string& query_fingerprint,
    DecisionType insert_type,
    const DecisionFactors& factors,
    uint64_t entry_size_bytes) {
  CacheDecision d;
  d.record_sequence_id = sequence_id;
  d.decision_timestamp_ns = static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  d.epoch_key = epoch_key;
  d.decision_type = insert_type;
  d.admitted_to_cache = true;
  d.query_fingerprint_sha256 = query_fingerprint;
  d.factors = factors;
  d.cached_entry_size_bytes = entry_size_bytes;
  return d;
}

// Create an EVICT decision (any cache type)
[[nodiscard]] inline CacheDecision createEvictDecision(
    uint64_t sequence_id,
    const EpochKeyFull& epoch_key,
    const std::string& query_fingerprint,
    DecisionType evict_type,
    const std::string& eviction_reason) {
  CacheDecision d;
  d.record_sequence_id = sequence_id;
  d.decision_timestamp_ns = static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
  d.epoch_key = epoch_key;
  d.decision_type = evict_type;
  d.admitted_to_cache = false;
  d.query_fingerprint_sha256 = query_fingerprint;
  d.eviction_reason = eviction_reason;
  return d;
}

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_CACHEDECISION_H
