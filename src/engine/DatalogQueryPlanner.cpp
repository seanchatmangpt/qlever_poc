//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team - Agent 4)

#include "engine/DatalogQueryPlanner.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "engine/IndexScan.h"
#include "engine/Join.h"
#include "engine/RuleExpansion.h"
#include "engine/Union.h"
#include "parser/GraphPatternOperation.h"
#include "parser/PropertyPath.h"
#include "parser/TripleComponent.h"
#include "parser/data/Types.h"
#include "util/Exception.h"
#include "util/Log.h"

// _____________________________________________________________________________
DatalogQueryPlanner::DatalogQueryPlanner(
    std::shared_ptr<RuleDatabase> ruleDatabase, QueryPlanner* queryPlanner)
    : ruleDatabase_(std::move(ruleDatabase)), queryPlanner_(queryPlanner) {
  AD_CONTRACT_CHECK(ruleDatabase_ != nullptr);
  AD_CONTRACT_CHECK(queryPlanner_ != nullptr);
}

// _____________________________________________________________________________
std::shared_ptr<QueryExecutionTree> DatalogQueryPlanner::planDatalogQuery(
    ParsedQuery& parsedQuery) {
  LOG(DEBUG) << "Planning Datalog query..." << std::endl;

  auto* qec = queryPlanner_->getQec();
  AD_CONTRACT_CHECK(qec != nullptr);

  // First, check if there are any rule predicates in the query
  auto rulePredicates = detectRulePredicates(parsedQuery._rootGraphPattern);

  if (rulePredicates.empty()) {
    // No rule predicates found, use standard query planner
    LOG(DEBUG) << "No rule predicates detected, using standard planner"
               << std::endl;
    return std::make_shared<QueryExecutionTree>(
        qec, queryPlanner_->createExecutionTree(parsedQuery));
  }

  LOG(DEBUG) << "Detected " << rulePredicates.size() << " rule predicates"
             << std::endl;

  // Extract all triple patterns from the graph pattern
  auto triples = extractTriplePatterns(parsedQuery._rootGraphPattern);

  if (triples.empty()) {
    throw std::runtime_error(
        "DatalogQueryPlanner: No triple patterns found in query");
  }

  // Build operations for each triple pattern
  std::vector<std::shared_ptr<QueryExecutionTree>> operations;
  operations.reserve(triples.size());

  for (const auto& triple : triples) {
    auto operation = buildOperationForTriple(triple, qec);
    if (operation) {
      operations.push_back(std::move(operation));
    }
  }

  if (operations.empty()) {
    throw std::runtime_error(
        "DatalogQueryPlanner: No valid operations could be created");
  }

  // If only one operation, return it directly
  if (operations.size() == 1) {
    return operations[0];
  }

  // Build and optimize the operation tree with joins
  return buildOperationTree(std::move(operations), qec);
}

// _____________________________________________________________________________
bool DatalogQueryPlanner::isRulePredicate(
    const std::string& predicateName) const {
  return ruleDatabase_->hasRuleFor(predicateName);
}

// _____________________________________________________________________________
ad_utility::HashSet<std::string> DatalogQueryPlanner::detectRulePredicates(
    const parsedQuery::GraphPattern& pattern) const {
  ad_utility::HashSet<std::string> rulePredicates;

  // Extract triples and check their predicates
  auto triples = extractTriplePatterns(pattern);
  for (const auto& triple : triples) {
    auto predicateName = extractPredicateName(triple.p_);
    if (!predicateName.empty() && isRulePredicate(predicateName)) {
      rulePredicates.insert(predicateName);
    }
  }

  return rulePredicates;
}

// _____________________________________________________________________________
std::vector<SparqlTriple> DatalogQueryPlanner::extractTriplePatterns(
    const parsedQuery::GraphPattern& pattern) const {
  std::vector<SparqlTriple> triples;

  // Recursively extract triples from nested graph patterns
  for (const auto& graphPatternOp : pattern._graphPatterns) {
    std::visit(
        [&triples, this](const auto& op) {
          using T = std::decay_t<decltype(op)>;

          // Handle BasicGraphPattern
          if constexpr (std::is_same_v<T, parsedQuery::BasicGraphPattern>) {
            for (const auto& triple : op._triples) {
              triples.push_back(triple);
            }
          }
          // Handle nested GroupGraphPattern
          else if constexpr (std::is_same_v<T,
                                            parsedQuery::GroupGraphPattern>) {
            auto nested = extractTriplePatterns(op._child);
            triples.insert(triples.end(), nested.begin(), nested.end());
          }
          // Handle Optional patterns
          else if constexpr (std::is_same_v<T, parsedQuery::Optional>) {
            auto nested = extractTriplePatterns(op._child);
            triples.insert(triples.end(), nested.begin(), nested.end());
          }
          // Handle Union patterns
          else if constexpr (std::is_same_v<T, parsedQuery::Union>) {
            for (const auto& child : op._children) {
              auto nested = extractTriplePatterns(child);
              triples.insert(triples.end(), nested.begin(), nested.end());
            }
          }
          // For other pattern types, we skip them in this basic implementation
        },
        graphPatternOp);
  }

  return triples;
}

