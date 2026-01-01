//  Copyright 2025, University of Freiburg,
//  Chair of Algorithms and Data Structures.
//  Author: Claude Code (AI Assistant) - EPIC 2 Step E

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_TRIPLEPATTERNNORMALIZER_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_TRIPLEPATTERNNORMALIZER_H

#include <string>
#include <vector>

#include "parser/SparqlTriple.h"

namespace queryCanonical {

// TriplePatternNormalizer provides safe commutative reordering of triple
// patterns within a Basic Graph Pattern (BGP). This is Step E of EPIC 2 -
// Query Shape Canonicalization.
//
// The normalizer reorders triples using a stable sort with the key:
// (predicate_normalized, subject_normalized, object_normalized)
//
// IMPORTANT: This class ONLY reorders triples within a flat BGP. It does NOT
// handle reordering across OPTIONAL, UNION, or subquery boundaries. The caller
// must group BGP triples before passing them to the normalizer.
//
// Example usage:
//   TriplePatternNormalizer normalizer;
//   std::vector<SparqlTriple> triples = { /* triples from one BGP */ };
//   auto normalized = normalizer.normalizeTriplePatterns(triples);
//
class TriplePatternNormalizer {
 public:
  // Construct a normalizer. The normalizer is stateless and can be reused.
  TriplePatternNormalizer() = default;

  // Normalize (reorder) triple patterns within a BGP.
  //
  // Input: A vector of triple patterns that belong to the same BGP (i.e., no
  //        OPTIONAL/UNION/subquery boundaries between them). The triples
  //        should already be α-renamed and have IRIs expanded.
  //
  // Output: A reordered vector of triple patterns, sorted by the stable key
  //         (predicate_normalized, subject_normalized, object_normalized).
  //
  // The sort is stable, meaning triples with the same sort key will maintain
  // their relative order from the input.
  std::vector<SparqlTriple> normalizeTriplePatterns(
      std::vector<SparqlTriple> patterns) const;

 private:
  // Convert a TripleComponent to a normalized string for comparison.
  // This ensures deterministic ordering regardless of the component type
  // (Variable, Iri, Literal, etc.).
  static std::string normalizeComponent(const TripleComponent& component);

  // Comparison function for sorting triples by (predicate, subject, object).
  // Returns true if triple 'a' should come before triple 'b' in the sorted
  // order.
  static bool compareTriples(const SparqlTriple& a, const SparqlTriple& b);
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_TRIPLEPATTERNNORMALIZER_H
