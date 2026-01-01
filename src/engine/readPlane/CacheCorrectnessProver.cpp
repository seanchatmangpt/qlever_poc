// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Code (AI Assistant)
//
// Purpose: Implementation of CacheCorrectnessProver for EPIC 4 Subsystem 3

#include "engine/readPlane/CacheCorrectnessProver.h"

#include <algorithm>
#include <chrono>
#include <random>
#include <sstream>

namespace readPlane {

// =============================================================================
// CacheDecisionLog Implementation
// =============================================================================

CacheDecisionLog::CacheDecisionLog(size_t capacity)
    : capacity_(capacity) {
  decisions_.reserve(capacity);
}

uint64_t CacheDecisionLog::append(CacheDecision decision) {
  // Assign sequence ID atomically
  uint64_t seq_id = sequence_counter_.fetch_add(1, std::memory_order_relaxed);
  decision.record_sequence_id = seq_id;

  // Set timestamp if not already set
  if (decision.decision_timestamp_ns == 0) {
    decision.decision_timestamp_ns = static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);

    // Ring buffer behavior: overwrite oldest if at capacity
    if (decisions_.size() >= capacity_) {
      // Remove oldest (front) to make room
      decisions_.erase(decisions_.begin());
    }

    decisions_.push_back(std::move(decision));
  }

  total_decisions_.fetch_add(1, std::memory_order_relaxed);
  return seq_id;
}

std::vector<CacheDecision> CacheDecisionLog::getDecisionsSince(
    uint64_t since_sequence_id) const {
  std::lock_guard<std::mutex> lock(mutex_);

  std::vector<CacheDecision> result;
  result.reserve(decisions_.size());

  for (const auto& d : decisions_) {
    if (d.record_sequence_id > since_sequence_id) {
      result.push_back(d);
    }
  }

  return result;
}

std::vector<CacheDecision> CacheDecisionLog::getAllDecisions() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return decisions_;
}

std::vector<CacheDecision> CacheDecisionLog::getRecentDecisions(size_t n) const {
  std::lock_guard<std::mutex> lock(mutex_);

  if (n >= decisions_.size()) {
    return decisions_;
  }

  return std::vector<CacheDecision>(
      decisions_.end() - static_cast<ptrdiff_t>(n),
      decisions_.end());
}

uint64_t CacheDecisionLog::countByType(DecisionType type) const {
  std::lock_guard<std::mutex> lock(mutex_);

  uint64_t count = 0;
  for (const auto& d : decisions_) {
    if (d.decision_type == type) {
      ++count;
    }
  }

  return count;
}

uint64_t CacheDecisionLog::getCurrentSequenceId() const {
  return sequence_counter_.load(std::memory_order_relaxed);
}

uint64_t CacheDecisionLog::getTotalDecisions() const {
  return total_decisions_.load(std::memory_order_relaxed);
}

nlohmann::ordered_json CacheDecisionLog::toJsonLD() const {
  std::lock_guard<std::mutex> lock(mutex_);

  nlohmann::ordered_json j;
  j["@context"] = {
      {"@vocab", "https://qlever.cs.uni-freiburg.de/epic4/cache#"}
  };
  j["@type"] = "CacheDecisionLog";
  j["currentSequenceId"] = sequence_counter_.load(std::memory_order_relaxed);
  j["totalDecisions"] = total_decisions_.load(std::memory_order_relaxed);

  nlohmann::ordered_json decisions_array = nlohmann::ordered_json::array();
  for (const auto& d : decisions_) {
    decisions_array.push_back(d.toJsonLD());
  }
  j["decisions"] = decisions_array;

  return j;
}

void CacheDecisionLog::clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  decisions_.clear();
  // Don't reset sequence counter - maintain monotonicity
}

// =============================================================================
// Proof Implementation
// =============================================================================

