// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_IRINORMALIZER_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_IRINORMALIZER_H

#include <string>
#include <string_view>

#include "parser/ParsedQuery.h"
#include "rdfTypes/Iri.h"
#include "rdfTypes/Literal.h"
#include "util/HashMap.h"

namespace queryCanonical {

// IriNormalizer provides deterministic normalization of IRIs and literals
// to canonical string forms. This ensures that equivalent IRIs (e.g., with
// different prefix declarations) are represented identically.
class IriNormalizer {
 public:
  using PrefixMap = ad_utility::HashMap<std::string, std::string>;

  // Construct an IriNormalizer. The optional prefixMap is used for prefix
  // expansion when normalizing prefixed names.
  explicit IriNormalizer(PrefixMap prefixMap = {});

  // Normalize an IRI to its canonical full form. Returns the IRI content
  // without angle brackets, ensuring byte-for-byte consistency across
  // restarts for the same IRI.
  std::string normalizeIri(const ad_utility::triple_component::Iri& iri) const;

  // Normalize a prefixed IRI to its full canonical form. Given a prefix
  // (e.g., "ex") and localName (e.g., "Person"), returns the expanded IRI
  // without angle brackets. Throws if the prefix is not found in the
  // prefix map.
  std::string normalizePrefix(const std::string& prefix,
                               const std::string& localName) const;

  // Normalize a literal to its canonical string form, including language
  // tags and datatype URIs. The format is:
  // - Plain literal: "content"
  // - With language tag: "content"@lang
  // - With datatype: "content"^^<datatypeIRI>
  std::string normalizeLiteral(
      const ad_utility::triple_component::Literal& literal) const;

 private:
  // Map from prefix labels to their full IRI expansions (with angle brackets)
  PrefixMap prefixMap_;

  // Helper to strip angle brackets from an IRI string
  static std::string stripAngleBrackets(std::string_view iri);
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_IRINORMALIZER_H
