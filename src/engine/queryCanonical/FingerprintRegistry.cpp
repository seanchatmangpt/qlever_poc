// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2 - Query Shape Canonicalization)

#include "engine/queryCanonical/FingerprintRegistry.h"

#include <algorithm>

namespace queryCanonical {

// ____________________________________________________________________________
FingerprintRegistry& FingerprintRegistry::getInstance() {
  static FingerprintRegistry instance;
  return instance;
}

// ____________________________________________________________________________
void FingerprintRegistry::recordFingerprint(
    const QueryFingerprint& fingerprint) {
  auto dataLock = data_.wlock();
  dataLock->shapeFrequencies_[fingerprint.shapeHash()]++;
  dataLock->totalQueries_++;
}

// ____________________________________________________________________________
std::map<uint64_t, uint32_t> FingerprintRegistry::getShapeFrequencies() const {
  auto dataLock = data_.rlock();
  std::map<uint64_t, uint32_t> result;
  for (const auto& [hash, count] : dataLock->shapeFrequencies_) {
    result[hash] = count;
  }
  return result;
}

// ____________________________________________________________________________
std::vector<std::pair<uint64_t, uint32_t>> FingerprintRegistry::getTopShapes(
    int k) const {
  auto dataLock = data_.rlock();

  // Convert to vector
  std::vector<std::pair<uint64_t, uint32_t>> shapes;
  shapes.reserve(dataLock->shapeFrequencies_.size());
  for (const auto& [hash, count] : dataLock->shapeFrequencies_) {
    shapes.emplace_back(hash, count);
  }

  // Sort by count (descending)
  std::sort(shapes.begin(), shapes.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  // Return top-K
  if (static_cast<size_t>(k) < shapes.size()) {
    shapes.resize(k);
  }

  return shapes;
}

// ____________________________________________________________________________
size_t FingerprintRegistry::getTotalQueries() const {
  auto dataLock = data_.rlock();
  return dataLock->totalQueries_;
}

// ____________________________________________________________________________
void FingerprintRegistry::clear() {
  auto dataLock = data_.wlock();
  dataLock->shapeFrequencies_.clear();
  dataLock->totalQueries_ = 0;
}

}  // namespace queryCanonical
