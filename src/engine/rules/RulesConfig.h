//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (EPIC 5 - Datalog Guards Implementation)

#ifndef QLEVER_SRC_ENGINE_RULES_RULESCONFIG_H
#define QLEVER_SRC_ENGINE_RULES_RULESCONFIG_H

#include <cstdint>
#include <limits>
#include <optional>

namespace rules {

/// Configuration for Datalog execution guards (bounded compute).
/// These guards ensure that Datalog execution terminates within
/// reasonable resource bounds.
///
/// When ANY guard is triggered during execution, the execution is
/// immediately stopped and the result outcome is set to GUARDED.
///
/// All guards are mandatory and actively enforced (no "advisory" guards).
struct RulesConfig {
  /// Maximum number of fixpoint iterations before aborting.
  /// Default: 1000 iterations
  /// Behavior: If exceeded, stop execution and set outcome=GUARDED
  uint64_t max_iterations = 1000;

  /// Maximum number of derived facts (total cumulative facts across all
  /// iterations). Default: 1,000,000 facts Behavior: If exceeded, stop
  /// execution and set outcome=GUARDED
  uint64_t max_derived_facts = 1'000'000;

  /// Maximum total number of rule firings across all rules and iterations.
  /// A "rule firing" is one application of a rule to produce new facts.
  /// Default: 10,000,000 firings
  /// Behavior: If exceeded, stop execution and set outcome=GUARDED
  uint64_t max_rule_fires_total = 10'000'000;

  /// Maximum runtime in milliseconds before aborting.
  /// Default: 30000 ms (30 seconds)
  /// Behavior: If exceeded, stop execution and set outcome=GUARDED
  uint64_t max_runtime_ms = 30'000;

  /// Maximum memory usage in bytes (optional, may not be enforceable on all
  /// platforms). Default: std::nullopt (no memory limit)
  /// Behavior: If set and exceeded, stop execution and set outcome=GUARDED
  /// Note: If memory tracking is not available, a warning is logged but
  /// execution continues.
  std::optional<uint64_t> max_memory_bytes = std::nullopt;

  /// Default constructor with standard guard values
  RulesConfig() = default;

  /// Constructor with custom guard values
  RulesConfig(uint64_t maxIterations, uint64_t maxDerivedFacts,
              uint64_t maxRuleFires, uint64_t maxRuntimeMs,
              std::optional<uint64_t> maxMemoryBytes = std::nullopt)
      : max_iterations(maxIterations),
        max_derived_facts(maxDerivedFacts),
        max_rule_fires_total(maxRuleFires),
        max_runtime_ms(maxRuntimeMs),
        max_memory_bytes(maxMemoryBytes) {}

  /// Create a permissive configuration with very high limits
  /// (useful for testing or trusted workloads)
  static RulesConfig permissive() {
    return RulesConfig{
        std::numeric_limits<uint64_t>::max(),  // max_iterations
        std::numeric_limits<uint64_t>::max(),  // max_derived_facts
        std::numeric_limits<uint64_t>::max(),  // max_rule_fires_total
        std::numeric_limits<uint64_t>::max(),  // max_runtime_ms
        std::nullopt                           // max_memory_bytes
    };
  }

  /// Create a strict configuration with tight limits
  /// (useful for untrusted workloads or quick tests)
  static RulesConfig strict() {
    return RulesConfig{
        10,           // max_iterations
        1'000,        // max_derived_facts
        10'000,       // max_rule_fires_total
        1'000,        // max_runtime_ms (1 second)
        100'000'000   // max_memory_bytes (100 MB)
    };
  }
};

}  // namespace rules

#endif  // QLEVER_SRC_ENGINE_RULES_RULESCONFIG_H
