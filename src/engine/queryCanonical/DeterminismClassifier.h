// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2 - Query Shape Canonicalization)

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_DETERMINISMCLASSIFIER_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_DETERMINISMCLASSIFIER_H

#include "engine/queryCanonical/QueryFingerprint.h"
#include "parser/ParsedQuery.h"

namespace queryCanonical {

// Analyzes query to determine if it is deterministic
// Checks for non-deterministic features:
// - NOW(), RAND(), UUID(), BNODE()
// - SERVICE calls
// - Other non-deterministic functions
class DeterminismClassifier {
 public:
  explicit DeterminismClassifier() = default;

  // Analyze query and return determinism features
  [[nodiscard]] DeterminismFeatures analyze(const ParsedQuery& query) const;

 private:
  // Helper to check graph pattern for non-deterministic features
  void analyzeGraphPattern(const parsedQuery::GraphPattern& pattern,
                           DeterminismFeatures& features) const;
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_DETERMINISMCLASSIFIER_H
