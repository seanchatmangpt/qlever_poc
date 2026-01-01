// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2: Query Shape Canonicalization - Step C)

#include "engine/queryCanonical/VariableNormalizer.h"

#include <variant>

#include "parser/Alias.h"
#include "parser/GraphPattern.h"
#include "parser/GraphPatternOperation.h"
#include "parser/SelectClause.h"
#include "parser/SparqlTriple.h"
#include "parser/data/OrderKey.h"

namespace queryCanonical {

// _____________________________________________________________________________
VariableNormalizer::VariableNormalizer(const ParsedQuery& query)
    : query_(query) {}

// _____________________________________________________________________________
std::map<std::string, std::string> VariableNormalizer::buildVariableMap() {
  if (!mapBuilt_) {
    // Reset state in case this is called multiple times
    variableMap_.clear();
    nextVarIndex_ = 0;

    // Traverse the entire query in depth-first, left-to-right order
    traverseQuery();

    mapBuilt_ = true;
  }
  return variableMap_;
}

// _____________________________________________________________________________
Variable VariableNormalizer::renameVariable(const Variable& orig) {
  // Ensure the map is built
  if (!mapBuilt_) {
    buildVariableMap();
  }

  std::string canonicalName = getCanonicalName(orig.name());
  return Variable(canonicalName);
}

// _____________________________________________________________________________
std::string VariableNormalizer::getCanonicalName(
    const std::string& originalName) {
  // Check if we've already assigned a canonical name
  auto it = variableMap_.find(originalName);
  if (it != variableMap_.end()) {
    return it->second;
  }

  // Assign a new canonical name
  std::string canonicalName = "?v" + std::to_string(nextVarIndex_++);
  variableMap_[originalName] = canonicalName;
  return canonicalName;
}

// _____________________________________________________________________________
void VariableNormalizer::traverseQuery() {
  // 1. Traverse SELECT clause (if present)
  traverseSelectClause();

  // 2. Traverse the root graph pattern
  traverseGraphPattern(query_._rootGraphPattern);

  // 3. Traverse solution modifiers (GROUP BY, HAVING, ORDER BY)
  traverseSolutionModifiers();
}

// _____________________________________________________________________________
void VariableNormalizer::traverseSelectClause() {
  if (!query_.hasSelectClause()) {
    return;
  }

  const auto& selectClause = query_.selectClause();

  // First, traverse selected variables (in order)
  for (const auto& var : selectClause.getSelectedVariables()) {
    traverseVariable(var);
  }

  // Then, traverse aliases (which may introduce additional variables)
  for (const auto& alias : selectClause.getAliases()) {
    // The target variable of the alias
    traverseVariable(alias._target);
    // The expression in the alias
    traverseExpression(alias._expression);
  }
}

// _____________________________________________________________________________
void VariableNormalizer::traverseGraphPattern(
    const parsedQuery::GraphPattern& pattern) {
  // First, traverse all filters in the pattern
  for (const auto& filter : pattern._filters) {
    traverseExpression(filter.expression_);
  }

  // Then, traverse all child operations in order
  for (const auto& operation : pattern._graphPatterns) {
    traverseGraphPatternOperation(operation);
  }
}

// _____________________________________________________________________________
void VariableNormalizer::traverseGraphPatternOperation(
    const parsedQuery::GraphPatternOperation& operation) {
  // Use std::visit to handle the variant
  operation.visit([this](const auto& op) {
    using T = std::decay_t<decltype(op)>;

    if constexpr (std::is_same_v<T, parsedQuery::BasicGraphPattern>) {
      traverseBasicGraphPattern(op);
    } else if constexpr (std::is_same_v<T, parsedQuery::Optional>) {
      traverseOptional(op);
    } else if constexpr (std::is_same_v<T, parsedQuery::Union>) {
      traverseUnion(op);
    } else if constexpr (std::is_same_v<T, parsedQuery::Minus>) {
      traverseMinus(op);
    } else if constexpr (std::is_same_v<T, parsedQuery::GroupGraphPattern>) {
      traverseGroupGraphPattern(op);
    } else if constexpr (std::is_same_v<T, parsedQuery::Bind>) {
      traverseBind(op);
    } else if constexpr (std::is_same_v<T, parsedQuery::Values>) {
      traverseValues(op);
    } else if constexpr (std::is_same_v<T, parsedQuery::Service>) {
      traverseService(op);
    } else if constexpr (std::is_same_v<T, parsedQuery::Subquery>) {
      traverseSubquery(op);
    } else if constexpr (std::is_same_v<T, parsedQuery::Describe>) {
      traverseDescribe(op);
    }
    // Other operation types (PathQuery, SpatialQuery, TextSearchQuery, Load,
    // NamedCachedResult, MaterializedViewQuery) are not yet supported.
    // They can be added as needed.
  });
}

// _____________________________________________________________________________
void VariableNormalizer::traverseBasicGraphPattern(
    const parsedQuery::BasicGraphPattern& pattern) {
  // Traverse all triples in order
  for (const auto& triple : pattern._triples) {
    traverseTriple(triple);
  }
}

// _____________________________________________________________________________
void VariableNormalizer::traverseOptional(const parsedQuery::Optional& opt) {
  traverseGraphPattern(opt._child);
}

// _____________________________________________________________________________
void VariableNormalizer::traverseUnion(const parsedQuery::Union& unionOp) {
  // Traverse children in left-to-right order
  traverseGraphPattern(unionOp._child1);
  traverseGraphPattern(unionOp._child2);
}

// _____________________________________________________________________________
void VariableNormalizer::traverseMinus(const parsedQuery::Minus& minus) {
  traverseGraphPattern(minus._child);
}

// _____________________________________________________________________________
void VariableNormalizer::traverseGroupGraphPattern(
    const parsedQuery::GroupGraphPattern& groupPattern) {
  // If there's a graph variable, traverse it
  if (auto* varPair =
          std::get_if<std::pair<Variable,
                                parsedQuery::GroupGraphPattern::
                                    GraphVariableBehaviour>>(
              &groupPattern.graphSpec_)) {
    traverseVariable(varPair->first);
  }

  // Traverse the child graph pattern
  traverseGraphPattern(groupPattern._child);
}

// _____________________________________________________________________________
void VariableNormalizer::traverseBind(const parsedQuery::Bind& bind) {
  // First, traverse the expression
  traverseExpression(bind._expression);

  // Then, the target variable
  traverseVariable(bind._target);
}

// _____________________________________________________________________________
void VariableNormalizer::traverseValues(const parsedQuery::Values& values) {
  // Traverse all variables in the VALUES clause
  for (const auto& var : values._inlineValues._variables) {
    traverseVariable(var);
  }
}

// _____________________________________________________________________________
void VariableNormalizer::traverseService(const parsedQuery::Service& service) {
  // Traverse visible variables
  for (const auto& var : service.visibleVariables_) {
    traverseVariable(var);
  }
}

// _____________________________________________________________________________
void VariableNormalizer::traverseSubquery(
    const parsedQuery::Subquery& subquery) {
  // Create a new normalizer for the subquery
  // This ensures that subquery variables are processed in the correct order
  const ParsedQuery& subqueryParsed = subquery.get();
  VariableNormalizer subNormalizer(subqueryParsed);

  // Build the subquery's variable map first
  auto subMap = subNormalizer.buildVariableMap();

  // Merge the subquery's variables into our map
  // Variables from the subquery are added in the order they appear in the
  // subquery
  for (const auto& [origName, _] : subMap) {
    getCanonicalName(origName);
  }
}

// _____________________________________________________________________________
void VariableNormalizer::traverseDescribe(
    const parsedQuery::Describe& describe) {
  // Traverse resources (which may be variables)
  for (const auto& resource : describe.resources_) {
    if (auto* var = std::get_if<Variable>(&resource)) {
      traverseVariable(*var);
    }
  }

  // Traverse the WHERE clause (subquery)
  traverseSubquery(describe.whereClause_);
}

// _____________________________________________________________________________
void VariableNormalizer::traverseTripleComponent(
    const TripleComponent& component) {
  if (component.isVariable()) {
    traverseVariable(component.getVariable());
  }
}

// _____________________________________________________________________________
void VariableNormalizer::traverseTriple(const SparqlTriple& triple) {
  // Traverse subject, predicate, object in order
  traverseTripleComponent(triple.s_);

  // Handle predicate (which may be a Variable or PropertyPath)
  if (auto predicateVar = triple.getPredicateVariable()) {
    traverseVariable(*predicateVar);
  }

  traverseTripleComponent(triple.o_);

  // Traverse additional scan columns
  for (const auto& [_, var] : triple.additionalScanColumns_) {
    traverseVariable(var);
  }
}

// _____________________________________________________________________________
void VariableNormalizer::traverseVariable(const Variable& var) {
  // This is where we actually record the variable
  getCanonicalName(var.name());
}

// _____________________________________________________________________________
void VariableNormalizer::traverseExpression(
    const sparqlExpression::SparqlExpressionPimpl& expression) {
  // Get all variables contained in the expression
  auto vars = expression.containedVariables();

  // Traverse them (they are already in a deterministic order)
  for (const auto* varPtr : vars) {
    traverseVariable(*varPtr);
  }
}

// _____________________________________________________________________________
void VariableNormalizer::traverseSolutionModifiers() {
  // 1. Traverse GROUP BY variables
  for (const auto& var : query_._groupByVariables) {
    traverseVariable(var);
  }

  // 2. Traverse HAVING clauses
  for (const auto& having : query_._havingClauses) {
    traverseExpression(having.expression_);
  }

  // 3. Traverse ORDER BY
  for (const auto& orderKey : query_._orderBy) {
    // VariableOrderKey has variable_
    traverseVariable(orderKey.variable_);
    // Note: ExpressionOrderKey is not in _orderBy, which is
    // std::vector<VariableOrderKey>
  }
}

}  // namespace queryCanonical