nlohmann::ordered_json Proof::toJsonLD() const {
  nlohmann::ordered_json j;

  // JSON-LD context
  j["@context"] = {
      {"@vocab", "https://qlever.cs.uni-freiburg.de/epic4/proof#"},
      {"evidence", "@nest"}
  };
  j["@type"] = "CacheCorrectnessProof";

  // Identity
  j["proofId"] = proof_id;
  j["proofSequenceId"] = proof_sequence_id;
  j["proofTimestampNs"] = proof_timestamp_ns;
  j["proofType"] = proof_type;

  // Status
  j["status"] = proofStatusToString(status);

  // Epoch context
  j["currentEpoch"] = current_epoch.toJsonLD();
  if (expected_epoch.has_value()) {
    j["expectedEpoch"] = expected_epoch->toJsonLD();
  }

  // Violation details (if any)
  if (status == ProofStatus::INVALID) {
    j["violationType"] = violationTypeToString(violation_type);
    j["violationMessage"] = violation_message;
    if (violating_decision_id.has_value()) {
      j["violatingDecisionId"] = *violating_decision_id;
    }
  }

  // Evidence
  if (!evidence.empty()) {
    j["evidence"] = evidence;
  }

  // Statistics
  nlohmann::ordered_json stats_json;
  stats_json["decisionsExamined"] = stats.decisions_examined;
  stats_json["hitsVerified"] = stats.hits_verified;
  stats_json["missesVerified"] = stats.misses_verified;
  stats_json["violationsFound"] = stats.violations_found;
  stats_json["proofGenerationNs"] = stats.proof_generation_ns;
  j["statistics"] = stats_json;

  return j;
}

// =============================================================================
// CacheSnapshot Implementation
// =============================================================================

nlohmann::ordered_json CacheSnapshot::toJsonLD() const {
  nlohmann::ordered_json j;

  j["@type"] = "CacheSnapshot";
  j["snapshotSequenceId"] = snapshot_sequence_id;
  j["currentEpoch"] = current_epoch.toJsonLD();

  nlohmann::ordered_json by_epoch = nlohmann::ordered_json::array();
  for (const auto& e : entries_by_epoch) {
    nlohmann::ordered_json entry;
    entry["epochManifest"] = e.epoch_manifest;
    entry["bytesEntries"] = e.bytes_entries;
    entry["planEntries"] = e.plan_entries;
    entry["negEntries"] = e.neg_entries;
    by_epoch.push_back(entry);
  }
  j["entriesByEpoch"] = by_epoch;

  j["totalBytesEntries"] = total_bytes_entries;
  j["totalPlanEntries"] = total_plan_entries;
  j["totalNegEntries"] = total_neg_entries;

  return j;
}

// =============================================================================
// CacheCorrectnessProver Implementation
// =============================================================================

CacheCorrectnessProver::CacheCorrectnessProver(
    std::shared_ptr<CacheDecisionLog> decision_log,
    const ::readCache::CacheConfig& config)
    : decision_log_(std::move(decision_log)),
      config_(config) {
  proofs_.reserve(MAX_PROOFS);
}

std::string CacheCorrectnessProver::generateProofId() {
  // Generate a pseudo-random ID based on sequence counter and timestamp
  uint64_t seq = proof_sequence_counter_.load(std::memory_order_relaxed);
  uint64_t ts = getTimestampNs();

  std::stringstream ss;
  ss << "proof-" << std::hex << seq << "-" << (ts & 0xFFFFFFFF);
  return ss.str();
}

uint64_t CacheCorrectnessProver::getTimestampNs() const {
  return static_cast<uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
}

void CacheCorrectnessProver::recordProof(const Proof& proof) {
  std::lock_guard<std::mutex> lock(proofs_mutex_);

  // Ring buffer behavior
  if (proofs_.size() >= MAX_PROOFS) {
    proofs_.erase(proofs_.begin());
  }

  proofs_.push_back(proof);
}

