// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Agent 8 - EPIC 10.1
//
// Purpose: DivergenceAbortHandler - Fail-closed divergence abort with
// structured error codes (EPIC 10.1: Workload Replay & Fail-Closed Divergence)
//
// SPEC-LOCK CONSTRAINTS (from EPIC 10.1):
// - Section 3.6: Fail-closed triggers ONLY on deterministic envelope mismatch
// - Section 3.7: Workload replay is required (not optional)
// - Abort must be immediate (no partial results)
// - Error codes must be machine-readable (structured, not prose)
// - No deferred abort permitted
// - All-or-nothing semantics enforced
//
// INVARIANTS:
// - Error codes are enums (machine-readable)
// - Abort is immediate (no continuation after divergence)
// - No partial results emitted (atomic failure)
// - Structured error reporting (JSON-LD serializable)

#ifndef QLEVER_SRC_ENGINE_READPLANE_DIVERGENCEABORTHANDLER_H
#define QLEVER_SRC_ENGINE_READPLANE_DIVERGENCEABORTHANDLER_H

#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/readPlane/EnvelopeDiff.h"
#include "engine/readPlane/ExecutionDigest.h"
#include "engine/readPlane/ReplayResult.h"
#include "util/json.h"

namespace readPlane {

// =============================================================================
// DivergenceErrorCode - Machine-readable structured error codes
// =============================================================================
// These codes are used for automated error handling, alerting, and triage.
// Each code maps to a specific divergence scenario with deterministic meaning.
enum class DivergenceErrorCode : uint32_t {
  // No divergence - success
  SUCCESS = 0x0000,

  // Envelope divergence errors (fail-closed)
  ENVELOPE_DIGEST_MISMATCH = 0x1001,         // Overall digest mismatch
  ENVELOPE_PLAN_HASH_MISMATCH = 0x1002,      // Plan hash differs
  ENVELOPE_RESOURCE_SIG_MISMATCH = 0x1003,   // Resource signature differs
  ENVELOPE_RESULT_SHAPE_MISMATCH = 0x1004,   // Result shape differs
  ENVELOPE_RESULT_LENGTH_MISMATCH = 0x1005,  // Result length differs
  ENVELOPE_MULTIPLE_MISMATCHES = 0x1006,     // Multiple components differ

  // Query fingerprint errors (should never happen for same query)
  QUERY_FINGERPRINT_MISMATCH = 0x2001,  // Different query executed

  // Replay infrastructure errors
  REPLAY_INVALID_CONFIGURATION = 0x3001,      // Configuration error
  REPLAY_MISSING_CONTEXT = 0x3002,            // Required context not set
  REPLAY_QUERY_PARSE_FAILED = 0x3003,         // Query parsing failed
  REPLAY_QUERY_EXECUTION_FAILED = 0x3004,     // Query execution failed
  REPLAY_DIGEST_COMPUTATION_FAILED = 0x3005,  // Digest computation failed

