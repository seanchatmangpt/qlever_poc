// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Agent 2 - EPIC 10.1 Definition Set A
//
// Purpose: GuardConfiguration - Schema for execution guards and fail-closed
// constraints (EPIC 10.1: Epoch Identity Normalization, Definition Set A)
//
// GuardConfiguration defines the structural constraints and invariants that
// govern deterministic query execution. These guards ensure:
// 1. Fail-closed semantics on envelope divergence
// 2. Deterministic epoch binding
// 3. Canonical equivalence validation
//
// SPEC-LOCK CONSTRAINTS (from EPIC 10.1):
// - Section 2: Epoch identity normalization (EpochKey tuple)
// - Section 3: Canonical serialization for determinism
// - Section 4: Guard rules for execution validation
// - Section 5: Deterministic comparison (no floating-point)
//
// INVARIANTS:
// - Configuration is immutable after initialization
// - Serialization is deterministic (sorted fields, fixed order)
// - Equality is based on all configuration fields
// - No platform-specific values permitted

#ifndef QLEVER_SRC_ENGINE_READPLANE_GUARDCONFIGURATION_H
#define QLEVER_SRC_ENGINE_READPLANE_GUARDCONFIGURATION_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "util/CryptographicHashUtils.h"
#include "util/json.h"

namespace readPlane {

// ===========================================================================
// GuardRuleType - Machine-readable guard rule classification
// ===========================================================================
// These codes specify which envelope component(s) must match for execution
// equivalence. Used to determine fail-closed divergence triggers.
enum class GuardRuleType : uint8_t {
  // Envelope-based guards (deterministic envelope matching)
  PLAN_HASH_MUST_MATCH = 0x01,          // QueryExecutionTree structure
  QUERY_FINGERPRINT_MUST_MATCH = 0x02,  // Query identity (EPIC 3)
  RESOURCE_ENVELOPE_MUST_MATCH = 0x04,  // Memory/cache behavior
  RESULT_SHAPE_MUST_MATCH = 0x08,   // Result structure (column count, types)
  RESULT_LENGTH_MUST_MATCH = 0x10,  // Number of result rows

  // Epoch-based guards (epoch binding validation)
  EPOCH_MUST_NOT_CHANGE = 0x20,      // Forbid epoch transition during replay
  EPOCH_MANIFEST_MUST_MATCH = 0x40,  // Manifest hash must remain constant

  // Composite guards
  ENVELOPE_STRICT = PLAN_HASH_MUST_MATCH | QUERY_FINGERPRINT_MUST_MATCH |
                    RESOURCE_ENVELOPE_MUST_MATCH | RESULT_SHAPE_MUST_MATCH |
                    RESULT_LENGTH_MUST_MATCH,  // All envelope components
  EPOCH_STRICT = EPOCH_MUST_NOT_CHANGE | EPOCH_MANIFEST_MUST_MATCH,

