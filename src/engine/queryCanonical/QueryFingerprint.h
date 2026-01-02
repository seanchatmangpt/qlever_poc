// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: QueryFingerprint data structure for query shape canonicalization
// and deterministic cache keying (EPIC 2 - ARD 5.1)

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_QUERYFINGERPRINT_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_QUERYFINGERPRINT_H

#include <cstdint>
#include <string>

#include "backports/three_way_comparison.h"
#include "global/Epoch.h"
#include "global/EpochManifest.h"

namespace queryCanonical {

using ad_utility::EpochId;

// Feature flags for query characteristics
// Used to identify queries with special properties that affect caching
enum class QueryFeatureFlag : uint32_t {
  NONE = 0,
  NONDETERMINISTIC_RESULT = 1 << 0,  // Query result may vary (e.g., RAND())
  DISTINCT = 1 << 1,                 // Query uses DISTINCT modifier
  REDUCED = 1 << 2,                  // Query uses REDUCED modifier
  AGGREGATION = 1 << 3,              // Query contains GROUP BY or aggregates
  LIMIT = 1 << 4,                    // Query has LIMIT clause
  OFFSET = 1 << 5,                   // Query has OFFSET clause
  ORDER_BY = 1 << 6,                 // Query has ORDER BY clause
  OPTIONAL = 1 << 7,                 // Query contains OPTIONAL patterns
  UNION = 1 << 8,                    // Query contains UNION patterns
  FILTER = 1 << 9,                   // Query contains FILTER expressions
  BIND = 1 << 10,                    // Query contains BIND operations
  VALUES = 1 << 11,                  // Query contains VALUES clause
  SERVICE = 1 << 12,                 // Query uses SERVICE (federated query)
  PROPERTY_PATH = 1 << 13,           // Query contains property paths
  SUBQUERY = 1 << 14,                // Query contains nested subqueries
  NEGATION = 1 << 15,                // Query contains MINUS or NOT EXISTS
  FULL_TEXT_SEARCH = 1 << 16,        // Query uses text index features
  SPATIAL_QUERY = 1 << 17,           // Query uses spatial/geographic features
  UPDATE_OPERATION = 1 << 18,        // Query is an UPDATE operation
  MATERIALIZED_VIEW = 1 << 19        // Query uses materialized views
};

// Bitwise operations for feature flags
inline QueryFeatureFlag operator|(QueryFeatureFlag a, QueryFeatureFlag b) {
  return static_cast<QueryFeatureFlag>(static_cast<uint32_t>(a) |
                                       static_cast<uint32_t>(b));
}

inline QueryFeatureFlag operator&(QueryFeatureFlag a, QueryFeatureFlag b) {
  return static_cast<QueryFeatureFlag>(static_cast<uint32_t>(a) &
                                       static_cast<uint32_t>(b));
}

inline QueryFeatureFlag& operator|=(QueryFeatureFlag& a, QueryFeatureFlag b) {
  a = a | b;
  return a;
}

inline bool hasFeature(QueryFeatureFlag flags, QueryFeatureFlag feature) {
  return (flags & feature) != QueryFeatureFlag::NONE;
}

// QueryFingerprint - Complete deterministic identifier for a query shape
//
// This structure captures all aspects of a query that affect its semantics
// and result shape, enabling deterministic cache keying and query
// canonicalization. The fingerprint is independent of variable names and
// syntactic variations.
//
// ARD 5.1 Requirements:
// - Epoch binding for reproducibility
// - Multiple SHA256 hashes for different query aspects
// - Feature flags for query characteristics
// - Shape feature vector for similarity detection
struct QueryFingerprint {
  // Epoch binding - ties query to specific data version
  EpochId epoch_id = 0;
  std::string epoch_manifest_sha256;  // Hash of complete epoch manifest

  // Query identity hashes (SHA256 hex strings, 64 characters each)
  std::string raw_query_sha256;        // Hash of original query text
  std::string normalized_text_sha256;  // Hash of normalized query text
  std::string shape_sha256;            // Hash of operator tree structure
  std::string params_sha256;           // Hash of parameter values

  // Query characteristics
  QueryFeatureFlag feature_flags = QueryFeatureFlag::NONE;

  // Shape feature vector hash for similarity detection
  // Hash of normalized feature vector representing query complexity profile
  std::string shape_feature_vector_hash;

  // Default constructor
  QueryFingerprint() = default;

  // Equality comparison
  QL_DEFINE_DEFAULTED_EQUALITY_OPERATOR_LOCAL(
      QueryFingerprint, epoch_id, epoch_manifest_sha256, raw_query_sha256,
      normalized_text_sha256, shape_sha256, params_sha256, feature_flags,
      shape_feature_vector_hash)

  // Hash support for use in hash maps
  template <typename H>
  friend H AbslHashValue(H h, const QueryFingerprint& fp) {
    return H::combine(std::move(h), fp.epoch_id, fp.epoch_manifest_sha256,
                      fp.raw_query_sha256, fp.normalized_text_sha256,
                      fp.shape_sha256, fp.params_sha256, fp.feature_flags,
                      fp.shape_feature_vector_hash);
  }

  // Validation - check if fingerprint is complete and valid
  bool isValid() const {
    return epoch_id > 0 && !epoch_manifest_sha256.empty() &&
           !raw_query_sha256.empty() && !normalized_text_sha256.empty() &&
           !shape_sha256.empty() && !params_sha256.empty() &&
           !shape_feature_vector_hash.empty();
  }

  // Get cache key string for this fingerprint
  // Combines all hashes into deterministic cache key
  std::string getCacheKey() const;

  // Human-readable representation for debugging
  std::string toString() const;

  // Serialization stubs - to be implemented when needed
  std::string serialize() const;
  static QueryFingerprint deserialize(const std::string& data);
};

}  // namespace queryCanonical

namespace queryCanonical {

// Determinism classification result
// Used by DeterminismClassifier to report which non-deterministic features
// are present in a query
struct DeterminismFeatures {
  bool hasNow = false;      // Query contains NOW() function
  bool hasRand = false;     // Query contains RAND() function
  bool hasUuid = false;     // Query contains UUID() function
  bool hasBnode = false;    // Query contains BNODE() function
  bool hasService = false;  // Query contains SERVICE clause
  bool hasNonDeterministicFunction =
      false;  // Other non-deterministic functions

  // Check if query is deterministic (cacheable)
  [[nodiscard]] bool isDeterministic() const {
    return !hasNow && !hasRand && !hasUuid && !hasBnode && !hasService &&
           !hasNonDeterministicFunction;
  }

  // Convert to QueryFeatureFlag for QueryFingerprint
  [[nodiscard]] queryCanonical::QueryFeatureFlag toFeatureFlag() const {
    if (!isDeterministic()) {
      return queryCanonical::QueryFeatureFlag::NONDETERMINISTIC_RESULT;
    }
    return queryCanonical::QueryFeatureFlag::NONE;
  }

  // Human-readable representation
  std::string toString() const;
};

// Statistics about the fingerprinting process
struct QueryFingerprintStats {
  std::chrono::milliseconds totalTime{0};
  std::chrono::milliseconds iriNormalizationTime{0};
  std::chrono::microseconds variableRenameTime{0};
  std::chrono::microseconds constantExtractionTime{0};
  std::chrono::microseconds serializationTime{0};
  std::chrono::microseconds featureAnalysisTime{0};
  size_t numConstants = 0;
  size_t numVariables = 0;
  size_t numTriples = 0;
};

}  // namespace queryCanonical

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_QUERYFINGERPRINT_H
