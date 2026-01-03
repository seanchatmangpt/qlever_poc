//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: EPIC 14.0 Agent 3 - Unified Evaluation Kernel

#ifndef QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDEVALUATIONKERNEL_H
#define QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDEVALUATIONKERNEL_H

#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#include "engine/QueryExecutionContext.h"
#include "engine/Result.h"
#include "engine/datalog/DatalogResourceGuards.h"
#include "engine/idTable/IdTable.h"
#include "util/AllocatorWithLimit.h"
#include "util/Exception.h"
#include "util/Log.h"

namespace qlever::formalism::unified {

// =============================================================================
// EVALUATION MODE TAGS
// =============================================================================

/// Tag types to select evaluation mode at compile time
struct ConstraintMode {};  // SHACL-style constraint evaluation
struct RuleMode {};        // Datalog-style rule expansion
struct PatternMode {};     // N3-style pattern matching

// =============================================================================
// EVALUATION STRATEGY CONCEPTS
// =============================================================================

/// Concept: A strategy must provide evaluation logic for a single step
template <typename Strategy>
concept EvaluationStrategy = requires(Strategy s, const IdTable& input) {
  // Execute one evaluation step: input -> output
  { s.evaluate(input) } -> std::same_as<IdTable>;

  // Check if result satisfies termination condition
  { s.shouldTerminate(input) } -> std::same_as<bool>;

  // Get result width (number of columns)
  { s.getResultWidth() } -> std::same_as<size_t>;
};

/// Concept: A constraint checker validates IdTable rows against rules
template <typename Checker>
concept ConstraintChecker = requires(Checker c, const IdTable& data) {
  // Evaluate constraint: returns rows that satisfy constraint
  { c.checkConstraint(data) } -> std::same_as<IdTable>;

  // Fast-path: can constraint be trivially satisfied/violated?
  { c.isTrivial() } -> std::same_as<bool>;
};

/// Concept: A SIMD optimizer can vectorize constraint checks
template <typename Optimizer>
concept SimdOptimizer = requires(Optimizer opt, const IdTable& data) {
  // Check if SIMD acceleration is available for this data
  { opt.canVectorize(data) } -> std::same_as<bool>;

  // Vectorized constraint evaluation (batch processing)
  { opt.evaluateVectorized(data) } -> std::same_as<IdTable>;
};

// =============================================================================
// RESOURCE BOUNDS
// =============================================================================

/// Resource bounds for evaluation (time, memory, iterations)
struct EvaluationBounds {
  /// Maximum number of fixpoint iterations
  size_t maxIterations = 1000;

  /// Maximum execution time in milliseconds
  std::chrono::milliseconds maxTime{30'000};

  /// Maximum memory usage in bytes
  size_t maxMemoryBytes = 1'000'000'000;

  /// Maximum number of facts/rows generated
  size_t maxFactCount = 1'000'000;

  /// Epoch ID for cache keying
  ad_utility::EpochId epochId = 0;

  /// Create from DatalogResourceGuards
  static EvaluationBounds fromResourceGuards(
      const datalog::DatalogResourceGuards& guards) {
    EvaluationBounds bounds;
    bounds.maxIterations = 1000;  // Default for unified kernel
    bounds.maxTime = guards.maxRuleTime;
    bounds.maxMemoryBytes = guards.maxMemoryBytes;
    bounds.maxFactCount = guards.maxFactCount;
    bounds.epochId = guards.epochId;
    return bounds;
  }

  /// Validate bounds configuration
  void validate() const {
    AD_CONTRACT_CHECK(maxIterations > 0);
    AD_CONTRACT_CHECK(maxTime.count() > 0);
    AD_CONTRACT_CHECK(maxMemoryBytes > 0);
    AD_CONTRACT_CHECK(maxFactCount > 0);
  }
};

// =============================================================================
// EVALUATION STATISTICS
// =============================================================================

/// Statistics collected during evaluation
struct EvaluationStats {
  size_t iterationsExecuted = 0;
  size_t factsGenerated = 0;
  size_t memoryUsedBytes = 0;
  std::chrono::milliseconds timeElapsed{0};
  bool reachedFixpoint = false;
  bool exceededBounds = false;
  std::string terminationReason;

