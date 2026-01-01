// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: NegKey data structure for negative cache keying
// (EPIC 3 - Task 4: NegativeCache Core)

#ifndef QLEVER_SRC_ENGINE_READCACHE_NEGKEY_H
#define QLEVER_SRC_ENGINE_READCACHE_NEGKEY_H

#include <cstdint>
#include <string>

#include "backports/three_way_comparison.h"

namespace readCache {

// NegKey - Lightweight key for negative cache lookups
//
// This key identifies queries or operations that have been verified to produce
// empty results or other fast-fail conditions. The key is designed to be
// compact and efficient for hash-based lookups.
//
// Design:
// - Uses shape_sha256 from QueryFingerprint as primary identifier
// - Includes epoch_id to ensure cache invalidation on data changes
// - Compact representation (string hash + epoch ID)
//
// Safety: Only queries with deterministic execution should use this cache.
// Non-deterministic queries (RAND(), NOW(), etc.) must not be cached here.
struct NegKey {
  // SHA256 hash of query shape (from QueryFingerprint)
  // This uniquely identifies the query structure
  std::string shape_sha256;

  // Epoch ID - ties the result to a specific data version
  // When data changes, epoch_id changes, invalidating cached results
  uint64_t epoch_id = 0;

  // Default constructor
  NegKey() = default;

  // Constructor with parameters
  NegKey(std::string shape, uint64_t epoch)
      : shape_sha256(std::move(shape)), epoch_id(epoch) {}

  // Equality comparison for hash map lookups
  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(NegKey, shape_sha256, epoch_id)

  // Hash support for use in hash maps
  template <typename H>
  friend H AbslHashValue(H h, const NegKey& key) {
    return H::combine(std::move(h), key.shape_sha256, key.epoch_id);
  }

  // Validation - check if key is valid
  [[nodiscard]] bool isValid() const {
    return !shape_sha256.empty() && epoch_id > 0;
  }
};

}  // namespace readCache

#endif  // QLEVER_SRC_ENGINE_READCACHE_NEGKEY_H
