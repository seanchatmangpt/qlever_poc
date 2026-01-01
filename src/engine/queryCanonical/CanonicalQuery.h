// Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
// Author: Claude Assistant
//
// Purpose: CanonicalQuery data structure for storing normalized query
// representation with fingerprint (EPIC 2 - ARD 5.1)

#ifndef QLEVER_SRC_ENGINE_QUERYCANONICAL_CANONICALQUERY_H
#define QLEVER_SRC_ENGINE_QUERYCANONICAL_CANONICALQUERY_H

#include <string>
#include <unordered_map>
#include <vector>

#include "engine/queryCanonical/QueryFingerprint.h"

namespace ad_utility {

// Parameter table entry - maps placeholder to actual value
// Used to reconstruct original query from canonical form
struct ParameterEntry {
  std::string placeholder;  // e.g., "?VAR_0", "?CONST_1"
  std::string value;        // Actual value (IRI, literal, variable name)
  std::string type;         // "IRI", "LITERAL", "VARIABLE", etc.

  ParameterEntry() = default;
  ParameterEntry(std::string ph, std::string val, std::string ty)
      : placeholder(std::move(ph)), value(std::move(val)), type(std::move(ty)) {
  }
};

// Lightweight operator tree representation for cache key generation
// Stores only essential structural information without full parse tree
struct OperatorTreeRepresentation {
  std::string operator_type;         // "JOIN", "FILTER", "OPTIONAL", etc.
  std::vector<size_t> child_indices;  // Indices of child operators
  std::string metadata;  // Additional operator-specific metadata (serialized)

  OperatorTreeRepresentation() = default;
  OperatorTreeRepresentation(std::string op_type,
                              std::vector<size_t> children = {},
                              std::string meta = "")
      : operator_type(std::move(op_type)),
        child_indices(std::move(children)),
        metadata(std::move(meta)) {}
};

// CanonicalQuery - Complete canonical representation of a SPARQL query
//
// This structure holds the fingerprint along with the normalized operator
// tree and parameter table. It enables efficient cache key generation and
// query reconstruction.
//
// The canonical form is:
// - Variables renamed to ?VAR_0, ?VAR_1, ...
// - Constants replaced with ?CONST_0, ?CONST_1, ...
// - IRIs normalized to canonical form
// - Operator tree in deterministic order
struct CanonicalQuery {
  // Complete fingerprint identifying this query shape
  QueryFingerprint fingerprint;

  // Normalized operator tree (lightweight representation)
  // Stored as flat vector with indices for parent-child relationships
  std::vector<OperatorTreeRepresentation> operator_tree;

  // Parameter table mapping placeholders to actual values
  std::vector<ParameterEntry> parameters;

  // Default constructor
  CanonicalQuery() = default;

  // Construct with fingerprint
  explicit CanonicalQuery(QueryFingerprint fp)
      : fingerprint(std::move(fp)) {}

  // Get cache key from fingerprint
  // This is the primary key used for query result caching
  std::string getCacheKey() const { return fingerprint.getCacheKey(); }

  // Get shape-only cache key (ignores parameters)
  // Used for finding similar queries with different parameter values
  std::string getShapeCacheKey() const { return fingerprint.shape_sha256; }

  // Check if query is cacheable (deterministic result)
  bool isCacheable() const {
    return !hasFeature(fingerprint.feature_flags,
                       QueryFeatureFlag::NONDETERMINISTIC_RESULT) &&
           !hasFeature(fingerprint.feature_flags, QueryFeatureFlag::SERVICE);
  }

  // Get number of parameters
  size_t parameterCount() const { return parameters.size(); }

  // Get number of operators in tree
  size_t operatorCount() const { return operator_tree.size(); }

  // Validation - check if canonical query is complete
  bool isValid() const {
    return fingerprint.isValid() && !operator_tree.empty();
  }

  // Human-readable representation for debugging
  std::string toString() const;

  // Serialization stubs - to be implemented when needed
  std::string serialize() const;
  static CanonicalQuery deserialize(const std::string& data);
};

}  // namespace ad_utility

#endif  // QLEVER_SRC_ENGINE_QUERYCANONICAL_CANONICALQUERY_H
