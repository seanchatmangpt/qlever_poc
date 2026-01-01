// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: Cache key types for read-through caching (EPIC 3)
// Implements deterministic cache keys incorporating epoch manifests to prevent
// stale cache hits across different data versions.

#ifndef QLEVER_SRC_ENGINE_READCACHE_READCACHEKEYS_H
#define QLEVER_SRC_ENGINE_READCACHE_READCACHEKEYS_H

#include <cstdint>
#include <string>

#include "backports/three_way_comparison.h"
#include "util/http/MediaTypes.h"

namespace readCache {

// EpochKey - Hash of epoch manifest
// Ties cache entries to a specific data version, preventing cross-epoch cache
// hits when data has changed.
struct EpochKey {
  std::string epoch_manifest_hash;  // SHA256 hex string (64 chars)

  EpochKey() = default;
  explicit EpochKey(std::string manifestHash)
      : epoch_manifest_hash(std::move(manifestHash)) {}

  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(EpochKey, epoch_manifest_hash)

  template <typename H>
  friend H AbslHashValue(H h, const EpochKey& key) {
    return H::combine(std::move(h), key.epoch_manifest_hash);
  }

  [[nodiscard]] bool isValid() const { return !epoch_manifest_hash.empty(); }
};

// PlanKey - (EpochKey, shape_sha256)
// Cache key for query execution plans. Plans are deterministic given the same
// query shape and data version.
struct PlanKey {
  EpochKey epoch_key;
  std::string shape_sha256;  // SHA256 hex string (64 chars)

  PlanKey() = default;
  PlanKey(EpochKey epochKey, std::string shapeSha256)
      : epoch_key(std::move(epochKey)), shape_sha256(std::move(shapeSha256)) {}

  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(PlanKey, epoch_key, shape_sha256)

  template <typename H>
  friend H AbslHashValue(H h, const PlanKey& key) {
    return H::combine(std::move(h), key.epoch_key, key.shape_sha256);
  }

  [[nodiscard]] bool isValid() const {
    return epoch_key.isValid() && !shape_sha256.empty();
  }
};

// BytesKey - (EpochKey, shape_sha256, params_sha256, OutputFormat, options_hash)
// Cache key for serialized query results (bytes). Includes output format and
// serialization options since the same result can have different byte
// representations.
struct BytesKey {
  EpochKey epoch_key;
  std::string shape_sha256;   // SHA256 hex string (64 chars)
  std::string params_sha256;  // SHA256 hex string (64 chars)
  ad_utility::MediaType output_format;
  uint64_t options_hash;  // Hash of serialization options (e.g., separator,
                          // escape chars)

  BytesKey() = default;
  BytesKey(EpochKey epochKey, std::string shapeSha256, std::string paramsSha256,
           ad_utility::MediaType format, uint64_t optionsHash)
      : epoch_key(std::move(epochKey)),
        shape_sha256(std::move(shapeSha256)),
        params_sha256(std::move(paramsSha256)),
        output_format(format),
        options_hash(optionsHash) {}

  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(BytesKey, epoch_key, shape_sha256,
                                              params_sha256, output_format,
                                              options_hash)

  template <typename H>
  friend H AbslHashValue(H h, const BytesKey& key) {
    return H::combine(std::move(h), key.epoch_key, key.shape_sha256,
                      key.params_sha256, key.output_format, key.options_hash);
  }

  [[nodiscard]] bool isValid() const {
    return epoch_key.isValid() && !shape_sha256.empty() &&
           !params_sha256.empty();
  }
};

// NegKey - (EpochKey, shape_sha256, params_sha256)
// Negative cache key to store "query returned empty result" or "query failed"
// status. Avoids re-executing queries known to be empty/failing.
struct NegKey {
  EpochKey epoch_key;
  std::string shape_sha256;   // SHA256 hex string (64 chars)
  std::string params_sha256;  // SHA256 hex string (64 chars)

  NegKey() = default;
  NegKey(EpochKey epochKey, std::string shapeSha256, std::string paramsSha256)
      : epoch_key(std::move(epochKey)),
        shape_sha256(std::move(shapeSha256)),
        params_sha256(std::move(paramsSha256)) {}

  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(NegKey, epoch_key, shape_sha256,
                                              params_sha256)

  template <typename H>
  friend H AbslHashValue(H h, const NegKey& key) {
    return H::combine(std::move(h), key.epoch_key, key.shape_sha256,
                      key.params_sha256);
  }

  [[nodiscard]] bool isValid() const {
    return epoch_key.isValid() && !shape_sha256.empty() &&
           !params_sha256.empty();
  }
};

}  // namespace readCache

#endif  // QLEVER_SRC_ENGINE_READCACHE_READCACHEKEYS_H