  /// Log statistics to QLever log
  void log() const {
    LOG(INFO) << "Unified Kernel Stats: iterations=" << iterationsExecuted
              << " facts=" << factsGenerated << " memory=" << memoryUsedBytes
              << " time=" << timeElapsed.count() << "ms"
              << " fixpoint=" << reachedFixpoint
              << " bounded=" << exceededBounds
              << " reason=" << terminationReason;
  }
};

// =============================================================================
// SEMI-NAIVE EVALUATION STATE
// =============================================================================

/// State for semi-naive evaluation (tracks delta between iterations)
template <int NumColumns>
class SemiNaiveState {
 private:
  IdTableStatic<NumColumns> cumulative_;  // All facts derived so far
  IdTableStatic<NumColumns> delta_;       // New facts in last iteration
  ad_utility::AllocatorWithLimit<Id> allocator_;

 public:
  explicit SemiNaiveState(ad_utility::AllocatorWithLimit<Id> allocator)
      : cumulative_(NumColumns, allocator),
        delta_(NumColumns, allocator),
        allocator_(std::move(allocator)) {}

  /// Get cumulative results (all facts)
  const IdTableStatic<NumColumns>& getCumulative() const { return cumulative_; }

  /// Get delta (new facts from last iteration)
  const IdTableStatic<NumColumns>& getDelta() const { return delta_; }

  /// Merge new iteration results into state
  /// Returns: number of new facts added
  size_t merge(IdTableStatic<NumColumns> newFacts) {
    size_t oldSize = cumulative_.size();

    // Sort both tables for efficient merge
    std::ranges::sort(newFacts);
    std::ranges::sort(cumulative_);

    // Compute delta: newFacts \ cumulative (set difference)
    delta_ = IdTableStatic<NumColumns>(NumColumns, allocator_);
    std::ranges::set_difference(newFacts, cumulative_,
                                 std::back_inserter(delta_));

    // Update cumulative: cumulative ∪ delta (set union)
    std::ranges::set_union(cumulative_, delta_,
                           std::back_inserter(cumulative_));

    return cumulative_.size() - oldSize;
  }

  /// Check if fixpoint reached (no new facts)
  bool isFixpoint() const { return delta_.empty(); }

  /// Clear state
  void clear() {
    cumulative_.clear();
    delta_.clear();
  }
};

// Dynamic version for runtime column count
using SemiNaiveStateDynamic = SemiNaiveState<0>;

// =============================================================================
// UNIFIED EVALUATION KERNEL (Main Template)
// =============================================================================

/// Unified evaluation kernel supporting Datalog, SHACL, and N3 formalisms
///
/// Template Parameters:
/// - Mode: ConstraintMode, RuleMode, or PatternMode
/// - NumColumns: Static column count (0 = dynamic)
///
/// Algorithm Overview:
/// ```
/// Input: Initial data, evaluation strategy, resource bounds
/// Output: Final result IdTable
///
/// 1. Initialize semi-naive state with input data
/// 2. While not fixpoint and within bounds:
///    a. Execute evaluation step on delta
///    b. Merge new results into state
///    c. Check resource guards (time, memory, facts)
///    d. Apply SIMD optimizations if available
/// 3. Return cumulative results
/// ```
///
/// Features:
/// - Generalized fixpoint iteration (supports all three modes)
/// - Semi-naive evaluation (only processes new facts)
/// - Resource bounds enforcement (iteration, time, memory, fact count)
/// - SIMD vectorization hooks
/// - Template-based dispatch (zero-cost abstraction)
/// - IdTable integration throughout
///
/// Example Usage (Constraint Mode):
/// ```cpp
/// auto kernel = UnifiedEvaluationKernel<ConstraintMode, 3>(qec, bounds);
/// auto strategy = ShaclConstraintStrategy(...);
/// auto result = kernel.evaluate(initialData, strategy);
/// ```
///
/// Example Usage (Rule Mode):
/// ```cpp
/// auto kernel = UnifiedEvaluationKernel<RuleMode, 0>(qec, bounds);
/// auto strategy = DatalogRuleStrategy(...);
/// auto result = kernel.evaluate(initialData, strategy);
/// ```
template <typename Mode, int NumColumns = 0>
class UnifiedEvaluationKernel {
 private:
  using TableType = IdTableStatic<NumColumns>;
  using StateType = SemiNaiveState<NumColumns>;

  QueryExecutionContext* qec_;
  EvaluationBounds bounds_;
  EvaluationStats stats_;
  ad_utility::AllocatorWithLimit<Id> allocator_;

 public:
  /// Construct kernel with bounds
  explicit UnifiedEvaluationKernel(QueryExecutionContext* qec,
                                   EvaluationBounds bounds)
      : qec_(qec),
        bounds_(std::move(bounds)),
        allocator_(qec->getAllocator()) {
    bounds_.validate();
  }

