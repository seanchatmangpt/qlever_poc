// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#ifndef QLEVER_SRC_ENGINE_REASONING_REASONINGENGINE_H
#define QLEVER_SRC_ENGINE_REASONING_REASONINGENGINE_H

#include <memory>
#include <optional>
#include <set>
#include <unordered_set>
#include <utility>
#include <vector>

#include "engine/QueryExecutionContext.h"
#include "engine/idTable/IdTable.h"
#include "engine/reasoning/Rule.h"
#include "index/Index.h"
#include "parser/ParsedQuery.h"
#include "parser/SparqlTriple.h"
#include "rdfTypes/RdfTypes.h"

namespace reasoning {

/// Statistics about a reasoning iteration.
struct IterationStats {
  size_t newFactsGenerated = 0;
  size_t totalFactsInDatabase = 0;
  uint64_t iterationTimeMs = 0;
};

/// Core reasoning engine implementing Semi-Naïve evaluation with fixpoint
/// iteration. This enables Datalog and N3 reasoning.
class ReasoningEngine {
 public:
  using FactSet = std::set<std::array<Id, 3>>;

  explicit ReasoningEngine(QueryExecutionContext* qec,
                          std::shared_ptr<RuleDatabase> ruleDatabase)
      : qec_(qec), ruleDatabase_(std::move(ruleDatabase)) {}

  /// Execute reasoning with Semi-Naïve evaluation.
  /// Iteratively applies rules until fixpoint (no new facts generated).
  /// Returns the set of all derived facts (triples).
  /// maxIterations: maximum number of iterations (0 = unlimited)
  [[nodiscard]] FactSet executeReasoning(size_t maxIterations = 0);

  /// Execute reasoning and return results as IdTable with the given variables.
  /// variables: the variables to bind in the result
  /// Returns an IdTable where each column corresponds to a variable in
  /// variables.
  [[nodiscard]] IdTable executeReasoningAsIdTable(
      const std::vector<Variable>& variables);

  /// Get statistics from the last reasoning execution.
  [[nodiscard]] const std::vector<IterationStats>& getIterationStats() const {
    return iterationStats_;
  }

  /// Clear accumulated facts (used for incremental reasoning).
  void clearDerivedFacts() {
    derivedFacts_.clear();
    deltaFacts_.clear();
    iterationStats_.clear();
  }

  /// Get all derived facts.
  [[nodiscard]] const FactSet& getDerivedFacts() const {
    return derivedFacts_;
  }

  /// Check if a fact (triple) has been derived.
  [[nodiscard]] bool isFact(Id subject, Id predicate, Id object) const {
    return derivedFacts_.count({subject, predicate, object}) > 0;
  }

 private:
  QueryExecutionContext* qec_;
  std::shared_ptr<RuleDatabase> ruleDatabase_;

  // Accumulated facts from all iterations
  FactSet derivedFacts_;

  // Delta facts from the current iteration (for Semi-Naïve optimization)
  FactSet deltaFacts_;

  // Statistics for each iteration
  std::vector<IterationStats> iterationStats_;

  /// Apply a single rule to the current delta facts.
  /// Returns newly derived facts.
  [[nodiscard]] FactSet applyRule(const Rule& rule,
                                  const FactSet& currentFacts);

  /// Convert a SPARQL triple pattern with substitutions to a concrete triple.
  /// Returns nullopt if unification fails.
  [[nodiscard]] std::optional<std::array<Id, 3>> instantiateTriple(
      const SparqlTriple& pattern, const std::vector<Id>& substitution,
      const std::vector<Variable>& variables) const;

  /// Unify a result row with a set of variables.
  /// Maps variables to their bound values (Ids).
  [[nodiscard]] std::vector<Id> unifyVariables(
      const IdTable& resultTable, size_t rowIndex,
      const std::vector<Variable>& variables) const;

  /// Execute a single query and return results as fact set.
  [[nodiscard]] FactSet queryToFacts(const std::vector<SparqlTriple>& pattern);

  /// Check if new facts were generated in this iteration.
  [[nodiscard]] bool hasConverged(const FactSet& newFacts) const {
    return newFacts.empty();
  }
};

}  // namespace reasoning

#endif  // QLEVER_SRC_ENGINE_REASONING_REASONINGENGINE_H
