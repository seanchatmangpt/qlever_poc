//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (EPIC 5 - Datalog Guards Implementation)

#ifndef QLEVER_SRC_ENGINE_RULES_RULEEXECUTIONCONTEXT_H
#define QLEVER_SRC_ENGINE_RULES_RULEEXECUTIONCONTEXT_H

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>

#include "engine/rules/RuleExecutionResult.h"
#include "engine/rules/RulesConfig.h"
#include "util/Log.h"

namespace rules {

/// Guard monitor for tracking and enforcing execution limits during
/// Datalog rule execution.
///
/// This class tracks all guard metrics (iterations, facts, rule fires,
/// runtime, memory) and provides methods to check if any guard has been
/// triggered.
///
/// Thread-safe: No. This class is designed to be used by a single thread
/// executing a Datalog query. If concurrent execution is needed, create
/// separate GuardMonitor instances per thread.
///
/// Usage:
/// ```cpp
/// GuardMonitor monitor(config);
/// monitor.start();
///
/// while (!monitor.shouldAbort()) {
///   // Run iteration
///   monitor.incrementIteration();
///   monitor.addDerivedFacts(newFacts);
///   monitor.recordRuleFire("rule1");
/// }
///
/// monitor.stop();
/// RuleExecutionResult result = monitor.createResult(...);
/// ```
class GuardMonitor {
 public:
  /// Create a guard monitor with the given configuration
  explicit GuardMonitor(const RulesConfig& config) : config_(config) {}

  /// Start the execution timer
  void start() {
    start_time_ = std::chrono::steady_clock::now();
    is_started_ = true;
  }

  /// Stop the execution timer and record final runtime
  void stop() {
    if (!is_started_) return;
    auto end_time = std::chrono::steady_clock::now();
    runtime_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
                      end_time - start_time_)
                      .count();
    is_stopped_ = true;
  }

  /// Increment the iteration counter
  void incrementIteration() { ++iterations_; }

  /// Get current iteration count
  [[nodiscard]] uint64_t getIterations() const { return iterations_; }

  /// Add newly derived facts (increments fact counter)
  void addDerivedFacts(uint64_t count) { derived_facts_ += count; }

  /// Get total derived facts count
  [[nodiscard]] uint64_t getDerivedFacts() const { return derived_facts_; }

  /// Record a rule firing (increment counter for the given rule)
  void recordRuleFire(const std::string& ruleName) {
    rule_fires_[ruleName]++;
    total_rule_fires_++;
  }

  /// Get total rule fires across all rules
  [[nodiscard]] uint64_t getTotalRuleFires() const { return total_rule_fires_; }

  /// Get per-rule fire counts
  [[nodiscard]] const std::map<std::string, uint64_t>& getRuleFires() const {
    return rule_fires_;
  }

  /// Update peak memory usage (in bytes)
  void updateMemoryUsage(uint64_t bytes) {
    if (bytes > memory_peak_bytes_) {
      memory_peak_bytes_ = bytes;
    }
  }

  /// Get peak memory usage
  [[nodiscard]] uint64_t getMemoryPeakBytes() const {
    return memory_peak_bytes_;
  }

  /// Get current runtime in milliseconds
  [[nodiscard]] uint64_t getRuntimeMs() const {
    if (is_stopped_) {
      return runtime_ms_;
    }
    if (!is_started_) {
      return 0;
    }
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now -
                                                                  start_time_)
        .count();
  }

  /// Check if any guard has been triggered
  /// Returns the name of the triggered guard, or std::nullopt if none
  [[nodiscard]] std::optional<std::string> checkGuards() const {
    // Check iteration limit
    if (iterations_ >= config_.max_iterations) {
      return "max_iterations";
    }

    // Check derived facts limit
    if (derived_facts_ >= config_.max_derived_facts) {
      return "max_derived_facts";
    }

    // Check total rule fires limit
    if (total_rule_fires_ >= config_.max_rule_fires_total) {
      return "max_rule_fires_total";
    }

    // Check runtime limit
    uint64_t currentRuntime = getRuntimeMs();
    if (currentRuntime >= config_.max_runtime_ms) {
      return "max_runtime_ms";
    }

    // Check memory limit (if set)
    if (config_.max_memory_bytes.has_value() &&
        memory_peak_bytes_ >= *config_.max_memory_bytes) {
      return "max_memory_bytes";
    }

    return std::nullopt;
  }

  /// Check if execution should abort due to guard trigger
  /// Logs a warning if a guard is triggered
  [[nodiscard]] bool shouldAbort() const {
    auto triggered = checkGuards();
    if (triggered) {
      LOG(WARNING) << "Datalog guard triggered: " << *triggered << std::endl;
      return true;
    }
    return false;
  }

  /// Abort execution if a guard has been triggered
  /// Throws an exception with details about which guard was triggered
  void abortIfGuardTriggered() const {
    auto triggered = checkGuards();
    if (triggered) {
      std::string message = "Datalog execution guard triggered: " + *triggered;
      message += "\n  Iterations: " + std::to_string(iterations_);
      message += " (limit: " + std::to_string(config_.max_iterations) + ")";
      message += "\n  Derived facts: " + std::to_string(derived_facts_);
      message +=
          " (limit: " + std::to_string(config_.max_derived_facts) + ")";
      message +=
          "\n  Total rule fires: " + std::to_string(total_rule_fires_);
      message += " (limit: " + std::to_string(config_.max_rule_fires_total) +
                 ")";
      message += "\n  Runtime: " + std::to_string(getRuntimeMs()) + " ms";
      message += " (limit: " + std::to_string(config_.max_runtime_ms) + " ms)";
      if (config_.max_memory_bytes) {
        message += "\n  Memory: " + std::to_string(memory_peak_bytes_) +
                   " bytes";
        message +=
            " (limit: " + std::to_string(*config_.max_memory_bytes) + " bytes)";
      }
      AD_THROW(message);
    }
  }

  /// Create a snapshot of current state for safe capture
  /// This is useful for creating results even if guards were triggered
  struct Snapshot {
    uint64_t iterations;
    uint64_t derived_facts;
    std::map<std::string, uint64_t> rule_fires;
    uint64_t runtime_ms;
    uint64_t memory_peak_bytes;
    std::optional<std::string> guard_triggered;
  };

  /// Capture current state as a snapshot
  [[nodiscard]] Snapshot snapshot() const {
    return Snapshot{iterations_,      derived_facts_,    rule_fires_,
                    getRuntimeMs(),   memory_peak_bytes_, checkGuards()};
  }

 private:
  /// Configuration with guard limits
  RulesConfig config_;

  /// Tracking state
  uint64_t iterations_ = 0;
  uint64_t derived_facts_ = 0;
  uint64_t total_rule_fires_ = 0;
  std::map<std::string, uint64_t> rule_fires_;
  uint64_t memory_peak_bytes_ = 0;

  /// Timing state
  bool is_started_ = false;
  bool is_stopped_ = false;
  std::chrono::steady_clock::time_point start_time_;
  uint64_t runtime_ms_ = 0;
};

}  // namespace rules

#endif  // QLEVER_SRC_ENGINE_RULES_RULEEXECUTIONCONTEXT_H
