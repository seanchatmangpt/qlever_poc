// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2 - Query Shape Canonicalization)

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_NORMALIZATIONTYPES_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_NORMALIZATIONTYPES_H

#include <string>
#include <vector>

#include "parser/data/Variable.h"
#include "rdfTypes/Iri.h"
#include "util/HashMap.h"

namespace queryCanonical {

// Normalized variable name
using NormalizedVariable = Variable;

// Normalized IRI
using NormalizedIri = ad_utility::triple_component::Iri;

// Constant placeholder (for lifted constants)
struct ConstantPlaceholder {
  std::string placeholderId;  // e.g., "?const_0", "?const_1"
  std::string originalValue;  // Original constant value

  bool operator==(const ConstantPlaceholder& other) const {
    return placeholderId == other.placeholderId;
  }

  template <typename H>
  friend H AbslHashValue(H h, const ConstantPlaceholder& cp) {
    return H::combine(std::move(h), cp.placeholderId);
  }
};

// Triple pattern (simplified representation)
struct TriplePattern {
  std::string subject;
  std::string predicate;
  std::string object;

  [[nodiscard]] std::string toString() const {
    return subject + " " + predicate + " " + object;
  }

  bool operator==(const TriplePattern& other) const {
    return subject == other.subject && predicate == other.predicate &&
           object == other.object;
  }
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_NORMALIZATIONTYPES_H