void CacheCorrectnessProver::updateStats(const Proof& proof) {
  stats_.proofs_generated.fetch_add(1, std::memory_order_relaxed);

  switch (proof.status) {
    case ProofStatus::VALID:
      stats_.proofs_valid.fetch_add(1, std::memory_order_relaxed);
      break;
    case ProofStatus::INVALID:
      stats_.proofs_invalid.fetch_add(1, std::memory_order_relaxed);

      // Track specific violation types
      switch (proof.violation_type) {
        case ViolationType::CROSS_EPOCH_HIT:
          stats_.cross_epoch_violations.fetch_add(1, std::memory_order_relaxed);
          break;
        case ViolationType::DETERMINISM_VIOLATION:
          stats_.determinism_violations.fetch_add(1, std::memory_order_relaxed);
          break;
        case ViolationType::FREQUENCY_THRESHOLD_VIOLATION:
          stats_.frequency_violations.fetch_add(1, std::memory_order_relaxed);
          break;
        default:
          break;
      }
      break;
    case ProofStatus::INCONCLUSIVE:
      stats_.proofs_inconclusive.fetch_add(1, std::memory_order_relaxed);
      break;
    default:
      break;
  }
}

bool CacheCorrectnessProver::verifyEpochBinding(
    const CacheDecision& decision,
    const EpochKeyFull& expected_epoch) const {
  // Check if epoch keys match
  return decision.epoch_key == expected_epoch;
}

bool CacheCorrectnessProver::verifyDeterminismConstraint(
    const CacheDecision& decision) const {
  // Only bytes and negative caches require determinism
  if (!requiresDeterminism(decision.decision_type)) {
    return true;  // Plan cache doesn't require determinism
  }

  // For inserts and hits, check determinism factor
  if (decision.isInsert() || decision.isHit()) {
    return decision.factors.is_deterministic;
  }

  // Misses don't need determinism check
  return true;
}

bool CacheCorrectnessProver::verifyFrequencyThreshold(
    const CacheDecision& decision) const {
  // Only inserts need frequency verification
  if (!decision.isInsert()) {
    return true;
  }

  // Check if meets frequency threshold OR in top-K
  return decision.factors.meets_frequency_threshold ||
         decision.factors.is_in_top_k_shapes;
}

bool CacheCorrectnessProver::requiresDeterminism(DecisionType type) const {
  // BytesCache and NegativeCache require determinism
  switch (type) {
    case DecisionType::BYTES_HIT:
    case DecisionType::BYTES_MISS:
    case DecisionType::BYTES_INSERT:
    case DecisionType::BYTES_EVICT:
    case DecisionType::NEG_HIT:
    case DecisionType::NEG_MISS:
    case DecisionType::NEG_INSERT:
    case DecisionType::NEG_EVICT:
      return true;

    case DecisionType::PLAN_HIT:
    case DecisionType::PLAN_MISS:
    case DecisionType::PLAN_INSERT:
    case DecisionType::PLAN_EVICT:
      return false;

    default:
      return false;
  }
}

// =============================================================================
// Proof Generation Methods
// =============================================================================