  // Default: strict mode (all guards enabled)
  ALL_GUARDS_STRICT = ENVELOPE_STRICT | EPOCH_STRICT
};

// Bitwise operations for combining guard rules
inline GuardRuleType operator|(GuardRuleType a, GuardRuleType b) {
  return static_cast<GuardRuleType>(static_cast<uint8_t>(a) |
                                    static_cast<uint8_t>(b));
}

inline GuardRuleType operator&(GuardRuleType a, GuardRuleType b) {
  return static_cast<GuardRuleType>(static_cast<uint8_t>(a) &
                                    static_cast<uint8_t>(b));
}

inline GuardRuleType& operator|=(GuardRuleType& a, GuardRuleType b) {
  a = a | b;
  return a;
}

// String representation for logging
inline std::string guardRuleTypeToString(GuardRuleType rule) {
  std::string result;
  if ((rule & GuardRuleType::PLAN_HASH_MUST_MATCH) != GuardRuleType(0)) {
    if (!result.empty()) result += "|";
    result += "PLAN_HASH_MUST_MATCH";
  }
  if ((rule & GuardRuleType::QUERY_FINGERPRINT_MUST_MATCH) !=
      GuardRuleType(0)) {
    if (!result.empty()) result += "|";
    result += "QUERY_FINGERPRINT_MUST_MATCH";
  }
  if ((rule & GuardRuleType::RESOURCE_ENVELOPE_MUST_MATCH) !=
      GuardRuleType(0)) {
    if (!result.empty()) result += "|";
    result += "RESOURCE_ENVELOPE_MUST_MATCH";
  }
  if ((rule & GuardRuleType::RESULT_SHAPE_MUST_MATCH) != GuardRuleType(0)) {
    if (!result.empty()) result += "|";
    result += "RESULT_SHAPE_MUST_MATCH";
  }
  if ((rule & GuardRuleType::RESULT_LENGTH_MUST_MATCH) != GuardRuleType(0)) {
    if (!result.empty()) result += "|";
    result += "RESULT_LENGTH_MUST_MATCH";
  }
  if ((rule & GuardRuleType::EPOCH_MUST_NOT_CHANGE) != GuardRuleType(0)) {
    if (!result.empty()) result += "|";
    result += "EPOCH_MUST_NOT_CHANGE";
  }
  if ((rule & GuardRuleType::EPOCH_MANIFEST_MUST_MATCH) != GuardRuleType(0)) {
    if (!result.empty()) result += "|";
    result += "EPOCH_MANIFEST_MUST_MATCH";
  }
  if (result.empty()) {
    result = "NONE";
  }
  return result;
}

// ===========================================================================
// AbortStrategy - Action taken when guard violation is detected
// ===========================================================================
enum class AbortStrategy : uint8_t {
  ABORT_IMMEDIATELY = 0x00,  // Throw exception immediately (fail-closed)
  LOG_AND_ABORT = 0x01,      // Log violation details, then abort
  ALERT_AND_ABORT = 0x02,    // Send alert, log, then abort
};

inline std::string abortStrategyToString(AbortStrategy strategy) {
  switch (strategy) {
    case AbortStrategy::ABORT_IMMEDIATELY:
      return "ABORT_IMMEDIATELY";
    case AbortStrategy::LOG_AND_ABORT:
      return "LOG_AND_ABORT";
    case AbortStrategy::ALERT_AND_ABORT:
      return "ALERT_AND_ABORT";
    default:
      return "UNKNOWN";
  }
}

// ===========================================================================
// GuardConfiguration - Schema for execution guards
// ===========================================================================
// GuardConfiguration specifies:
// 1. Which envelope components must match for equivalence
// 2. What happens when a guard violation is detected
// 3. Timeout behavior for guard checks
// 4. Logging and monitoring configuration
//
// EPIC 10.1 requires:
// - Deterministic serialization (for hashing)
// - Immutability (configuration cannot change mid-replay)
// - Explicit guard rules (no implicit assumptions)
struct GuardConfiguration {
  // =========================================================================
  // Guard Rules (What must match)
  // =========================================================================

  // Bitfield of active guard rules
  GuardRuleType active_guards = GuardRuleType::ALL_GUARDS_STRICT;

  // =========================================================================
  // Abort Behavior (What happens on violation)
  // =========================================================================

  // Strategy for handling guard violations
  AbortStrategy abort_strategy = AbortStrategy::ABORT_IMMEDIATELY;

  // Maximum time (ms) to wait before aborting on guard check timeout
  uint32_t abort_timeout_ms = 1000;

  // =========================================================================
  // Validation Constraints
  // =========================================================================

  // Minimum required envelope match quality (0.0-1.0)
  // Used for fuzzy matching in cache decisions (not for correctness)
  // CONSTRAINT: Must be <= 1.0 for validity
  // NOTE: Envelope correctness uses exact matching (guard rules), not fuzzy
  double envelope_match_threshold = 1.0;

