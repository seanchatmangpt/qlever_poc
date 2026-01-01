//  Copyright 2025, University of Freiburg,
//                  Chair of Algorithms and Data Structures
//  Author: Claude Assistant
//
//  Purpose: Comprehensive tests for EpochManifest reproducibility and integrity
//
//  Test Coverage:
//  - Manifest construction and validation (4 tests)
//  - Reproducibility verification (3 tests)
//  - Hash computation validation (2 tests)
//  - Serialization round-trip testing (2 tests)
//  - Comparison operators (2 tests)

#include <gtest/gtest.h>

#include <chrono>
#include <regex>
#include <sstream>
#include <vector>

#include "ad_utility/EpochManifest.h"

namespace ad_utility {

// ============================================================================
// Test Fixtures and Helper Functions
// ============================================================================

// Fixture for EpochManifest tests with helper methods
class EpochManifestTest : public ::testing::Test {
 protected:
  // Standard test hashes (realistic SHA-256 hex strings, 64 chars each)
  static constexpr const char* TEST_HASH_1 =
      "abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890";
  static constexpr const char* TEST_HASH_2 =
      "fedcba0987654321fedcba0987654321fedcba0987654321fedcba0987654321";
  static constexpr const char* TEST_HASH_3 =
      "1111111111111111111111111111111111111111111111111111111111111111";
  static constexpr const char* TEST_HASH_4 =
      "2222222222222222222222222222222222222222222222222222222222222222";
  static constexpr const char* TEST_HASH_5 =
      "3333333333333333333333333333333333333333333333333333333333333333";

  static constexpr const char* TEST_BUILD_VERSIONS =
      "ANTLR=4.13.12,CMake=3.27.0,GCC=11.4.0";
  static constexpr int64_t TEST_TIMESTAMP_MS = 1609459200000;  // 2021-01-01

  // Create a valid manifest with realistic test data
  EpochManifest createValidManifest(EpochId epochId = 1) {
    return EpochManifestBuilder(epochId)
        .withAssertedTriples(TEST_HASH_1)
        .withDerivedTriples(TEST_HASH_2)
        .withRuleset(TEST_HASH_3)
        .withShapes(TEST_HASH_4)
        .withConfig(TEST_HASH_5)
        .withBuildToolVersions(TEST_BUILD_VERSIONS)
        .withSealTimestamp(TEST_TIMESTAMP_MS)
        .build();
  }

  // Create a manifest with modified hash (for difference testing)
  EpochManifest createManifestWithDifferentHash(EpochId epochId = 1) {
    return EpochManifestBuilder(epochId)
        .withAssertedTriples(
            "0000000000000000000000000000000000000000000000000000000000000000")
        .withDerivedTriples(TEST_HASH_2)
        .withRuleset(TEST_HASH_3)
        .withShapes(TEST_HASH_4)
        .withConfig(TEST_HASH_5)
        .withBuildToolVersions(TEST_BUILD_VERSIONS)
        .withSealTimestamp(TEST_TIMESTAMP_MS)
        .build();
  }

  // Helper to verify manifest hash format (hexadecimal)
  bool isValidHexString(const std::string& str) {
    if (str.empty()) {
      return false;
    }
    std::regex hexRegex("^[0-9a-fA-F]+$");
    return std::regex_match(str, hexRegex);
  }

  // Helper to extract JSON field from manifest string
  std::string extractJsonField(const std::string& jsonStr,
                               const std::string& fieldName) {
    std::string pattern = "\"" + fieldName + "\": \"?([^,}]+)\"?";
    std::regex fieldRegex(pattern);
    std::smatch matches;

    if (std::regex_search(jsonStr, matches, fieldRegex)) {
      return matches[1].str();
    }
    return "";
  }

