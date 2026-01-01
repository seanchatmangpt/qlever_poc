// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Code (AI Assistant)
//
// Purpose: CacheCorrectnessProver for EPIC 4 Subsystem 3 - Read Cache Correctness
// Formalizes when cached results are admissible and proves zero cross-epoch leakage.
// All proofs are machine-readable structured artifacts (JSON-LD).
//
// BINDING INVARIANTS (from EPIC4_SHARED_INVARIANTS.md):
// - Epoch binding: Every cache entry keyed by (epochId, manifestSha256)
// - Determinism rules: BytesCache + NegativeCache require deterministic queries
// - Cache decisions: ALL decisions logged as CacheDecision
// - Atomic visibility: CacheDecision log is append-only
// - Failure = abort: Cache ambiguity or epoch mismatch -> logged as MISS
// - Admissibility proof: For every cache hit, produce formal proof of correctness

#ifndef QLEVER_SRC_ENGINE_READPLANE_CACHECORRECTNESSPROVER_H
#define QLEVER_SRC_ENGINE_READPLANE_CACHECORRECTNESSPROVER_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "engine/readPlane/CacheDecision.h"
#include "engine/readCache/AdmissionPolicy.h"
#include "engine/readCache/CacheConfig.h"
#include "util/json.h"

namespace readPlane {

// ProofStatus - Outcome of a proof verification
enum class ProofStatus : uint8_t {
  VALID = 0,           // Proof is valid
  INVALID = 1,         // Proof is invalid (correctness violation)
  INCONCLUSIVE = 2,    // Cannot determine validity
  SKIPPED = 3,         // Proof generation skipped (e.g., not applicable)
};

inline std::string proofStatusToString(ProofStatus status) {
  switch (status) {
    case ProofStatus::VALID: return "VALID";
    case ProofStatus::INVALID: return "INVALID";
    case ProofStatus::INCONCLUSIVE: return "INCONCLUSIVE";
    case ProofStatus::SKIPPED: return "SKIPPED";
    default: return "UNKNOWN";
  }
}

// ViolationType - Type of correctness violation detected
enum class ViolationType : uint8_t {
  NONE = 0,                          // No violation
  CROSS_EPOCH_HIT = 1,               // Hit from different epoch (CRITICAL)
  DETERMINISM_VIOLATION = 2,         // Non-deterministic in deterministic cache
  FREQUENCY_THRESHOLD_VIOLATION = 3, // Admitted without meeting threshold
  ADMISSION_POLICY_BYPASS = 4,       // Backdoor cache insertion
  SILENT_DECISION = 5,               // Cache operation without CacheDecision
  EPOCH_MISMATCH_NOT_LOGGED = 6,     // Epoch mismatch not logged as MISS
  STALE_EPOCH_NOT_EVICTED = 7,       // Stale epoch entries still in cache
};

inline std::string violationTypeToString(ViolationType type) {
  switch (type) {
    case ViolationType::NONE: return "NONE";
    case ViolationType::CROSS_EPOCH_HIT: return "CROSS_EPOCH_HIT";
    case ViolationType::DETERMINISM_VIOLATION: return "DETERMINISM_VIOLATION";
    case ViolationType::FREQUENCY_THRESHOLD_VIOLATION:
      return "FREQUENCY_THRESHOLD_VIOLATION";
    case ViolationType::ADMISSION_POLICY_BYPASS: return "ADMISSION_POLICY_BYPASS";
    case ViolationType::SILENT_DECISION: return "SILENT_DECISION";
    case ViolationType::EPOCH_MISMATCH_NOT_LOGGED:
      return "EPOCH_MISMATCH_NOT_LOGGED";
    case ViolationType::STALE_EPOCH_NOT_EVICTED: return "STALE_EPOCH_NOT_EVICTED";
    default: return "UNKNOWN";
  }
}

// Proof - Machine-readable proof artifact
// All proofs are structured as JSON-LD for deterministic serialization
struct Proof {
  // Proof identity
  std::string proof_id;               // Unique proof identifier (UUID)
  std::string proof_type;             // What this proof verifies

  // Proof timestamp (deterministic, based on sequence ID)
  uint64_t proof_sequence_id = 0;
  uint64_t proof_timestamp_ns = 0;

  // Proof status
  ProofStatus status = ProofStatus::INCONCLUSIVE;

  // Violation details (if INVALID)
  ViolationType violation_type = ViolationType::NONE;
  std::string violation_message;
  std::optional<uint64_t> violating_decision_id;

  // Epoch context
  EpochKeyFull current_epoch;
  std::optional<EpochKeyFull> expected_epoch;  // For epoch soundness proofs

  // Evidence (machine-readable)
  nlohmann::ordered_json evidence;

  // Statistics collected during proof generation
  struct ProofStats {
    uint64_t decisions_examined = 0;
    uint64_t hits_verified = 0;
    uint64_t misses_verified = 0;
    uint64_t violations_found = 0;
    uint64_t proof_generation_ns = 0;
  } stats;

  // Default constructor
  Proof() = default;

