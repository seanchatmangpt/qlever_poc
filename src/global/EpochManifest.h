//  Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
//  Author: Claude Assistant
//
//  Purpose: EpochManifest provides reproducible snapshot of epoch state
//  for deterministic cache keying and ETag generation

#ifndef AD_UTILITY_EPOCH_MANIFEST_H
#define AD_UTILITY_EPOCH_MANIFEST_H

#include <cstdint>
#include <string>
#include <string_view>

namespace ad_utility {

// Forward declaration to break circular dependency
// (EpochId is defined in Epoch.h as uint64_t)
using EpochId = uint64_t;

// Complete snapshot of epoch state for reproducibility
// Enables deterministic cache keys and ETags
struct EpochManifest {
  EpochId epochId_;

  // Content hashes (SHA-256 hex strings)
  // These represent the cryptographic signatures of data bundles
  std::string assertedTriplesHash_;  // hash(asserted_triples_bundle)
  std::string derivedTriplesHash_;   // hash(derived_bundle)
  std::string rulesetHash_;          // hash(ruleset.n3)
  std::string shapesHash_;           // hash(shapes.ttl)
  std::string configHash_;           // hash(config.yaml)

  // Build metadata for reproducibility tracking
  std::string buildToolVersions_;  // e.g., "ANTLR=4.13.12,CMake=3.27.0"
  int64_t sealTimestampMs_;        // when manifest was sealed (milliseconds)

  // Default constructor
  EpochManifest() = default;

  // Explicit constructor for testing/construction
  explicit EpochManifest(EpochId epochId) : epochId_(epochId) {}

  // Validation methods
  // Returns true if all required fields are present and valid
  bool isValid() const;

  // Hash of entire manifest (for ETags and cache keys)
  // This is a SHA-256 hash of all fields concatenated
  std::string getManifestHash() const;

  // Human-readable format for logging and debugging
  std::string toString() const;

  // Reproducibility check: compare all hashes and metadata
  bool matches(const EpochManifest& other) const;
};

// Builder for safe and readable manifest construction
// Fluent interface for constructing EpochManifest objects
class EpochManifestBuilder {
 private:
  EpochManifest manifest_;

 public:
  // Initialize builder with epoch ID
  explicit EpochManifestBuilder(EpochId epochId);

  // Chainable methods for setting hash values
  EpochManifestBuilder& withAssertedTriples(std::string_view hash);
  EpochManifestBuilder& withDerivedTriples(std::string_view hash);
  EpochManifestBuilder& withRuleset(std::string_view hash);
  EpochManifestBuilder& withShapes(std::string_view hash);
  EpochManifestBuilder& withConfig(std::string_view hash);
  EpochManifestBuilder& withBuildToolVersions(std::string_view versions);
  EpochManifestBuilder& withSealTimestamp(int64_t timestampMs);

  // Build and return the manifest
  EpochManifest build() const;
};

}  // namespace ad_utility

#endif  // AD_UTILITY_EPOCH_MANIFEST_H