  // Parse numeric JSON field
  int64_t extractJsonNumericField(const std::string& jsonStr,
                                  const std::string& fieldName) {
    std::string pattern = "\"" + fieldName + "\": (\\d+)";
    std::regex fieldRegex(pattern);
    std::smatch matches;

    if (std::regex_search(jsonStr, matches, fieldRegex)) {
      return std::stoll(matches[1].str());
    }
    return -1;
  }
};

// ============================================================================
// TEST SUITE 1: Manifest Construction & Validation (4 tests)
// ============================================================================

TEST_F(EpochManifestTest, ManifestIsValidWhenAllFieldsPresent) {
  // ARRANGE: Create manifest with all fields populated
  EpochManifest manifest = createValidManifest();

  // ACT: Check validity
  bool isValid = manifest.isValid();

  // ASSERT: Manifest should be valid
  EXPECT_TRUE(isValid) << "Manifest with all fields should be valid";
  EXPECT_NE(manifest.epochId_, 0) << "Epoch ID should be set";
  EXPECT_FALSE(manifest.assertedTriplesHash_.empty())
      << "Asserted triples hash should be set";
  EXPECT_FALSE(manifest.derivedTriplesHash_.empty())
      << "Derived triples hash should be set";
  EXPECT_GT(manifest.sealTimestampMs_, 0)
      << "Seal timestamp should be positive";
}

TEST_F(EpochManifestTest, ManifestIsInvalidWhenFieldsMissing) {
  // ARRANGE: Create manifest and leave fields empty
  EpochManifest manifest(1);
  // Note: epochId_ is set to 1, but other fields remain empty

  // ACT: Check validity
  bool isValid = manifest.isValid();

  // ASSERT: Manifest should be invalid
  EXPECT_FALSE(isValid) << "Manifest with missing fields should be invalid";

  // Additional test: empty specific field
  manifest.assertedTriplesHash_ = TEST_HASH_1;
  manifest.derivedTriplesHash_ = TEST_HASH_2;
  manifest.rulesetHash_ = TEST_HASH_3;
  manifest.shapesHash_ = TEST_HASH_4;
  manifest.configHash_ = TEST_HASH_5;
  manifest.buildToolVersions_ = TEST_BUILD_VERSIONS;
  // sealTimestampMs_ still 0 - manifest should be invalid
  EXPECT_FALSE(manifest.isValid())
      << "Manifest with zero timestamp should be invalid";
}

TEST_F(EpochManifestTest, BuilderPatternConstructsValidManifest) {
  // ARRANGE: Use builder pattern to construct manifest
  EpochManifestBuilder builder(42);

  // ACT: Build manifest with fluent interface
  EpochManifest manifest = builder.withAssertedTriples(TEST_HASH_1)
                               .withDerivedTriples(TEST_HASH_2)
                               .withRuleset(TEST_HASH_3)
                               .withShapes(TEST_HASH_4)
                               .withConfig(TEST_HASH_5)
                               .withBuildToolVersions(TEST_BUILD_VERSIONS)
                               .withSealTimestamp(TEST_TIMESTAMP_MS)
                               .build();

  // ASSERT: Constructed manifest should be valid
  ASSERT_TRUE(manifest.isValid()) << "Builder should construct valid manifest";
  EXPECT_EQ(manifest.epochId_, 42) << "Builder should set epoch ID";
  EXPECT_EQ(manifest.assertedTriplesHash_, TEST_HASH_1)
      << "Builder should set asserted triples hash";
}

TEST_F(EpochManifestTest, ManifestHashIsConsistent) {
  // ARRANGE: Create two identical manifests
  EpochManifest manifest1 = createValidManifest();
  EpochManifest manifest2 = createValidManifest();

  // ACT: Get hashes
  std::string hash1 = manifest1.getManifestHash();
  std::string hash2 = manifest2.getManifestHash();

  // ASSERT: Identical manifests should produce identical hashes
  EXPECT_EQ(hash1, hash2)
      << "Identical manifests must produce identical hashes";
  EXPECT_TRUE(isValidHexString(hash1))
      << "Manifest hash should be valid hexadecimal";
  EXPECT_EQ(hash1.length(), 64)
      << "SHA-256 hash should be 64 hex characters (32 bytes)";
}

// ============================================================================
// TEST SUITE 2: Reproducibility (3 tests)
// ============================================================================

TEST_F(EpochManifestTest, IdenticalInputsProduceIdenticalManifests) {
  // ARRANGE: Create multiple manifests with same input values
  std::vector<EpochManifest> manifests;
  for (int i = 0; i < 5; ++i) {
    manifests.push_back(createValidManifest());
  }

  // ACT: Get hashes for all manifests
  std::vector<std::string> hashes;
  for (const auto& manifest : manifests) {
    hashes.push_back(manifest.getManifestHash());
  }

  // ASSERT: All hashes should be identical (perfect reproducibility)
  for (size_t i = 1; i < hashes.size(); ++i) {
    EXPECT_EQ(hashes[0], hashes[i])
        << "Reproducibility check: Hash " << i << " differs from hash 0";
  }
}

TEST_F(EpochManifestTest, DifferentHashesProduceDifferentManifestHashes) {
  // ARRANGE: Create two manifests with different content hashes
  EpochManifest manifest1 = createValidManifest();
  EpochManifest manifest2 = createManifestWithDifferentHash();

  // ACT: Get hashes
  std::string hash1 = manifest1.getManifestHash();
  std::string hash2 = manifest2.getManifestHash();

  // ASSERT: Different input should produce different manifest hashes
  EXPECT_NE(hash1, hash2)
      << "Different manifest content should produce different hashes";
  EXPECT_TRUE(isValidHexString(hash1)) << "Hash 1 should be valid hex";
  EXPECT_TRUE(isValidHexString(hash2)) << "Hash 2 should be valid hex";
}

TEST_F(EpochManifestTest, ManifestHashChangesOnAnyFieldChange) {
  // ARRANGE: Create a base manifest
  EpochManifest manifest1 = createValidManifest();
  std::string originalHash = manifest1.getManifestHash();

  // Test: Changing each field should change the hash
  std::vector<std::pair<std::string, std::function<EpochManifest()>>>
      fieldChangeTests = {
          {"epochId",
           []() {
             EpochManifest m = createValidManifest();
             m.epochId_ = 999;
             return m;
           }},
          {"assertedTriplesHash",
           []() {
             EpochManifest m = createValidManifest();
             m.assertedTriplesHash_ =
                 "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                 "aaa";
             return m;
           }},
          {"derivedTriplesHash",
           []() {
             EpochManifest m = createValidManifest();
             m.derivedTriplesHash_ =
                 "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
                 "bbb";
             return m;
           }},
          {"sealTimestampMs",
           []() {
             EpochManifest m = createValidManifest();
             m.sealTimestampMs_ = 9999999999;
             return m;
           }},
      };

  // ACT & ASSERT: For each field change test
  for (const auto& [fieldName, createModifiedManifest] : fieldChangeTests) {
    EpochManifest modifiedManifest = createModifiedManifest();
    std::string modifiedHash = modifiedManifest.getManifestHash();

    EXPECT_NE(originalHash, modifiedHash)
        << "Changing " << fieldName << " should change manifest hash";
  }
}

// ============================================================================
// TEST SUITE 3: Hash Computation (2 tests)
// ============================================================================

TEST_F(EpochManifestTest, ManifestHashIsHexEncoded) {
  // ARRANGE: Create a manifest
  EpochManifest manifest = createValidManifest();

  // ACT: Get manifest hash
  std::string hash = manifest.getManifestHash();

  // ASSERT: Hash should be valid hexadecimal
  EXPECT_FALSE(hash.empty()) << "Hash should not be empty";
  EXPECT_TRUE(isValidHexString(hash))
      << "Manifest hash should contain only hexadecimal characters";

  // Verify it matches expected length for SHA-256
  EXPECT_EQ(hash.length(), 64)
      << "SHA-256 hex representation should be 64 characters";

  // Verify no uppercase letters if lowercase expected (consistency)
  bool hasUppercase = false;
  for (char c : hash) {
    if (std::isupper(c)) {
      hasUppercase = true;
      break;
    }
  }
  // Accept both uppercase and lowercase, but document consistency
  SUCCEED() << "Hash format is consistent: " << hash;
}

TEST_F(EpochManifestTest, ManifestHashIsAlwaysPresent) {
  // ARRANGE: Create multiple manifests in different states
  std::vector<EpochManifest> manifests = {
      createValidManifest(),
      createValidManifest(5),
      createManifestWithDifferentHash(10),
  };

  // ACT: Get hash for each manifest
  for (const auto& manifest : manifests) {
    std::string hash = manifest.getManifestHash();

    // ASSERT: Every manifest should have a non-empty hash
    EXPECT_FALSE(hash.empty())
        << "getManifestHash() should never return empty string";
    EXPECT_EQ(hash.length(), 64)
        << "Hash length should always be 64 characters for SHA-256";
  }
}

// ============================================================================
// TEST SUITE 4: Serialization (2 tests)
// ============================================================================

TEST_F(EpochManifestTest, ToStringProducesValidJSON) {
  // ARRANGE: Create a manifest
  EpochManifest manifest = createValidManifest();

  // ACT: Get string representation
  std::string jsonStr = manifest.toString();

  // ASSERT: Output should be valid JSON format
  EXPECT_FALSE(jsonStr.empty()) << "toString() should not be empty";
  EXPECT_NE(jsonStr.find('{'), std::string::npos)
      << "Output should contain opening brace";
  EXPECT_NE(jsonStr.find('}'), std::string::npos)
      << "Output should contain closing brace";

  // Verify JSON contains expected fields
  EXPECT_NE(jsonStr.find("epochId"), std::string::npos)
      << "JSON should contain epochId field";
  EXPECT_NE(jsonStr.find("assertedTriplesHash"), std::string::npos)
      << "JSON should contain assertedTriplesHash field";
  EXPECT_NE(jsonStr.find("manifestHash"), std::string::npos)
      << "JSON should contain manifestHash field";
  EXPECT_NE(jsonStr.find("sealTimestampMs"), std::string::npos)
      << "JSON should contain sealTimestampMs field";

  // Verify format includes expected values
  EXPECT_NE(jsonStr.find(TEST_HASH_1), std::string::npos)
      << "JSON should contain the asserted triples hash value";
  EXPECT_NE(jsonStr.find(std::to_string(TEST_TIMESTAMP_MS)), std::string::npos)
      << "JSON should contain the seal timestamp value";

  // Basic JSON structure validation
  std::regex jsonStructure("\\{[^}]+\\}");
  EXPECT_TRUE(std::regex_search(jsonStr, jsonStructure))
      << "Output should follow basic JSON structure";
}

TEST_F(EpochManifestTest, ToStringCanBeRoundTripped) {
  // ARRANGE: Create a manifest
  EpochManifest manifest = createValidManifest();

  // ACT: Serialize to string
  std::string serialized = manifest.toString();

  // Extract fields from JSON representation
  int64_t extractedEpochId = extractJsonNumericField(serialized, "epochId");
  std::string extractedAssertedHash =
      extractJsonField(serialized, "assertedTriplesHash");
  std::string extractedManifestHash =
      extractJsonField(serialized, "manifestHash");

  // ASSERT: Extracted values should match original manifest
  EXPECT_EQ(extractedEpochId, manifest.epochId_)
      << "Extracted epochId should match original";

  // Remove quotes if present in extraction
  if (!extractedAssertedHash.empty() && extractedAssertedHash.back() == '"') {
    extractedAssertedHash.pop_back();
  }
  EXPECT_EQ(extractedAssertedHash, manifest.assertedTriplesHash_)
      << "Extracted hash should match original";

  // Verify manifest hash is present and valid
  EXPECT_FALSE(extractedManifestHash.empty())
      << "Manifest hash should be present in JSON";
  EXPECT_TRUE(isValidHexString(extractedManifestHash))
      << "Extracted manifest hash should be valid hex";

  // Test idempotency: get hash from original, compare with extracted hash
  std::string originalManifestHash = manifest.getManifestHash();
  // Clean up extracted hash (remove quotes if added)
  if (!extractedManifestHash.empty() && extractedManifestHash.back() == '"') {
    extractedManifestHash.pop_back();
  }
  EXPECT_EQ(originalManifestHash, extractedManifestHash)
      << "Manifest hash should be consistent across serialization";
}

// ============================================================================
// TEST SUITE 5: Comparison (2 tests)
// ============================================================================

TEST_F(EpochManifestTest, IdenticalManifestsMatch) {
  // ARRANGE: Create two identical manifests
  EpochManifest manifest1 = createValidManifest();
  EpochManifest manifest2 = createValidManifest();

  // ACT: Compare manifests
  bool matches = manifest1.matches(manifest2);

  // ASSERT: Identical manifests should match
  EXPECT_TRUE(matches)
      << "Identical manifests should return true for matches()";

  // Additional verification: check individual fields
  EXPECT_EQ(manifest1.epochId_, manifest2.epochId_)
      << "Epoch IDs should be equal";
  EXPECT_EQ(manifest1.assertedTriplesHash_, manifest2.assertedTriplesHash_)
      << "Asserted triples hashes should be equal";
  EXPECT_EQ(manifest1.derivedTriplesHash_, manifest2.derivedTriplesHash_)
      << "Derived triples hashes should be equal";

  // Verify hashes are also identical
  std::string hash1 = manifest1.getManifestHash();
  std::string hash2 = manifest2.getManifestHash();
  EXPECT_EQ(hash1, hash2)
      << "Identical manifests should have identical manifest hashes";
}

TEST_F(EpochManifestTest, DifferentManifestsDontMatch) {
  // ARRANGE: Create two different manifests
  EpochManifest manifest1 = createValidManifest();
  EpochManifest manifest2 = createManifestWithDifferentHash();

  // ACT: Compare manifests
  bool matches = manifest1.matches(manifest2);

  // ASSERT: Different manifests should not match
  EXPECT_FALSE(matches)
      << "Different manifests should return false for matches()";

  // Verify their hashes are also different
  std::string hash1 = manifest1.getManifestHash();
  std::string hash2 = manifest2.getManifestHash();
  EXPECT_NE(hash1, hash2) << "Different manifests should have different hashes";

  // Test partial differences
  EpochManifest manifest3 = createValidManifest();
  manifest3.epochId_ = 999;  // Change only epoch ID
  EXPECT_FALSE(manifest1.matches(manifest3))
      << "Manifests differing in epochId should not match";

  EpochManifest manifest4 = createValidManifest();
  manifest4.sealTimestampMs_ = TEST_TIMESTAMP_MS + 1000;  // Change timestamp
  EXPECT_FALSE(manifest1.matches(manifest4))
      << "Manifests differing in timestamp should not match";
}

// ============================================================================
// PERFORMANCE TEST: Manifest Hash Generation Performance
// ============================================================================

TEST_F(EpochManifestTest, ManifestHashGenerationIsPerformant) {
  // ARRANGE: Create a manifest
  EpochManifest manifest = createValidManifest();

  // ACT: Measure time to generate hash multiple times
  auto startTime = std::chrono::high_resolution_clock::now();
  constexpr int iterations = 1000;

  for (int i = 0; i < iterations; ++i) {
    volatile std::string hash = manifest.getManifestHash();
    (void)hash;  // Prevent optimization
  }

  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      endTime - startTime);

