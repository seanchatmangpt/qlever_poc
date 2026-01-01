//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team - Agent 6)

#ifndef QLEVER_SRC_ENGINE_FIXPOINTCOMPUTATION_H
#define QLEVER_SRC_ENGINE_FIXPOINTCOMPUTATION_H

#include <memory>
#include <string>
#include <vector>

#include "engine/Operation.h"
#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "engine/RuleExpansion.h"
#include "parser/DatalogRule.h"
#include "parser/RuleDatabase.h"

/// Operation for evaluating recursive Datalog rules via fixpoint iteration.
///
/// This operation implements the semi-naive evaluation algorithm for recursive
/// Datalog rules. It iteratively expands rules until a fixpoint is reached
/// (no new facts are derived).
///
/// Algorithm Overview:
/// ```
/// Iteration 0: results₀ = RuleExpansion.execute()
/// Iteration 1: results₁ = RuleExpansion.execute() with results₀ as base facts
/// Iteration i: resultsᵢ = RuleExpansion.execute() with ⋃(results₀..resultsᵢ₋₁)
/// ...
/// Until: resultsᵢ ⊆ ⋃(results₀..resultsᵢ₋₁) (fixpoint reached)
/// Return: ⋃(results₀..resultsᵢ)
/// ```
///
/// Example - Transitive Closure:
/// ```
/// Rule: ancestor(?X, ?Y) :- parent(?X, ?Y).
/// Rule: ancestor(?X, ?Z) :- parent(?X, ?Y), ancestor(?Y, ?Z).
///
/// Base facts: parent(a, b), parent(b, c)
///
/// Iteration 0: ancestor(a, b), ancestor(b, c)  [from base rule]
/// Iteration 1: ancestor(a, c)                   [using iteration 0]
/// Iteration 2: (no new facts)                   [fixpoint reached]
/// Result: {ancestor(a, b), ancestor(b, c), ancestor(a, c)}
/// ```
///
/// Features:
/// - Iteration limit to prevent infinite loops
/// - Early termination when no new results
/// - Memory constraints enforcement
/// - Automatic deduplication via union with previous results
/// - Detailed logging for debugging
class FixpointComputation : public Operation {
 public:
  /// Construct a FixpointComputation operation
  /// @param qec Query execution context
  /// @param ruleDatabase Database containing all Datalog rules
  /// @param rulePredicate The predicate name of the recursive rule
  /// @param arguments Variables/constants bound to the rule head arguments
  /// @param maxIterations Maximum number of iterations (default: 1000)
  FixpointComputation(QueryExecutionContext* qec,
                      std::shared_ptr<RuleDatabase> ruleDatabase,
                      std::string rulePredicate,
                      std::vector<TripleComponent> arguments,
                      size_t maxIterations = 1000);

  /// Get descriptor for this operation
  std::string getDescriptor() const override;

  /// Get the width (number of columns) of the result
  size_t getResultWidth() const override;

  /// Get the estimated cost of executing this operation
  size_t getCostEstimate() override;

  /// Get the estimated size (number of rows) of the result
  uint64_t getSizeEstimateBeforeLimit() override;

  /// Get the multiplicity estimate for a column
  float getMultiplicity(size_t col) override;

  /// Check if the result is known to be empty
  bool knownEmptyResult() override;

  /// Get the columns by which the result is sorted
  std::vector<ColumnIndex> resultSortedOn() const override;

  /// Get children (the RuleExpansion operation)
  std::vector<QueryExecutionTree*> getChildren() override;

 private:
  /// Compute the result by iterating until fixpoint
  Result computeResult(bool requestLaziness) override;

  /// Get the cache key for this operation
  std::string getCacheKeyImpl() const override;

  /// Clone this operation
  std::unique_ptr<Operation> cloneImpl() const override;

  /// Compute the variable to column map
  VariableToColumnMap computeVariableToColumnMap() const override;

  /// Run the fixpoint iteration algorithm
  /// @return The final IdTable containing all derived facts
  IdTable runIterations();

  /// Merge two IdTables, removing duplicates
  /// @param table1 First table (will be modified to contain the union)
  /// @param table2 Second table to merge in
  /// @return Number of new rows added
  size_t mergeAndDeduplicate(IdTable& table1, const IdTable& table2);

  /// Check if a new iteration produced new facts
  /// @param currentSize Size before merging new iteration
  /// @param newSize Size after merging new iteration
  /// @return true if new facts were added, false if fixpoint reached
  bool hasNewFacts(size_t currentSize, size_t newSize) const;

  /// Check memory usage and abort if exceeds limit
  /// @param currentSize Current cumulative result size
  void checkMemoryLimit(size_t currentSize);

  /// Log statistics for an iteration
  /// @param iteration Iteration number
  /// @param newRows Number of new rows added
  /// @param totalRows Total cumulative rows
  void logIterationStats(size_t iteration, size_t newRows,
                         size_t totalRows) const;

 private:
  /// Database containing all Datalog rules
  std::shared_ptr<RuleDatabase> ruleDatabase_;

  /// Name of the rule predicate to expand
  std::string rulePredicate_;

  /// Arguments (variables/constants) bound to the rule head
  std::vector<TripleComponent> arguments_;

  /// Maximum number of iterations before aborting
  size_t maxIterations_;

  /// The RuleExpansion operation (created lazily)
  std::shared_ptr<QueryExecutionTree> ruleExpansionTree_;

  /// Cache for size estimate
  bool sizeEstimateComputed_ = false;
  uint64_t sizeEstimate_ = 0;

  /// Cache for multiplicities
  std::vector<float> multiplicities_;
};

#endif  // QLEVER_SRC_ENGINE_FIXPOINTCOMPUTATION_H