  // =========================================================================
  // Monitoring and Observability
  // =========================================================================

  // Enable detailed logging of guard violations (for debugging)
  bool log_guard_violations = false;

  // Enable metrics collection for guard checks
  bool collect_guard_metrics = true;

  // Maximum number of violations to log before suppressing (prevent spam)
  uint32_t max_logged_violations = 100;

  // =========================================================================
  // Constructors and Factories
  // =========================================================================

  GuardConfiguration() = default;

  // Factory: Create strict configuration (all guards enabled)
  [[nodiscard]] static GuardConfiguration createStrict() {
    GuardConfiguration config;
    config.active_guards = GuardRuleType::ALL_GUARDS_STRICT;
    config.abort_strategy = AbortStrategy::ABORT_IMMEDIATELY;
    config.envelope_match_threshold = 1.0;
    config.log_guard_violations = false;
    config.collect_guard_metrics = true;
    return config;
  }

  // Factory: Create permissive configuration (envelope-only, no epoch)
  [[nodiscard]] static GuardConfiguration createEnvelopeOnly() {
    GuardConfiguration config;
    config.active_guards = GuardRuleType::ENVELOPE_STRICT;
    config.abort_strategy = AbortStrategy::LOG_AND_ABORT;
    config.envelope_match_threshold = 1.0;
    config.log_guard_violations = true;
    config.collect_guard_metrics = true;
    return config;
  }

  // =========================================================================
  // Comparison and Validation
  // =========================================================================

  // Equality operator - all fields must match for equivalence
  bool operator==(const GuardConfiguration& other) const {
    return active_guards == other.active_guards &&
           abort_strategy == other.abort_strategy &&
           abort_timeout_ms == other.abort_timeout_ms &&
           envelope_match_threshold == other.envelope_match_threshold &&
           log_guard_violations == other.log_guard_violations &&
           collect_guard_metrics == other.collect_guard_metrics &&
           max_logged_violations == other.max_logged_violations;
  }

  bool operator!=(const GuardConfiguration& other) const {
    return !(*this == other);
  }

  // Validate configuration constraints
  [[nodiscard]] bool isValid() const {
    // envelope_match_threshold must be in [0.0, 1.0]
    if (envelope_match_threshold < 0.0 || envelope_match_threshold > 1.0) {
      return false;
    }
    // abort_timeout_ms must be positive
    if (abort_timeout_ms == 0) {
      return false;
    }
    return true;
  }

  // Check if a specific guard rule is active
  [[nodiscard]] bool hasGuard(GuardRuleType rule) const {
    return (active_guards & rule) == rule;
  }

  // =========================================================================
  // Canonical Serialization (for deterministic hashing)
  // =========================================================================

  // Serialize to deterministic byte representation for hashing
  // Format: Fixed-width binary with alphabetically sorted string fields
  // Guarantees: Same configuration → same serialization → same hash
  [[nodiscard]] std::string toCanonicalBytes() const;

  // Deserialize from canonical byte representation
  [[nodiscard]] static GuardConfiguration fromCanonicalBytes(
      const std::string& bytes);

  // Compute deterministic hash (SHA256 of canonical bytes)
  // Returns 64-character lowercase hex string
  [[nodiscard]] std::string computeCanonicalHash() const;

  // =========================================================================
  // JSON-LD Serialization (for portability)
  // =========================================================================

  // Serialize to JSON-LD format with deterministic field ordering
  [[nodiscard]] nlohmann::ordered_json toJsonLD() const;

  // Deserialize from JSON-LD format
  [[nodiscard]] static GuardConfiguration fromJsonLD(
      const nlohmann::ordered_json& json);

  // =========================================================================
  // String Representation (for logging/debugging)
  // =========================================================================

  // Human-readable summary (not for correctness)
  [[nodiscard]] std::string toString() const;
};

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_GUARDCONFIGURATION_H