  // Builder-style setters
  Proof& withId(std::string id) {
    proof_id = std::move(id);
    return *this;
  }

  Proof& withType(std::string type) {
    proof_type = std::move(type);
    return *this;
  }

  Proof& withSequenceId(uint64_t seq) {
    proof_sequence_id = seq;
    return *this;
  }

  Proof& withStatus(ProofStatus s) {
    status = s;
    return *this;
  }

  Proof& withViolation(ViolationType type, std::string message) {
    status = ProofStatus::INVALID;
    violation_type = type;
    violation_message = std::move(message);
    return *this;
  }

  Proof& withEvidence(nlohmann::ordered_json ev) {
    evidence = std::move(ev);
    return *this;
  }

  // Convert to JSON-LD
  [[nodiscard]] nlohmann::ordered_json toJsonLD() const;

  // Check if proof is valid
  [[nodiscard]] bool isValid() const { return status == ProofStatus::VALID; }

  // Check if proof found a violation
  [[nodiscard]] bool hasViolation() const {
    return status == ProofStatus::INVALID;
  }
};

// CacheSnapshot - Point-in-time snapshot of cache state for proofs
// Used to verify cache correctness properties
struct CacheSnapshot {
  uint64_t snapshot_sequence_id = 0;
  EpochKeyFull current_epoch;

  // Cache entry counts by epoch
  struct EpochCounts {
    std::string epoch_manifest;
    uint64_t bytes_entries = 0;
    uint64_t plan_entries = 0;
    uint64_t neg_entries = 0;
  };
  std::vector<EpochCounts> entries_by_epoch;

  // Total counts
  uint64_t total_bytes_entries = 0;
  uint64_t total_plan_entries = 0;
  uint64_t total_neg_entries = 0;

  // Convert to JSON-LD
  [[nodiscard]] nlohmann::ordered_json toJsonLD() const;
};

// CacheCorrectnessProver - Generates formal proofs of cache correctness
//
// DESIGN:
// - Wraps cache operations to intercept and verify all decisions
// - Produces machine-readable proof artifacts (JSON-LD)
// - Detects correctness violations and reports them
//
// INVARIANTS:
// 1. Epoch Soundness: No cross-epoch cache hits
// 2. Determinism Enforcement: Bytes/Neg caches only have deterministic entries
// 3. Frequency Threshold: Only queries meeting threshold are admitted
// 4. Admission Policy Consistency: No backdoor cache insertions
// 5. Silent Failure Prevention: Every operation has a CacheDecision
//
class CacheCorrectnessProver {
 public:
  // Constructor with decision log and config
  explicit CacheCorrectnessProver(
      std::shared_ptr<CacheDecisionLog> decision_log,
      const ::readCache::CacheConfig& config);

  // Destructor
  ~CacheCorrectnessProver() = default;

  // Non-copyable
  CacheCorrectnessProver(const CacheCorrectnessProver&) = delete;
  CacheCorrectnessProver& operator=(const CacheCorrectnessProver&) = delete;

  // ===== Proof Generation Methods =====

  // Prove epoch soundness: no cross-epoch cache hits
  // Scans all cache decisions and verifies epoch consistency
  //
  // INVARIANT: if (cache_entry.epoch_key != query.epoch_key) { FAIL }
  //
  // Returns: Proof artifact with status VALID or INVALID
  [[nodiscard]] Proof proveEpochSoundness(const EpochKeyFull& current_epoch);

  // Prove determinism enforcement: bytes/neg caches only have deterministic entries
  // Verifies that non-deterministic queries were never admitted to these caches
  //
  // INVARIANT: BytesCache and NegativeCache entries must be deterministic
  //
  // Returns: Proof artifact with status VALID or INVALID
  [[nodiscard]] Proof proveDeterminismEnforcement();

  // Prove that a specific cache decision is correct
  // Verifies all factors and constraints for the decision
  //
  // Checks:
  // - Epoch binding is correct
  // - Determinism constraint (for bytes/neg caches)
  // - Frequency threshold (if applicable)
  // - Top-K membership (if applicable)
  // - Admission policy consistency
  //
  // Returns: Proof artifact for this specific decision
  [[nodiscard]] Proof proveCacheAdmissibilityDecision(
      const CacheDecision& decision);

  // Prove frequency threshold enforcement
  // Verifies that only queries meeting frequency threshold were admitted
  //
  // INVARIANT: observed_freq >= threshold OR in_top_k
  //
  // Returns: Proof artifact with status VALID or INVALID
  [[nodiscard]] Proof proveFrequencyThresholdEnforcement();

  // Prove admission policy consistency
  // Verifies that all cache insertions went through AdmissionPolicy
  //
  // Returns: Proof artifact with status VALID or INVALID
  [[nodiscard]] Proof proveAdmissionPolicyConsistency();

  // ===== Cross-Epoch Contamination Detection =====

