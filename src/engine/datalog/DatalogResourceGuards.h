//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Agent 6 - EPIC 10.2 Datalog/N3 Guardrails

#ifndef QLEVER_SRC_ENGINE_DATALOG_RESOURCEGUARDS_H
#define QLEVER_SRC_ENGINE_DATALOG_RESOURCEGUARDS_H

#include <chrono>
#include <cstdint>
#include <stdexcept>

#include "global/Epoch.h"
#include "global/EpochManifest.h"

namespace datalog {

/// Resource limits for Datalog/N3 rule execution to prevent runaway computation
/// and ensure epoch isolation.
///
/// EPIC 10.2 Specification:
/// - Epoch semantics: Execution strictly bounded by EpochID
/// - No cross-contamination: Results cached with (QueryFingerprint, EpochID) key
/// - Resource limits: MaxFactCount, MaxRuleTime, MaxMemory
struct DatalogResourceGuards {
  /// Maximum number of facts generated per rule iteration
  /// Prevents fact explosion in recursive rules
  /// Default: 1,000,000 facts
  size_t maxFactCount = 1'000'000;

  /// Maximum wall-clock time per rule execution (in milliseconds)
  /// Prevents infinite loops and excessive computation
  /// Default: 30,000 ms (30 seconds)
  std::chrono::milliseconds maxRuleTime{30'000};

  /// Maximum heap growth during rule execution (in bytes)
  /// Prevents OOM conditions
  /// Default: 1 GB
  size_t maxMemoryBytes = 1'000'000'000;

  /// Epoch ID that bounds this execution
  /// All cache keys must include this to prevent cross-epoch contamination
  ad_utility::EpochId epochId = 0;

  /// Manifest hash for deterministic cache keying
  /// Derived from EpochManifest.getManifestHash()
  std::string manifestHash;

  /// Validate that guards are properly configured
  void validate() const {
    if (maxFactCount == 0) {
      throw std::invalid_argument("maxFactCount must be > 0");
    }
    if (maxRuleTime.count() == 0) {
      throw std::invalid_argument("maxRuleTime must be > 0");
    }
    if (maxMemoryBytes == 0) {
      throw std::invalid_argument("maxMemoryBytes must be > 0");
    }
    // Note: epochId can be 0 for initial epoch
    // manifestHash can be empty if no manifest is bound
  }

  /// Create guards from QueryExecutionContext
  /// Automatically extracts epoch ID and manifest hash
  static DatalogResourceGuards fromContext(
      const class QueryExecutionContext* qec);
};

/// Exception thrown when a resource guard is violated
class ResourceGuardViolation : public std::runtime_error {
 public:
  explicit ResourceGuardViolation(const std::string& msg)
      : std::runtime_error(msg) {}
};

/// Timer for tracking rule execution time
class RuleExecutionTimer {
 private:
  std::chrono::steady_clock::time_point startTime_;
  std::chrono::milliseconds timeLimit_;

 public:
  explicit RuleExecutionTimer(std::chrono::milliseconds limit)
      : startTime_(std::chrono::steady_clock::now()), timeLimit_(limit) {}

  /// Check if time limit has been exceeded
  /// Throws ResourceGuardViolation if limit exceeded
  void check() const {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime_);
    if (elapsed > timeLimit_) {
      throw ResourceGuardViolation(
          "Rule execution time limit exceeded: " +
          std::to_string(elapsed.count()) + "ms > " +
          std::to_string(timeLimit_.count()) + "ms");
    }
  }

  /// Get elapsed time in milliseconds
  std::chrono::milliseconds elapsed() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime_);
  }
};

/// Tracker for fact count limits
class FactCountTracker {
 private:
  size_t currentFactCount_ = 0;
  size_t limit_;

 public:
  explicit FactCountTracker(size_t limit) : limit_(limit) {}

  /// Record facts generated in this iteration
  void addFacts(size_t count) {
    currentFactCount_ += count;
    if (currentFactCount_ > limit_) {
      throw ResourceGuardViolation(
          "Fact count limit exceeded: " + std::to_string(currentFactCount_) +
          " > " + std::to_string(limit_));
    }
  }

  /// Get current fact count
  size_t count() const { return currentFactCount_; }

  /// Get limit
  size_t limit() const { return limit_; }
};

/// Tracker for memory usage
class MemoryUsageTracker {
 private:
  size_t initialMemory_ = 0;
  size_t currentMemory_ = 0;
  size_t limit_;

 public:
  explicit MemoryUsageTracker(size_t limit) : limit_(limit) {}

  /// Record memory allocation
  void recordAllocation(size_t bytes) {
    currentMemory_ += bytes;
    if (currentMemory_ - initialMemory_ > limit_) {
      throw ResourceGuardViolation(
          "Memory limit exceeded: " +
          std::to_string(currentMemory_ - initialMemory_) + " > " +
          std::to_string(limit_));
    }
  }

  /// Record memory deallocation
  void recordDeallocation(size_t bytes) {
    if (currentMemory_ >= bytes) {
      currentMemory_ -= bytes;
    }
  }

  /// Set baseline memory (called at start of execution)
  void setBaseline(size_t baseline) {
    initialMemory_ = baseline;
    currentMemory_ = baseline;
  }

  /// Get current memory usage above baseline
  size_t usage() const {
    return currentMemory_ > initialMemory_ ? currentMemory_ - initialMemory_
                                           : 0;
  }

  /// Get limit
  size_t limit() const { return limit_; }
};

}  // namespace datalog

#endif  // QLEVER_SRC_ENGINE_DATALOG_RESOURCEGUARDS_H
