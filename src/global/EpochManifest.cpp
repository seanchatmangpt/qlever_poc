//  Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
//  Author: Claude Assistant
//
//  Purpose: Implementation of EpochManifest for reproducible snapshots

#include "ad_utility/EpochManifest.h"

#include <absl/strings/str_join.h>

#include <chrono>
#include <sstream>

#include "ad_utility/CryptographicHashUtils.h"

namespace ad_utility {

// Helper function to convert hash bytes to hex string
std::string bytesToHexString(const std::vector<unsigned char>& bytes) {
  return absl::StrJoin(bytes, "", [](std::string* out, unsigned char b) {
    absl::AlphaNumFormatter()(out, absl::Hex(b, absl::kZeroPad2));
  });
}

// ============================================================================
// EpochManifest Implementation
// ============================================================================

bool EpochManifest::isValid() const {
  // All required fields must be present and non-empty
  return epochId_ > 0 && !assertedTriplesHash_.empty() &&
         !derivedTriplesHash_.empty() && !rulesetHash_.empty() &&
         !shapesHash_.empty() && !configHash_.empty() &&
         !buildToolVersions_.empty() && sealTimestampMs_ > 0;
}

std::string EpochManifest::getManifestHash() const {
  // Concatenate all fields to create deterministic manifest signature
  std::string manifestContent = absl::StrCat(
      "epochId=", epochId_, ",", "assertedTriplesHash=", assertedTriplesHash_,
      ",", "derivedTriplesHash=", derivedTriplesHash_, ",",
      "rulesetHash=", rulesetHash_, ",", "shapesHash=", shapesHash_, ",",
      "configHash=", configHash_, ",", "buildToolVersions=", buildToolVersions_,
      ",", "sealTimestampMs=", sealTimestampMs_);

  // Compute SHA-256 hash of manifest content
  ad_utility::HashSha256 sha256;
  auto hashBytes = sha256(manifestContent);

  // Convert binary hash to hex string
  return bytesToHexString(hashBytes);
}

std::string EpochManifest::toString() const {
  std::ostringstream oss;
  oss << "{\n"
      << "  \"epochId\": " << epochId_ << ",\n"
      << "  \"assertedTriplesHash\": \"" << assertedTriplesHash_ << "\",\n"
      << "  \"derivedTriplesHash\": \"" << derivedTriplesHash_ << "\",\n"
      << "  \"rulesetHash\": \"" << rulesetHash_ << "\",\n"
      << "  \"shapesHash\": \"" << shapesHash_ << "\",\n"
      << "  \"configHash\": \"" << configHash_ << "\",\n"
      << "  \"buildToolVersions\": \"" << buildToolVersions_ << "\",\n"
      << "  \"sealTimestampMs\": " << sealTimestampMs_ << ",\n"
      << "  \"manifestHash\": \"" << getManifestHash() << "\"\n"
      << "}";
  return oss.str();
}

bool EpochManifest::matches(const EpochManifest& other) const {
  return epochId_ == other.epochId_ &&
         assertedTriplesHash_ == other.assertedTriplesHash_ &&
         derivedTriplesHash_ == other.derivedTriplesHash_ &&
         rulesetHash_ == other.rulesetHash_ &&
         shapesHash_ == other.shapesHash_ && configHash_ == other.configHash_ &&
         buildToolVersions_ == other.buildToolVersions_ &&
         sealTimestampMs_ == other.sealTimestampMs_;
}

// ============================================================================
// EpochManifestBuilder Implementation
// ============================================================================

EpochManifestBuilder::EpochManifestBuilder(EpochId epochId)
    : manifest_(epochId) {
  // Initialize timestamp to current time if not set
  manifest_.sealTimestampMs_ =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();
}

EpochManifestBuilder& EpochManifestBuilder::withAssertedTriples(
    std::string_view hash) {
  manifest_.assertedTriplesHash_ = std::string(hash);
  return *this;
}

EpochManifestBuilder& EpochManifestBuilder::withDerivedTriples(
    std::string_view hash) {
  manifest_.derivedTriplesHash_ = std::string(hash);
  return *this;
}

EpochManifestBuilder& EpochManifestBuilder::withRuleset(std::string_view hash) {
  manifest_.rulesetHash_ = std::string(hash);
  return *this;
}

EpochManifestBuilder& EpochManifestBuilder::withShapes(std::string_view hash) {
  manifest_.shapesHash_ = std::string(hash);
  return *this;
}

EpochManifestBuilder& EpochManifestBuilder::withConfig(std::string_view hash) {
  manifest_.configHash_ = std::string(hash);
  return *this;
}

EpochManifestBuilder& EpochManifestBuilder::withBuildToolVersions(
    std::string_view versions) {
  manifest_.buildToolVersions_ = std::string(versions);
  return *this;
}

EpochManifestBuilder& EpochManifestBuilder::withSealTimestamp(
    int64_t timestampMs) {
  manifest_.sealTimestampMs_ = timestampMs;
  return *this;
}

EpochManifest EpochManifestBuilder::build() const { return manifest_; }

}  // namespace ad_utility
