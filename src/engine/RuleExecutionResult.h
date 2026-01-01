//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (EPIC 5 - Datalog Conformance)

#ifndef QLEVER_SRC_ENGINE_RULEEXECUTIONRESULT_H
#define QLEVER_SRC_ENGINE_RULEEXECUTIONRESULT_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>

/// Result of executing Datalog rules with guards and metrics
///
/// This structure captures the complete outcome of a Datalog fixpoint
/// computation, including derived facts, iteration count, rule firing
/// statistics, and guard trigger information.
///
/// Example:
///   RuleExecutionResult result{
///     .outcome = "OK",
///     .derivedFacts = 42,
///     .iterations = 3,
///     .ruleFires = {{"rule_0_base", 10}, {"rule_1_transitive", 32}},
///     .outputDigestSha256 = "abc123...",
///     .guardTriggered = std::nullopt,
///     .error = std::nullopt,
///     .runtimeMs = 150
///   };
struct RuleExecutionResult {
  /// Execution outcome: "OK", "GUARDED", or "ERROR"
  std::string outcome;

  /// Total number of facts derived during execution
  uint64_t derivedFacts = 0;

  /// Number of fixpoint iterations performed
  uint64_t iterations = 0;

  /// Map from rule name to number of times that rule fired
  /// Rule names follow convention: "rule_<index>_<description>"
  std::map<std::string, uint64_t> ruleFires;

  /// SHA-256 digest of the output facts (for reproducibility verification)
  /// Computed from canonicalized output to ensure deterministic results
  std::string outputDigestSha256;

  /// Name of guard that triggered (if any): "max_iterations",
  /// "max_derived_facts", "max_rule_fires", or "max_runtime"
  std::optional<std::string> guardTriggered;

  /// Error message (if outcome == "ERROR")
  std::optional<std::string> error;

  /// Total runtime in milliseconds
  uint64_t runtimeMs = 0;

  /// Default constructor
  RuleExecutionResult() = default;

  /// Check if execution completed successfully without guard triggers
  [[nodiscard]] bool isOk() const { return outcome == "OK"; }

  /// Check if execution was stopped by a guard
  [[nodiscard]] bool wasGuarded() const { return outcome == "GUARDED"; }

  /// Check if execution encountered an error
  [[nodiscard]] bool hasError() const { return outcome == "ERROR"; }

  /// Get total number of rule fires across all rules
  [[nodiscard]] uint64_t getTotalRuleFires() const {
    uint64_t total = 0;
    for (const auto& [ruleName, count] : ruleFires) {
      total += count;
    }
    return total;
  }

  /// Convert to human-readable string
  [[nodiscard]] std::string toString() const {
    std::string result = "Outcome: " + outcome + "\n";
    result += "Derived Facts: " + std::to_string(derivedFacts) + "\n";
    result += "Iterations: " + std::to_string(iterations) + "\n";
    result += "Total Rule Fires: " + std::to_string(getTotalRuleFires()) + "\n";
    if (guardTriggered) {
      result += "Guard Triggered: " + *guardTriggered + "\n";
    }
    if (error) {
      result += "Error: " + *error + "\n";
    }
    result += "Runtime: " + std::to_string(runtimeMs) + " ms\n";
    result += "Digest: " + outputDigestSha256 + "\n";
    return result;
  }
};

#endif  // QLEVER_SRC_ENGINE_RULEEXECUTIONRESULT_H
