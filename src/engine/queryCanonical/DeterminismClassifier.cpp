// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: AI Agent Implementation

#include "engine/queryCanonical/DeterminismClassifier.h"

#include <variant>

#include "parser/GraphPattern.h"
#include "parser/GraphPatternOperation.h"
#include "parser/SelectClause.h"
#include "parser/TripleComponent.h"

namespace queryCanonical {

// ____________________________________________________________________________
DeterminismFeatures DeterminismClassifier::analyze(
    const ParsedQuery& query) const {
  DeterminismFeatures features;

  // Analyze the query's graph pattern
  analyzeGraphPattern(query._rootGraphPattern, features);

  // TODO: Analyze SELECT clause for expressions containing NOW/RAND
  // TODO: Analyze ORDER BY, GROUP BY, HAVING for non-deterministic expressions

  return features;
}

// ____________________________________________________________________________
void DeterminismClassifier::analyzeGraphPattern(
    const parsedQuery::GraphPattern& pattern,
    DeterminismFeatures& features) const {
  // Check filters for non-deterministic expressions
  for (const auto& filter : pattern._filters) {
    checkExpressionForNonDeterminism(filter.expression_, features);
  }

  // Recursively analyze all graph pattern operations
  for (const auto& op : pattern._graphPatterns) {
    analyzeGraphPatternOperation(op, features);
  }
}

// ____________________________________________________________________________
void DeterminismClassifier::analyzeGraphPatternOperation(
    const parsedQuery::GraphPatternOperation& operation,
    DeterminismFeatures& features) const {
  // Use visitor pattern to handle different operation types
  operation.visit(
      [&](const parsedQuery::Optional& opt) {
        analyzeGraphPattern(opt._child, features);
      },
      [&](const parsedQuery::Union& un) {
        analyzeGraphPattern(un._child1, features);
        analyzeGraphPattern(un._child2, features);
      },
      [&](const parsedQuery::Minus& minus) {
        analyzeGraphPattern(minus._child, features);
      },
      [&](const parsedQuery::Bind& bind) {
        checkExpressionForNonDeterminism(bind._expression, features);
      },
      [&](const parsedQuery::Values&) {
        // VALUES don't affect determinism
      },
      [&](const parsedQuery::Service&) {
        features.hasService = true;
      },
      [&](const parsedQuery::Subquery& subquery) {
        // Recursively analyze the subquery
        DeterminismClassifier subClassifier;
        auto subFeatures = subClassifier.analyze(subquery.get());
        // Merge features
        features.hasNow |= subFeatures.hasNow;
        features.hasRand |= subFeatures.hasRand;
        features.hasUuid |= subFeatures.hasUuid;
        features.hasBnode |= subFeatures.hasBnode;
        features.hasService |= subFeatures.hasService;
        features.hasNonDeterministicFunction |=
            subFeatures.hasNonDeterministicFunction;
      },
      [&](const parsedQuery::PathQuery&) {
        // Path queries don't affect determinism
      },
      [&](const parsedQuery::SpatialQuery&) {
        // Spatial queries don't affect determinism
      },
      [&](const parsedQuery::TextSearchQuery&) {
        // Text search doesn't affect determinism
      },
      [&](const parsedQuery::GroupGraphPattern& group) {
        analyzeGraphPattern(group._child, features);
      },
      [&](const parsedQuery::TransPath& path) {
        analyzeGraphPattern(path._childGraphPattern, features);
      },
      [&](const parsedQuery::BasicGraphPattern&) {
        // Basic graph patterns don't affect determinism
      },
      [&](const parsedQuery::Describe&) {
        // Describe queries don't add non-determinism
      },
      [&](const parsedQuery::Load&) {
        // Load operations don't add non-determinism
      },
      [&](const parsedQuery::NamedCachedResult&) {
        // Cached results don't affect determinism
      },
      [&](const parsedQuery::MaterializedViewQuery&) {
        // Materialized views don't affect determinism
      });
}

// ____________________________________________________________________________
void DeterminismClassifier::checkExpressionForNonDeterminism(
    const sparqlExpression::SparqlExpressionPimpl& expr,
    DeterminismFeatures& features) const {
  const std::string& descriptor = expr.getDescriptor();

  // Check for specific non-deterministic functions
  // Be conservative: look for function names in the descriptor
  if (descriptor.find("NOW") != std::string::npos) {
    features.hasNow = true;
  }
  if (descriptor.find("RAND") != std::string::npos) {
    features.hasRand = true;
  }
  if (descriptor.find("UUID") != std::string::npos) {
    features.hasUuid = true;
  }
  if (descriptor.find("STRUUID") != std::string::npos) {
    features.hasUuid = true;
  }
  if (descriptor.find("BNODE") != std::string::npos) {
    features.hasBnode = true;
  }
}

}  // namespace queryCanonical