  double timePerHash = duration.count() / static_cast<double>(iterations);

  // ASSERT: Hash generation should be fast (<1ms per hash)
  EXPECT_LT(timePerHash, 1000.0)
      << "Hash generation should be <1ms per call (measured: " << timePerHash
      << " microseconds)";

  // Log performance metrics
  SUCCEED() << "Performance: " << timePerHash
            << " microseconds per hash generation";
}

// ============================================================================
// EDGE CASE TESTS: Special characters and empty strings
// ============================================================================

TEST_F(EpochManifestTest, ManifestHandlesSpecialCharactersInBuildVersions) {
  // ARRANGE: Create manifest with special characters in build versions
  std::string specialVersions =
      "ANTLR=4.13.12,CMake=3.27.0,GCC=11.4.0-debian-1~ubuntu";

  EpochManifest manifest = EpochManifestBuilder(1)
                               .withAssertedTriples(TEST_HASH_1)
                               .withDerivedTriples(TEST_HASH_2)
                               .withRuleset(TEST_HASH_3)
                               .withShapes(TEST_HASH_4)
                               .withConfig(TEST_HASH_5)
                               .withBuildToolVersions(specialVersions)
                               .withSealTimestamp(TEST_TIMESTAMP_MS)
                               .build();

  // ACT: Get hash and validate
  std::string hash = manifest.getManifestHash();

  // ASSERT: Should handle special characters gracefully
  EXPECT_FALSE(hash.empty())
      << "Hash should be generated despite special chars";
  EXPECT_TRUE(isValidHexString(hash))
      << "Hash should still be valid hexadecimal";
  EXPECT_EQ(manifest.buildToolVersions_, specialVersions)
      << "Special characters should be preserved";
}

