//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team - Agent 3)

#ifndef QLEVER_SRC_ENGINE_RULEEXPANSION_H
#define QLEVER_SRC_ENGINE_RULEEXPANSION_H

#include <memory>
#include <string>
#include <vector>

#include "engine/Operation.h"
#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "parser/DatalogRule.h"
#include "parser/RuleDatabase.h"
#include "rdfTypes/Variable.h"

/// Operation for expanding and executing Datalog rules.
///
/// This operation takes a Datalog rule and expands it by mapping the head
/// variables to the arguments provided in the query, then creates and executes
/// a QueryExecutionTree for the rule body.
///
/// Example:
///   Rule: ancestor(?X, ?Y) :- parent(?X, ?Z), ancestor(?Z, ?Y).
///   Query: ancestor(?a, ?b)
///   Expansion:
///     Maps: ?X → ?a, ?Y → ?b, ?Z → ?temp_1
///     Expanded body: parent(?a, ?temp_1), ancestor(?temp_1, ?b)
///
/// The operation reuses existing QLever operations (IndexScan, Join, etc.)
/// to execute the expanded rule body efficiently.
class RuleExpansion : public Operation {
 public:
  /// Construct a RuleExpansion operation
  /// @param qec Query execution context
  /// @param ruleDatabase Database containing all Datalog rules
  /// @param rulePredicate The predicate name of the rule to expand
  /// @param arguments Variables/constants bound to the rule head arguments
  RuleExpansion(QueryExecutionContext* qec,
                std::shared_ptr<RuleDatabase> ruleDatabase,
                std::string rulePredicate, std::vector<TripleComponent> arguments);

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

  /// Get children (the expanded execution tree)
  std::vector<QueryExecutionTree*> getChildren() override;

 private:
  /// Compute the result by expanding rules and executing the body
  Result computeResult(bool requestLaziness) override;

  /// Get the cache key for this operation
  std::string getCacheKeyImpl() const override;

  /// Clone this operation
  std::unique_ptr<Operation> cloneImpl() const override;

  /// Compute the variable to column map
  VariableToColumnMap computeVariableToColumnMap() const override;

  /// Expand a Datalog rule into SPARQL triple patterns
  /// Maps head variables to argument variables and creates body patterns
  /// @param rule The rule to expand
  /// @return Vector of expanded SPARQL triples with mapped variables
  std::vector<SparqlTriple> expandRuleToPatterns(const DatalogRule& rule);

  /// Create a mapping from head variables to query arguments
  /// @param rule The rule being expanded
  /// @return Map from head variable name to the actual argument (var/constant)
  std::map<Variable, TripleComponent> createVariableMapping(
      const DatalogRule& rule);

  /// Build a QueryExecutionTree from the expanded rule body patterns
  /// @param expandedPatterns The SPARQL patterns with mapped variables
  /// @param rule The original rule (for filters/constraints)
  /// @return Execution tree that computes the rule body
  std::shared_ptr<QueryExecutionTree> buildExecutionTree(
      const std::vector<SparqlTriple>& expandedPatterns,
      const DatalogRule& rule);

  /// Generate a unique temporary variable name
  /// @param baseName Base name for the variable (e.g., "temp")
  /// @param index Unique index for this variable
  /// @return A unique variable name like "?temp_1"
  Variable generateTempVariable(const std::string& baseName, size_t index);

  /// Apply variable mapping to a TripleComponent
  /// @param component The component to map (may be variable or constant)
  /// @param mapping The variable mapping to apply
  /// @return Mapped component (variable substituted or constant unchanged)
  TripleComponent applyMapping(
      const TripleComponent& component,
      const std::map<Variable, TripleComponent>& mapping);

 private:
  /// Database containing all Datalog rules
  std::shared_ptr<RuleDatabase> ruleDatabase_;

  /// Name of the rule predicate to expand
  std::string rulePredicate_;

  /// Arguments (variables/constants) bound to the rule head
  std::vector<TripleComponent> arguments_;

  /// The expanded execution tree (created lazily on first use)
  std::shared_ptr<QueryExecutionTree> expandedTree_;

  /// Counter for generating unique temporary variables
  size_t tempVarCounter_ = 0;

  /// Cache for size estimate
  bool sizeEstimateComputed_ = false;
  uint64_t sizeEstimate_ = 0;

  /// Cache for multiplicities
  std::vector<float> multiplicities_;
};

#endif  // QLEVER_SRC_ENGINE_RULEEXPANSION_H