  // Abort handling errors
  ABORT_PARTIAL_RESULTS_DETECTED =
      0x4001,  // Partial results emitted (violation)
  ABORT_DEFERRED_ABORT_DETECTED = 0x4002,  // Abort was deferred (violation)
  ABORT_HANDLER_FAILURE = 0x4003           // Abort handler itself failed
};

// Convert error code to string name (for logging)
inline std::string divergenceErrorCodeToString(DivergenceErrorCode code) {
  switch (code) {
    case DivergenceErrorCode::SUCCESS:
      return "SUCCESS";
    case DivergenceErrorCode::ENVELOPE_DIGEST_MISMATCH:
      return "ENVELOPE_DIGEST_MISMATCH";
    case DivergenceErrorCode::ENVELOPE_PLAN_HASH_MISMATCH:
      return "ENVELOPE_PLAN_HASH_MISMATCH";
    case DivergenceErrorCode::ENVELOPE_RESOURCE_SIG_MISMATCH:
      return "ENVELOPE_RESOURCE_SIG_MISMATCH";
    case DivergenceErrorCode::ENVELOPE_RESULT_SHAPE_MISMATCH:
      return "ENVELOPE_RESULT_SHAPE_MISMATCH";
    case DivergenceErrorCode::ENVELOPE_RESULT_LENGTH_MISMATCH:
      return "ENVELOPE_RESULT_LENGTH_MISMATCH";
    case DivergenceErrorCode::ENVELOPE_MULTIPLE_MISMATCHES:
      return "ENVELOPE_MULTIPLE_MISMATCHES";
    case DivergenceErrorCode::QUERY_FINGERPRINT_MISMATCH:
      return "QUERY_FINGERPRINT_MISMATCH";
    case DivergenceErrorCode::REPLAY_INVALID_CONFIGURATION:
      return "REPLAY_INVALID_CONFIGURATION";
    case DivergenceErrorCode::REPLAY_MISSING_CONTEXT:
      return "REPLAY_MISSING_CONTEXT";
    case DivergenceErrorCode::REPLAY_QUERY_PARSE_FAILED:
      return "REPLAY_QUERY_PARSE_FAILED";
    case DivergenceErrorCode::REPLAY_QUERY_EXECUTION_FAILED:
      return "REPLAY_QUERY_EXECUTION_FAILED";
    case DivergenceErrorCode::REPLAY_DIGEST_COMPUTATION_FAILED:
      return "REPLAY_DIGEST_COMPUTATION_FAILED";
    case DivergenceErrorCode::ABORT_PARTIAL_RESULTS_DETECTED:
      return "ABORT_PARTIAL_RESULTS_DETECTED";
    case DivergenceErrorCode::ABORT_DEFERRED_ABORT_DETECTED:
      return "ABORT_DEFERRED_ABORT_DETECTED";
    case DivergenceErrorCode::ABORT_HANDLER_FAILURE:
      return "ABORT_HANDLER_FAILURE";
    default:
      return "UNKNOWN";
  }
}

// =============================================================================
// StructuredDivergenceError - Machine-readable structured error
// =============================================================================
struct StructuredDivergenceError {
  // Error code (machine-readable enum)
  DivergenceErrorCode error_code = DivergenceErrorCode::SUCCESS;

  // Workload context
  uint64_t workload_record_sequence_id = 0;
  uint64_t replay_run_id = 0;

  // Digest information
  std::string expected_digest;
  std::string actual_digest;

  // Detailed diff (optional, for debugging)
  std::optional<EnvelopeDiff> envelope_diff;

  // Timestamp of detection (nanoseconds)
  uint64_t detection_timestamp_ns = 0;

  // Abort semantics verification
  bool partial_results_emitted = false;  // Must be false for valid fail-closed
  bool abort_was_immediate = true;       // Must be true for valid fail-closed

  // Serialize to JSON-LD (machine-readable, structured)
  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "StructuredDivergenceError";
    j["@severity"] = "FAIL_CLOSED";
    j["abort_was_immediate"] = abort_was_immediate;
    j["actual_digest"] = actual_digest;
    j["detection_timestamp_ns"] = detection_timestamp_ns;
    j["error_code"] = static_cast<uint32_t>(error_code);
    j["error_code_name"] = divergenceErrorCodeToString(error_code);
    j["expected_digest"] = expected_digest;
    j["partial_results_emitted"] = partial_results_emitted;
    j["replay_run_id"] = replay_run_id;
    j["workload_record_sequence_id"] = workload_record_sequence_id;

    if (envelope_diff.has_value()) {
      j["envelope_diff"] = envelope_diff->toJsonLD();
    }

    return j;
  }

  // Check if error is valid (fail-closed semantics enforced)
  [[nodiscard]] bool isValidFailClosed() const {
    // Valid fail-closed requires:
    // 1. No partial results emitted
    // 2. Abort was immediate
    return !partial_results_emitted && abort_was_immediate;
  }