// _____________________________________________________________________________
std::shared_ptr<QueryExecutionTree> DatalogQueryPlanner::buildOperationForTriple(
    const SparqlTriple& triple, QueryExecutionContext* qec) {
  // Check if this triple has a rule predicate
  if (hasRulePredicate(triple)) {
    return createRuleExpansionOperation(triple, qec);
  } else {
    // Use standard index scan for non-rule predicates
    return createIndexScanOperation(triple, qec);
  }
}

// _____________________________________________________________________________
std::shared_ptr<QueryExecutionTree> DatalogQueryPlanner::buildOperationTree(
    std::vector<std::shared_ptr<QueryExecutionTree>> operations,
    QueryExecutionContext* qec) {
  AD_CONTRACT_CHECK(!operations.empty());

  // If only one operation, return it
  if (operations.size() == 1) {
    return operations[0];
  }

  // Use optimized join ordering
  return optimizeJoinOrder(std::move(operations), qec);
}

// _____________________________________________________________________________
std::vector<std::array<ColumnIndex, 2>> DatalogQueryPlanner::findJoinColumns(
    const QueryExecutionTree& left, const QueryExecutionTree& right) const {
  std::vector<std::array<ColumnIndex, 2>> joinColumns;

  // Get variable to column mappings for both trees
  const auto& leftVars = left.getVariableColumns();
  const auto& rightVars = right.getVariableColumns();

  // Find common variables
  for (const auto& [leftVar, leftCol] : leftVars) {
    auto rightIt = rightVars.find(leftVar);
    if (rightIt != rightVars.end()) {
      // Found a common variable
      joinColumns.push_back({leftCol.columnIndex_, rightIt->second.columnIndex_});
    }
  }

  return joinColumns;
}

// _____________________________________________________________________________
std::shared_ptr<QueryExecutionTree> DatalogQueryPlanner::optimizeJoinOrder(
    std::vector<std::shared_ptr<QueryExecutionTree>> operations,
    QueryExecutionContext* qec) {
  AD_CONTRACT_CHECK(operations.size() >= 2);

  // Greedy join ordering: always join the two operations with lowest combined cost
  while (operations.size() > 1) {
    size_t bestLeft = 0;
    size_t bestRight = 1;
    size_t bestCost = std::numeric_limits<size_t>::max();

    // Find the best pair to join
    for (size_t i = 0; i < operations.size(); ++i) {
      for (size_t j = i + 1; j < operations.size(); ++j) {
        auto joinCols = findJoinColumns(*operations[i], *operations[j]);

        // Only consider pairs that can actually be joined
        if (!joinCols.empty()) {
          // Estimate cost as product of sizes
          size_t cost = operations[i]->getSizeEstimate() *
                        operations[j]->getSizeEstimate();

          if (cost < bestCost) {
            bestCost = cost;
            bestLeft = i;
            bestRight = j;
          }
        }
      }
    }

    // If no joinable pair found, try cartesian product of first two
    if (bestCost == std::numeric_limits<size_t>::max()) {
      LOG(WARNING) << "No common variables found, using cartesian product"
                   << std::endl;
      bestLeft = 0;
      bestRight = 1;
    }

    // Create the join operation
    auto joinCols = findJoinColumns(*operations[bestLeft], *operations[bestRight]);

    std::shared_ptr<QueryExecutionTree> joined;
    if (!joinCols.empty()) {
      // Normal join on common variables
      auto joinOp = std::make_shared<Join>(
          qec, operations[bestLeft], operations[bestRight], joinCols[0][0],
          joinCols[0][1]);
      joined = std::make_shared<QueryExecutionTree>(qec, std::move(joinOp));
    } else {
      // Cartesian product (no common variables)
      // For now, just create a regular join that will produce a cartesian product
      auto joinOp = std::make_shared<Join>(qec, operations[bestLeft],
                                           operations[bestRight], 0, 0, false);
      joined = std::make_shared<QueryExecutionTree>(qec, std::move(joinOp));
    }

    // Remove the two joined operations and add the result
    // Remove the larger index first to avoid invalidating indices
    size_t removeFirst = std::max(bestLeft, bestRight);
    size_t removeSecond = std::min(bestLeft, bestRight);

    operations.erase(operations.begin() + removeFirst);
    operations.erase(operations.begin() + removeSecond);
    operations.push_back(joined);
  }

  return operations[0];
}