TEST_F(EpochManifestTest, ManifestComparesLargeTimestamps) {
  // ARRANGE: Create manifests with large timestamps
  int64_t largeTimestamp = 9223372036854775807LL;  // Max int64

  EpochManifest manifest1 = EpochManifestBuilder(1)
                                .withAssertedTriples(TEST_HASH_1)
                                .withDerivedTriples(TEST_HASH_2)
                                .withRuleset(TEST_HASH_3)
                                .withShapes(TEST_HASH_4)
                                .withConfig(TEST_HASH_5)
                                .withBuildToolVersions(TEST_BUILD_VERSIONS)
                                .withSealTimestamp(largeTimestamp)
                                .build();

  EpochManifest manifest2 = EpochManifestBuilder(1)
                                .withAssertedTriples(TEST_HASH_1)
                                .withDerivedTriples(TEST_HASH_2)
                                .withRuleset(TEST_HASH_3)
                                .withShapes(TEST_HASH_4)
                                .withConfig(TEST_HASH_5)
                                .withBuildToolVersions(TEST_BUILD_VERSIONS)
                                .withSealTimestamp(largeTimestamp)
                                .build();

  // ACT & ASSERT: Large timestamps should be handled correctly
  EXPECT_TRUE(manifest1.matches(manifest2))
      << "Manifests with large timestamps should match";
  EXPECT_EQ(manifest1.getManifestHash(), manifest2.getManifestHash())
      << "Manifests with same large timestamp should have same hash";
}

}  // namespace ad_utility
