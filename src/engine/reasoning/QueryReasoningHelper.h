// Copyright 2025 - University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#ifndef QLEVER_SRC_ENGINE_REASONING_QUERYREASONINGHELPER_H
#define QLEVER_SRC_ENGINE_REASONING_QUERYREASONINGHELPER_H

#include <memory>
#include <optional>
#include <vector>

#include "engine/QueryExecutionContext.h"
#include "engine/QueryExecutionTree.h"
#include "engine/reasoning/N3Parser.h"
#include "engine/reasoning/Rule.h"
#include "parser/ParsedQuery.h"

namespace reasoning {

/// Helper class to integrate reasoning into QueryPlanner.
/// Detects N3 rule syntax and creates appropriate ReasoningOperation.
class QueryReasoningHelper {
 public:
  explicit QueryReasoningHelper(QueryExecutionContext* qec) : qec_(qec) {}

  /// Check if a query contains N3 rule definitions.
  [[nodiscard]] static bool isReasoningQuery(
      const ParsedQuery& parsedQuery);

  /// Check if a SPARQL string contains N3 rule syntax.
  [[nodiscard]] static bool containsN3Rules(const std::string& sparqlQuery);

  /// Parse rules from a query and create a RuleDatabase.
  [[nodiscard]] std::shared_ptr<RuleDatabase> extractRulesFromQuery(
      const ParsedQuery& parsedQuery);

  /// Parse rules from raw SPARQL string.
  [[nodiscard]] std::shared_ptr<RuleDatabase> extractRulesFromSparql(
      const std::string& sparqlQuery);

  /// Create a ReasoningOperation from parsed rules.
  /// Returns a QueryExecutionTree containing the ReasoningOperation.
  [[nodiscard]] std::optional<std::shared_ptr<QueryExecutionTree>>
  createReasoningExecutionTree(std::shared_ptr<RuleDatabase> ruleDatabase,
                               const std::vector<Variable>& outputVariables);

  /// Estimate cost of reasoning given a set of rules.
  /// Used by QueryPlanner for cost-based optimization.
  [[nodiscard]] static size_t estimateReasoningCost(
      const RuleDatabase& ruleDatabase);

  /// Check if a rule is optimizable (e.g., can be converted to property path).
  [[nodiscard]] static bool isOptimizableRule(const Rule& rule);

  /// Convert a simple transitive rule to a property path pattern.
  /// Returns nullopt if rule is not optimizable.
  [[nodiscard]] static std::optional<std::string> convertToPropertyPath(
      const Rule& rule);

 private:
  QueryExecutionContext* qec_;
  N3Parser parser_;

  /// Extract rules from the graph pattern of a query.
  [[nodiscard]] std::vector<std::shared_ptr<Rule>>
  extractRulesFromGraphPattern(const parsedQuery::GraphPattern& pattern);
};

}  // namespace reasoning

#endif  // QLEVER_SRC_ENGINE_REASONING_QUERYREASONINGHELPER_H
