// Copyright 2024, University of Freiburg
// Chair of Algorithms and Data Structures
// Author: Claude Agent (EPIC 2 - Query Shape Canonicalization)

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_FINGERPRINTREGISTRY_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_FINGERPRINTREGISTRY_H

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

#include "engine/queryCanonical/QueryFingerprint.h"
#include "util/HashMap.h"
#include "util/Synchronized.h"

namespace queryCanonical {

// Thread-safe singleton registry of observed query fingerprints
// Tracks:
// - Frequency of each query shape (by hash)
// - Statistics about fingerprinting
// - Hooks for observability events
class FingerprintRegistry {
 public:
  // Get singleton instance
  static FingerprintRegistry& getInstance();

  // Record a fingerprint (thread-safe)
  void recordFingerprint(const QueryFingerprint& fingerprint);

  // Get shape frequencies (hash -> count)
  [[nodiscard]] std::map<uint64_t, uint32_t> getShapeFrequencies() const;

  // Get top-K most frequent shapes
  [[nodiscard]] std::vector<std::pair<uint64_t, uint32_t>> getTopShapes(
      int k) const;

  // Get total number of queries fingerprinted
  [[nodiscard]] size_t getTotalQueries() const;

  // Clear registry (for testing)
  void clear();

 private:
  FingerprintRegistry() = default;
  ~FingerprintRegistry() = default;

  // Disable copy and move
  FingerprintRegistry(const FingerprintRegistry&) = delete;
  FingerprintRegistry& operator=(const FingerprintRegistry&) = delete;

  // Thread-safe storage of shape frequencies
  struct RegistryData {
    ad_utility::HashMap<uint64_t, uint32_t> shapeFrequencies_;
    size_t totalQueries_ = 0;
  };

  ad_utility::Synchronized<RegistryData> data_;
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_FINGERPRINTREGISTRY_H