  // Scan all caches for entries from epochs other than current
  // This is a CRITICAL correctness check
  //
  // If any cross-epoch entries found: INVALID proof with details
  // If all entries are current epoch: VALID proof
  //
  // Parameters:
  //   current_epoch: The expected epoch for all cache entries
  //   snapshot_fn: Function to get current cache snapshot (for inspection)
  //
  // Returns: Proof artifact with contamination details if found
  [[nodiscard]] Proof detectCrossEpochContamination(
      const EpochKeyFull& current_epoch,
      std::function<CacheSnapshot()> snapshot_fn);

  // ===== Real-Time Verification =====

  // Verify a cache hit before returning result
  // Called by cache wrappers to ensure hit is valid
  //
  // Returns: true if hit is valid, false if hit should be treated as miss
  [[nodiscard]] bool verifyCacheHit(
      const CacheDecision& hit_decision,
      const EpochKeyFull& current_epoch);

  // Verify a cache insertion before committing
  // Called by cache wrappers to ensure insertion is valid
  //
  // Returns: true if insertion is valid, false if should be rejected
  [[nodiscard]] bool verifyCacheInsertion(
      const CacheDecision& insert_decision,
      const EpochKeyFull& current_epoch,
      bool is_deterministic);

  // ===== Statistics and Monitoring =====

  struct ProverStats {
    std::atomic<uint64_t> proofs_generated{0};
    std::atomic<uint64_t> proofs_valid{0};
    std::atomic<uint64_t> proofs_invalid{0};
    std::atomic<uint64_t> proofs_inconclusive{0};
    std::atomic<uint64_t> hits_verified{0};
    std::atomic<uint64_t> hits_rejected{0};
    std::atomic<uint64_t> insertions_verified{0};
    std::atomic<uint64_t> insertions_rejected{0};
    std::atomic<uint64_t> cross_epoch_violations{0};
    std::atomic<uint64_t> determinism_violations{0};
    std::atomic<uint64_t> frequency_violations{0};
  };

  // Get current statistics
  [[nodiscard]] ProverStats getStats() const;

  // Reset statistics
  void resetStats();

  // ===== Configuration =====

  // Set the current epoch (for validation)
  void setCurrentEpoch(const EpochKeyFull& epoch);

  // Get the current epoch
  [[nodiscard]] EpochKeyFull getCurrentEpoch() const;

  // Enable/disable real-time verification (performance tradeoff)
  void setRealTimeVerification(bool enabled);

  // Check if real-time verification is enabled
  [[nodiscard]] bool isRealTimeVerificationEnabled() const;

  // ===== Proof Log =====

  // Get all proofs generated
  [[nodiscard]] std::vector<Proof> getAllProofs() const;

  // Get proofs since a given sequence ID
  [[nodiscard]] std::vector<Proof> getProofsSince(uint64_t since_id) const;

  // Export all proofs as JSON-LD array
  [[nodiscard]] nlohmann::ordered_json proofsToJsonLD() const;

 private:
  // Decision log (shared with cache wrappers)
  std::shared_ptr<CacheDecisionLog> decision_log_;

  // Cache configuration (for thresholds, etc.)
  ::readCache::CacheConfig config_;

  // Current epoch (for validation)
  mutable std::mutex epoch_mutex_;
  EpochKeyFull current_epoch_;

  // Real-time verification flag
  std::atomic<bool> real_time_verification_{true};

  // Proof sequence counter
  std::atomic<uint64_t> proof_sequence_counter_{1};

  // Proof log
  mutable std::mutex proofs_mutex_;
  std::vector<Proof> proofs_;
  static constexpr size_t MAX_PROOFS = 10000;

  // Statistics
  mutable ProverStats stats_;

  // ===== Helper Methods =====

  // Generate a unique proof ID
  [[nodiscard]] std::string generateProofId();

  // Get current timestamp in nanoseconds (deterministic within sequence)
  [[nodiscard]] uint64_t getTimestampNs() const;

  // Record a proof in the log
  void recordProof(const Proof& proof);

  // Verify epoch binding for a decision
  [[nodiscard]] bool verifyEpochBinding(
      const CacheDecision& decision,
      const EpochKeyFull& expected_epoch) const;

  // Verify determinism constraint for a decision
  [[nodiscard]] bool verifyDeterminismConstraint(
      const CacheDecision& decision) const;

  // Verify frequency threshold for a decision
  [[nodiscard]] bool verifyFrequencyThreshold(
      const CacheDecision& decision) const;

  // Check if decision type requires determinism
  [[nodiscard]] bool requiresDeterminism(DecisionType type) const;

  // Update statistics based on proof
  void updateStats(const Proof& proof);
};

// Global prover instance (optional, for integration with cache system)
// May be null if prover is not enabled
extern std::shared_ptr<CacheCorrectnessProver> globalCacheProver;

// Initialize the global prover
void initGlobalCacheProver(
    std::shared_ptr<CacheDecisionLog> decision_log,
    const ::readCache::CacheConfig& config);

// Get the global prover (may return nullptr)
[[nodiscard]] std::shared_ptr<CacheCorrectnessProver> getGlobalCacheProver();

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_CACHECORRECTNESSPROVER_H