Proof CacheCorrectnessProver::proveEpochSoundness(
    const EpochKeyFull& current_epoch) {
  auto start_time = std::chrono::steady_clock::now();

  Proof proof;
  proof.proof_id = generateProofId();
  proof.proof_type = "EPOCH_SOUNDNESS";
  proof.proof_sequence_id = proof_sequence_counter_.fetch_add(
      1, std::memory_order_relaxed);
  proof.proof_timestamp_ns = getTimestampNs();
  proof.current_epoch = current_epoch;
  proof.status = ProofStatus::VALID;

  // Get all decisions
  auto decisions = decision_log_->getAllDecisions();
  proof.stats.decisions_examined = decisions.size();

  nlohmann::ordered_json evidence_hits = nlohmann::ordered_json::array();
  nlohmann::ordered_json evidence_violations = nlohmann::ordered_json::array();

  for (const auto& d : decisions) {
    // Only examine hits - misses don't return cached data
    if (d.isHit()) {
      ++proof.stats.hits_verified;

      // Check epoch binding
      if (d.epoch_key != current_epoch) {
        // CROSS-EPOCH HIT DETECTED - CRITICAL VIOLATION
        ++proof.stats.violations_found;

        nlohmann::ordered_json violation;
        violation["decisionId"] = d.record_sequence_id;
        violation["decisionType"] = decisionTypeToString(d.decision_type);
        violation["decisionEpochId"] = d.epoch_key.epoch_id;
        violation["decisionEpochManifest"] = d.epoch_key.manifest_sha256;
        violation["expectedEpochId"] = current_epoch.epoch_id;
        violation["expectedEpochManifest"] = current_epoch.manifest_sha256;
        evidence_violations.push_back(violation);

        // Record first violation
        if (proof.status == ProofStatus::VALID) {
          proof.status = ProofStatus::INVALID;
          proof.violation_type = ViolationType::CROSS_EPOCH_HIT;
          proof.violation_message =
              "Cache hit returned data from different epoch";
          proof.violating_decision_id = d.record_sequence_id;
          proof.expected_epoch = current_epoch;
        }
      } else {
        // Record verified hit
        nlohmann::ordered_json hit;
        hit["decisionId"] = d.record_sequence_id;
        hit["epochVerified"] = true;
        evidence_hits.push_back(hit);
      }
    }
  }

  // Build evidence
  nlohmann::ordered_json evidence;
  evidence["totalHitsExamined"] = proof.stats.hits_verified;
  evidence["verifiedHits"] = evidence_hits;
  if (!evidence_violations.empty()) {
    evidence["violations"] = evidence_violations;
  }
  proof.evidence = evidence;

  auto end_time = std::chrono::steady_clock::now();
  proof.stats.proof_generation_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          end_time - start_time).count();

  recordProof(proof);
  updateStats(proof);

  return proof;
}

Proof CacheCorrectnessProver::proveDeterminismEnforcement() {
  auto start_time = std::chrono::steady_clock::now();

  Proof proof;
  proof.proof_id = generateProofId();
  proof.proof_type = "DETERMINISM_ENFORCEMENT";
  proof.proof_sequence_id = proof_sequence_counter_.fetch_add(
      1, std::memory_order_relaxed);
  proof.proof_timestamp_ns = getTimestampNs();

  {
    std::lock_guard<std::mutex> lock(epoch_mutex_);
    proof.current_epoch = current_epoch_;
  }

  proof.status = ProofStatus::VALID;

  auto decisions = decision_log_->getAllDecisions();
  proof.stats.decisions_examined = decisions.size();

  nlohmann::ordered_json evidence_violations = nlohmann::ordered_json::array();

  for (const auto& d : decisions) {
    // Check bytes and negative cache insertions
    if (requiresDeterminism(d.decision_type) && d.isInsert()) {
      if (!d.factors.is_deterministic) {
        // NON-DETERMINISTIC ENTRY IN DETERMINISTIC CACHE - VIOLATION
        ++proof.stats.violations_found;

        nlohmann::ordered_json violation;
        violation["decisionId"] = d.record_sequence_id;
        violation["decisionType"] = decisionTypeToString(d.decision_type);
        violation["isDeterministic"] = d.factors.is_deterministic;
        violation["shapeSha256"] = d.factors.shape_sha256;
        evidence_violations.push_back(violation);

        if (proof.status == ProofStatus::VALID) {
          proof.status = ProofStatus::INVALID;
          proof.violation_type = ViolationType::DETERMINISM_VIOLATION;
          proof.violation_message =
              "Non-deterministic query admitted to deterministic cache";
          proof.violating_decision_id = d.record_sequence_id;
        }
      }
    }
  }

  nlohmann::ordered_json evidence;
  evidence["totalDecisionsExamined"] = proof.stats.decisions_examined;
  evidence["violationsFound"] = proof.stats.violations_found;
  if (!evidence_violations.empty()) {
    evidence["violations"] = evidence_violations;
  }
  proof.evidence = evidence;

  auto end_time = std::chrono::steady_clock::now();
  proof.stats.proof_generation_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          end_time - start_time).count();

  recordProof(proof);
  updateStats(proof);

  return proof;
}