  // Get human-readable summary (for logging only)
  [[nodiscard]] std::string getSummary() const {
    std::ostringstream oss;
    oss << "DivergenceError[" << divergenceErrorCodeToString(error_code)
        << "] run_id=" << replay_run_id
        << " seq_id=" << workload_record_sequence_id
        << " expected=" << expected_digest.substr(0, 16) << "..."
        << " actual=" << actual_digest.substr(0, 16) << "...";
    if (!isValidFailClosed()) {
      oss << " [FAIL-CLOSED VIOLATION]";
    }
    return oss.str();
  }
};

// =============================================================================
// DivergenceAbortException - Exception thrown on fail-closed abort
// =============================================================================
// This exception is thrown to immediately halt replay when divergence is
// detected. It carries structured error information for machine-readable
// reporting.
class DivergenceAbortException : public std::runtime_error {
 private:
  StructuredDivergenceError error_;

 public:
  explicit DivergenceAbortException(StructuredDivergenceError error)
      : std::runtime_error(error.getSummary()), error_(std::move(error)) {}

  // Get the structured error
  [[nodiscard]] const StructuredDivergenceError& getError() const {
    return error_;
  }

  // Get error code
  [[nodiscard]] DivergenceErrorCode getErrorCode() const {
    return error_.error_code;
  }

  // Check if fail-closed semantics were enforced
  [[nodiscard]] bool isValidFailClosed() const {
    return error_.isValidFailClosed();
  }
};

// =============================================================================
// DivergenceAbortHandler - Stateless handler for fail-closed divergence abort
// =============================================================================
// This class provides static methods for handling divergence detection and
// abort. It ensures fail-closed semantics: immediate abort, no partial
// results, structured error codes.
class DivergenceAbortHandler {
 public:
  // No instances - all methods are static
  DivergenceAbortHandler() = delete;

  // ========================================================================
  // Main Entry Points
  // ========================================================================

  // Classify divergence from EnvelopeDiff and return structured error code
  [[nodiscard]] static DivergenceErrorCode classifyDivergence(
      const EnvelopeDiff& diff) {
    if (diff.is_identical) {
      return DivergenceErrorCode::SUCCESS;
    }

    // Query fingerprint mismatch is most severe
    if (diff.fingerprint_differs) {
      return DivergenceErrorCode::QUERY_FINGERPRINT_MISMATCH;
    }

    // Multiple components differ
    if (diff.differenceCount() > 1) {
      return DivergenceErrorCode::ENVELOPE_MULTIPLE_MISMATCHES;
    }

    // Single component classification
    if (diff.plan_changed) {
      return DivergenceErrorCode::ENVELOPE_PLAN_HASH_MISMATCH;
    }
    if (diff.resource_envelope_changed) {
      return DivergenceErrorCode::ENVELOPE_RESOURCE_SIG_MISMATCH;
    }
    if (diff.result_shape_changed) {
      return DivergenceErrorCode::ENVELOPE_RESULT_SHAPE_MISMATCH;
    }
    if (diff.result_length_changed) {
      return DivergenceErrorCode::ENVELOPE_RESULT_LENGTH_MISMATCH;
    }

    // Fallback: generic digest mismatch
    return DivergenceErrorCode::ENVELOPE_DIGEST_MISMATCH;
  }

  // Create structured error from digest comparison
  [[nodiscard]] static StructuredDivergenceError createError(
      const ExecutionDigest& expected, const ExecutionDigest& actual,
      uint64_t workload_record_sequence_id, uint64_t replay_run_id) {
    StructuredDivergenceError error;

    // Compute envelope diff
    EnvelopeDiff diff = EnvelopeDiff::compute(expected, actual);

    // Classify divergence
    error.error_code = classifyDivergence(diff);
    error.workload_record_sequence_id = workload_record_sequence_id;
    error.replay_run_id = replay_run_id;
    error.expected_digest = expected.digest_hash;
    error.actual_digest = actual.digest_hash;
    error.envelope_diff = diff;
    error.detection_timestamp_ns = getCurrentTimestampNs();

    // Fail-closed semantics: no partial results, immediate abort
    error.partial_results_emitted = false;
    error.abort_was_immediate = true;

    return error;
  }

