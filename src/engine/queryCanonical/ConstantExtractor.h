// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Authors: AI Agent Implementation (EPIC 2 - Query Shape Canonicalization)

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_CONSTANTEXTRACTOR_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_CONSTANTEXTRACTOR_H

#include <string>
#include <vector>

#include "engine/queryCanonical/ConstantPlaceholder.h"
#include "parser/ParsedQuery.h"
#include "rdfTypes/Iri.h"
#include "rdfTypes/Literal.h"

namespace queryCanonical {

// Extracts and categorizes constants from SPARQL queries for query shape
// canonicalization. This class walks a ParsedQuery and identifies all constant
// values (IRIs, literals, etc.) that should be replaced with placeholders.
//
// The extraction is deterministic: constants are collected in a consistent
// order (sorted by byte representation) to ensure identical queries produce
// identical query shapes and parameter hashes.
class ConstantExtractor {
 public:
  // Construct a ConstantExtractor for the given parsed query
  explicit ConstantExtractor(const ParsedQuery& query);

  // Extract all constants from the query in deterministic order.
  // Returns a vector of constant values (as strings) that can be used to
  // compute params_sha256.
  std::vector<std::string> extractConstants();

  // Classify a constant based on its type and value.
  // This determines which placeholder category it belongs to.
  static PlaceholderType classifyConstant(
      const ad_utility::triple_component::Iri& iri);
  static PlaceholderType classifyConstant(
      const ad_utility::triple_component::Literal& literal);

  // Create a placeholder for a constant value at the given index
  static Placeholder replaceWithPlaceholder(
      const ad_utility::triple_component::Iri& iri, uint32_t index);
  static Placeholder replaceWithPlaceholder(
      const ad_utility::triple_component::Literal& literal, uint32_t index);

 private:
  const ParsedQuery& query_;

  // Helper to classify numeric datatypes
  static bool isNumericDatatype(std::string_view datatype);

  // Helper to classify temporal datatypes
  static bool isTemporalDatatype(std::string_view datatype);

  // Helper to collect constants from various query components
  void collectFromGraphPattern(
      const parsedQuery::GraphPattern& pattern,
      std::vector<std::pair<std::string, PlaceholderType>>& constants);
  void collectFromTriple(
      const class SparqlTriple& triple,
      std::vector<std::pair<std::string, PlaceholderType>>& constants);
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_CONSTANTEXTRACTOR_H