Proof CacheCorrectnessProver::proveCacheAdmissibilityDecision(
    const CacheDecision& decision) {
  auto start_time = std::chrono::steady_clock::now();

  Proof proof;
  proof.proof_id = generateProofId();
  proof.proof_type = "CACHE_ADMISSIBILITY";
  proof.proof_sequence_id = proof_sequence_counter_.fetch_add(
      1, std::memory_order_relaxed);
  proof.proof_timestamp_ns = getTimestampNs();
  proof.current_epoch = decision.epoch_key;
  proof.stats.decisions_examined = 1;
  proof.status = ProofStatus::VALID;

  nlohmann::ordered_json evidence;
  evidence["decisionId"] = decision.record_sequence_id;
  evidence["decisionType"] = decisionTypeToString(decision.decision_type);
  evidence["admitted"] = decision.admitted_to_cache;

  // Verify epoch binding
  EpochKeyFull expected_epoch;
  {
    std::lock_guard<std::mutex> lock(epoch_mutex_);
    expected_epoch = current_epoch_;
  }

  bool epoch_valid = verifyEpochBinding(decision, expected_epoch);
  evidence["epochBindingValid"] = epoch_valid;

  if (!epoch_valid && decision.isHit()) {
    proof.status = ProofStatus::INVALID;
    proof.violation_type = ViolationType::CROSS_EPOCH_HIT;
    proof.violation_message = "Decision uses wrong epoch";
    proof.violating_decision_id = decision.record_sequence_id;
    proof.expected_epoch = expected_epoch;
    ++proof.stats.violations_found;
  }

  // Verify determinism constraint
  bool determinism_valid = verifyDeterminismConstraint(decision);
  evidence["determinismConstraintValid"] = determinism_valid;

  if (!determinism_valid && proof.status == ProofStatus::VALID) {
    proof.status = ProofStatus::INVALID;
    proof.violation_type = ViolationType::DETERMINISM_VIOLATION;
    proof.violation_message =
        "Non-deterministic query in deterministic cache";
    proof.violating_decision_id = decision.record_sequence_id;
    ++proof.stats.violations_found;
  }

  // Verify frequency threshold
  bool frequency_valid = verifyFrequencyThreshold(decision);
  evidence["frequencyThresholdValid"] = frequency_valid;

  if (!frequency_valid && decision.admitted_to_cache &&
      proof.status == ProofStatus::VALID) {
    proof.status = ProofStatus::INVALID;
    proof.violation_type = ViolationType::FREQUENCY_THRESHOLD_VIOLATION;
    proof.violation_message =
        "Admitted without meeting frequency threshold or top-K";
    proof.violating_decision_id = decision.record_sequence_id;
    ++proof.stats.violations_found;
  }

  // Add decision factors to evidence
  evidence["factors"] = decision.factors.toJsonLD();
  proof.evidence = evidence;

  auto end_time = std::chrono::steady_clock::now();
  proof.stats.proof_generation_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          end_time - start_time).count();

  recordProof(proof);
  updateStats(proof);

  return proof;
}

