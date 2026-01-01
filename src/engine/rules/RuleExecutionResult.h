//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (EPIC 5 - Datalog Guards Implementation)

#ifndef QLEVER_SRC_ENGINE_RULES_RULEEXECUTIONRESULT_H
#define QLEVER_SRC_ENGINE_RULES_RULEEXECUTIONRESULT_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>

#include "util/Exception.h"

namespace rules {

/// Outcome of a Datalog rule execution.
enum class ExecutionOutcome {
  /// Execution completed successfully within all guards
  OK,

  /// Execution was stopped because a guard was triggered
  /// (exceeded max iterations, facts, rule fires, runtime, or memory)
  GUARDED,

  /// Execution failed due to an error (parsing, validation, etc.)
  ERROR
};

/// Convert ExecutionOutcome to string for debugging/logging
inline std::string toString(ExecutionOutcome outcome) {
  switch (outcome) {
    case ExecutionOutcome::OK:
      return "OK";
    case ExecutionOutcome::GUARDED:
      return "GUARDED";
    case ExecutionOutcome::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

/// Standardized result structure for Datalog rule execution.
///
/// This structure provides a stable, reproducible interface for
/// offline conformance testing and benchmarking.
///
/// Key guarantees:
/// - Deterministic: Same input always produces same output_digest
/// - Stable: Structure will not change across QLever versions
/// - Complete: All relevant execution metrics are captured
///
/// Usage:
/// ```cpp
/// RulesService service;
/// RuleExecutionResult result = service.execute(input);
/// if (result.outcome == ExecutionOutcome::OK) {
///   // Process result.derived_facts
/// } else if (result.outcome == ExecutionOutcome::GUARDED) {
///   LOG(WARNING) << "Guard triggered: " << *result.guard_triggered;
/// }
/// ```
struct RuleExecutionResult {
  /// Outcome of the execution (OK, GUARDED, or ERROR)
  ExecutionOutcome outcome = ExecutionOutcome::ERROR;

  /// Total number of derived facts (cumulative across all iterations)
  /// This count does NOT include duplicates (facts are deduplicated)
  uint64_t derived_facts = 0;

  /// Number of fixpoint iterations performed
  /// (iterations = 0 means no iterations were run)
  uint64_t iterations = 0;

  /// Per-rule fire counts (map from rule name to number of times fired)
  /// Keys are sorted alphabetically for deterministic digest
  std::map<std::string, uint64_t> rule_fires;

  /// SHA-256 hash of derived facts for reproducibility checking
  /// Format: 64-character hex string (lowercase)
  /// Empty string if no facts were derived or execution failed
  /// Digest is computed over:
  /// 1. Sorted facts (lexicographically by subject, predicate, object)
  /// 2. Sorted rule_fires (alphabetically by rule name)
  std::string output_digest_sha256;

  /// If outcome == GUARDED, this indicates which guard was triggered
  /// Possible values:
  /// - "max_iterations"
  /// - "max_derived_facts"
  /// - "max_rule_fires_total"
  /// - "max_runtime_ms"
  /// - "max_memory_bytes"
  std::optional<std::string> guard_triggered;

  /// If outcome == ERROR, this contains the error message
  /// Includes stack trace and source location for debugging
  std::optional<std::string> error;

  /// Total runtime in milliseconds (wall-clock time)
  /// Measured from start to end of execution
  uint64_t runtime_ms = 0;

  /// Peak memory usage in bytes during execution
  /// Value of 0 indicates memory tracking was not available
  uint64_t memory_peak_bytes = 0;

  /// Default constructor creates an ERROR result
  RuleExecutionResult() = default;

  /// Create a successful result
  static RuleExecutionResult ok(uint64_t derivedFacts, uint64_t iterations,
                                std::map<std::string, uint64_t> ruleFires,
                                std::string digest, uint64_t runtimeMs,
                                uint64_t memoryPeakBytes = 0) {
    RuleExecutionResult result;
    result.outcome = ExecutionOutcome::OK;
    result.derived_facts = derivedFacts;
    result.iterations = iterations;
    result.rule_fires = std::move(ruleFires);
    result.output_digest_sha256 = std::move(digest);
    result.runtime_ms = runtimeMs;
    result.memory_peak_bytes = memoryPeakBytes;
    return result;
  }

  /// Create a guarded result (guard triggered)
  static RuleExecutionResult guarded(std::string guardName, uint64_t iterations,
                                     uint64_t derivedFacts,
                                     std::map<std::string, uint64_t> ruleFires,
                                     uint64_t runtimeMs,
                                     uint64_t memoryPeakBytes = 0) {
    RuleExecutionResult result;
    result.outcome = ExecutionOutcome::GUARDED;
    result.guard_triggered = std::move(guardName);
    result.iterations = iterations;
    result.derived_facts = derivedFacts;
    result.rule_fires = std::move(ruleFires);
    result.runtime_ms = runtimeMs;
    result.memory_peak_bytes = memoryPeakBytes;
    // No digest for guarded results (partial results not returned)
    result.output_digest_sha256 = "";
    return result;
  }

  /// Create an error result
  static RuleExecutionResult error(std::string errorMessage) {
    RuleExecutionResult result;
    result.outcome = ExecutionOutcome::ERROR;
    result.error = std::move(errorMessage);
    result.output_digest_sha256 = "";
    return result;
  }

  /// Convert result to human-readable string for debugging
  std::string toString() const {
    std::string s = "RuleExecutionResult{\n";
    s += "  outcome: " + rules::toString(outcome) + "\n";
    s += "  derived_facts: " + std::to_string(derived_facts) + "\n";
    s += "  iterations: " + std::to_string(iterations) + "\n";
    s += "  rule_fires: {";
    bool first = true;
    for (const auto& [rule, count] : rule_fires) {
      if (!first) s += ", ";
      s += rule + ": " + std::to_string(count);
      first = false;
    }
    s += "}\n";
    s += "  output_digest: " + output_digest_sha256 + "\n";
    if (guard_triggered) {
      s += "  guard_triggered: " + *guard_triggered + "\n";
    }
    if (error) {
      s += "  error: " + *error + "\n";
    }
    s += "  runtime_ms: " + std::to_string(runtime_ms) + "\n";
    s += "  memory_peak_bytes: " + std::to_string(memory_peak_bytes) + "\n";
    s += "}";
    return s;
  }
};

}  // namespace rules

#endif  // QLEVER_SRC_ENGINE_RULES_RULEEXECUTIONRESULT_H
