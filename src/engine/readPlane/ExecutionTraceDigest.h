// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: ExecutionTraceEvent for tracing query execution (informational only).
// Part of EPIC 4 Subsystem 2: Workload Replay Subsystem.
//
// NOTE: This file provides trace event types for debugging and observability.
// The main ExecutionDigest structure is in ExecutionDigest.h (from Subsystem 1).

#ifndef QLEVER_SRC_ENGINE_READPLANE_EXECUTIONTRACEDIGEST_H
#define QLEVER_SRC_ENGINE_READPLANE_EXECUTIONTRACEDIGEST_H

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "engine/readPlane/ExecutionDigest.h"
#include "global/Epoch.h"
#include "util/json.h"

namespace readPlane {

// =============================================================================
// ExecutionTraceEvent - Wall-clock events for debugging (NOT for correctness)
// =============================================================================
// These events contain wall-clock times which are EXCLUDED from digest
// computation. They are purely informational for debugging and observability.

enum class TraceEventType {
  QUERY_START,
  CACHE_LOOKUP,
  PLAN_COMPILE,
  EXECUTION_START,
  EXECUTION_END,
  RESULT_SERIALIZE,
  QUERY_COMPLETE
};

inline std::string traceEventTypeToString(TraceEventType type) {
  switch (type) {
    case TraceEventType::QUERY_START:
      return "QUERY_START";
    case TraceEventType::CACHE_LOOKUP:
      return "CACHE_LOOKUP";
    case TraceEventType::PLAN_COMPILE:
      return "PLAN_COMPILE";
    case TraceEventType::EXECUTION_START:
      return "EXECUTION_START";
    case TraceEventType::EXECUTION_END:
      return "EXECUTION_END";
    case TraceEventType::RESULT_SERIALIZE:
      return "RESULT_SERIALIZE";
    case TraceEventType::QUERY_COMPLETE:
      return "QUERY_COMPLETE";
    default:
      return "UNKNOWN";
  }
}

struct ExecutionTraceEvent {
  TraceEventType event_type;
  uint64_t wall_clock_ns;  // Wall-clock time (informational only, excluded from
                           // digest)
  std::string description;

  // Create a trace event at the current time
  static ExecutionTraceEvent now(TraceEventType type,
                                 std::string desc = "") {
    auto now_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count();
    return {type, static_cast<uint64_t>(now_ns), std::move(desc)};
  }

  // JSON-LD serialization (wall-clock time marked as excluded from hash)
  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "ExecutionTraceEvent";
    j["description"] = description;
    j["event_type"] = traceEventTypeToString(event_type);
    j["wall_clock_ns"] = wall_clock_ns;
    j["wall_clock_ns@excluded_from_hash"] = true;
    return j;
  }
};

// =============================================================================
// DigestComparisonResult - Result of comparing two digests
// =============================================================================
enum class DigestComparisonOutcome {
  MATCH,                    // Digests are identical
  DIVERGENCE,               // Digests differ (fail-closed)
  CROSS_EPOCH_REPRODUCIBLE  // Same digest, different epochs (success with note)
};

struct DigestComparisonResult {
  DigestComparisonOutcome outcome;
  std::string expected_digest;
  std::string actual_digest;
  ad_utility::EpochId expected_epoch;
  ad_utility::EpochId actual_epoch;
  std::string divergence_details;

  // Check if comparison indicates success (MATCH or CROSS_EPOCH_REPRODUCIBLE)
  [[nodiscard]] bool isSuccess() const {
    return outcome == DigestComparisonOutcome::MATCH ||
           outcome == DigestComparisonOutcome::CROSS_EPOCH_REPRODUCIBLE;
  }

  // Check if comparison indicates failure (DIVERGENCE)
  [[nodiscard]] bool isDivergence() const {
    return outcome == DigestComparisonOutcome::DIVERGENCE;
  }

  // JSON-LD serialization
  nlohmann::ordered_json toJsonLD() const {
    nlohmann::ordered_json j;
    j["@type"] = "DigestComparisonResult";
    j["actual_digest"] = actual_digest;
    j["actual_epoch"] = actual_epoch;
    j["divergence_details"] = divergence_details;
    j["expected_digest"] = expected_digest;
    j["expected_epoch"] = expected_epoch;
    switch (outcome) {
      case DigestComparisonOutcome::MATCH:
        j["outcome"] = "MATCH";
        break;
      case DigestComparisonOutcome::DIVERGENCE:
        j["outcome"] = "DIVERGENCE";
        break;
      case DigestComparisonOutcome::CROSS_EPOCH_REPRODUCIBLE:
        j["outcome"] = "CROSS_EPOCH_REPRODUCIBLE";
        break;
    }
    return j;
  }

  // Factory method: compare two digests
  static DigestComparisonResult compare(const ExecutionDigest& expected,
                                        const ExecutionDigest& actual,
                                        ad_utility::EpochId expected_epoch,
                                        ad_utility::EpochId actual_epoch) {
    DigestComparisonResult result;
    result.expected_digest = expected.digest_hash;
    result.actual_digest = actual.digest_hash;
    result.expected_epoch = expected_epoch;
    result.actual_epoch = actual_epoch;

    if (expected.digest_hash == actual.digest_hash) {
      if (expected_epoch != actual_epoch) {
        // Same digest, different epochs - cross-epoch reproducible
        result.outcome = DigestComparisonOutcome::CROSS_EPOCH_REPRODUCIBLE;
        result.divergence_details =
            "Digest matches across epochs: expected epoch " +
            std::to_string(expected_epoch) + ", actual epoch " +
            std::to_string(actual_epoch);
      } else {
        // Same digest, same epoch - perfect match
        result.outcome = DigestComparisonOutcome::MATCH;
      }
    } else {
      // Digests differ - divergence
      result.outcome = DigestComparisonOutcome::DIVERGENCE;

      // Build detailed divergence info
      std::ostringstream oss;
      oss << "Digest mismatch: ";
      oss << "expected=" << expected.digest_hash.substr(0, 16) << "..., ";
      oss << "actual=" << actual.digest_hash.substr(0, 16) << "...";

      if (expected.plan_hash != actual.plan_hash) {
        oss << "; plan_hash differs";
      }
      if (expected.result_length_hash != actual.result_length_hash) {
        oss << "; result_length differs";
      }
      if (expected.result_shape_hash != actual.result_shape_hash) {
        oss << "; result_shape differs";
      }
      if (expected.resource_signature != actual.resource_signature) {
        oss << "; resource_signature differs";
      }

      result.divergence_details = oss.str();
    }

    return result;
  }
};

}  // namespace readPlane

#endif  // QLEVER_SRC_ENGINE_READPLANE_EXECUTIONTRACEDIGEST_H