Proof CacheCorrectnessProver::proveFrequencyThresholdEnforcement() {
  auto start_time = std::chrono::steady_clock::now();

  Proof proof;
  proof.proof_id = generateProofId();
  proof.proof_type = "FREQUENCY_THRESHOLD_ENFORCEMENT";
  proof.proof_sequence_id = proof_sequence_counter_.fetch_add(
      1, std::memory_order_relaxed);
  proof.proof_timestamp_ns = getTimestampNs();

  {
    std::lock_guard<std::mutex> lock(epoch_mutex_);
    proof.current_epoch = current_epoch_;
  }

  proof.status = ProofStatus::VALID;

  auto decisions = decision_log_->getAllDecisions();
  proof.stats.decisions_examined = decisions.size();

  nlohmann::ordered_json evidence_violations = nlohmann::ordered_json::array();

  for (const auto& d : decisions) {
    // Only check insertions that were admitted
    if (d.isInsert() && d.admitted_to_cache) {
      if (!d.factors.meets_frequency_threshold && !d.factors.is_in_top_k_shapes) {
        // FREQUENCY THRESHOLD VIOLATION
        ++proof.stats.violations_found;

        nlohmann::ordered_json violation;
        violation["decisionId"] = d.record_sequence_id;
        violation["decisionType"] = decisionTypeToString(d.decision_type);
        violation["observedFrequency"] = d.factors.observed_frequency;
        violation["configuredThreshold"] = d.factors.configured_frequency_threshold;
        violation["isInTopK"] = d.factors.is_in_top_k_shapes;
        evidence_violations.push_back(violation);

        if (proof.status == ProofStatus::VALID) {
          proof.status = ProofStatus::INVALID;
          proof.violation_type = ViolationType::FREQUENCY_THRESHOLD_VIOLATION;
          proof.violation_message =
              "Admitted without meeting frequency threshold or top-K";
          proof.violating_decision_id = d.record_sequence_id;
        }
      }
    }
  }

  nlohmann::ordered_json evidence;
  evidence["totalDecisionsExamined"] = proof.stats.decisions_examined;
  evidence["configuredBytesThreshold"] = config_.freq_threshold_bytes;
  evidence["configuredPlanThreshold"] = config_.freq_threshold_plan;
  evidence["configuredTopK"] = config_.top_k_shapes;
  evidence["violationsFound"] = proof.stats.violations_found;
  if (!evidence_violations.empty()) {
    evidence["violations"] = evidence_violations;
  }
  proof.evidence = evidence;

  auto end_time = std::chrono::steady_clock::now();
  proof.stats.proof_generation_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          end_time - start_time).count();

  recordProof(proof);
  updateStats(proof);

  return proof;
}

Proof CacheCorrectnessProver::proveAdmissionPolicyConsistency() {
  auto start_time = std::chrono::steady_clock::now();

  Proof proof;
  proof.proof_id = generateProofId();
  proof.proof_type = "ADMISSION_POLICY_CONSISTENCY";
  proof.proof_sequence_id = proof_sequence_counter_.fetch_add(
      1, std::memory_order_relaxed);
  proof.proof_timestamp_ns = getTimestampNs();

  {
    std::lock_guard<std::mutex> lock(epoch_mutex_);
    proof.current_epoch = current_epoch_;
  }

  proof.status = ProofStatus::VALID;

  auto decisions = decision_log_->getAllDecisions();
  proof.stats.decisions_examined = decisions.size();

  // Count insertions by type and admission reason
  uint64_t total_insertions = 0;
  uint64_t insertions_with_reason = 0;

  for (const auto& d : decisions) {
    if (d.isInsert() && d.admitted_to_cache) {
      ++total_insertions;

      // Check if admission has a valid reason
      if (!d.factors.admission_reason.empty()) {
        ++insertions_with_reason;
      }
    }
  }

  nlohmann::ordered_json evidence;
  evidence["totalInsertions"] = total_insertions;
  evidence["insertionsWithReason"] = insertions_with_reason;
  evidence["insertionsWithoutReason"] = total_insertions - insertions_with_reason;

  // If there are insertions without reasons, that's suspicious
  if (total_insertions > insertions_with_reason) {
    evidence["warning"] = "Some insertions lack admission reasons - potential bypass";
  }

  proof.evidence = evidence;

  auto end_time = std::chrono::steady_clock::now();
  proof.stats.proof_generation_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          end_time - start_time).count();

  recordProof(proof);
  updateStats(proof);

  return proof;
}

