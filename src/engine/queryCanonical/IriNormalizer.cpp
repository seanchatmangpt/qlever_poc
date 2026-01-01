// Copyright 2025, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude AI Assistant

#include "engine/queryCanonical/IriNormalizer.h"

#include <stdexcept>

#include "backports/StartsWithAndEndsWith.h"

namespace queryCanonical {

// _____________________________________________________________________________
IriNormalizer::IriNormalizer(PrefixMap prefixMap)
    : prefixMap_(std::move(prefixMap)) {}

// _____________________________________________________________________________
std::string IriNormalizer::normalizeIri(
    const ad_utility::triple_component::Iri& iri) const {
  // Get the IRI content without angle brackets
  auto content = iri.getContent();
  return std::string(content.toStringRepresentation());
}

// _____________________________________________________________________________
std::string IriNormalizer::normalizePrefix(
    const std::string& prefix, const std::string& localName) const {
  // Look up the prefix in the map
  auto it = prefixMap_.find(prefix);
  if (it == prefixMap_.end()) {
    throw std::runtime_error("Prefix '" + prefix +
                             "' not found in prefix map");
  }

  // The prefix map stores IRIs with angle brackets (e.g., "<http://...>")
  // We need to strip the trailing ">" and append the local name
  std::string fullIri = it->second;

  // Strip angle brackets and append local name
  fullIri = stripAngleBrackets(fullIri);
  return fullIri + localName;
}

// _____________________________________________________________________________
std::string IriNormalizer::normalizeLiteral(
    const ad_utility::triple_component::Literal& literal) const {
  // Get the base content (the literal value without quotes)
  auto content = literal.getContent();
  std::string result = "\"";
  result += content.toStringRepresentation();
  result += "\"";

  // Add language tag if present
  if (literal.hasLanguageTag()) {
    auto langTag = literal.getLanguageTag();
    result += "@";
    result += langTag.toStringRepresentation();
  }
  // Add datatype if present
  else if (literal.hasDatatype()) {
    auto datatype = literal.getDatatype();
    result += "^^<";
    result += datatype.toStringRepresentation();
    result += ">";
  }

  return result;
}

// _____________________________________________________________________________
std::string IriNormalizer::stripAngleBrackets(std::string_view iri) {
  std::string result(iri);

  // Remove leading '<' if present
  if (ad_utility::startsWith(result, "<")) {
    result = result.substr(1);
  }

  // Remove trailing '>' if present
  if (ad_utility::endsWith(result, ">")) {
    result = result.substr(0, result.size() - 1);
  }

  return result;
}

}  // namespace queryCanonical
