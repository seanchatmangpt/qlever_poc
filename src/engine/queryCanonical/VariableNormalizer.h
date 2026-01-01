// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2: Query Shape Canonicalization - Step C)

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_VARIABLENORMALIZER_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_VARIABLENORMALIZER_H

#include <map>
#include <string>

#include "parser/ParsedQuery.h"
#include "rdfTypes/Variable.h"

namespace queryCanonical {

// VariableNormalizer performs α-renaming (variable renaming) to canonical
// names for query shape canonicalization. It traverses a ParsedQuery AST in
// depth-first, left-to-right order and assigns canonical variable names
// (?v0, ?v1, ...) based on the order of first appearance.
//
// Example:
//   Original: SELECT ?x ?y WHERE { ?x ?p ?y . ?y ?q ?z }
//   Canonical: SELECT ?v0 ?v1 WHERE { ?v0 ?v2 ?v1 . ?v1 ?v3 ?v4 }
//
// The traversal is deterministic: the same ParsedQuery structure always
// produces the same variable mapping, regardless of the original variable
// names.
class VariableNormalizer {
 public:
  // Construct a normalizer for the given ParsedQuery.
  explicit VariableNormalizer(const ParsedQuery& query);

  // Build a map from original variable names to canonical names (?v0, ?v1,
  // ...). Variables are renamed in order of first appearance during a
  // depth-first, left-to-right traversal of the query tree.
  //
  // The map keys are the original variable names (including the leading '?'),
  // and the values are the canonical names.
  //
  // This method is idempotent: calling it multiple times returns the same map.
  std::map<std::string, std::string> buildVariableMap();

  // Rename a variable using the canonical mapping. If the variable has not
  // been seen during traversal, it will be added to the map with the next
  // available canonical name.
  //
  // Returns a new Variable with the canonical name.
  Variable renameVariable(const Variable& orig);

 private:
  const ParsedQuery& query_;
  std::map<std::string, std::string> variableMap_;
  size_t nextVarIndex_ = 0;
  bool mapBuilt_ = false;

  // Helper: Get or create a canonical name for a variable.
  // If the variable is new, assigns it the next canonical name (?v0, ?v1, ...).
  std::string getCanonicalName(const std::string& originalName);

  // Traversal methods for different parts of the query AST.
  // These methods traverse the query tree in depth-first, left-to-right order.

  // Traverse the entire ParsedQuery.
  void traverseQuery();

  // Traverse SELECT clause (variables and aliases).
  void traverseSelectClause();

  // Traverse a GraphPattern (recursively).
  void traverseGraphPattern(const parsedQuery::GraphPattern& pattern);

  // Traverse a GraphPatternOperation (variant visitor).
  void traverseGraphPatternOperation(
      const parsedQuery::GraphPatternOperation& operation);

  // Traverse specific operation types.
  void traverseBasicGraphPattern(
      const parsedQuery::BasicGraphPattern& pattern);
  void traverseOptional(const parsedQuery::Optional& optional);
  void traverseUnion(const parsedQuery::Union& unionOp);
  void traverseMinus(const parsedQuery::Minus& minus);
  void traverseGroupGraphPattern(
      const parsedQuery::GroupGraphPattern& groupPattern);
  void traverseBind(const parsedQuery::Bind& bind);
  void traverseValues(const parsedQuery::Values& values);
  void traverseService(const parsedQuery::Service& service);
  void traverseSubquery(const parsedQuery::Subquery& subquery);
  void traverseDescribe(const parsedQuery::Describe& describe);

  // Traverse triple components (subject, predicate, object).
  void traverseTripleComponent(const TripleComponent& component);

  // Traverse a SPARQL triple.
  void traverseTriple(const SparqlTriple& triple);

  // Traverse a Variable.
  void traverseVariable(const Variable& var);

  // Traverse SPARQL expressions (for BIND, FILTER, etc.).
  void traverseExpression(
      const sparqlExpression::SparqlExpressionPimpl& expression);

  // Traverse solution modifiers (ORDER BY, GROUP BY, HAVING).
  void traverseSolutionModifiers();
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_VARIABLENORMALIZER_H