Proof CacheCorrectnessProver::detectCrossEpochContamination(
    const EpochKeyFull& current_epoch,
    std::function<CacheSnapshot()> snapshot_fn) {
  auto start_time = std::chrono::steady_clock::now();

  Proof proof;
  proof.proof_id = generateProofId();
  proof.proof_type = "CROSS_EPOCH_CONTAMINATION";
  proof.proof_sequence_id = proof_sequence_counter_.fetch_add(
      1, std::memory_order_relaxed);
  proof.proof_timestamp_ns = getTimestampNs();
  proof.current_epoch = current_epoch;
  proof.status = ProofStatus::VALID;

  // Get cache snapshot
  CacheSnapshot snapshot = snapshot_fn();
  proof.stats.decisions_examined = snapshot.total_bytes_entries +
                                   snapshot.total_plan_entries +
                                   snapshot.total_neg_entries;

  nlohmann::ordered_json stale_epochs = nlohmann::ordered_json::array();
  uint64_t total_stale_entries = 0;

  for (const auto& epoch_counts : snapshot.entries_by_epoch) {
    if (epoch_counts.epoch_manifest != current_epoch.manifest_sha256) {
      // Found entries from different epoch - CONTAMINATION
      uint64_t stale_count = epoch_counts.bytes_entries +
                             epoch_counts.plan_entries +
                             epoch_counts.neg_entries;
      total_stale_entries += stale_count;

      nlohmann::ordered_json stale;
      stale["epochManifest"] = epoch_counts.epoch_manifest;
      stale["bytesEntries"] = epoch_counts.bytes_entries;
      stale["planEntries"] = epoch_counts.plan_entries;
      stale["negEntries"] = epoch_counts.neg_entries;
      stale["totalStale"] = stale_count;
      stale_epochs.push_back(stale);
    }
  }

  if (total_stale_entries > 0) {
    proof.status = ProofStatus::INVALID;
    proof.violation_type = ViolationType::STALE_EPOCH_NOT_EVICTED;
    proof.violation_message =
        "Cache contains entries from stale epochs";
    proof.stats.violations_found = total_stale_entries;
  }

  nlohmann::ordered_json evidence;
  evidence["currentEpochManifest"] = current_epoch.manifest_sha256;
  evidence["totalEntriesExamined"] = proof.stats.decisions_examined;
  evidence["totalStaleEntries"] = total_stale_entries;
  evidence["snapshot"] = snapshot.toJsonLD();
  if (!stale_epochs.empty()) {
    evidence["staleEpochs"] = stale_epochs;
  }
  proof.evidence = evidence;

  auto end_time = std::chrono::steady_clock::now();
  proof.stats.proof_generation_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          end_time - start_time).count();

  recordProof(proof);
  updateStats(proof);

  return proof;
}

// =============================================================================
// Real-Time Verification Methods
// =============================================================================

