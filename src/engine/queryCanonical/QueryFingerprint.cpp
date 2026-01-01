// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2 - Query Shape Canonicalization)

#include "engine/queryCanonical/QueryFingerprint.h"

#include <absl/hash/hash.h>

#include <sstream>

namespace queryCanonical {

// ____________________________________________________________________________
std::string DeterminismFeatures::toString() const {
  std::ostringstream oss;
  oss << "DeterminismFeatures{";
  if (hasNow) oss << "NOW ";
  if (hasRand) oss << "RAND ";
  if (hasUuid) oss << "UUID ";
  if (hasBnode) oss << "BNODE ";
  if (hasService) oss << "SERVICE ";
  if (hasNonDeterministicFunction) oss << "NONDET ";
  oss << (isDeterministic() ? "[DETERMINISTIC]" : "[NON-DETERMINISTIC]");
  oss << "}";
  return oss.str();
}

// ____________________________________________________________________________
QueryFingerprint::QueryFingerprint(std::string canonicalSerialization,
                                   DeterminismFeatures features,
                                   ad_utility::EpochId epochId,
                                   std::string manifestSha256)
    : canonicalSerialization_(std::move(canonicalSerialization)),
      shapeHash_(computeShapeHash(canonicalSerialization_)),
      features_(features),
      epochId_(epochId),
      manifestSha256_(std::move(manifestSha256)) {}

// ____________________________________________________________________________
uint64_t QueryFingerprint::computeShapeHash(const std::string& canonical) {
  return absl::Hash<std::string>{}(canonical);
}

}  // namespace queryCanonical
