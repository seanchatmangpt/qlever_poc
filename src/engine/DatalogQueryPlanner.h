//  Copyright 2026, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code Agent (Datalog Implementation Team - Agent 4)

#ifndef QLEVER_SRC_ENGINE_DATALOGQUERYPLANNER_H
#define QLEVER_SRC_ENGINE_DATALOGQUERYPLANNER_H

#include <memory>
#include <string>
#include <vector>

#include "engine/QueryExecutionTree.h"
#include "engine/QueryPlanner.h"
#include "parser/GraphPattern.h"
#include "parser/ParsedQuery.h"
#include "parser/PropertyPath.h"
#include "parser/RuleDatabase.h"
#include "parser/SparqlTriple.h"
#include "parser/data/Types.h"
#include "rdfTypes/Variable.h"

/// Query planner for Datalog queries that handles rule detection and expansion.
///
/// This planner extends the standard SPARQL query planning to support Datalog
/// rules. It detects which predicates in a query are defined by rules versus
/// stored directly in the index, and creates appropriate execution plans.
///
/// Planning Algorithm:
/// 1. Walk the query's GraphPattern tree to identify all triple patterns
/// 2. For each triple pattern, check if the predicate has Datalog rules
/// 3. For rule-defined predicates: Create RuleExpansion operations
/// 4. For index-defined predicates: Create standard IndexScan operations
/// 5. Combine multiple patterns with Join operations
/// 6. Use cost estimation to optimize join ordering
/// 7. Handle UNION for multiple rules with same predicate
///
/// Example:
///   Query: ancestor(?x, ?y), friend(?x, ?z)
///   Rules: ancestor/2 has Datalog rules
///   Plan: RuleExpansion(ancestor) JOIN IndexScan(friend)
class DatalogQueryPlanner {
 public:
  /// Construct a DatalogQueryPlanner
  /// @param ruleDatabase Database containing all Datalog rules
  /// @param queryPlanner The standard query planner for cost estimation
  DatalogQueryPlanner(std::shared_ptr<RuleDatabase> ruleDatabase,
                      QueryPlanner* queryPlanner);

  /// Plan a Datalog query by detecting and expanding rules
  /// @param parsedQuery The parsed SPARQL query with potential Datalog predicates
  /// @return Query execution tree that handles both rules and index scans
  /// @throws std::runtime_error if planning fails
  std::shared_ptr<QueryExecutionTree> planDatalogQuery(
      ParsedQuery& parsedQuery);

  /// Check if a predicate is defined by Datalog rules
  /// @param predicateName The predicate name (IRI) to check
  /// @return true if rules exist for this predicate, false otherwise
  bool isRulePredicate(const std::string& predicateName) const;

 private:
  /// Detect all rule-defined predicates in a graph pattern
  /// @param pattern The graph pattern to analyze
  /// @return Set of predicate names that have rules defined
  ad_utility::HashSet<std::string> detectRulePredicates(
      const parsedQuery::GraphPattern& pattern) const;

  /// Extract all triple patterns from a graph pattern
  /// @param pattern The graph pattern to analyze
  /// @return Vector of all SPARQL triple patterns in the pattern
  std::vector<SparqlTriple> extractTriplePatterns(
      const parsedQuery::GraphPattern& pattern) const;

  /// Build an execution tree for a single triple pattern
  /// @param triple The triple pattern
  /// @param qec Query execution context
  /// @return Execution tree for this pattern (either RuleExpansion or IndexScan)
  std::shared_ptr<QueryExecutionTree> buildOperationForTriple(
      const SparqlTriple& triple, QueryExecutionContext* qec);

  /// Build an execution tree from multiple operations
  /// Combines operations with Join, handling variable connections
  /// @param operations Vector of operation trees to combine
  /// @param qec Query execution context
  /// @return Combined execution tree with optimal join ordering
  std::shared_ptr<QueryExecutionTree> buildOperationTree(
      std::vector<std::shared_ptr<QueryExecutionTree>> operations,
      QueryExecutionContext* qec);

  /// Find join columns between two execution trees
  /// @param left Left execution tree
  /// @param right Right execution tree
  /// @return Pairs of column indices that can be joined
  std::vector<std::array<ColumnIndex, 2>> findJoinColumns(
      const QueryExecutionTree& left, const QueryExecutionTree& right) const;

  /// Optimize join order using cost estimation
  /// Uses greedy algorithm: always join the two operations with lowest cost
  /// @param operations Vector of operation trees to join
  /// @param qec Query execution context
  /// @return Optimally joined execution tree
  std::shared_ptr<QueryExecutionTree> optimizeJoinOrder(
      std::vector<std::shared_ptr<QueryExecutionTree>> operations,
      QueryExecutionContext* qec);

  /// Create a RuleExpansion operation for a rule-defined predicate
  /// @param triple The triple pattern with rule predicate
  /// @param qec Query execution context
  /// @return Execution tree for the rule expansion
  std::shared_ptr<QueryExecutionTree> createRuleExpansionOperation(
      const SparqlTriple& triple, QueryExecutionContext* qec);

  /// Create an IndexScan operation for an index-defined predicate
  /// @param triple The triple pattern with index predicate
  /// @param qec Query execution context
  /// @return Execution tree for the index scan
  std::shared_ptr<QueryExecutionTree> createIndexScanOperation(
      const SparqlTriple& triple, QueryExecutionContext* qec);

  /// Create a UNION of multiple RuleExpansion operations
  /// Used when a predicate has multiple rules defined
  /// @param predicateName The predicate with multiple rules
  /// @param arguments The arguments from the query
  /// @param qec Query execution context
  /// @return Execution tree that unions all rule expansions
  std::shared_ptr<QueryExecutionTree> createUnionOfRules(
      const std::string& predicateName,
      const std::vector<TripleComponent>& arguments, QueryExecutionContext* qec);

  /// Extract predicate name from a triple component
  /// @param component The predicate component (may be IRI or variable)
  /// @return Predicate name if it's an IRI, empty string if variable
  std::string extractPredicateName(const TripleComponent& component) const;

  /// Extract predicate name from a VarOrPath (used in SparqlTriple)
  /// @param predicate The predicate (may be Variable or PropertyPath)
  /// @return Predicate name if it's an IRI, empty string otherwise
  std::string extractPredicateName(
      const ad_utility::sparql_types::VarOrPath& predicate) const;

  /// Extract arguments (subject and object) from a triple
  /// @param triple The triple pattern
  /// @return Vector containing [subject, object] as TripleComponents
  std::vector<TripleComponent> extractArguments(
      const SparqlTriple& triple) const;

  /// Check if a triple pattern has a rule-defined predicate
  /// @param triple The triple to check
  /// @return true if predicate is defined by rules, false otherwise
  bool hasRulePredicate(const SparqlTriple& triple) const;

 private:
  /// Database containing all Datalog rules
  std::shared_ptr<RuleDatabase> ruleDatabase_;

  /// Query planner for cost estimation and optimization
  QueryPlanner* queryPlanner_;
};

#endif  // QLEVER_SRC_ENGINE_DATALOGQUERYPLANNER_H