// _____________________________________________________________________________
std::shared_ptr<QueryExecutionTree>
DatalogQueryPlanner::createRuleExpansionOperation(const SparqlTriple& triple,
                                                  QueryExecutionContext* qec) {
  auto predicateName = extractPredicateName(triple.p_);
  auto arguments = extractArguments(triple);

  // Check if multiple rules exist for this predicate
  auto rules = ruleDatabase_->getRulesByPredicate(predicateName);

  if (rules.empty()) {
    throw std::runtime_error("DatalogQueryPlanner: No rules found for predicate " +
                           predicateName);
  }

  if (rules.size() == 1) {
    // Single rule: create RuleExpansion directly
    auto ruleExpOp = std::make_shared<RuleExpansion>(
        qec, ruleDatabase_, predicateName, arguments);
    return std::make_shared<QueryExecutionTree>(qec, std::move(ruleExpOp));
  } else {
    // Multiple rules: create UNION of RuleExpansion operations
    return createUnionOfRules(predicateName, arguments, qec);
  }
}

// _____________________________________________________________________________
std::shared_ptr<QueryExecutionTree>
DatalogQueryPlanner::createIndexScanOperation(const SparqlTriple& triple,
                                              QueryExecutionContext* qec) {
  // Convert SparqlTriple to SparqlTripleSimple
  auto simpleTriple = triple.getSimple();

  // Determine the best permutation for this triple pattern
  // For now, use PSO as default
  auto indexScanOp = std::make_shared<IndexScan>(
      qec, Permutation::Enum::PSO, simpleTriple);

  return std::make_shared<QueryExecutionTree>(qec, std::move(indexScanOp));
}

// _____________________________________________________________________________
std::shared_ptr<QueryExecutionTree> DatalogQueryPlanner::createUnionOfRules(
    const std::string& predicateName,
    const std::vector<TripleComponent>& arguments, QueryExecutionContext* qec) {
  auto rules = ruleDatabase_->getRulesByPredicate(predicateName);

  if (rules.empty()) {
    throw std::runtime_error("DatalogQueryPlanner: No rules found for predicate " +
                           predicateName);
  }

  // Create a RuleExpansion operation for each rule
  std::vector<std::shared_ptr<QueryExecutionTree>> ruleExpansions;
  ruleExpansions.reserve(rules.size());

  for (const auto& rule : rules) {
    auto ruleExpOp = std::make_shared<RuleExpansion>(qec, ruleDatabase_,
                                                     predicateName, arguments);
    ruleExpansions.push_back(
        std::make_shared<QueryExecutionTree>(qec, std::move(ruleExpOp)));
  }

  // If only one rule, return it directly
  if (ruleExpansions.size() == 1) {
    return ruleExpansions[0];
  }

  // Create UNION tree for multiple rules
  // Union only takes 2 children, so we build a balanced binary tree of unions
  while (ruleExpansions.size() > 1) {
    std::vector<std::shared_ptr<QueryExecutionTree>> nextLevel;

    for (size_t i = 0; i + 1 < ruleExpansions.size(); i += 2) {
      auto unionOp = std::make_shared<Union>(qec, ruleExpansions[i],
                                             ruleExpansions[i + 1]);
      nextLevel.push_back(
          std::make_shared<QueryExecutionTree>(qec, std::move(unionOp)));
    }

    // If odd number of elements, carry the last one forward
    if (ruleExpansions.size() % 2 == 1) {
      nextLevel.push_back(ruleExpansions.back());
    }

    ruleExpansions = std::move(nextLevel);
  }

  return ruleExpansions[0];
}

// _____________________________________________________________________________
std::string DatalogQueryPlanner::extractPredicateName(
    const TripleComponent& component) const {
  if (component.isIri()) {
    return component.getIri().toStringRepresentation();
  }
  // Return empty string for variables or other types
  return "";
}

// Overload for VarOrPath (used in SparqlTriple.p_)
std::string DatalogQueryPlanner::extractPredicateName(
    const ad_utility::sparql_types::VarOrPath& predicate) const {
  // Check if it's a PropertyPath with an IRI
  if (std::holds_alternative<PropertyPath>(predicate)) {
    const auto& path = std::get<PropertyPath>(predicate);
    auto iri = path.toIri();
    if (iri.has_value()) {
      return iri.value().toStringRepresentation();
    }
  }
  // If it's a Variable, return empty string
  return "";
}

// _____________________________________________________________________________
std::vector<TripleComponent> DatalogQueryPlanner::extractArguments(
    const SparqlTriple& triple) const {
  // For Datalog rules, arguments are typically subject and object
  return {triple.s_, triple.o_};
}

// _____________________________________________________________________________
bool DatalogQueryPlanner::hasRulePredicate(const SparqlTriple& triple) const {
  auto predicateName = extractPredicateName(triple.p_);
  return !predicateName.empty() && isRulePredicate(predicateName);
}