  /// Main evaluation entry point
  ///
  /// @param initialData Starting facts/constraints
  /// @param strategy Evaluation strategy (constraint/rule/pattern)
  /// @return Final result table
  template <EvaluationStrategy Strategy>
  Result evaluate(TableType initialData, Strategy& strategy) {
    // Start timing
    auto startTime = std::chrono::steady_clock::now();

    // Initialize semi-naive state
    StateType state(allocator_);
    state.merge(std::move(initialData));

    // Initialize resource trackers
    datalog::RuleExecutionTimer timer(bounds_.maxTime);
    datalog::FactCountTracker factTracker(bounds_.maxFactCount);
    datalog::MemoryUsageTracker memoryTracker(bounds_.maxMemoryBytes);

    // Fixpoint iteration loop
    size_t iteration = 0;
    while (iteration < bounds_.maxIterations && !state.isFixpoint()) {
      // Check resource guards
      checkResourceGuards(timer, factTracker, memoryTracker);

      // Execute evaluation step on delta (semi-naive)
      TableType newFacts = executeSingleStep(state.getDelta(), strategy);

      // Merge new facts into state
      size_t addedFacts = state.merge(std::move(newFacts));

      // Update statistics
      stats_.iterationsExecuted = ++iteration;
      stats_.factsGenerated += addedFacts;
      factTracker.addFacts(addedFacts);

      // Log progress
      if (iteration % 10 == 0) {
        LOG(DEBUG) << "Iteration " << iteration << ": added " << addedFacts
                   << " facts, total=" << state.getCumulative().size();
      }
    }

    // Finalize statistics
    auto endTime = std::chrono::steady_clock::now();
    stats_.timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime);
    stats_.reachedFixpoint = state.isFixpoint();
    stats_.terminationReason = determineTerminationReason(state, iteration);

    // Log final statistics
    stats_.log();

    // Convert to Result
    return createResult(state.getCumulative(), strategy.getResultWidth());
  }

  /// Evaluate with constraint checker (constraint mode specialization)
  template <ConstraintChecker Checker>
    requires std::same_as<Mode, ConstraintMode>
  Result evaluateConstraints(TableType initialData, Checker& checker) {
    // Constraint mode: single-pass evaluation (no fixpoint iteration)
    auto startTime = std::chrono::steady_clock::now();

    // Check for trivial cases
    if (checker.isTrivial()) {
      stats_.iterationsExecuted = 0;
      stats_.terminationReason = "Trivial constraint";
      return createResult(std::move(initialData), checker.getResultWidth());
    }

    // Apply constraint check
    TableType result = checker.checkConstraint(initialData);

    // Update statistics
    stats_.iterationsExecuted = 1;
    stats_.factsGenerated = result.size();
    stats_.timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime);
    stats_.terminationReason = "Constraint check complete";

    stats_.log();
    return createResult(std::move(result), checker.getResultWidth());
  }

  /// Get evaluation statistics
  const EvaluationStats& getStats() const { return stats_; }

 private:
  /// Execute single evaluation step
  template <EvaluationStrategy Strategy>
  TableType executeSingleStep(const TableType& delta, Strategy& strategy) {
    // Try SIMD optimization if available
    if constexpr (SimdOptimizer<Strategy>) {
      if (strategy.canVectorize(delta)) {
        return strategy.evaluateVectorized(delta);
      }
    }

    // Fall back to standard evaluation
    return strategy.evaluate(delta);
  }

  /// Check resource guards and throw if exceeded
  void checkResourceGuards(const datalog::RuleExecutionTimer& timer,
                           const datalog::FactCountTracker& factTracker,
                           const datalog::MemoryUsageTracker& memoryTracker) {
    timer.check();  // Throws if time limit exceeded

    // Check fact count
    if (factTracker.count() > bounds_.maxFactCount) {
      stats_.exceededBounds = true;
      throw datalog::ResourceGuardViolation(
          "Fact count limit exceeded: " + std::to_string(factTracker.count()));
    }

    // Check memory usage
    if (memoryTracker.usage() > bounds_.maxMemoryBytes) {
      stats_.exceededBounds = true;
      throw datalog::ResourceGuardViolation(
          "Memory limit exceeded: " + std::to_string(memoryTracker.usage()));
    }
  }

  /// Determine why evaluation terminated
  std::string determineTerminationReason(const StateType& state,
                                         size_t iteration) const {
    if (state.isFixpoint()) {
      return "Fixpoint reached";
    }
    if (iteration >= bounds_.maxIterations) {
      return "Max iterations reached";
    }
    return "Unknown termination";
  }