  // Abort with fail-closed semantics (throws DivergenceAbortException)
  [[noreturn]] static void abortFailClosed(
      const StructuredDivergenceError& error) {
    // Verify fail-closed semantics before throwing
    if (!error.isValidFailClosed()) {
      // This is a critical violation - fail-closed was not enforced
      StructuredDivergenceError violation_error = error;
      violation_error.error_code =
          DivergenceErrorCode::ABORT_PARTIAL_RESULTS_DETECTED;
      throw DivergenceAbortException(violation_error);
    }

    // Throw structured exception
    throw DivergenceAbortException(error);
  }

  // ========================================================================
  // High-Level Convenience Methods
  // ========================================================================

  // Handle divergence detection: classify, create error, abort
  [[noreturn]] static void handleDivergence(
      const ExecutionDigest& expected, const ExecutionDigest& actual,
      uint64_t workload_record_sequence_id, uint64_t replay_run_id) {
    // Create structured error
    auto error = createError(expected, actual, workload_record_sequence_id,
                             replay_run_id);

    // Abort with fail-closed semantics
    abortFailClosed(error);
  }

  // Verify that ReplayResult enforces fail-closed semantics
  // Returns true if valid, throws if fail-closed violation detected
  [[nodiscard]] static bool verifyFailClosedSemantics(
      const ReplayResult& result) {
    if (result.execution_status == ReplayStatus::DIVERGENCE) {
      // Divergence detected - must have divergence artifact
      if (!result.divergence_artifact.has_value()) {
        // Missing artifact is a violation
        StructuredDivergenceError error;
        error.error_code = DivergenceErrorCode::ABORT_HANDLER_FAILURE;
        error.workload_record_sequence_id = result.workload_record_id;
        error.replay_run_id = result.replay_run_id;
        error.partial_results_emitted = false;
        error.abort_was_immediate = false;
        error.detection_timestamp_ns = getCurrentTimestampNs();
        throw DivergenceAbortException(error);
      }
    }

    return true;
  }

  // ========================================================================
  // Utility Methods
  // ========================================================================

  // Get current timestamp in nanoseconds
  static uint64_t getCurrentTimestampNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
  }

  // Convert error code to exit code (for process termination)
  [[nodiscard]] static int errorCodeToExitCode(DivergenceErrorCode code) {
    // Map error codes to exit codes
    // 0 = success, 1-127 = various failure modes
    switch (code) {
      case DivergenceErrorCode::SUCCESS:
        return 0;
      case DivergenceErrorCode::ENVELOPE_DIGEST_MISMATCH:
        return 10;
      case DivergenceErrorCode::ENVELOPE_PLAN_HASH_MISMATCH:
        return 11;
      case DivergenceErrorCode::ENVELOPE_RESOURCE_SIG_MISMATCH:
        return 12;
      case DivergenceErrorCode::ENVELOPE_RESULT_SHAPE_MISMATCH:
        return 13;
      case DivergenceErrorCode::ENVELOPE_RESULT_LENGTH_MISMATCH:
        return 14;
      case DivergenceErrorCode::ENVELOPE_MULTIPLE_MISMATCHES:
        return 15;
      case DivergenceErrorCode::QUERY_FINGERPRINT_MISMATCH:
        return 20;
      case DivergenceErrorCode::REPLAY_INVALID_CONFIGURATION:
        return 30;
      case DivergenceErrorCode::REPLAY_MISSING_CONTEXT:
        return 31;
      case DivergenceErrorCode::REPLAY_QUERY_PARSE_FAILED:
        return 32;
      case DivergenceErrorCode::REPLAY_QUERY_EXECUTION_FAILED:
        return 33;
      case DivergenceErrorCode::REPLAY_DIGEST_COMPUTATION_FAILED:
        return 34;
      case DivergenceErrorCode::ABORT_PARTIAL_RESULTS_DETECTED:
        return 40;
      case DivergenceErrorCode::ABORT_DEFERRED_ABORT_DETECTED:
        return 41;
      case DivergenceErrorCode::ABORT_HANDLER_FAILURE:
        return 42;
      default:
        return 1;
    }
  }
};

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_DIVERGENCEABORTHANDLER_H
