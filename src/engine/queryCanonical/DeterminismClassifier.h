// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: AI Agent Implementation

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_DETERMINISMCLASSIFIER_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_DETERMINISMCLASSIFIER_H

#include "engine/queryCanonical/QueryFingerprint.h"
#include "engine/sparqlExpressions/SparqlExpressionPimpl.h"
#include "parser/ParsedQuery.h"
#include "parser/data/SparqlFilter.h"

namespace queryCanonical {

// Analyzes a ParsedQuery to determine its determinism characteristics and
// extract feature flags. This is used for query shape canonicalization to
// identify whether queries produce deterministic results and what features
// they use.
//
// Non-deterministic queries (containing NOW(), RAND(), SERVICE, etc.) cannot
// have their results cached, as they may produce different outputs for
// identical inputs.
//
// ARD 7.1 Requirements:
// - Detect NOW(), RAND(), UUID(), STRUUID(), BNODE() functions
// - Detect SERVICE clauses
// - Return DeterminismFeatures structure with detailed flags
class DeterminismClassifier {
 public:
  // Constructor
  explicit DeterminismClassifier() = default;

  // Analyze query and return determinism features
  // This performs a complete walk of the query tree to identify all
  // non-deterministic elements
  [[nodiscard]] DeterminismFeatures analyze(const ParsedQuery& query) const;

 private:
  // Helper to check graph pattern for non-deterministic features
  void analyzeGraphPattern(const parsedQuery::GraphPattern& pattern,
                           DeterminismFeatures& features) const;

  // Helper to check graph pattern operation
  void analyzeGraphPatternOperation(
      const parsedQuery::GraphPatternOperation& operation,
      DeterminismFeatures& features) const;

  // Check a SPARQL expression for non-deterministic functions
  void checkExpressionForNonDeterminism(
      const sparqlExpression::SparqlExpressionPimpl& expr,
      DeterminismFeatures& features) const;
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_DETERMINISMCLASSIFIER_H