bool CacheCorrectnessProver::verifyCacheHit(
    const CacheDecision& hit_decision,
    const EpochKeyFull& current_epoch) {
  if (!real_time_verification_.load(std::memory_order_relaxed)) {
    return true;  // Verification disabled
  }

  // Verify epoch binding
  if (hit_decision.epoch_key != current_epoch) {
    stats_.hits_rejected.fetch_add(1, std::memory_order_relaxed);
    stats_.cross_epoch_violations.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  // Verify determinism for bytes/neg caches
  if (requiresDeterminism(hit_decision.decision_type)) {
    if (!hit_decision.factors.is_deterministic) {
      stats_.hits_rejected.fetch_add(1, std::memory_order_relaxed);
      stats_.determinism_violations.fetch_add(1, std::memory_order_relaxed);
      return false;
    }
  }

  stats_.hits_verified.fetch_add(1, std::memory_order_relaxed);
  return true;
}

bool CacheCorrectnessProver::verifyCacheInsertion(
    const CacheDecision& insert_decision,
    const EpochKeyFull& current_epoch,
    bool is_deterministic) {
  if (!real_time_verification_.load(std::memory_order_relaxed)) {
    return true;  // Verification disabled
  }

  // Verify epoch binding
  if (insert_decision.epoch_key != current_epoch) {
    stats_.insertions_rejected.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  // Verify determinism for bytes/neg caches
  if (requiresDeterminism(insert_decision.decision_type) && !is_deterministic) {
    stats_.insertions_rejected.fetch_add(1, std::memory_order_relaxed);
    stats_.determinism_violations.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  // Verify frequency threshold
  if (!insert_decision.factors.meets_frequency_threshold &&
      !insert_decision.factors.is_in_top_k_shapes) {
    stats_.insertions_rejected.fetch_add(1, std::memory_order_relaxed);
    stats_.frequency_violations.fetch_add(1, std::memory_order_relaxed);
    return false;
  }

  stats_.insertions_verified.fetch_add(1, std::memory_order_relaxed);
  return true;
}

// =============================================================================
// Statistics and Configuration Methods
// =============================================================================

CacheCorrectnessProver::ProverStats CacheCorrectnessProver::getStats() const {
  return stats_;
}

void CacheCorrectnessProver::resetStats() {
  stats_.proofs_generated.store(0, std::memory_order_relaxed);
  stats_.proofs_valid.store(0, std::memory_order_relaxed);
  stats_.proofs_invalid.store(0, std::memory_order_relaxed);
  stats_.proofs_inconclusive.store(0, std::memory_order_relaxed);
  stats_.hits_verified.store(0, std::memory_order_relaxed);
  stats_.hits_rejected.store(0, std::memory_order_relaxed);
  stats_.insertions_verified.store(0, std::memory_order_relaxed);
  stats_.insertions_rejected.store(0, std::memory_order_relaxed);
  stats_.cross_epoch_violations.store(0, std::memory_order_relaxed);
  stats_.determinism_violations.store(0, std::memory_order_relaxed);
  stats_.frequency_violations.store(0, std::memory_order_relaxed);
}

void CacheCorrectnessProver::setCurrentEpoch(const EpochKeyFull& epoch) {
  std::lock_guard<std::mutex> lock(epoch_mutex_);
  current_epoch_ = epoch;
}

EpochKeyFull CacheCorrectnessProver::getCurrentEpoch() const {
  std::lock_guard<std::mutex> lock(epoch_mutex_);
  return current_epoch_;
}

void CacheCorrectnessProver::setRealTimeVerification(bool enabled) {
  real_time_verification_.store(enabled, std::memory_order_relaxed);
}

bool CacheCorrectnessProver::isRealTimeVerificationEnabled() const {
  return real_time_verification_.load(std::memory_order_relaxed);
}

std::vector<Proof> CacheCorrectnessProver::getAllProofs() const {
  std::lock_guard<std::mutex> lock(proofs_mutex_);
  return proofs_;
}

std::vector<Proof> CacheCorrectnessProver::getProofsSince(uint64_t since_id) const {
  std::lock_guard<std::mutex> lock(proofs_mutex_);

  std::vector<Proof> result;
  for (const auto& p : proofs_) {
    if (p.proof_sequence_id > since_id) {
      result.push_back(p);
    }
  }

  return result;
}

nlohmann::ordered_json CacheCorrectnessProver::proofsToJsonLD() const {
  std::lock_guard<std::mutex> lock(proofs_mutex_);

  nlohmann::ordered_json j;
  j["@context"] = {
      {"@vocab", "https://qlever.cs.uni-freiburg.de/epic4/proof#"}
  };
  j["@type"] = "ProofLog";
  j["totalProofs"] = proofs_.size();

  nlohmann::ordered_json proofs_array = nlohmann::ordered_json::array();
  for (const auto& p : proofs_) {
    proofs_array.push_back(p.toJsonLD());
  }
  j["proofs"] = proofs_array;

  return j;
}

// =============================================================================
// Global Prover Instance
// =============================================================================

std::shared_ptr<CacheCorrectnessProver> globalCacheProver = nullptr;

void initGlobalCacheProver(
    std::shared_ptr<CacheDecisionLog> decision_log,
    const ::readCache::CacheConfig& config) {
  globalCacheProver = std::make_shared<CacheCorrectnessProver>(
      std::move(decision_log), config);
}

std::shared_ptr<CacheCorrectnessProver> getGlobalCacheProver() {
  return globalCacheProver;
}

}  // namespace readPlane