  /// Convert IdTable to Result
  Result createResult(TableType table, size_t resultWidth) {
    // Convert static table to dynamic if needed
    IdTable dynamicTable = std::move(table);

    // Create result with appropriate metadata
    return {std::move(dynamicTable), {}, LocalVocab{}};
  }
};

// =============================================================================
// CONVENIENCE ALIASES
// =============================================================================

/// Constraint evaluation kernel (SHACL-style)
template <int NumColumns = 0>
using ConstraintEvaluationKernel =
    UnifiedEvaluationKernel<ConstraintMode, NumColumns>;

/// Rule evaluation kernel (Datalog-style)
template <int NumColumns = 0>
using RuleEvaluationKernel = UnifiedEvaluationKernel<RuleMode, NumColumns>;

/// Pattern evaluation kernel (N3-style)
template <int NumColumns = 0>
using PatternEvaluationKernel =
    UnifiedEvaluationKernel<PatternMode, NumColumns>;

// =============================================================================
// SIMD OPTIMIZATION HELPERS
// =============================================================================

/// Vectorized constraint checker (batch evaluation)
class VectorizedConstraintChecker {
 public:
  /// Check if vectorization is beneficial for given table size
  static bool shouldVectorize(size_t numRows) {
    // Vectorization overhead is only worth it for larger tables
    return numRows >= 64;  // Threshold: 64 rows minimum
  }

  /// Vectorized equality check (SIMD-optimized)
  /// Compares all rows in column against target value
  /// Returns: Bitmap of matching rows
  static std::vector<bool> vectorizedEquals(const IdTable& table,
                                            size_t columnIndex, Id targetValue,
                                            size_t numRows) {
    std::vector<bool> matches(numRows, false);

    // TODO: Replace with actual SIMD intrinsics (AVX2/AVX-512)
    // For now, use standard loop (compiler may auto-vectorize)
    for (size_t i = 0; i < numRows; ++i) {
      matches[i] = (table(i, columnIndex) == targetValue);
    }

    return matches;
  }

  /// Vectorized range check (SIMD-optimized)
  /// Returns: Bitmap of rows where minVal <= value <= maxVal
  static std::vector<bool> vectorizedRangeCheck(const IdTable& table,
                                                 size_t columnIndex, Id minVal,
                                                 Id maxVal, size_t numRows) {
    std::vector<bool> matches(numRows, false);

    // TODO: Replace with actual SIMD intrinsics
    for (size_t i = 0; i < numRows; ++i) {
      Id value = table(i, columnIndex);
      matches[i] = (value >= minVal && value <= maxVal);
    }

    return matches;
  }

  /// Batch filter: Remove rows not matching bitmap
  static IdTable filterByBitmap(IdTable table,
                                 const std::vector<bool>& keepMask) {
    AD_CONTRACT_CHECK(table.size() == keepMask.size());

    IdTable result(table.numColumns(), table.getAllocator());
    result.reserve(std::ranges::count(keepMask, true));

    for (size_t i = 0; i < table.size(); ++i) {
      if (keepMask[i]) {
        result.push_back(table[i]);
      }
    }

    return result;
  }
};

// =============================================================================
// FORMALISM-SPECIFIC STRATEGY EXAMPLES
// =============================================================================

/// Example: SHACL minCount constraint strategy
class MinCountConstraintStrategy {
 private:
  size_t minCount_;
  size_t propertyColumn_;

 public:
  MinCountConstraintStrategy(size_t minCount, size_t propertyColumn)
      : minCount_(minCount), propertyColumn_(propertyColumn) {}

  IdTable checkConstraint(const IdTable& data) const {
    // Group by subject, count property values, filter by minCount
    // Simplified implementation (real version would use hash map)
    IdTable result(data.numColumns(), data.getAllocator());

    // TODO: Implement actual grouping and counting logic
    // For now, return data as-is (placeholder)
    return IdTable(data);
  }

  bool isTrivial() const { return minCount_ == 0; }

  size_t getResultWidth() const { return 1; }  // Subject column only
};

/// Example: Datalog transitive closure strategy
class TransitiveClosureStrategy {
 public:
  IdTable evaluate(const IdTable& delta) {
    // Compute one step of transitive closure
    // R^(i+1) = R^i ∪ (R^i ⨝ R^i)
    // Simplified placeholder
    return IdTable(delta);
  }

  bool shouldTerminate(const IdTable& input) { return input.empty(); }

  size_t getResultWidth() const { return 2; }  // (subject, object) pairs
};

}  // namespace qlever::formalism::unified

#endif  // QLEVER_SRC_ENGINE_FORMALISM_UNIFIED_UNIFIEDEVALUATIONKERNEL_H
